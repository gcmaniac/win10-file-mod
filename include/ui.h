#ifndef UI_H
#define UI_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

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
