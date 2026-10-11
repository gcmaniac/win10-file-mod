#include "common.h"
#include "crypto.h"
#include "vault.h"
#include "service.h"
#include "ui.h"
#include <shellapi.h>

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow
) {
    (void)hPrevInstance;
    (void)pCmdLine;

    /* Parse Command-Line Arguments */
    int nArgs = 0;
    LPWSTR *szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (szArgList && nArgs > 1) {
        if (_wcsicmp(szArgList[1], L"--service") == 0) {
            LocalFree(szArgList);
            return service_run_daemon(hInstance);
        } else if (_wcsicmp(szArgList[1], L"--auto-decrypt") == 0 && nArgs > 2) {
            bool ok = service_auto_decrypt_single_file(szArgList[2], true);
            LocalFree(szArgList);
            return ok ? 0 : 1;
        } else if (_wcsicmp(szArgList[1], L"--auto-decrypt-folder") == 0 && nArgs > 2) {
            int count = service_auto_decrypt_folder(szArgList[2]);
            LocalFree(szArgList);
            return (count >= 0) ? 0 : 1;
        } else if (_wcsicmp(szArgList[1], L"--install-service") == 0) {
            service_install();
            LocalFree(szArgList);
            return 0;
        } else if (_wcsicmp(szArgList[1], L"--uninstall-service") == 0) {
            service_uninstall();
            LocalFree(szArgList);
            return 0;
        } else if (_wcsicmp(szArgList[1], L"--start-service") == 0) {
            service_start_process();
            LocalFree(szArgList);
            return 0;
        } else if (_wcsicmp(szArgList[1], L"--stop-service") == 0) {
            service_stop_process();
            LocalFree(szArgList);
            return 0;
        }
        LocalFree(szArgList);
    }

    /* If an instance is already running (e.g. hidden in taskbar/tray), restore and focus it */
    HWND hExisting = FindWindowW(L"Win10FileModMainClass", NULL);
    if (hExisting) {
        ShowWindow(hExisting, SW_SHOW);
        ShowWindow(hExisting, SW_RESTORE);
        SetForegroundWindow(hExisting);
        SendMessageW(hExisting, WM_COMMAND, MAKEWPARAM(ID_MAINTRAY_RESTORE, 0), 0);
        return 0;
    }

    /* Initialize Windows CNG Cryptography */
    if (!crypto_init()) {
        MessageBoxW(
            NULL,
            L"Gagal menginisialisasi modul kriptografi Windows CNG (bcrypt.dll)!\n"
            L"Pastikan sistem operasi Anda adalah Windows 7 SP1, 10, atau yang lebih baru.",
            L"Fatal Error",
            MB_ICONERROR | MB_OK
        );
        return 1;
    }

    /* Initialize security vault and auto-encrypt all decrypted files upon startup */
    vault_init();
    tracker_init();
    tracker_encrypt_all_decrypted_files();

    /* Initialize Win32 UI and Common Controls */
    if (!ui_init(hInstance)) {
        MessageBoxW(
            NULL,
            L"Gagal mendaftarkan kelas jendela Win32!",
            L"Fatal Error",
            MB_ICONERROR | MB_OK
        );
        crypto_cleanup();
        return 1;
    }

    /* Create and show main application window */
    HWND hWnd = ui_create_main_window(hInstance, nCmdShow);
    if (!hWnd) {
        MessageBoxW(
            NULL,
            L"Gagal membuat jendela aplikasi utama!",
            L"Fatal Error",
            MB_ICONERROR | MB_OK
        );
        crypto_cleanup();
        return 1;
    }

    /* Win32 Message Loop with Tab index navigation and Enter-to-submit support */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            HWND hFocus = GetFocus();
            if (hFocus) {
                int ctrlId = GetDlgCtrlID(hFocus);
                if (ctrlId == IDC_LOGIN_EDIT_PASS) {
                    SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_LOGIN_BTN_MASUK, BN_CLICKED), (LPARAM)hFocus);
                    continue;
                } else if (ctrlId == IDC_SETUP_EDIT_EMAIL) {
                    SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_SETUP_BTN_SAVE, BN_CLICKED), (LPARAM)hFocus);
                    continue;
                }
            }
        }
        if (!IsDialogMessageW(hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    /* Cleanup cryptographic handles before termination */
    crypto_cleanup();

    return (int)msg.wParam;
}
