#ifndef UI_H
#define UI_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Control IDs used across UI and message loop */
#define IDC_SETUP_EDIT_EMAIL    1108
#define IDC_SETUP_BTN_SAVE      1109
#define IDC_LOGIN_EDIT_PASS     1204
#define IDC_LOGIN_BTN_MASUK     1206
#define ID_MAINTRAY_RESTORE     6001
#define ID_MAINTRAY_EXIT        6002

/* Initialize Common Controls and register UI window class */
bool ui_init(HINSTANCE hInstance);

/* Create and display the main application window */
HWND ui_create_main_window(HINSTANCE hInstance, int nCmdShow);

/* Window message procedure for the main window */
LRESULT CALLBACK ui_wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
