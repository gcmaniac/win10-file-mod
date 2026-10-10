#include "common.h"
#include "crypto.h"
#include "ui.h"

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow
) {
    (void)hPrevInstance;
    (void)pCmdLine;

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

    /* Win32 Message Loop with Tab index navigation support */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!IsDialogMessageW(hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    /* Cleanup cryptographic handles before termination */
    crypto_cleanup();

    return (int)msg.wParam;
}
