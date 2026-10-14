#include "service.h"
#include "crypto.h"
#include "vault.h"
#include "scanner.h"
#include "shredder.h"
#include "i18n.h"
#include <ole2.h>
#include <exdisp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <time.h>

#define ID_TRAY_TITLE               4001
#define ID_TRAY_STATUS              4002
#define ID_TRAY_OPEN_APP            4003
#define ID_TRAY_TOGGLE_AUTODEC      4004
#define ID_TRAY_INSTALL             4005
#define ID_TRAY_UNINSTALL           4006
#define ID_TRAY_EXIT                4007

#define FOLDER_CACHE_SIZE           32
#define CACHE_COOLDOWN_MS           8000

typedef struct {
    wchar_t path[MAX_PATH * 2];
    DWORD last_check;
} FolderCacheEntry;

static FolderCacheEntry g_FolderCache[FOLDER_CACHE_SIZE];
static int g_FolderCacheCount = 0;

static HANDLE g_hStopEvent = NULL;
static HANDLE g_hMonitorThread = NULL;
static HWND g_hServiceWnd = NULL;
static NOTIFYICONDATAW g_nid = {0};
static bool g_bAutoDecryptActive = true;

/* ------------------------------------------------------------------------- */
/* Registry Helper Functions                                                 */
/* ------------------------------------------------------------------------- */

static bool reg_set_val_sz(HKEY hRoot, const wchar_t *subKey, const wchar_t *valName, const wchar_t *data) {
    HKEY hKey = NULL;
    DWORD disposition = 0;
    LSTATUS status = RegCreateKeyExW(
        hRoot,
        subKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        NULL,
        &hKey,
        &disposition
    );
    if (status != ERROR_SUCCESS) return false;

    DWORD bytes = (DWORD)((wcslen(data) + 1) * sizeof(wchar_t));
    status = RegSetValueExW(hKey, valName, 0, REG_SZ, (const BYTE *)data, bytes);
    RegCloseKey(hKey);
    return (status == ERROR_SUCCESS);
}

static bool reg_del_val(HKEY hRoot, const wchar_t *subKey, const wchar_t *valName) {
    HKEY hKey = NULL;
    LSTATUS status = RegOpenKeyExW(hRoot, subKey, 0, KEY_SET_VALUE, &hKey);
    if (status != ERROR_SUCCESS) return false;

    status = RegDeleteValueW(hKey, valName);
    RegCloseKey(hKey);
    return (status == ERROR_SUCCESS);
}

/* ------------------------------------------------------------------------- */
/* Notification & Cache Helpers                                              */
/* ------------------------------------------------------------------------- */

static void show_tray_notification(const wchar_t *title, const wchar_t *message) {
    if (!g_hServiceWnd) return;
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_hServiceWnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    wcsncpy(nid.szInfoTitle, title, 63);
    wcsncpy(nid.szInfo, message, 255);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

static bool folder_should_decrypt(const wchar_t *folder) {
    DWORD now = GetTickCount();
    for (int i = 0; i < g_FolderCacheCount; i++) {
        if (_wcsicmp(g_FolderCache[i].path, folder) == 0) {
            if (now - g_FolderCache[i].last_check < CACHE_COOLDOWN_MS) {
                return false; /* Cooldown active */
            }
            return true;
        }
    }
    return true;
}

static void record_folder_decrypted(const wchar_t *folder) {
    DWORD now = GetTickCount();
    for (int i = 0; i < g_FolderCacheCount; i++) {
        if (_wcsicmp(g_FolderCache[i].path, folder) == 0) {
            g_FolderCache[i].last_check = now;
            return;
        }
    }
    if (g_FolderCacheCount < FOLDER_CACHE_SIZE) {
        wcsncpy(g_FolderCache[g_FolderCacheCount].path, folder, MAX_PATH * 2 - 1);
        g_FolderCache[g_FolderCacheCount].last_check = now;
        g_FolderCacheCount++;
    } else {
        memmove(&g_FolderCache[0], &g_FolderCache[1], (FOLDER_CACHE_SIZE - 1) * sizeof(FolderCacheEntry));
        wcsncpy(g_FolderCache[FOLDER_CACHE_SIZE - 1].path, folder, MAX_PATH * 2 - 1);
        g_FolderCache[FOLDER_CACHE_SIZE - 1].last_check = now;
    }
}

/* ------------------------------------------------------------------------- */
/* Decrypted File Tracking & 30-Minute Inactivity Subsystem                  */
/* ------------------------------------------------------------------------- */

#define TRACKED_MAX_FILES       256
#define INACTIVITY_TIMEOUT_MS   (30ULL * 60ULL * 1000ULL) /* 30 minutes */

typedef struct {
    wchar_t plain_path[MAX_PATH * 2];
    DWORD last_access_tick;
    time_t last_access_time;
    FILETIME last_write_time;
    bool active;
} TrackedDecryptedFile;

static TrackedDecryptedFile g_TrackedFiles[TRACKED_MAX_FILES];
static int g_TrackedCount = 0;
static CRITICAL_SECTION g_TrackerCS;
static bool g_bTrackerCSInit = false;

static void get_tracker_file_path(wchar_t *out, size_t max_len) {
    GetModuleFileNameW(NULL, out, (DWORD)max_len);
    wchar_t *p = wcsrchr(out, L'\\');
    if (p) *(p + 1) = L'\0';
    wcsncat(out, L"win10_file_enc_tracker.dat", max_len - wcslen(out) - 1);
}

static void tracker_save(void) {
    wchar_t path[MAX_PATH * 2] = {0};
    get_tracker_file_path(path, MAX_PATH * 2);

    FILE *f = _wfopen(path, L"wb");
    if (!f) return;

    const char magic[4] = {'T', 'K', '0', '1'};
    fwrite(magic, 1, 4, f);

    uint32_t active_count = 0;
    for (int i = 0; i < g_TrackedCount; i++) {
        if (g_TrackedFiles[i].active) active_count++;
    }
    fwrite(&active_count, sizeof(active_count), 1, f);

    for (int i = 0; i < g_TrackedCount; i++) {
        if (!g_TrackedFiles[i].active) continue;
        fwrite(g_TrackedFiles[i].plain_path, sizeof(wchar_t), MAX_PATH * 2, f);
        int64_t t = (int64_t)g_TrackedFiles[i].last_access_time;
        fwrite(&t, sizeof(t), 1, f);
        fwrite(&g_TrackedFiles[i].last_write_time, sizeof(FILETIME), 1, f);
    }
    fclose(f);
}

static void tracker_load(void) {
    wchar_t path[MAX_PATH * 2] = {0};
    get_tracker_file_path(path, MAX_PATH * 2);

    FILE *f = _wfopen(path, L"rb");
    if (!f) return;

    char magic[4] = {0};
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "TK01", 4) != 0) {
        fclose(f);
        return;
    }

    uint32_t count = 0;
    if (fread(&count, sizeof(count), 1, f) != 1) {
        fclose(f);
        return;
    }

    g_TrackedCount = 0;
    DWORD now_tick = GetTickCount();
    for (uint32_t i = 0; i < count && g_TrackedCount < TRACKED_MAX_FILES; i++) {
        wchar_t p[MAX_PATH * 2] = {0};
        int64_t t = 0;
        FILETIME ft = {0};
        if (fread(p, sizeof(wchar_t), MAX_PATH * 2, f) != MAX_PATH * 2) break;
        if (fread(&t, sizeof(t), 1, f) != 1) break;
        if (fread(&ft, sizeof(FILETIME), 1, f) != 1) break;

        DWORD attrs = GetFileAttributesW(p);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            int idx = g_TrackedCount++;
            wcsncpy(g_TrackedFiles[idx].plain_path, p, MAX_PATH * 2 - 1);
            g_TrackedFiles[idx].last_access_tick = now_tick;
            g_TrackedFiles[idx].last_access_time = (time_t)t;
            g_TrackedFiles[idx].last_write_time = ft;
            g_TrackedFiles[idx].active = true;
        }
    }
    fclose(f);
}

void tracker_init(void) {
    if (!g_bTrackerCSInit) {
        InitializeCriticalSection(&g_TrackerCS);
        g_bTrackerCSInit = true;
        tracker_load();
    }
}

void tracker_register_decrypted_file(const wchar_t *plain_path) {
    if (!plain_path || wcslen(plain_path) == 0) return;
    tracker_init();

    EnterCriticalSection(&g_TrackerCS);
    for (int i = 0; i < g_TrackedCount; i++) {
        if (_wcsicmp(g_TrackedFiles[i].plain_path, plain_path) == 0) {
            g_TrackedFiles[i].active = true;
            g_TrackedFiles[i].last_access_tick = GetTickCount();
            g_TrackedFiles[i].last_access_time = time(NULL);
            HANDLE hFile = CreateFileW(plain_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                GetFileTime(hFile, NULL, NULL, &g_TrackedFiles[i].last_write_time);
                CloseHandle(hFile);
            }
            LeaveCriticalSection(&g_TrackerCS);
            tracker_save();
            return;
        }
    }

    if (g_TrackedCount < TRACKED_MAX_FILES) {
        int idx = g_TrackedCount++;
        wcsncpy(g_TrackedFiles[idx].plain_path, plain_path, MAX_PATH * 2 - 1);
        g_TrackedFiles[idx].active = true;
        g_TrackedFiles[idx].last_access_tick = GetTickCount();
        g_TrackedFiles[idx].last_access_time = time(NULL);
        HANDLE hFile = CreateFileW(plain_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            GetFileTime(hFile, NULL, NULL, &g_TrackedFiles[idx].last_write_time);
            CloseHandle(hFile);
        }
    }
    LeaveCriticalSection(&g_TrackerCS);
    tracker_save();
}

void tracker_touch_file(const wchar_t *plain_path) {
    if (!plain_path || wcslen(plain_path) == 0) return;
    tracker_init();

    EnterCriticalSection(&g_TrackerCS);
    for (int i = 0; i < g_TrackedCount; i++) {
        if (g_TrackedFiles[i].active && _wcsicmp(g_TrackedFiles[i].plain_path, plain_path) == 0) {
            g_TrackedFiles[i].last_access_tick = GetTickCount();
            g_TrackedFiles[i].last_access_time = time(NULL);
            HANDLE hFile = CreateFileW(plain_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                GetFileTime(hFile, NULL, NULL, &g_TrackedFiles[i].last_write_time);
                CloseHandle(hFile);
            }
            LeaveCriticalSection(&g_TrackerCS);
            tracker_save();
            return;
        }
    }
    LeaveCriticalSection(&g_TrackerCS);
}

void tracker_remove_file(const wchar_t *plain_path) {
    if (!plain_path || wcslen(plain_path) == 0) return;
    tracker_init();

    EnterCriticalSection(&g_TrackerCS);
    for (int i = 0; i < g_TrackedCount; i++) {
        if (_wcsicmp(g_TrackedFiles[i].plain_path, plain_path) == 0) {
            g_TrackedFiles[i].active = false;
        }
    }
    LeaveCriticalSection(&g_TrackerCS);
    tracker_save();
}

bool tracker_is_file_decrypted(const wchar_t *plain_path) {
    if (!plain_path || wcslen(plain_path) == 0) return false;
    tracker_init();

    EnterCriticalSection(&g_TrackerCS);
    for (int i = 0; i < g_TrackedCount; i++) {
        if (g_TrackedFiles[i].active && _wcsicmp(g_TrackedFiles[i].plain_path, plain_path) == 0) {
            DWORD attrs = GetFileAttributesW(plain_path);
            if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                LeaveCriticalSection(&g_TrackerCS);
                return true;
            }
        }
    }
    LeaveCriticalSection(&g_TrackerCS);
    return false;
}

/* ------------------------------------------------------------------------- */
/* Decryption Operations                                                     */
/* ------------------------------------------------------------------------- */

bool service_auto_decrypt_single_file(const wchar_t *enc_path, bool launch_after) {
    if (!enc_path || wcslen(enc_path) == 0) return false;

    if (!crypto_init()) return false;
    vault_init();

    if (!vault_is_configured()) {
        MessageBoxW(
            NULL,
            i18n_get(STR_SERVICE_VAULT_NOT_SETUP),
            L"Win10 File Encryptor",
            MB_ICONWARNING | MB_OK
        );
        return false;
    }

    wchar_t email[MAX_EMAIL_LEN] = {0};
    if (!vault_get_recovery_email(email, MAX_EMAIL_LEN) || wcslen(email) == 0) {
        return false;
    }

    HANDLE hEnc = CreateFileW(enc_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hEnc == INVALID_HANDLE_VALUE) return false;

    FileHeader header;
    DWORD br = 0;
    BOOL ok = ReadFile(hEnc, &header, sizeof(header), &br, NULL);
    CloseHandle(hEnc);

    if (!ok || br != sizeof(header) || memcmp(header.magic, HEADER_MAGIC, 4) != 0) {
        return false;
    }

    uint8_t key[KEY_SIZE_AES256];
    if (!crypto_derive_key(email, header.salt, sizeof(header.salt), key)) {
        return false;
    }

    wchar_t out_path[MAX_PATH * 2] = {0};
    bool dec_ok = crypto_decrypt_file(enc_path, NULL, key, out_path, MAX_PATH * 2, NULL, NULL, NULL);
    SecureZeroMemory(key, sizeof(key));
    SecureZeroMemory(email, sizeof(email));

    if (dec_ok) {
        /* Remove ciphertext */
        DeleteFileW(enc_path);

        /* Refresh Windows Explorer shell */
        SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, enc_path, NULL);
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, out_path, NULL);

        tracker_register_decrypted_file(out_path);

        if (launch_after) {
            ShellExecuteW(NULL, L"open", out_path, NULL, NULL, SW_SHOWNORMAL);
        }
        return true;
    } else {
        MessageBoxW(
            NULL,
            i18n_get(STR_SERVICE_DEC_FAIL),
            L"Auto-Decrypt Error",
            MB_ICONERROR | MB_OK
        );
        return false;
    }
}

int service_auto_decrypt_folder(const wchar_t *folder_path) {
    if (!folder_path || wcslen(folder_path) == 0) return 0;

    if (!crypto_init()) return 0;
    vault_init();
    if (!vault_is_configured()) return 0;

    wchar_t email[MAX_EMAIL_LEN] = {0};
    if (!vault_get_recovery_email(email, MAX_EMAIL_LEN) || wcslen(email) == 0) {
        return 0;
    }

    FileList *list = scanner_create();
    if (!list) return 0;
    if (!scanner_scan(folder_path, false, OP_MODE_DECRYPT, list)) {
        scanner_free(list);
        return 0;
    }

    int decrypted_count = 0;
    for (size_t i = 0; i < list->count; i++) {
        const wchar_t *enc_path = list->paths[i];
        HANDLE hEnc = CreateFileW(enc_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hEnc == INVALID_HANDLE_VALUE) continue;

        FileHeader header;
        DWORD br = 0;
        BOOL ok = ReadFile(hEnc, &header, sizeof(header), &br, NULL);
        CloseHandle(hEnc);

        if (!ok || br != sizeof(header) || memcmp(header.magic, HEADER_MAGIC, 4) != 0) {
            continue;
        }

        uint8_t key[KEY_SIZE_AES256];
        if (!crypto_derive_key(email, header.salt, sizeof(header.salt), key)) {
            continue;
        }

        wchar_t out_path[MAX_PATH * 2] = {0};
        if (crypto_decrypt_file(enc_path, NULL, key, out_path, MAX_PATH * 2, NULL, NULL, NULL)) {
            DeleteFileW(enc_path);
            SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, enc_path, NULL);
            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, out_path, NULL);
            tracker_register_decrypted_file(out_path);
            decrypted_count++;
        }
        SecureZeroMemory(key, sizeof(key));
    }

    scanner_free(list);
    SecureZeroMemory(email, sizeof(email));

    if (decrypted_count > 0) {
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, folder_path, NULL);
    }

    return decrypted_count;
}

/* ------------------------------------------------------------------------- */
/* Auto-Encryption & Inactivity Manager                                      */
/* ------------------------------------------------------------------------- */

bool service_auto_encrypt_single_file(const wchar_t *plain_path) {
    if (!plain_path || wcslen(plain_path) == 0) return false;

    DWORD attrs = GetFileAttributesW(plain_path);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    const wchar_t *ext = wcsrchr(plain_path, L'.');
    if (ext && _wcsicmp(ext, ENC_FILE_EXTENSION) == 0) {
        return true; /* Already encrypted */
    }

    if (!crypto_init()) return false;
    vault_init();
    if (!vault_is_configured()) return false;

    wchar_t email[MAX_EMAIL_LEN] = {0};
    if (!vault_get_recovery_email(email, MAX_EMAIL_LEN) || wcslen(email) == 0) {
        return false;
    }

    uint8_t file_salt[SALT_SIZE];
    crypto_generate_random(file_salt, sizeof(file_salt));

    uint8_t derived_key[KEY_SIZE_AES256];
    if (!crypto_derive_key(email, file_salt, sizeof(file_salt), derived_key)) {
        return false;
    }

    wchar_t tmp_enc_path[MAX_PATH * 2];
    wchar_t final_enc_path[MAX_PATH * 2];
    swprintf(tmp_enc_path, MAX_PATH * 2, L"%ls%ls", plain_path, TMP_FILE_EXTENSION);
    swprintf(final_enc_path, MAX_PATH * 2, L"%ls%ls", plain_path, ENC_FILE_EXTENSION);

    bool enc_ok = crypto_encrypt_file(
        plain_path,
        tmp_enc_path,
        derived_key,
        file_salt,
        CIPHER_AES_256_GCM,
        NULL,
        NULL,
        NULL
    );

    SecureZeroMemory(derived_key, sizeof(derived_key));
    SecureZeroMemory(email, sizeof(email));

    if (enc_ok) {
        if (shredder_commit_and_wipe_original(plain_path, tmp_enc_path, final_enc_path)) {
            SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, plain_path, NULL);
            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, final_enc_path, NULL);
            tracker_remove_file(plain_path);
            return true;
        } else {
            DeleteFileW(tmp_enc_path);
            return false;
        }
    } else {
        DeleteFileW(tmp_enc_path);
        return false;
    }
}

int tracker_check_inactivity(void) {
    tracker_init();

    int encrypted_count = 0;
    DWORD now_tick = GetTickCount();
    time_t now_time = time(NULL);

    EnterCriticalSection(&g_TrackerCS);
    for (int i = 0; i < g_TrackedCount; i++) {
        if (!g_TrackedFiles[i].active) continue;

        const wchar_t *path = g_TrackedFiles[i].plain_path;
        DWORD attrs = GetFileAttributesW(path);
        if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            g_TrackedFiles[i].active = false;
            continue;
        }

        /* Check if file has been modified or accessed externally */
        HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            FILETIME ftWrite = {0};
            FILETIME ftAccess = {0};
            GetFileTime(hFile, NULL, &ftAccess, &ftWrite);
            CloseHandle(hFile);

            if (CompareFileTime(&ftWrite, &g_TrackedFiles[i].last_write_time) > 0) {
                g_TrackedFiles[i].last_write_time = ftWrite;
                g_TrackedFiles[i].last_access_tick = now_tick;
                g_TrackedFiles[i].last_access_time = now_time;
                continue;
            }
        }

        DWORD elapsed_ms = now_tick - g_TrackedFiles[i].last_access_tick;
        double elapsed_sec = difftime(now_time, g_TrackedFiles[i].last_access_time);

        if (elapsed_ms >= INACTIVITY_TIMEOUT_MS || elapsed_sec >= 1800.0) {
            wchar_t encrypt_target[MAX_PATH * 2];
            wcsncpy(encrypt_target, path, MAX_PATH * 2 - 1);
            g_TrackedFiles[i].active = false;

            LeaveCriticalSection(&g_TrackerCS);
            if (service_auto_encrypt_single_file(encrypt_target)) {
                encrypted_count++;
            }
            EnterCriticalSection(&g_TrackerCS);
        }
    }
    LeaveCriticalSection(&g_TrackerCS);

    if (encrypted_count > 0) {
        tracker_save();
    }
    return encrypted_count;
}

int tracker_encrypt_all_decrypted_files(void) {
    tracker_init();
    int count = 0;

    EnterCriticalSection(&g_TrackerCS);
    for (int i = 0; i < g_TrackedCount; i++) {
        if (!g_TrackedFiles[i].active) continue;

        wchar_t path[MAX_PATH * 2];
        wcsncpy(path, g_TrackedFiles[i].plain_path, MAX_PATH * 2 - 1);
        g_TrackedFiles[i].active = false;

        DWORD attrs = GetFileAttributesW(path);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            LeaveCriticalSection(&g_TrackerCS);
            if (service_auto_encrypt_single_file(path)) {
                count++;
            }
            EnterCriticalSection(&g_TrackerCS);
        }
    }
    LeaveCriticalSection(&g_TrackerCS);

    tracker_save();
    return count;
}

/* ------------------------------------------------------------------------- */
/* Active Windows Explorer Watcher Thread                                    */
/* ------------------------------------------------------------------------- */

static DWORD WINAPI explorer_monitor_thread(LPVOID lpParam) {
    (void)lpParam;
    CoInitialize(NULL);

    while (WaitForSingleObject(g_hStopEvent, 750) == WAIT_TIMEOUT) {
        if (!g_bAutoDecryptActive) continue;

        IShellWindows *psw = NULL;
        HRESULT hr = CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_ALL, &IID_IShellWindows, (void **)&psw);
        if (FAILED(hr) || !psw) continue;

        long count = 0;
        psw->lpVtbl->get_Count(psw, &count);

        for (long i = 0; i < count; i++) {
            VARIANT vIndex;
            VariantInit(&vIndex);
            vIndex.vt = VT_I4;
            vIndex.lVal = i;

            IDispatch *pDisp = NULL;
            if (SUCCEEDED(psw->lpVtbl->Item(psw, vIndex, &pDisp)) && pDisp) {
                IWebBrowserApp *pApp = NULL;
                if (SUCCEEDED(pDisp->lpVtbl->QueryInterface(pDisp, &IID_IWebBrowserApp, (void **)&pApp)) && pApp) {
                    BSTR bstrUrl = NULL;
                    pApp->lpVtbl->get_LocationURL(pApp, &bstrUrl);
                    if (bstrUrl) {
                        wchar_t folderPath[MAX_PATH * 2] = {0};
                        DWORD pathLen = MAX_PATH * 2;
                        if (SUCCEEDED(PathCreateFromUrlW(bstrUrl, folderPath, &pathLen, 0))) {
                            DWORD attr = GetFileAttributesW(folderPath);
                            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
                                wchar_t searchPattern[MAX_PATH * 2] = {0};
                                swprintf(searchPattern, MAX_PATH * 2, L"%ls\\*.enc", folderPath);
                                WIN32_FIND_DATAW fd;
                                HANDLE hFind = FindFirstFileW(searchPattern, &fd);
                                if (hFind != INVALID_HANDLE_VALUE) {
                                    FindClose(hFind);

                                    if (folder_should_decrypt(folderPath)) {
                                        int decCount = service_auto_decrypt_folder(folderPath);
                                        if (decCount > 0) {
                                            record_folder_decrypted(folderPath);
                                            wchar_t notifMsg[512] = {0};
                                            swprintf(notifMsg, 512, i18n_get(STR_SERVICE_AUTO_DECRYPTED_FOLDER), decCount);
                                            show_tray_notification(i18n_get(STR_SERVICE_BALLOON_TITLE), notifMsg);
                                        }
                                    }
                                }
                            }
                        }
                        SysFreeString(bstrUrl);
                    }
                    pApp->lpVtbl->Release(pApp);
                }
                pDisp->lpVtbl->Release(pDisp);
            }
        }

        psw->lpVtbl->Release(psw);

        /* Periodically check 30-minute inactivity auto-encryption */
        int encCount = tracker_check_inactivity();
        if (encCount > 0) {
            wchar_t notifMsg[512] = {0};
            swprintf(notifMsg, 512, L"%d file(s) automatically re-encrypted after 30 minutes of inactivity.", encCount);
            show_tray_notification(i18n_get(STR_SERVICE_BALLOON_TITLE), notifMsg);
        }
    }

    tracker_encrypt_all_decrypted_files();
    CoUninitialize();
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Service Window & Tray Menu Handler                                        */
/* ------------------------------------------------------------------------- */

static void open_main_application(void) {
    HWND hExisting = FindWindowW(L"Win10FileModMainClass", NULL);
    if (hExisting) {
        ShowWindow(hExisting, SW_SHOW);
        ShowWindow(hExisting, SW_RESTORE);
        SetForegroundWindow(hExisting);
        return;
    }
    wchar_t exePath[MAX_PATH * 2] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH * 2 - 1);
    ShellExecuteW(NULL, L"open", exePath, NULL, NULL, SW_SHOWNORMAL);
}

static LRESULT CALLBACK service_wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_APP_TRAYMSG: {
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                POINT pt;
                GetCursorPos(&pt);

                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING | MF_DISABLED, ID_TRAY_TITLE, i18n_get(STR_SERVICE_TRAY_TITLE));
                AppendMenuW(hMenu, MF_STRING | MF_DISABLED, ID_TRAY_STATUS,
                    g_bAutoDecryptActive ? i18n_get(STR_SERVICE_STATUS_RUNNING) : i18n_get(STR_SERVICE_STATUS_STOPPED));
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

                AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN_APP, L"🖥️ Buka Aplikasi Utama");
                UINT decFlags = g_bAutoDecryptActive ? (MF_STRING | MF_CHECKED) : MF_STRING;
                AppendMenuW(hMenu, decFlags, ID_TRAY_TOGGLE_AUTODEC, L"🛡️ Auto-Decrypt Folder & Berkas");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

                AppendMenuW(hMenu, MF_STRING, ID_TRAY_INSTALL, i18n_get(STR_SERVICE_INSTALL_INTEGRATION));
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_UNINSTALL, i18n_get(STR_SERVICE_UNINSTALL_INTEGRATION));
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

                AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"❌ Tutup Layanan");

                SetForegroundWindow(hWnd);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
                DestroyMenu(hMenu);

                if (cmd == ID_TRAY_OPEN_APP) {
                    open_main_application();
                } else if (cmd == ID_TRAY_TOGGLE_AUTODEC) {
                    g_bAutoDecryptActive = !g_bAutoDecryptActive;
                } else if (cmd == ID_TRAY_INSTALL) {
                    service_install();
                    show_tray_notification(i18n_get(STR_SERVICE_BALLOON_TITLE), i18n_get(STR_SERVICE_INTEGRATION_INSTALLED));
                } else if (cmd == ID_TRAY_UNINSTALL) {
                    service_uninstall();
                    show_tray_notification(i18n_get(STR_SERVICE_BALLOON_TITLE), i18n_get(STR_SERVICE_INTEGRATION_REMOVED));
                } else if (cmd == ID_TRAY_EXIT) {
                    PostQuitMessage(0);
                }
            } else if (lParam == WM_LBUTTONDBLCLK) {
                open_main_application();
            }
            return 0;
        }

        case WM_APP_SERVICE_STOP:
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

int service_run_daemon(HINSTANCE hInstance) {
    /* Ensure single instance of service */
    HANDLE hMutex = CreateMutexW(NULL, TRUE, SERVICE_MUTEX_NAME);
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0; /* Already running */
    }

    i18n_init(LANG_EN);
    crypto_init();
    vault_init();

    /* Automatically re-encrypt all files left decrypted upon service startup */
    tracker_init();
    tracker_encrypt_all_decrypted_files();

    /* Register hidden window class */
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = service_wnd_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = SERVICE_WINDOW_CLASS;
    RegisterClassExW(&wc);

    g_hServiceWnd = CreateWindowExW(
        0,
        SERVICE_WINDOW_CLASS,
        L"Win10FileEnc_Service",
        WS_OVERLAPPED,
        0, 0, 0, 0,
        HWND_MESSAGE,
        NULL,
        hInstance,
        NULL
    );

    /* Initialize System Tray Icon */
    HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    if (!hIcon) hIcon = LoadIconW(NULL, IDI_SHIELD);

    SecureZeroMemory(&g_nid, sizeof(NOTIFYICONDATAW));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hServiceWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_APP_TRAYMSG;
    g_nid.hIcon = hIcon;
    wcsncpy(g_nid.szTip, L"Win10 File Protection & Auto-Decrypt Service", 127);
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    /* Start Explorer Window Monitoring Thread */
    g_hStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    g_hMonitorThread = CreateThread(NULL, 0, explorer_monitor_thread, NULL, 0, NULL);

    /* Message Loop */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    /* Terminate monitor thread cleanly */
    if (g_hStopEvent) SetEvent(g_hStopEvent);
    if (g_hMonitorThread) {
        WaitForSingleObject(g_hMonitorThread, 2000);
        CloseHandle(g_hMonitorThread);
        g_hMonitorThread = NULL;
    }
    if (g_hStopEvent) {
        CloseHandle(g_hStopEvent);
        g_hStopEvent = NULL;
    }

    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    DestroyWindow(g_hServiceWnd);
    UnregisterClassW(SERVICE_WINDOW_CLASS, hInstance);

    CloseHandle(hMutex);
    crypto_cleanup();
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Management & Installation APIs                                            */
/* ------------------------------------------------------------------------- */

bool service_is_installed(void) {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\.enc", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

bool service_is_running(void) {
    HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, SERVICE_MUTEX_NAME);
    if (hMutex) {
        CloseHandle(hMutex);
        return true;
    }
    return (FindWindowW(SERVICE_WINDOW_CLASS, NULL) != NULL);
}

static void get_app_exe_path(wchar_t *out_path, size_t max_len) {
    GetModuleFileNameW(NULL, out_path, (DWORD)max_len);
    wchar_t *slash = wcsrchr(out_path, L'\\');
    if (slash) {
        if (_wcsicmp(slash + 1, L"win10_file_enc.exe") != 0) {
            wchar_t candidate[MAX_PATH * 2] = {0};
            wcsncpy(candidate, out_path, slash - out_path);
            candidate[slash - out_path] = L'\0';
            wchar_t *parent_slash = wcsrchr(candidate, L'\\');
            if (parent_slash) {
                wchar_t rootCandidate[MAX_PATH * 2] = {0};
                wcsncpy(rootCandidate, candidate, parent_slash - candidate + 1);
                wcsncat(rootCandidate, L"win10_file_enc.exe", MAX_PATH * 2 - wcslen(rootCandidate) - 1);
                if (GetFileAttributesW(rootCandidate) != INVALID_FILE_ATTRIBUTES) {
                    wcsncpy(out_path, rootCandidate, max_len - 1);
                    return;
                }
            }
            *(slash + 1) = L'\0';
            wcsncat(out_path, L"win10_file_enc.exe", max_len - wcslen(out_path) - 1);
        }
    }
}

bool service_install(void) {
    wchar_t exePath[MAX_PATH * 2] = {0};
    get_app_exe_path(exePath, MAX_PATH * 2 - 1);

    wchar_t cmdService[MAX_PATH * 2 + 32] = {0};
    swprintf(cmdService, MAX_PATH * 2 + 32, L"\"%ls\" --service", exePath);

    wchar_t cmdAutoDec[MAX_PATH * 2 + 32] = {0};
    swprintf(cmdAutoDec, MAX_PATH * 2 + 32, L"\"%ls\" --auto-decrypt \"%%1\"", exePath);

    wchar_t cmdAutoDecFolder[MAX_PATH * 2 + 32] = {0};
    swprintf(cmdAutoDecFolder, MAX_PATH * 2 + 32, L"\"%ls\" --auto-decrypt-folder \"%%1\"", exePath);

    wchar_t cmdAutoDecFolderBg[MAX_PATH * 2 + 32] = {0};
    swprintf(cmdAutoDecFolderBg, MAX_PATH * 2 + 32, L"\"%ls\" --auto-decrypt-folder \"%%V\"", exePath);

    wchar_t iconStr[MAX_PATH * 2 + 16] = {0};
    swprintf(iconStr, MAX_PATH * 2 + 16, L"\"%ls\",0", exePath);

    /* 1. Run on Windows Startup */
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"Win10FileEncService", cmdService);

    /* 2. File Association .enc */
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\.enc", NULL, L"Win10EncFile");
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\.enc", L"Content Type", L"application/x-win10-file-enc");

    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Win10EncFile", NULL, L"Win10 Encrypted Protected File");
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Win10EncFile\\DefaultIcon", NULL, iconStr);

    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Win10EncFile\\shell\\open", NULL, L"Buka & Dekripsi Otomatis");
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Win10EncFile\\shell\\open\\command", NULL, cmdAutoDec);

    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Win10EncFile\\shell\\decrypt", NULL, L"Dekripsi Berkas Terproteksi");
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Win10EncFile\\shell\\decrypt\\command", NULL, cmdAutoDec);

    /* 3. Directory Context Menu */
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\Win10AutoDecrypt", NULL, L"Dekripsi Berkas Terproteksi di Folder ini");
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\Win10AutoDecrypt", L"Icon", iconStr);
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\Win10AutoDecrypt\\command", NULL, cmdAutoDecFolder);

    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\Background\\shell\\Win10AutoDecrypt", NULL, L"Dekripsi Berkas Terproteksi di Folder ini");
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\Background\\shell\\Win10AutoDecrypt", L"Icon", iconStr);
    reg_set_val_sz(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\Background\\shell\\Win10AutoDecrypt\\command", NULL, cmdAutoDecFolderBg);

    /* 4. Notify Explorer */
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    /* 5. Start Background Service process if not yet running */
    if (!service_is_running()) {
        service_start_process();
    }

    return true;
}

bool service_uninstall(void) {
    /* 1. Stop background service if running */
    service_stop_process();

    /* 2. Remove autostart */
    reg_del_val(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"Win10FileEncService");

    /* 3. Remove .enc associations */
    SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\.enc");
    SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Win10EncFile");

    /* 4. Remove Directory context menu */
    SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\Win10AutoDecrypt");
    SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\Background\\shell\\Win10AutoDecrypt");

    /* 5. Notify Explorer */
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    return true;
}

bool service_start_process(void) {
    if (service_is_running()) return true;

    wchar_t exePath[MAX_PATH * 2] = {0};
    get_app_exe_path(exePath, MAX_PATH * 2 - 1);

    wchar_t cmdLine[MAX_PATH * 2 + 32] = {0};
    swprintf(cmdLine, MAX_PATH * 2 + 32, L"\"%ls\" --service", exePath);

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    BOOL created = CreateProcessW(
        NULL,
        cmdLine,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (created) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}

bool service_stop_process(void) {
    HWND hWnd = FindWindowW(SERVICE_WINDOW_CLASS, NULL);
    if (hWnd) {
        PostMessageW(hWnd, WM_APP_SERVICE_STOP, 0, 0);
        return true;
    }
    return false;
}
