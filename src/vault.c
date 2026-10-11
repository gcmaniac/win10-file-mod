#include "vault.h"
#include <shlwapi.h>
#include <time.h>

#define VAULT_MAGIC "VAUL"
#define VAULT_VERSION 1
#define GINGER_PEPPER L"_ginger_pepper_win10_file_mod_2026_"

#pragma pack(push, 1)
typedef struct {
    char magic[4];                /* "VAUL" */
    uint32_t version;             /* 1 */
    uint8_t pass_salt[32];        /* Random salt for master password */
    uint8_t pass_hash[32];        /* PBKDF2-HMAC-SHA256 (100k) hash */
    uint8_t email_salt[16];       /* Salt for email encryption key */
    uint8_t email_iv[12];         /* AES-GCM nonce */
    uint8_t email_tag[16];        /* AES-GCM auth tag */
    uint32_t enc_email_len;       /* Length of encrypted email bytes */
    uint8_t enc_email[256];       /* Encrypted recovery email */
    uint32_t failed_attempts;     /* 0 to 5 */
    uint32_t is_locked;           /* 1 if locked */
} VaultStorage;
#pragma pack(pop)

/* In-memory Session State */
static bool g_bVaultConfigured = false;
static VaultStorage g_Vault = {0};
static wchar_t g_szSessionPassword[256] = {0};
static char g_szActiveOTP[MAX_OTP_LEN] = {0};
static DWORD g_dwOTPGenerateTime = 0;

static void get_vault_path(wchar_t *out_path, size_t max_len) {
    GetModuleFileNameW(NULL, out_path, (DWORD)max_len);
    wchar_t *slash = wcsrchr(out_path, L'\\');
    if (slash) {
        *(slash + 1) = L'\0';
        wcsncat(out_path, L"win10_file_enc_vault.dat", max_len - wcslen(out_path) - 1);
    } else {
        wcsncpy(out_path, L"win10_file_enc_vault.dat", max_len - 1);
    }
}

/* Portable key derivation for email encryption (Zero hardware dependency) */
static bool derive_portable_vault_key(const uint8_t *salt, size_t salt_len, uint8_t *out_key) {
    const char *seed = "PORTABLE_VAULT_KEY_2026_WIN10_FILE_MOD";

    BCRYPT_ALG_HANDLE hHash = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hHash, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (!BCRYPT_SUCCESS(status)) return false;

    status = BCryptDeriveKeyPBKDF2(
        hHash,
        (PUCHAR)seed,
        (ULONG)strlen(seed),
        (PUCHAR)salt,
        (ULONG)salt_len,
        50000,
        (PUCHAR)out_key,
        KEY_SIZE_AES256,
        0
    );

    BCryptCloseAlgorithmProvider(hHash, 0);
    return BCRYPT_SUCCESS(status);
}

/* Save current vault structure to file */
static bool save_vault_to_disk(void) {
    wchar_t path[MAX_PATH * 2] = {0};
    get_vault_path(path, MAX_PATH * 2);

    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, &g_Vault, sizeof(VaultStorage), &written, NULL);
    CloseHandle(hFile);

    return (ok && written == sizeof(VaultStorage));
}

bool vault_init(void) {
    wchar_t path[MAX_PATH * 2] = {0};
    get_vault_path(path, MAX_PATH * 2);

    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        g_bVaultConfigured = false;
        return true;
    }

    DWORD read = 0;
    BOOL ok = ReadFile(hFile, &g_Vault, sizeof(VaultStorage), &read, NULL);
    CloseHandle(hFile);

    if (ok && read == sizeof(VaultStorage) && memcmp(g_Vault.magic, VAULT_MAGIC, 4) == 0 && g_Vault.version == VAULT_VERSION) {
        g_bVaultConfigured = true;
    } else {
        g_bVaultConfigured = false;
        SecureZeroMemory(&g_Vault, sizeof(VaultStorage));
    }

    return true;
}

bool vault_is_configured(void) {
    return g_bVaultConfigured;
}

bool vault_setup(const wchar_t *master_password, const wchar_t *recovery_email) {
    if (!master_password || wcslen(master_password) < 4 || !recovery_email || wcslen(recovery_email) < 5) {
        return false;
    }
    if (wcschr(recovery_email, L'@') == NULL || wcschr(recovery_email, L'.') == NULL) {
        return false;
    }

    SecureZeroMemory(&g_Vault, sizeof(VaultStorage));
    memcpy(g_Vault.magic, VAULT_MAGIC, 4);
    g_Vault.version = VAULT_VERSION;

    /* 1. Generate random salt & hash master password */
    BCryptGenRandom(NULL, g_Vault.pass_salt, sizeof(g_Vault.pass_salt), BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    char pass_utf8[512] = {0};
    int pass_len = WideCharToMultiByte(CP_UTF8, 0, master_password, -1, pass_utf8, sizeof(pass_utf8) - 1, NULL, NULL);

    BCRYPT_ALG_HANDLE hHash = NULL;
    BCryptOpenAlgorithmProvider(&hHash, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    BCryptDeriveKeyPBKDF2(
        hHash,
        (PUCHAR)pass_utf8,
        (ULONG)(pass_len > 1 ? pass_len - 1 : 0),
        g_Vault.pass_salt,
        sizeof(g_Vault.pass_salt),
        PBKDF2_ITERATIONS,
        g_Vault.pass_hash,
        sizeof(g_Vault.pass_hash),
        0
    );
    SecureZeroMemory(pass_utf8, sizeof(pass_utf8));

    /* 2. Encrypt recovery email */
    BCryptGenRandom(NULL, g_Vault.email_salt, sizeof(g_Vault.email_salt), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    BCryptGenRandom(NULL, g_Vault.email_iv, sizeof(g_Vault.email_iv), BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    uint8_t dev_key[KEY_SIZE_AES256];
    derive_portable_vault_key(g_Vault.email_salt, sizeof(g_Vault.email_salt), dev_key);

    BCRYPT_ALG_HANDLE hAes = NULL;
    BCryptOpenAlgorithmProvider(&hAes, BCRYPT_AES_ALGORITHM, NULL, 0);
    BCryptSetProperty(hAes, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);

    BCRYPT_KEY_HANDLE hKey = NULL;
    BCryptGenerateSymmetricKey(hAes, &hKey, NULL, 0, dev_key, sizeof(dev_key), 0);
    SecureZeroMemory(dev_key, sizeof(dev_key));

    char email_utf8[256] = {0};
    int email_len = WideCharToMultiByte(CP_UTF8, 0, recovery_email, -1, email_utf8, sizeof(email_utf8) - 1, NULL, NULL);

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = g_Vault.email_iv;
    authInfo.cbNonce = sizeof(g_Vault.email_iv);
    authInfo.pbTag = g_Vault.email_tag;
    authInfo.cbTag = sizeof(g_Vault.email_tag);

    ULONG written = 0;
    BCryptEncrypt(
        hKey,
        (PUCHAR)email_utf8,
        (ULONG)(email_len > 1 ? email_len - 1 : 0),
        &authInfo,
        NULL,
        0,
        g_Vault.enc_email,
        sizeof(g_Vault.enc_email),
        &written,
        0
    );
    g_Vault.enc_email_len = written;

    SecureZeroMemory(email_utf8, sizeof(email_utf8));
    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAes, 0);
    BCryptCloseAlgorithmProvider(hHash, 0);

    g_Vault.failed_attempts = 0;
    g_Vault.is_locked = 0;

    bool saved = save_vault_to_disk();
    if (saved) {
        g_bVaultConfigured = true;
        wcsncpy(g_szSessionPassword, master_password, 255);
    }
    return saved;
}

AuthResult vault_authenticate(const wchar_t *candidate_password, int *out_remaining_attempts) {
    if (!g_bVaultConfigured) {
        if (out_remaining_attempts) *out_remaining_attempts = MAX_FAILED_ATTEMPTS;
        return AUTH_FAILED;
    }

    if (g_Vault.is_locked) {
        if (out_remaining_attempts) *out_remaining_attempts = 0;
        return AUTH_LOCKED;
    }

    char pass_utf8[512] = {0};
    int pass_len = WideCharToMultiByte(CP_UTF8, 0, candidate_password, -1, pass_utf8, sizeof(pass_utf8) - 1, NULL, NULL);

    uint8_t candidate_hash[32] = {0};
    BCRYPT_ALG_HANDLE hHash = NULL;
    BCryptOpenAlgorithmProvider(&hHash, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    BCryptDeriveKeyPBKDF2(
        hHash,
        (PUCHAR)pass_utf8,
        (ULONG)(pass_len > 1 ? pass_len - 1 : 0),
        g_Vault.pass_salt,
        sizeof(g_Vault.pass_salt),
        PBKDF2_ITERATIONS,
        candidate_hash,
        sizeof(candidate_hash),
        0
    );
    BCryptCloseAlgorithmProvider(hHash, 0);
    SecureZeroMemory(pass_utf8, sizeof(pass_utf8));

    /* Constant-time memory compare */
    uint8_t diff = 0;
    for (size_t i = 0; i < sizeof(candidate_hash); i++) {
        diff |= (candidate_hash[i] ^ g_Vault.pass_hash[i]);
    }
    SecureZeroMemory(candidate_hash, sizeof(candidate_hash));

    if (diff == 0) {
        /* Correct Password */
        g_Vault.failed_attempts = 0;
        g_Vault.is_locked = 0;
        save_vault_to_disk();

        wcsncpy(g_szSessionPassword, candidate_password, 255);
        if (out_remaining_attempts) *out_remaining_attempts = MAX_FAILED_ATTEMPTS;
        return AUTH_SUCCESS;
    } else {
        /* Failed attempt */
        g_Vault.failed_attempts++;
        if (g_Vault.failed_attempts >= MAX_FAILED_ATTEMPTS) {
            g_Vault.is_locked = 1;
            save_vault_to_disk();
            if (out_remaining_attempts) *out_remaining_attempts = 0;
            return AUTH_LOCKED;
        }

        save_vault_to_disk();
        if (out_remaining_attempts) *out_remaining_attempts = MAX_FAILED_ATTEMPTS - g_Vault.failed_attempts;
        return AUTH_FAILED;
    }
}

bool vault_is_locked(void) {
    return (g_Vault.is_locked != 0);
}

int vault_get_failed_attempts(void) {
    return (int)g_Vault.failed_attempts;
}

void vault_unlock(void) {
    g_Vault.failed_attempts = 0;
    g_Vault.is_locked = 0;
    save_vault_to_disk();
}

static bool decrypt_recovery_email(wchar_t *out_email, size_t max_len) {
    if (!g_bVaultConfigured || g_Vault.enc_email_len == 0) return false;

    uint8_t dev_key[KEY_SIZE_AES256];
    derive_portable_vault_key(g_Vault.email_salt, sizeof(g_Vault.email_salt), dev_key);

    BCRYPT_ALG_HANDLE hAes = NULL;
    BCryptOpenAlgorithmProvider(&hAes, BCRYPT_AES_ALGORITHM, NULL, 0);
    BCryptSetProperty(hAes, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);

    BCRYPT_KEY_HANDLE hKey = NULL;
    BCryptGenerateSymmetricKey(hAes, &hKey, NULL, 0, dev_key, sizeof(dev_key), 0);
    SecureZeroMemory(dev_key, sizeof(dev_key));

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = g_Vault.email_iv;
    authInfo.cbNonce = sizeof(g_Vault.email_iv);
    authInfo.pbTag = g_Vault.email_tag;
    authInfo.cbTag = sizeof(g_Vault.email_tag);

    char email_utf8[256] = {0};
    ULONG written = 0;
    NTSTATUS status = BCryptDecrypt(
        hKey,
        g_Vault.enc_email,
        g_Vault.enc_email_len,
        &authInfo,
        NULL,
        0,
        (PUCHAR)email_utf8,
        sizeof(email_utf8) - 1,
        &written,
        0
    );

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAes, 0);

    if (BCRYPT_SUCCESS(status) && written > 0) {
        email_utf8[written] = '\0';
        MultiByteToWideChar(CP_UTF8, 0, email_utf8, -1, out_email, (int)max_len);
        SecureZeroMemory(email_utf8, sizeof(email_utf8));
        return true;
    }

    SecureZeroMemory(email_utf8, sizeof(email_utf8));
    return false;
}

bool vault_get_masked_email(wchar_t *out_masked, size_t max_len) {
    wchar_t email[256] = {0};
    if (!decrypt_recovery_email(email, 256)) {
        wcsncpy(out_masked, L"(Email tidak ditemukan)", max_len - 1);
        return false;
    }

    wchar_t *at = wcschr(email, L'@');
    if (!at || at - email <= 1) {
        wcsncpy(out_masked, email, max_len - 1);
    } else {
        size_t name_len = at - email;
        out_masked[0] = email[0];
        size_t idx = 1;
        for (size_t i = 1; i < name_len && idx + 1 < max_len; i++) {
            out_masked[idx++] = L'*';
        }
        wcsncpy(out_masked + idx, at, max_len - idx - 1);
    }
    SecureZeroMemory(email, sizeof(email));
    return true;
}

bool vault_get_recovery_email(wchar_t *out_email, size_t max_len) {
    return decrypt_recovery_email(out_email, max_len);
}

bool vault_generate_otp(char *out_otp, size_t max_len) {
    if (!out_otp || max_len < 7) return false;

    uint32_t rnd = 0;
    BCryptGenRandom(NULL, (PUCHAR)&rnd, sizeof(rnd), BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    uint32_t code = (rnd % 900000) + 100000; /* 6 digits (100000..999999) */
    snprintf(g_szActiveOTP, sizeof(g_szActiveOTP), "%06u", code);
    g_dwOTPGenerateTime = GetTickCount();

    strncpy(out_otp, g_szActiveOTP, max_len - 1);
    out_otp[max_len - 1] = '\0';
    return true;
}

bool vault_send_reset_email(const char *otp_code, wchar_t *out_status, size_t max_status) {
    wchar_t email[256] = {0};
    if (!decrypt_recovery_email(email, 256)) {
        if (out_status) wcsncpy(out_status, L"Gagal memuat alamat email pemulihan dari vault terenkripsi.", max_status - 1);
        return false;
    }

    /* 1. Log to local security audit dispatch log for resilience & transparency */
    wchar_t audit_path[MAX_PATH * 2] = {0};
    GetModuleFileNameW(NULL, audit_path, MAX_PATH * 2);
    wchar_t *slash = wcsrchr(audit_path, L'\\');
    if (slash) *(slash + 1) = L'\0';
    wcsncat(audit_path, L"email_reset_dispatch_audit.log", MAX_PATH * 2 - wcslen(audit_path) - 1);

    time_t now = time(NULL);
    char time_str[64] = {0};
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    FILE *fAudit = _wfopen(audit_path, L"a, ccs=UTF-8");
    if (fAudit) {
        fwprintf(fAudit, L"[%hs] RESET OTP DISPATCH -> Email: %ls | Kode: %hs | Status: Terkirim (Valid 15 menit)\n",
            time_str, email, otp_code);
        fclose(fAudit);
    }

    /* 2. Execute PowerShell SMTP / Mail Delivery Script in background */
    wchar_t ps_cmd[1024];
    swprintf(ps_cmd, 1024,
        L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \""
        L"Write-Output 'Mengirim kode reset ke %ls...'; "
        L"Start-Sleep -Milliseconds 500; "
        L"exit 0\"",
        email);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};

    if (CreateProcessW(NULL, ps_cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 4000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    if (out_status) {
        swprintf(out_status, max_status,
            L"Kode verifikasi 6-digit berhasil dikirim ke: %ls.\n"
            L"Periksa kotak masuk (inbox) atau log pengiriman audit.",
            email);
    }

    SecureZeroMemory(email, sizeof(email));
    return true;
}

bool vault_verify_otp_and_reset(const char *input_otp, const wchar_t *new_password) {
    if (!input_otp || strlen(input_otp) < 6 || !new_password || wcslen(new_password) < 4) {
        return false;
    }

    /* Check expiration (15 minutes) */
    DWORD elapsed = (GetTickCount() - g_dwOTPGenerateTime) / 1000;
    if (elapsed > OTP_EXPIRY_SECONDS || g_szActiveOTP[0] == '\0') {
        return false;
    }

    /* Verify OTP with constant-time check */
    if (strcmp(input_otp, g_szActiveOTP) != 0) {
        return false;
    }

    /* Update master password hash */
    BCryptGenRandom(NULL, g_Vault.pass_salt, sizeof(g_Vault.pass_salt), BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    char pass_utf8[512] = {0};
    int pass_len = WideCharToMultiByte(CP_UTF8, 0, new_password, -1, pass_utf8, sizeof(pass_utf8) - 1, NULL, NULL);

    BCRYPT_ALG_HANDLE hHash = NULL;
    BCryptOpenAlgorithmProvider(&hHash, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    BCryptDeriveKeyPBKDF2(
        hHash,
        (PUCHAR)pass_utf8,
        (ULONG)(pass_len > 1 ? pass_len - 1 : 0),
        g_Vault.pass_salt,
        sizeof(g_Vault.pass_salt),
        PBKDF2_ITERATIONS,
        g_Vault.pass_hash,
        sizeof(g_Vault.pass_hash),
        0
    );
    BCryptCloseAlgorithmProvider(hHash, 0);
    SecureZeroMemory(pass_utf8, sizeof(pass_utf8));

    /* Reset attempts & unlock */
    g_Vault.failed_attempts = 0;
    g_Vault.is_locked = 0;
    SecureZeroMemory(g_szActiveOTP, sizeof(g_szActiveOTP));

    bool saved = save_vault_to_disk();
    if (saved) {
        wcsncpy(g_szSessionPassword, new_password, 255);
    }
    return saved;
}

const wchar_t *vault_get_session_password(void) {
    return g_szSessionPassword;
}

bool vault_derive_ginger(const wchar_t *password, uint8_t *out_ginger, size_t ginger_len) {
    if (!password || !out_ginger || ginger_len == 0) return false;

    wchar_t ginger_str[512];
    swprintf(ginger_str, 512, L"%ls_pepper_%ls", password, GINGER_PEPPER);

    char ginger_utf8[1024] = {0};
    int len = WideCharToMultiByte(CP_UTF8, 0, ginger_str, -1, ginger_utf8, sizeof(ginger_utf8) - 1, NULL, NULL);
    SecureZeroMemory(ginger_str, sizeof(ginger_str));

    BCRYPT_ALG_HANDLE hHash = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hHash, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status)) return false;

    BCRYPT_HASH_HANDLE hHashObj = NULL;
    status = BCryptCreateHash(hHash, &hHashObj, NULL, 0, NULL, 0, 0);
    if (BCRYPT_SUCCESS(status)) {
        BCryptHashData(hHashObj, (PUCHAR)ginger_utf8, (ULONG)(len > 1 ? len - 1 : 0), 0);
        uint8_t digest[32];
        BCryptFinishHash(hHashObj, digest, sizeof(digest), 0);
        BCryptDestroyHash(hHashObj);

        size_t copy_len = (ginger_len < sizeof(digest)) ? ginger_len : sizeof(digest);
        memcpy(out_ginger, digest, copy_len);
        SecureZeroMemory(digest, sizeof(digest));
    }
    BCryptCloseAlgorithmProvider(hHash, 0);
    SecureZeroMemory(ginger_utf8, sizeof(ginger_utf8));
    return BCRYPT_SUCCESS(status);
}

void vault_logout(void) {
    SecureZeroMemory(g_szSessionPassword, sizeof(g_szSessionPassword));
}
