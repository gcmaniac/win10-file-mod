#include "ui.h"
#include "crypto.h"
#include "scanner.h"
#include "shredder.h"
#include "renamer.h"
#include <shlobj.h>

/* Menu IDs */
#define IDM_FILE_OPEN           2001
#define IDM_FILE_EXIT           2002
#define IDM_VIEW_ENCRYPT        2003
#define IDM_VIEW_RENAME         2004
#define IDM_HELP_GUIDE          2005
#define IDM_HELP_ABOUT          2006

/* Tab Control ID */
#define IDC_MAIN_TAB            3000

/* Page 1 (Encryption) IDs */
#define IDC_GRP_FOLDER          1001
#define IDC_EDIT_FOLDER         1002
#define IDC_BTN_BROWSE          1003
#define IDC_CHK_RECURSIVE       1004
#define IDC_CHK_REMEMBER        1005

#define IDC_GRP_MODE            1015
#define IDC_RADIO_ENCRYPT       1016
#define IDC_RADIO_DECRYPT       1017

#define IDC_GRP_CRYPTO          1020
#define IDC_LBL_CIPHER          1021
#define IDC_COMBO_CIPHER        1022
#define IDC_LBL_PASS            1023
#define IDC_EDIT_PASS           1024
#define IDC_LBL_PASS_CONFIRM    1025
#define IDC_EDIT_PASS_CONFIRM   1026
#define IDC_CHK_SHOW_PASS       1027
#define IDC_CHK_CUSTOM_SALT     1028
#define IDC_EDIT_SALT           1029

#define IDC_BTN_START           1030
#define IDC_BTN_CANCEL          1031
#define IDC_PROGRESS            1032
#define IDC_STATIC_STATUS       1033
#define IDC_LOG_BOX             1034

/* Page 2 (Bulk Renamer) IDs */
#define IDC_REN_GRP_FOLDER      3101
#define IDC_REN_EDIT_FOLDER     3102
#define IDC_REN_BTN_BROWSE      3103
#define IDC_REN_CHK_RECURSIVE   3104
#define IDC_REN_CHK_REMEMBER    3105

#define IDC_REN_GRP_METHOD      3106
#define IDC_REN_LBL_METHOD      3107
#define IDC_REN_COMBO_METHOD    3108
#define IDC_REN_LBL_P1          3109
#define IDC_REN_EDIT_P1         3110
#define IDC_REN_LBL_P2          3111
#define IDC_REN_EDIT_P2         3112
#define IDC_REN_COMBO_P3        3113

/* Dedicated Number Step Controls */
#define IDC_REN_BTN_DEC1        3114
#define IDC_REN_EDIT_NUM1       3115
#define IDC_REN_BTN_INC1        3116
#define IDC_REN_SPIN1           3117

#define IDC_REN_BTN_DEC2        3118
#define IDC_REN_EDIT_NUM2       3119
#define IDC_REN_BTN_INC2        3120
#define IDC_REN_SPIN2           3121

#define IDC_REN_LISTVIEW        3122
#define IDC_REN_BTN_START       3123
#define IDC_REN_BTN_UNDO        3124
#define IDC_REN_BTN_PREVIEW     3125
#define IDC_REN_STATIC_STATUS   3126

/* Custom Thread Messages */
#define WM_APP_LOG_MSG          (WM_APP + 1)
#define WM_APP_PROGRESS         (WM_APP + 2)
#define WM_APP_FINISHED         (WM_APP + 3)
#define WM_APP_REN_FINISHED     (WM_APP + 5)

/* Global Window Handles */
static HWND g_hWndMain = NULL;
static HWND g_hTab = NULL;

/* Page 1 Handles */
static HWND g_hEncCtrls[64] = {0};
static int  g_nEncCtrls = 0;
static HWND g_hEditFolder = NULL;
static HWND g_hBtnBrowse = NULL;
static HWND g_hChkRecursive = NULL;
static HWND g_hChkRemember1 = NULL;
static HWND g_hRadioEncrypt = NULL;
static HWND g_hRadioDecrypt = NULL;
static HWND g_hComboCipher = NULL;
static HWND g_hEditPass = NULL;
static HWND g_hEditPassConfirm = NULL;
static HWND g_hChkShowPass = NULL;
static HWND g_hChkCustomSalt = NULL;
static HWND g_hEditSalt = NULL;
static HWND g_hBtnStart = NULL;
static HWND g_hBtnCancel = NULL;
static HWND g_hProgress = NULL;
static HWND g_hStaticStatus = NULL;
static HWND g_hLogBox = NULL;

/* Page 2 Handles */
static HWND g_hRenCtrls[64] = {0};
static int  g_nRenCtrls = 0;
static HWND g_hRenEditFolder = NULL;
static HWND g_hRenBtnBrowse = NULL;
static HWND g_hRenChkRecursive = NULL;
static HWND g_hRenChkRemember2 = NULL;
static HWND g_hRenComboMethod = NULL;
static HWND g_hRenLblP1 = NULL;
static HWND g_hRenEditP1 = NULL;
static HWND g_hRenLblP2 = NULL;
static HWND g_hRenEditP2 = NULL;
static HWND g_hRenComboP3 = NULL;

/* Number Step Buttons & Edits */
static HWND g_hRenBtnDec1 = NULL;
static HWND g_hRenEditNum1 = NULL;
static HWND g_hRenBtnInc1 = NULL;
static HWND g_hRenSpin1 = NULL;

static HWND g_hRenBtnDec2 = NULL;
static HWND g_hRenEditNum2 = NULL;
static HWND g_hRenBtnInc2 = NULL;
static HWND g_hRenSpin2 = NULL;

static HWND g_hRenList = NULL;
static HWND g_hRenBtnStart = NULL;
static HWND g_hRenBtnUndo = NULL;
static HWND g_hRenBtnPreview = NULL;
static HWND g_hRenStaticStatus = NULL;

static HFONT g_hFont = NULL;
static HFONT g_hFontBold = NULL;
static HANDLE g_hWorkerThread = NULL;
static volatile bool g_bCancelRequested = false;
static bool g_bIsWorking = false;

/* Settings State */
static bool g_bRememberFolder = true;
static wchar_t g_szLastFolder[MAX_PATH * 2] = {0};

/* Renamer State */
static RenameItemList *g_pRenameList = NULL;
static UndoHistory    *g_pUndoHistory = NULL;

typedef struct {
    JobConfig config;
    HWND hWnd;
} WorkerParams;

static void set_font(HWND hCtrl, HFONT hFont) {
    if (hCtrl && hFont) {
        SendMessage(hCtrl, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
}

static void register_ctrl(HWND *array, int *count, int max_cap, HWND hCtrl) {
    if (hCtrl && *count < max_cap) {
        array[*count] = hCtrl;
        (*count)++;
    }
}

/* ========================================================================= */
/* Settings Persistence (Portable settings.ini)                             */
/* ========================================================================= */

static void get_settings_path(wchar_t *out_path, size_t max_len) {
    GetModuleFileNameW(NULL, out_path, (DWORD)max_len);
    wchar_t *dot = wcsrchr(out_path, L'.');
    if (dot) {
        wcscpy(dot, L"_settings.ini");
    } else {
        wcsncat(out_path, L"_settings.ini", max_len - wcslen(out_path) - 1);
    }
}

static void load_settings(void) {
    wchar_t ini[MAX_PATH * 2] = {0};
    get_settings_path(ini, MAX_PATH * 2);

    int remember = GetPrivateProfileIntW(L"Settings", L"RememberFolder", 1, ini);
    g_bRememberFolder = (remember != 0);

    if (g_bRememberFolder) {
        GetPrivateProfileStringW(L"Settings", L"LastFolder", L"", g_szLastFolder, MAX_PATH * 2, ini);
    } else {
        g_szLastFolder[0] = L'\0';
    }
}

static void save_settings(const wchar_t *last_folder) {
    wchar_t ini[MAX_PATH * 2] = {0};
    get_settings_path(ini, MAX_PATH * 2);

    WritePrivateProfileStringW(L"Settings", L"RememberFolder", g_bRememberFolder ? L"1" : L"0", ini);
    if (g_bRememberFolder && last_folder && last_folder[0] != L'\0') {
        wcsncpy(g_szLastFolder, last_folder, MAX_PATH * 2 - 1);
        WritePrivateProfileStringW(L"Settings", L"LastFolder", last_folder, ini);
    } else if (!g_bRememberFolder) {
        g_szLastFolder[0] = L'\0';
        WritePrivateProfileStringW(L"Settings", L"LastFolder", L"", ini);
    }
}

static void sanitize_path(const wchar_t *input, wchar_t *output, size_t max_out) {
    const wchar_t *start = input;
    while (*start == L' ' || *start == L'\t' || *start == L'\"') start++;
    wcsncpy(output, start, max_out - 1);
    output[max_out - 1] = L'\0';

    size_t len = wcslen(output);
    while (len > 0 && (output[len - 1] == L' ' || output[len - 1] == L'\t' || output[len - 1] == L'\"')) {
        output[--len] = L'\0';
    }
    if (len > 3 && (output[len - 1] == L'\\' || output[len - 1] == L'/')) {
        output[--len] = L'\0';
    }
}

static void update_method_ui_labels(int method_idx);
static void trigger_live_preview(void);
static void scan_renamer_folder(HWND hWnd);

static void show_page(int page_idx) {
    for (int i = 0; i < g_nEncCtrls; i++) {
        if (g_hEncCtrls[i]) ShowWindow(g_hEncCtrls[i], (page_idx == 0) ? SW_SHOW : SW_HIDE);
    }
    for (int i = 0; i < g_nRenCtrls; i++) {
        if (g_hRenCtrls[i]) ShowWindow(g_hRenCtrls[i], (page_idx == 1) ? SW_SHOW : SW_HIDE);
    }

    if (page_idx == 1) {
        wchar_t f1[MAX_PATH * 2] = {0};
        wchar_t f2[MAX_PATH * 2] = {0};
        GetWindowTextW(g_hEditFolder, f1, MAX_PATH * 2);
        GetWindowTextW(g_hRenEditFolder, f2, MAX_PATH * 2);
        if (wcslen(f2) == 0 && wcslen(f1) > 0) {
            SetWindowTextW(g_hRenEditFolder, f1);
            scan_renamer_folder(g_hWndMain);
        }
        update_method_ui_labels((int)SendMessage(g_hRenComboMethod, CB_GETCURSEL, 0, 0));
        trigger_live_preview();
    } else if (page_idx == 0) {
        wchar_t f1[MAX_PATH * 2] = {0};
        wchar_t f2[MAX_PATH * 2] = {0};
        GetWindowTextW(g_hEditFolder, f1, MAX_PATH * 2);
        GetWindowTextW(g_hRenEditFolder, f2, MAX_PATH * 2);
        if (wcslen(f1) == 0 && wcslen(f2) > 0) {
            SetWindowTextW(g_hEditFolder, f2);
        }
    }

    InvalidateRect(g_hWndMain, NULL, TRUE);
    UpdateWindow(g_hWndMain);
}

static void append_log(HWND hLog, const wchar_t *text) {
    if (!hLog || !text) return;
    int len = GetWindowTextLengthW(hLog);
    SendMessageW(hLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hLog, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(hLog, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    SendMessageW(hLog, EM_SCROLLCARET, 0, 0);
}

static void browse_folder_to_edit(HWND hWnd, HWND hEdit) {
    BROWSEINFOW bi = {0};
    bi.hwndOwner = hWnd;
    bi.lpszTitle = L"Pilih folder target:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH * 2] = {0};
        if (SHGetPathFromIDListW(pidl, path)) {
            SetWindowTextW(hEdit, path);
            save_settings(path);

            if (hEdit == g_hRenEditFolder) {
                scan_renamer_folder(hWnd);
            }
        }
        CoTaskMemFree(pidl);
    }
}

/* ========================================================================= */
/* Bulk Renamer Helper Functions                                             */
/* ========================================================================= */

static void get_rename_config_from_ui(RenameConfig *cfg) {
    SecureZeroMemory(cfg, sizeof(RenameConfig));
    int method_idx = (int)SendMessage(g_hRenComboMethod, CB_GETCURSEL, 0, 0);

    wchar_t p1[128] = {0};
    wchar_t p2[128] = {0};
    wchar_t n1[32] = {0};
    wchar_t n2[32] = {0};
    GetWindowTextW(g_hRenEditP1, p1, 128);
    GetWindowTextW(g_hRenEditP2, p2, 128);
    GetWindowTextW(g_hRenEditNum1, n1, 31);
    GetWindowTextW(g_hRenEditNum2, n2, 31);

    switch (method_idx) {
        case 0: /* Replace */
            cfg->method = RENAME_METHOD_REPLACE;
            wcsncpy(cfg->find_text, p1, 127);
            wcsncpy(cfg->replace_text, p2, 127);
            cfg->case_sensitive = false;
            break;
        case 1: /* Add Prefix / Suffix */
            cfg->method = RENAME_METHOD_ADD;
            wcsncpy(cfg->add_text, p1, 127);
            cfg->add_pos = (SendMessage(g_hRenComboP3, CB_GETCURSEL, 0, 0) == 1) ? ADD_POS_SUFFIX : ADD_POS_PREFIX;
            break;
        case 2: /* Pattern Tag */
            cfg->method = RENAME_METHOD_PATTERN;
            wcsncpy(cfg->pattern, p1, 255);
            cfg->start_number = _wtoi(n2);
            if (cfg->start_number <= 0) cfg->start_number = 1;
            cfg->number_step = 1;
            break;
        case 3: /* Case Change */
            cfg->method = RENAME_METHOD_CASE;
            cfg->case_type = (CaseChangeType)SendMessage(g_hRenComboP3, CB_GETCURSEL, 0, 0);
            cfg->apply_to_extension = false;
            break;
        case 4: /* Trim */
            cfg->method = RENAME_METHOD_TRIM;
            cfg->trim_start_count = _wtoi(n1);
            cfg->trim_end_count = _wtoi(n2);
            cfg->trim_whitespace = true;
            break;
        default:
            cfg->method = RENAME_METHOD_REPLACE;
            break;
    }
}

static void step_number_edit(HWND hEdit, int delta, int min_val, int max_val) {
    wchar_t txt[32] = {0};
    GetWindowTextW(hEdit, txt, 31);
    int val = _wtoi(txt);
    val += delta;
    if (val < min_val) val = min_val;
    if (val > max_val) val = max_val;
    wchar_t new_txt[32];
    swprintf(new_txt, 32, L"%d", val);
    SetWindowTextW(hEdit, new_txt);
    trigger_live_preview();
}

static void update_method_ui_labels(int method_idx) {
    /* Hide all variable controls first */
    ShowWindow(g_hRenLblP1, SW_HIDE);
    ShowWindow(g_hRenEditP1, SW_HIDE);
    ShowWindow(g_hRenLblP2, SW_HIDE);
    ShowWindow(g_hRenEditP2, SW_HIDE);
    ShowWindow(g_hRenComboP3, SW_HIDE);

    ShowWindow(g_hRenBtnDec1, SW_HIDE);
    ShowWindow(g_hRenEditNum1, SW_HIDE);
    ShowWindow(g_hRenBtnInc1, SW_HIDE);
    ShowWindow(g_hRenSpin1, SW_HIDE);

    ShowWindow(g_hRenBtnDec2, SW_HIDE);
    ShowWindow(g_hRenEditNum2, SW_HIDE);
    ShowWindow(g_hRenBtnInc2, SW_HIDE);
    ShowWindow(g_hRenSpin2, SW_HIDE);

    switch (method_idx) {
        case 0: /* Replace */
            SetWindowTextW(g_hRenLblP1, L"Cari Teks (Find):");
            SetWindowTextW(g_hRenLblP2, L"Ganti Dengan (Replace):");
            SetWindowPos(g_hRenLblP1, NULL, 300, 140, 110, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenEditP1, NULL, 415, 137, 150, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenLblP2, NULL, 575, 140, 135, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenEditP2, NULL, 715, 137, 95, 24, SWP_NOZORDER);

            ShowWindow(g_hRenLblP1, SW_SHOW);
            ShowWindow(g_hRenEditP1, SW_SHOW);
            ShowWindow(g_hRenLblP2, SW_SHOW);
            ShowWindow(g_hRenEditP2, SW_SHOW);
            break;

        case 1: /* Add */
            SetWindowTextW(g_hRenLblP1, L"Teks Tambahan:");
            SetWindowTextW(g_hRenLblP2, L"Posisi:");
            SetWindowPos(g_hRenLblP1, NULL, 300, 140, 100, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenEditP1, NULL, 405, 137, 180, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenLblP2, NULL, 595, 140, 60, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenComboP3, NULL, 660, 137, 150, 120, SWP_NOZORDER);

            SendMessage(g_hRenComboP3, CB_RESETCONTENT, 0, 0);
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)L"Awalan (Prefix)");
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)L"Akhiran (Suffix)");
            SendMessage(g_hRenComboP3, CB_SETCURSEL, 0, 0);

            ShowWindow(g_hRenLblP1, SW_SHOW);
            ShowWindow(g_hRenEditP1, SW_SHOW);
            ShowWindow(g_hRenLblP2, SW_SHOW);
            ShowWindow(g_hRenComboP3, SW_SHOW);
            break;

        case 2: /* Pattern */
            SetWindowTextW(g_hRenLblP1, L"Format Pola:");
            SetWindowTextW(g_hRenLblP2, L"Mulai Nomor:");
            SetWindowPos(g_hRenLblP1, NULL, 300, 140, 85, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenEditP1, NULL, 390, 137, 185, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenLblP2, NULL, 585, 140, 85, 20, SWP_NOZORDER);

            SetWindowPos(g_hRenBtnDec2, NULL, 675, 137, 28, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenEditNum2, NULL, 706, 137, 45, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnInc2, NULL, 754, 137, 28, 24, SWP_NOZORDER);

            if (GetWindowTextLengthW(g_hRenEditP1) == 0) {
                SetWindowTextW(g_hRenEditP1, L"Map_<inc:01>");
            }
            if (GetWindowTextLengthW(g_hRenEditNum2) == 0) {
                SetWindowTextW(g_hRenEditNum2, L"1");
            }

            ShowWindow(g_hRenLblP1, SW_SHOW);
            ShowWindow(g_hRenEditP1, SW_SHOW);
            ShowWindow(g_hRenLblP2, SW_SHOW);
            ShowWindow(g_hRenBtnDec2, SW_SHOW);
            ShowWindow(g_hRenEditNum2, SW_SHOW);
            ShowWindow(g_hRenBtnInc2, SW_SHOW);
            ShowWindow(g_hRenSpin2, SW_SHOW);
            break;

        case 3: /* Case */
            SetWindowTextW(g_hRenLblP1, L"Pilihan Huruf:");
            SetWindowPos(g_hRenLblP1, NULL, 300, 140, 100, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenComboP3, NULL, 405, 137, 250, 120, SWP_NOZORDER);

            SendMessage(g_hRenComboP3, CB_RESETCONTENT, 0, 0);
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)L"huruf kecil (lowercase)");
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)L"HURUF BESAR (UPPERCASE)");
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)L"Huruf Judul (Title Case)");
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)L"Huruf awal besar");
            SendMessage(g_hRenComboP3, CB_SETCURSEL, 2, 0);

            ShowWindow(g_hRenLblP1, SW_SHOW);
            ShowWindow(g_hRenComboP3, SW_SHOW);
            break;

        case 4: /* Trim */
            SetWindowTextW(g_hRenLblP1, L"Pangkas Depan:");
            SetWindowTextW(g_hRenLblP2, L"Pangkas Belakang:");
            SetWindowPos(g_hRenLblP1, NULL, 300, 140, 105, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnDec1, NULL, 410, 137, 28, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenEditNum1, NULL, 441, 137, 40, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnInc1, NULL, 484, 137, 28, 24, SWP_NOZORDER);

            SetWindowPos(g_hRenLblP2, NULL, 525, 140, 120, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnDec2, NULL, 650, 137, 28, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenEditNum2, NULL, 681, 137, 40, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnInc2, NULL, 724, 137, 28, 24, SWP_NOZORDER);

            if (GetWindowTextLengthW(g_hRenEditNum1) == 0) {
                SetWindowTextW(g_hRenEditNum1, L"0");
            }
            if (GetWindowTextLengthW(g_hRenEditNum2) == 0) {
                SetWindowTextW(g_hRenEditNum2, L"0");
            }

            ShowWindow(g_hRenLblP1, SW_SHOW);
            ShowWindow(g_hRenBtnDec1, SW_SHOW);
            ShowWindow(g_hRenEditNum1, SW_SHOW);
            ShowWindow(g_hRenBtnInc1, SW_SHOW);
            ShowWindow(g_hRenSpin1, SW_SHOW);

            ShowWindow(g_hRenLblP2, SW_SHOW);
            ShowWindow(g_hRenBtnDec2, SW_SHOW);
            ShowWindow(g_hRenEditNum2, SW_SHOW);
            ShowWindow(g_hRenBtnInc2, SW_SHOW);
            ShowWindow(g_hRenSpin2, SW_SHOW);
            break;
    }
}

static void populate_renamer_listview(void) {
    if (!g_pRenameList || !g_hRenList) return;

    SendMessage(g_hRenList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_hRenList);

    for (size_t i = 0; i < g_pRenameList->count; i++) {
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;

        wchar_t idx_str[16];
        swprintf(idx_str, 16, L"%zu", i + 1);
        lvi.pszText = idx_str;
        ListView_InsertItem(g_hRenList, &lvi);

        ListView_SetItemText(g_hRenList, (int)i, 1, g_pRenameList->items[i].orig_name);
        ListView_SetItemText(g_hRenList, (int)i, 2, g_pRenameList->items[i].new_name);

        wchar_t sz_str[32];
        double mb = (double)g_pRenameList->items[i].file_size / (1024.0 * 1024.0);
        if (mb >= 1.0) {
            swprintf(sz_str, 32, L"%.2f MB", mb);
        } else {
            swprintf(sz_str, 32, L"%.1f KB", (double)g_pRenameList->items[i].file_size / 1024.0);
        }
        ListView_SetItemText(g_hRenList, (int)i, 3, sz_str);

        ListView_SetItemText(g_hRenList, (int)i, 4, g_pRenameList->items[i].status_text);
    }

    SendMessage(g_hRenList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hRenList, NULL, TRUE);

    wchar_t status[256];
    swprintf(status, 256, L"Terdeteksi %zu berkas. Siap pratinjau / ubah nama.", g_pRenameList->count);
    SetWindowTextW(g_hRenStaticStatus, status);
}

static void trigger_live_preview(void) {
    if (!g_pRenameList || g_pRenameList->count == 0) return;

    RenameConfig cfg;
    get_rename_config_from_ui(&cfg);
    renamer_compute_preview(g_pRenameList, &cfg);

    /* Update listview items in-place */
    for (size_t i = 0; i < g_pRenameList->count; i++) {
        ListView_SetItemText(g_hRenList, (int)i, 2, g_pRenameList->items[i].new_name);
        ListView_SetItemText(g_hRenList, (int)i, 4, g_pRenameList->items[i].status_text);
    }
}

static void scan_renamer_folder(HWND hWnd) {
    (void)hWnd;
    wchar_t raw[MAX_PATH * 2] = {0};
    GetWindowTextW(g_hRenEditFolder, raw, MAX_PATH * 2);

    wchar_t folder[MAX_PATH * 2] = {0};
    sanitize_path(raw, folder, MAX_PATH * 2);
    SetWindowTextW(g_hRenEditFolder, folder);

    if (wcslen(folder) == 0) {
        return;
    }

    DWORD attr = GetFileAttributesW(folder);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return;
    }

    save_settings(folder);

    bool recursive = (SendMessage(g_hRenChkRecursive, BM_GETCHECK, 0, 0) == BST_CHECKED);
    renamer_scan_folder(g_pRenameList, folder, recursive);

    trigger_live_preview();
    populate_renamer_listview();
}

static DWORD WINAPI renamer_worker_thread(LPVOID lpParam) {
    HWND hWnd = (HWND)lpParam;
    if (!g_pRenameList || g_pRenameList->count == 0) {
        PostMessageW(hWnd, WM_APP_REN_FINISHED, 0, 0);
        return 0;
    }

    size_t count = g_pRenameList->count;
    renamer_execute_batch(g_pRenameList, g_pUndoHistory, NULL, NULL, &g_bCancelRequested);

    PostMessageW(hWnd, WM_APP_REN_FINISHED, (WPARAM)count, 0);
    return 0;
}

static void start_batch_rename(HWND hWnd) {
    if (g_bIsWorking) return;
    if (!g_pRenameList || g_pRenameList->count == 0) {
        MessageBoxW(hWnd, L"Daftar berkas masih kosong! Pilih folder yang berisi berkas.", L"Peringatan", MB_OK | MB_ICONWARNING);
        return;
    }

    bool has_any_collision = false;
    for (size_t i = 0; i < g_pRenameList->count; i++) {
        if (g_pRenameList->items[i].has_collision) {
            has_any_collision = true;
            break;
        }
    }

    if (has_any_collision) {
        int r = MessageBoxW(hWnd,
            L"Terdapat berkas dengan konflik nama duplikat!\nBerkas yang berkonflik akan dilewati.\nLanjutkan proses?",
            L"Konfirmasi Konflik", MB_YESNO | MB_ICONWARNING);
        if (r != IDYES) return;
    } else {
        int r = MessageBoxW(hWnd,
            L"Apakah Anda yakin ingin menjalankan perubahan nama untuk seluruh berkas di daftar?",
            L"Konfirmasi Batch Rename", MB_YESNO | MB_ICONQUESTION);
        if (r != IDYES) return;
    }

    g_bIsWorking = true;
    g_bCancelRequested = false;
    EnableWindow(g_hRenBtnStart, FALSE);
    EnableWindow(g_hRenBtnUndo, FALSE);

    SetWindowTextW(g_hRenStaticStatus, L"Sedang memproses batch rename...");
    g_hWorkerThread = CreateThread(NULL, 0, renamer_worker_thread, (LPVOID)hWnd, 0, NULL);
}

static void undo_batch_rename(HWND hWnd) {
    if (g_bIsWorking) return;
    if (!g_pUndoHistory || g_pUndoHistory->count == 0) {
        MessageBoxW(hWnd, L"Tidak ada riwayat rename sebelumnya untuk di-undo.", L"Informasi", MB_OK | MB_ICONINFORMATION);
        return;
    }

    int r = MessageBoxW(hWnd,
        L"Apakah Anda ingin mengembalikan nama seluruh berkas ke nama sebelumnya (Undo)?",
        L"Konfirmasi Undo", MB_YESNO | MB_ICONQUESTION);
    if (r != IDYES) return;

    renamer_undo_batch(g_pUndoHistory, NULL, NULL, NULL);

    scan_renamer_folder(hWnd);
    MessageBoxW(hWnd, L"Berhasil mengembalikan nama seluruh berkas ke semula!", L"Sukses", MB_OK | MB_ICONINFORMATION);
}

/* ========================================================================= */
/* Encryption Worker Thread                                                  */
/* ========================================================================= */

static DWORD WINAPI encrypt_worker_thread(LPVOID lpParam) {
    WorkerParams *wp = (WorkerParams *)lpParam;
    HWND hWnd = wp->hWnd;
    JobConfig cfg = wp->config;

    FileList *files = scanner_create();
    if (!files) {
        PostMessageW(hWnd, WM_APP_FINISHED, 0, 0);
        free(wp);
        return 0;
    }

    wchar_t msg[512];
    swprintf(msg, 512, L"[*] Memulai pemindaian berkas di: %ls", cfg.folder_path);
    PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(msg));

    scanner_scan(cfg.folder_path, cfg.recursive, cfg.op_mode, files);

    if (files->count == 0) {
        PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(L"[!] Tidak ditemukan berkas yang sesuai kriteria."));
        PostMessageW(hWnd, WM_APP_FINISHED, 0, 0);
        scanner_free(files);
        free(wp);
        return 0;
    }

    double totalMB = (double)files->total_bytes / (1024.0 * 1024.0);
    swprintf(msg, 512, L"[*] Ditemukan %zu berkas (Total: %.2f MB). Memulai proses...", files->count, totalMB);
    PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(msg));

    size_t success_count = 0;
    size_t fail_count = 0;

    for (size_t i = 0; i < files->count; i++) {
        if (g_bCancelRequested) {
            PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(L"[!] Proses dibatalkan oleh pengguna."));
            break;
        }

        const wchar_t *src_file = files->paths[i];
        PostMessageW(hWnd, WM_APP_PROGRESS, (WPARAM)(i + 1), (LPARAM)files->count);

        if (cfg.op_mode == OP_MODE_ENCRYPT_AND_SHRED) {
            uint8_t file_salt[SALT_SIZE];
            crypto_generate_random(file_salt, sizeof(file_salt));

            uint8_t derived_key[KEY_SIZE_AES256];
            crypto_derive_key(
                cfg.password,
                cfg.use_custom_salt ? cfg.custom_salt : NULL,
                file_salt,
                sizeof(file_salt),
                derived_key
            );

            wchar_t tmp_enc_path[MAX_PATH * 2];
            wchar_t final_enc_path[MAX_PATH * 2];
            swprintf(tmp_enc_path, MAX_PATH * 2, L"%ls%ls", src_file, TMP_FILE_EXTENSION);
            swprintf(final_enc_path, MAX_PATH * 2, L"%ls%ls", src_file, ENC_FILE_EXTENSION);

            bool enc_ok = crypto_encrypt_file(
                src_file,
                tmp_enc_path,
                derived_key,
                file_salt,
                cfg.cipher_id,
                NULL,
                NULL,
                &g_bCancelRequested
            );

            SecureZeroMemory(derived_key, sizeof(derived_key));

            if (enc_ok && !g_bCancelRequested) {
                if (shredder_commit_and_wipe_original(src_file, tmp_enc_path, final_enc_path)) {
                    success_count++;
                    const wchar_t *fn = wcsrchr(src_file, L'\\');
                    fn = fn ? fn + 1 : src_file;
                    swprintf(msg, 512, L"[ENCRYPTED & SHREDDED] %ls", fn);
                    PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(msg));
                } else {
                    fail_count++;
                    swprintf(msg, 512, L"[ERROR SHREDDING] Gagal menghapus file asli: %ls", src_file);
                    PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(msg));
                }
            } else {
                fail_count++;
                swprintf(msg, 512, L"[ERROR ENCRYPT] Gagal mengenkripsi: %ls", src_file);
                PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(msg));
            }
        } else {
            /* DECRYPT MODE */
            HANDLE hSrc = CreateFileW(src_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
            if (hSrc == INVALID_HANDLE_VALUE) {
                fail_count++;
                continue;
            }

            FileHeader header;
            DWORD br = 0;
            bool read_ok = ReadFile(hSrc, &header, sizeof(header), &br, NULL) && (br == sizeof(header));
            CloseHandle(hSrc);

            if (!read_ok || memcmp(header.magic, HEADER_MAGIC, 4) != 0) {
                fail_count++;
                swprintf(msg, 512, L"[INVALID FORMAT] Bukan berkas terenkripsi valid: %ls", src_file);
                PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(msg));
                continue;
            }

            uint8_t derived_key[KEY_SIZE_AES256];
            crypto_derive_key(
                cfg.password,
                cfg.use_custom_salt ? cfg.custom_salt : NULL,
                header.salt,
                sizeof(header.salt),
                derived_key
            );

            wchar_t out_decrypted[MAX_PATH * 2] = {0};
            bool dec_ok = crypto_decrypt_file(
                src_file,
                NULL,
                derived_key,
                out_decrypted,
                MAX_PATH * 2,
                NULL,
                NULL,
                &g_bCancelRequested
            );

            SecureZeroMemory(derived_key, sizeof(derived_key));

            if (dec_ok && !g_bCancelRequested) {
                success_count++;
                const wchar_t *fn = wcsrchr(out_decrypted, L'\\');
                fn = fn ? fn + 1 : out_decrypted;
                swprintf(msg, 512, L"[DECRYPTED] %ls", fn);
                PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(msg));
            } else {
                fail_count++;
                const wchar_t *fn = wcsrchr(src_file, L'\\');
                fn = fn ? fn + 1 : src_file;
                swprintf(msg, 512, L"[DECRYPT FAILED] Password salah atau data rusak: %ls", fn);
                PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(msg));
            }
        }
    }

    scanner_free(files);
    SecureZeroMemory(&cfg, sizeof(cfg));
    free(wp);

    PostMessageW(hWnd, WM_APP_FINISHED, (WPARAM)success_count, (LPARAM)fail_count);
    return 0;
}

static void start_encryption_processing(HWND hWnd) {
    if (g_bIsWorking) return;

    wchar_t raw_folder[MAX_PATH * 2] = {0};
    GetWindowTextW(g_hEditFolder, raw_folder, MAX_PATH * 2);

    wchar_t folder[MAX_PATH * 2] = {0};
    sanitize_path(raw_folder, folder, MAX_PATH * 2);
    SetWindowTextW(g_hEditFolder, folder);

    if (wcslen(folder) == 0) {
        MessageBoxW(hWnd, L"Silakan pilih folder target terlebih dahulu!", L"Peringatan", MB_OK | MB_ICONWARNING);
        return;
    }

    DWORD attribs = GetFileAttributesW(folder);
    if (attribs == INVALID_FILE_ATTRIBUTES || !(attribs & FILE_ATTRIBUTE_DIRECTORY)) {
        MessageBoxW(hWnd, L"Folder target tidak valid atau tidak ditemukan!", L"Peringatan", MB_OK | MB_ICONWARNING);
        return;
    }

    save_settings(folder);

    wchar_t pass[256] = {0};
    GetWindowTextW(g_hEditPass, pass, 256);
    if (wcslen(pass) == 0) {
        MessageBoxW(hWnd, L"Password tidak boleh kosong!", L"Peringatan", MB_OK | MB_ICONWARNING);
        return;
    }

    bool is_encrypt = (SendMessage(g_hRadioEncrypt, BM_GETCHECK, 0, 0) == BST_CHECKED);

    if (is_encrypt) {
        wchar_t pass_confirm[256] = {0};
        GetWindowTextW(g_hEditPassConfirm, pass_confirm, 256);
        if (wcscmp(pass, pass_confirm) != 0) {
            MessageBoxW(hWnd, L"Konfirmasi password tidak cocok! Silakan periksa kembali.", L"Peringatan", MB_OK | MB_ICONWARNING);
            SecureZeroMemory(pass_confirm, sizeof(pass_confirm));
            SecureZeroMemory(pass, sizeof(pass));
            return;
        }
        SecureZeroMemory(pass_confirm, sizeof(pass_confirm));

        int res = MessageBoxW(
            hWnd,
            L"PERINGATAN KEAMANAN:\n\n"
            L"1. Seluruh file asli akan OTOMATIS DIHAPUS SECARA PERMANEN (secure wipe & delete) setelah berhasil dienkripsi.\n"
            L"2. Simpan password Anda dengan aman. Jika password lupa, file TIDAK DAPAT dipulihkan oleh siapa pun.\n\n"
            L"Apakah Anda yakin ingin memulai proses enkripsi sekarang?",
            L"Konfirmasi Enkripsi & Penghapusan File Asli",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2
        );

        if (res != IDYES) {
            SecureZeroMemory(pass, sizeof(pass));
            return;
        }
    }

    WorkerParams *wp = (WorkerParams *)malloc(sizeof(WorkerParams));
    if (!wp) return;

    wcsncpy(wp->config.folder_path, folder, MAX_PATH * 2 - 1);
    wp->config.recursive = (SendMessage(g_hChkRecursive, BM_GETCHECK, 0, 0) == BST_CHECKED);
    wp->config.op_mode = is_encrypt ? OP_MODE_ENCRYPT_AND_SHRED : OP_MODE_DECRYPT;
    wp->config.cipher_id = CIPHER_AES_256_GCM;
    wcsncpy(wp->config.password, pass, 255);
    SecureZeroMemory(pass, sizeof(pass));

    wp->config.use_custom_salt = (SendMessage(g_hChkCustomSalt, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (wp->config.use_custom_salt) {
        GetWindowTextW(g_hEditSalt, wp->config.custom_salt, 128);
    } else {
        wp->config.custom_salt[0] = L'\0';
    }

    wp->hWnd = hWnd;

    g_bIsWorking = true;
    g_bCancelRequested = false;
    EnableWindow(g_hBtnStart, FALSE);
    EnableWindow(g_hBtnCancel, TRUE);
    EnableWindow(g_hBtnBrowse, FALSE);
    EnableWindow(g_hEditFolder, FALSE);
    EnableWindow(g_hEditPass, FALSE);
    EnableWindow(g_hEditPassConfirm, FALSE);
    EnableWindow(g_hRadioEncrypt, FALSE);
    EnableWindow(g_hRadioDecrypt, FALSE);

    SendMessage(g_hProgress, PBM_SETRANGE32, 0, 100);
    SendMessage(g_hProgress, PBM_SETPOS, 0, 0);
    SetWindowTextW(g_hStaticStatus, L"Memproses berkas...");

    g_hWorkerThread = CreateThread(NULL, 0, encrypt_worker_thread, wp, 0, NULL);
}

static void cancel_processing(HWND hWnd) {
    (void)hWnd;
    if (g_bIsWorking) {
        g_bCancelRequested = true;
        SetWindowTextW(g_hStaticStatus, L"Membatalkan proses...");
    }
}

static void on_finished(HWND hWnd, size_t success_count, size_t fail_count) {
    g_bIsWorking = false;
    if (g_hWorkerThread) {
        CloseHandle(g_hWorkerThread);
        g_hWorkerThread = NULL;
    }

    EnableWindow(g_hBtnStart, TRUE);
    EnableWindow(g_hBtnCancel, FALSE);
    EnableWindow(g_hBtnBrowse, TRUE);
    EnableWindow(g_hEditFolder, TRUE);
    EnableWindow(g_hEditPass, TRUE);
    EnableWindow(g_hEditPassConfirm, TRUE);
    EnableWindow(g_hRadioEncrypt, TRUE);
    EnableWindow(g_hRadioDecrypt, TRUE);

    wchar_t status_txt[256];
    swprintf(status_txt, 256, L"Selesai! Berhasil: %zu | Gagal/Dilewati: %zu", success_count, fail_count);
    SetWindowTextW(g_hStaticStatus, status_txt);

    wchar_t done_msg[512];
    swprintf(done_msg, 512, L"Proses selesai.\n\nBerhasil diproses: %zu berkas\nGagal / Dilewati: %zu berkas", success_count, fail_count);
    MessageBoxW(hWnd, done_msg, L"Informasi", MB_OK | MB_ICONINFORMATION);
}

static void on_renamer_finished(HWND hWnd, size_t total_count) {
    (void)total_count;
    g_bIsWorking = false;
    if (g_hWorkerThread) {
        CloseHandle(g_hWorkerThread);
        g_hWorkerThread = NULL;
    }

    EnableWindow(g_hRenBtnStart, TRUE);
    EnableWindow(g_hRenBtnUndo, TRUE);

    populate_renamer_listview();
    SetWindowTextW(g_hRenStaticStatus, L"Batch Rename Selesai!");

    MessageBoxW(hWnd, L"Proses Batch Rename berhasil selesai!", L"Informasi", MB_OK | MB_ICONINFORMATION);
}

/* ========================================================================= */
/* Window Creation & Message Loop                                            */
/* ========================================================================= */

static HMENU create_app_menu(void) {
    HMENU hMenuBar = CreateMenu();

    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_OPEN, L"&Buka Folder...\tCtrl+O");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"&Keluar\tAlt+F4");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"&Berkas");

    HMENU hViewMenu = CreatePopupMenu();
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_ENCRYPT, L"🔒 Enkripsi && Shredder\tCtrl+1");
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_RENAME, L"🏷️ Bulk File Renamer\tCtrl+2");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, L"&Moda Alat");

    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_GUIDE, L"&Panduan Penggunaan");
    AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"&Tentang Aplikasi");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Bantuan");

    return hMenuBar;
}

bool ui_init(HINSTANCE hInstance) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);

    load_settings();

    g_pRenameList = renamer_create_list();
    g_pUndoHistory = renamer_create_undo();

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ui_wnd_proc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"Win10FileModMainClass";

    return (RegisterClassExW(&wc) != 0);
}

HWND ui_create_main_window(HINSTANCE hInstance, int nCmdShow) {
    int winWidth = 860;
    int winHeight = 720;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - winWidth) / 2;
    int y = (screenH - winHeight) / 2;

    HWND hWnd = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        L"Win10FileModMainClass",
        L"Win10 File Mod - Tool Suite: Enkripsi & Bulk Renamer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, winWidth, winHeight,
        NULL, create_app_menu(), hInstance, NULL
    );

    if (hWnd) {
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
    }

    return hWnd;
}

LRESULT CALLBACK ui_wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            g_hWndMain = hWnd;
            g_hFont = CreateFontW(
                -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );
            g_hFontBold = CreateFontW(
                -12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );

            /* Top Tab Control with WS_TABSTOP */
            g_hTab = CreateWindowExW(
                0, WC_TABCONTROLW, L"",
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
                10, 5, 825, 650, hWnd, (HMENU)IDC_MAIN_TAB, NULL, NULL
            );
            set_font(g_hTab, g_hFontBold);

            TCITEMW tie = {0};
            tie.mask = TCIF_TEXT;
            tie.pszText = L"🔒 1. Enkripsi & Secure Shredder";
            TabCtrl_InsertItem(g_hTab, 0, &tie);
            tie.pszText = L"🏷️ 2. Bulk File Renamer (Advanced)";
            TabCtrl_InsertItem(g_hTab, 1, &tie);

            /* ================= PAGE 1: ENCRYPTION CONTROLS ================= */
            HWND hGrp1 = CreateWindowExW(0, L"BUTTON", L" 1. Folder Target & Opsi Pemindaian ",
                WS_CHILD | BS_GROUPBOX, 25, 40, 795, 95, hWnd, (HMENU)IDC_GRP_FOLDER, NULL, NULL);
            set_font(hGrp1, g_hFontBold);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, hGrp1);

            HWND hLblF = CreateWindowExW(0, L"STATIC", L"Lokasi Folder:",
                WS_CHILD, 40, 65, 95, 20, hWnd, NULL, NULL, NULL);
            set_font(hLblF, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, hLblF);

            g_hEditFolder = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_szLastFolder,
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 140, 62, 555, 24, hWnd, (HMENU)IDC_EDIT_FOLDER, NULL, NULL);
            set_font(g_hEditFolder, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hEditFolder);

            g_hBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 705, 61, 100, 26, hWnd, (HMENU)IDC_BTN_BROWSE, NULL, NULL);
            set_font(g_hBtnBrowse, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hBtnBrowse);

            g_hChkRecursive = CreateWindowExW(0, L"BUTTON", L"Pindai semua subfolder di dalamnya (Rekursif)",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 140, 98, 270, 20, hWnd, (HMENU)IDC_CHK_RECURSIVE, NULL, NULL);
            set_font(g_hChkRecursive, g_hFont);
            SendMessage(g_hChkRecursive, BM_SETCHECK, BST_CHECKED, 0);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hChkRecursive);

            g_hChkRemember1 = CreateWindowExW(0, L"BUTTON", L"Ingat folder terakhir yang digunakan",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 420, 98, 250, 20, hWnd, (HMENU)IDC_CHK_REMEMBER, NULL, NULL);
            set_font(g_hChkRemember1, g_hFont);
            SendMessage(g_hChkRemember1, BM_SETCHECK, g_bRememberFolder ? BST_CHECKED : BST_UNCHECKED, 0);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hChkRemember1);

            /* Group 2: Mode */
            HWND hGrp2 = CreateWindowExW(0, L"BUTTON", L" 2. Mode Operasi ",
                WS_CHILD | BS_GROUPBOX, 25, 145, 795, 60, hWnd, (HMENU)IDC_GRP_MODE, NULL, NULL);
            set_font(hGrp2, g_hFontBold);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, hGrp2);

            g_hRadioEncrypt = CreateWindowExW(0, L"BUTTON", L"Enkripsi & Hapus Permanen File Asli (Secure Shred)",
                WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 40, 170, 360, 20, hWnd, (HMENU)IDC_RADIO_ENCRYPT, NULL, NULL);
            set_font(g_hRadioEncrypt, g_hFontBold);
            SendMessage(g_hRadioEncrypt, BM_SETCHECK, BST_CHECKED, 0);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hRadioEncrypt);

            g_hRadioDecrypt = CreateWindowExW(0, L"BUTTON", L"Dekripsi File .enc",
                WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP, 420, 170, 250, 20, hWnd, (HMENU)IDC_RADIO_DECRYPT, NULL, NULL);
            set_font(g_hRadioDecrypt, g_hFontBold);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hRadioDecrypt);

            /* Group 3: Kriptografi */
            HWND hGrp3 = CreateWindowExW(0, L"BUTTON", L" 3. Pengaturan Kriptografi & Kunci ",
                WS_CHILD | BS_GROUPBOX, 25, 215, 795, 160, hWnd, (HMENU)IDC_GRP_CRYPTO, NULL, NULL);
            set_font(hGrp3, g_hFontBold);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, hGrp3);

            HWND hLblC = CreateWindowExW(0, L"STATIC", L"Algoritma Enkripsi:",
                WS_CHILD, 40, 240, 120, 20, hWnd, NULL, NULL, NULL);
            set_font(hLblC, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, hLblC);

            g_hComboCipher = CreateWindowExW(0, L"COMBOBOX", L"",
                WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 165, 237, 640, 120, hWnd, (HMENU)IDC_COMBO_CIPHER, NULL, NULL);
            set_font(g_hComboCipher, g_hFont);
            SendMessageW(g_hComboCipher, CB_ADDSTRING, 0, (LPARAM)L"AES-256-GCM (Hardware AES-NI, Militer AEAD, Anti-Tamper) [Default]");
            SendMessageW(g_hComboCipher, CB_ADDSTRING, 0, (LPARAM)L"AES-256-CBC + HMAC-SHA256 (Standar Terotentikasi)");
            SendMessage(g_hComboCipher, CB_SETCURSEL, 0, 0);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hComboCipher);

            HWND hLblP = CreateWindowExW(0, L"STATIC", L"Password / Kunci:",
                WS_CHILD, 40, 275, 120, 20, hWnd, NULL, NULL, NULL);
            set_font(hLblP, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, hLblP);

            g_hEditPass = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL | WS_TABSTOP, 165, 272, 280, 24, hWnd, (HMENU)IDC_EDIT_PASS, NULL, NULL);
            set_font(g_hEditPass, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hEditPass);

            HWND hLblPC = CreateWindowExW(0, L"STATIC", L"Konfirmasi Pass:",
                WS_CHILD, 460, 275, 110, 20, hWnd, NULL, NULL, NULL);
            set_font(hLblPC, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, hLblPC);

            g_hEditPassConfirm = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL | WS_TABSTOP, 575, 272, 230, 24, hWnd, (HMENU)IDC_EDIT_PASS_CONFIRM, NULL, NULL);
            set_font(g_hEditPassConfirm, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hEditPassConfirm);

            g_hChkShowPass = CreateWindowExW(0, L"BUTTON", L"Tampilkan Karakter Password",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 165, 305, 240, 20, hWnd, (HMENU)IDC_CHK_SHOW_PASS, NULL, NULL);
            set_font(g_hChkShowPass, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hChkShowPass);

            g_hChkCustomSalt = CreateWindowExW(0, L"BUTTON", L"Custom Salt Tambahan (Pepper):",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 40, 338, 220, 20, hWnd, (HMENU)IDC_CHK_CUSTOM_SALT, NULL, NULL);
            set_font(g_hChkCustomSalt, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hChkCustomSalt);

            g_hEditSalt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | WS_DISABLED | WS_TABSTOP, 265, 335, 540, 24, hWnd, (HMENU)IDC_EDIT_SALT, NULL, NULL);
            set_font(g_hEditSalt, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hEditSalt);

            /* Group 4: Progress & Buttons */
            g_hBtnStart = CreateWindowExW(0, L"BUTTON", L"MULAI PROSES",
                WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 25, 390, 650, 36, hWnd, (HMENU)IDC_BTN_START, NULL, NULL);
            set_font(g_hBtnStart, g_hFontBold);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hBtnStart);

            g_hBtnCancel = CreateWindowExW(0, L"BUTTON", L"BATAL",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP | WS_DISABLED, 685, 390, 135, 36, hWnd, (HMENU)IDC_BTN_CANCEL, NULL, NULL);
            set_font(g_hBtnCancel, g_hFontBold);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hBtnCancel);

            g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                WS_CHILD | PBS_SMOOTH, 25, 435, 795, 18, hWnd, (HMENU)IDC_PROGRESS, NULL, NULL);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hProgress);

            g_hStaticStatus = CreateWindowExW(0, L"STATIC", L"Siap. Pilih folder target dan masukkan password.",
                WS_CHILD, 25, 460, 795, 20, hWnd, (HMENU)IDC_STATIC_STATUS, NULL, NULL);
            set_font(g_hStaticStatus, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hStaticStatus);

            g_hLogBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                25, 485, 795, 160, hWnd, (HMENU)IDC_LOG_BOX, NULL, NULL);
            set_font(g_hLogBox, g_hFont);
            register_ctrl(g_hEncCtrls, &g_nEncCtrls, 64, g_hLogBox);

            append_log(g_hLogBox, L"=== Windows 10 Native File Encryptor & Bulk Mod Tool ===");
            append_log(g_hLogBox, L"Modul Kriptografi: Windows CNG (bcrypt.dll) AES-256-GCM.");

            /* ================= PAGE 2: BULK RENAMER CONTROLS ================= */
            HWND hRenG1 = CreateWindowExW(0, L"BUTTON", L" 1. Pemilihan Folder Sumber (Langsung Dipindai Otomatis) ",
                WS_CHILD | BS_GROUPBOX, 25, 40, 795, 65, hWnd, (HMENU)IDC_REN_GRP_FOLDER, NULL, NULL);
            set_font(hRenG1, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, hRenG1);

            g_hRenEditFolder = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_szLastFolder,
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 35, 65, 450, 24, hWnd, (HMENU)IDC_REN_EDIT_FOLDER, NULL, NULL);
            set_font(g_hRenEditFolder, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditFolder);

            g_hRenBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 495, 64, 80, 26, hWnd, (HMENU)IDC_REN_BTN_BROWSE, NULL, NULL);
            set_font(g_hRenBtnBrowse, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnBrowse);

            g_hRenChkRecursive = CreateWindowExW(0, L"BUTTON", L"Subfolder",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 585, 66, 85, 22, hWnd, (HMENU)IDC_REN_CHK_RECURSIVE, NULL, NULL);
            set_font(g_hRenChkRecursive, g_hFont);
            SendMessage(g_hRenChkRecursive, BM_SETCHECK, BST_CHECKED, 0);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenChkRecursive);

            g_hRenChkRemember2 = CreateWindowExW(0, L"BUTTON", L"Ingat folder",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 680, 66, 130, 22, hWnd, (HMENU)IDC_REN_CHK_REMEMBER, NULL, NULL);
            set_font(g_hRenChkRemember2, g_hFont);
            SendMessage(g_hRenChkRemember2, BM_SETCHECK, g_bRememberFolder ? BST_CHECKED : BST_UNCHECKED, 0);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenChkRemember2);

            /* Group 2: Aturan / Metode Rename */
            HWND hRenG2 = CreateWindowExW(0, L"BUTTON", L" 2. Metode & Aturan Penamaan Baru (Advanced Renamer Style) ",
                WS_CHILD | BS_GROUPBOX, 25, 115, 795, 80, hWnd, (HMENU)IDC_REN_GRP_METHOD, NULL, NULL);
            set_font(hRenG2, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, hRenG2);

            HWND hLblM = CreateWindowExW(0, L"STATIC", L"Metode:",
                WS_CHILD, 35, 140, 55, 20, hWnd, (HMENU)IDC_REN_LBL_METHOD, NULL, NULL);
            set_font(hLblM, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, hLblM);

            g_hRenComboMethod = CreateWindowExW(0, L"COMBOBOX", L"",
                WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 95, 137, 195, 140, hWnd, (HMENU)IDC_REN_COMBO_METHOD, NULL, NULL);
            set_font(g_hRenComboMethod, g_hFont);
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"1. Ganti Teks (Find & Replace)");
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"2. Tambah Teks (Prefix/Suffix)");
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"3. Pola Tag (<name>, <inc:01>)");
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"4. Ubah Huruf Besar/Kecil");
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"5. Pangkas Karakter (Trim)");
            SendMessage(g_hRenComboMethod, CB_SETCURSEL, 0, 0);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenComboMethod);

            /* Text labels & Text edit fields */
            g_hRenLblP1 = CreateWindowExW(0, L"STATIC", L"Cari Teks (Find):",
                WS_CHILD, 300, 140, 110, 20, hWnd, (HMENU)IDC_REN_LBL_P1, NULL, NULL);
            set_font(g_hRenLblP1, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenLblP1);

            g_hRenEditP1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 415, 137, 150, 24, hWnd, (HMENU)IDC_REN_EDIT_P1, NULL, NULL);
            set_font(g_hRenEditP1, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditP1);

            g_hRenLblP2 = CreateWindowExW(0, L"STATIC", L"Ganti Dengan (Replace):",
                WS_CHILD, 575, 140, 135, 20, hWnd, (HMENU)IDC_REN_LBL_P2, NULL, NULL);
            set_font(g_hRenLblP2, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenLblP2);

            g_hRenEditP2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 715, 137, 95, 24, hWnd, (HMENU)IDC_REN_EDIT_P2, NULL, NULL);
            set_font(g_hRenEditP2, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditP2);

            g_hRenComboP3 = CreateWindowExW(0, L"COMBOBOX", L"",
                WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 660, 137, 150, 120, hWnd, (HMENU)IDC_REN_COMBO_P3, NULL, NULL);
            set_font(g_hRenComboP3, g_hFont);
            ShowWindow(g_hRenComboP3, SW_HIDE);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenComboP3);

            /* Number Step Controls Group 1 ([-] [num] [+]) */
            g_hRenBtnDec1 = CreateWindowExW(0, L"BUTTON", L"-",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 410, 137, 28, 24, hWnd, (HMENU)IDC_REN_BTN_DEC1, NULL, NULL);
            set_font(g_hRenBtnDec1, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnDec1);

            g_hRenEditNum1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | ES_CENTER | ES_NUMBER | WS_TABSTOP, 441, 137, 40, 24, hWnd, (HMENU)IDC_REN_EDIT_NUM1, NULL, NULL);
            set_font(g_hRenEditNum1, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditNum1);

            g_hRenBtnInc1 = CreateWindowExW(0, L"BUTTON", L"+",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 484, 137, 28, 24, hWnd, (HMENU)IDC_REN_BTN_INC1, NULL, NULL);
            set_font(g_hRenBtnInc1, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnInc1);

            g_hRenSpin1 = CreateWindowExW(0, UPDOWN_CLASSW, NULL,
                WS_CHILD | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                0, 0, 0, 0, hWnd, (HMENU)IDC_REN_SPIN1, NULL, NULL);
            SendMessage(g_hRenSpin1, UDM_SETBUDDY, (WPARAM)g_hRenEditNum1, 0);
            SendMessage(g_hRenSpin1, UDM_SETRANGE32, 0, 100);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenSpin1);

            /* Number Step Controls Group 2 ([-] [num] [+]) */
            g_hRenBtnDec2 = CreateWindowExW(0, L"BUTTON", L"-",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 650, 137, 28, 24, hWnd, (HMENU)IDC_REN_BTN_DEC2, NULL, NULL);
            set_font(g_hRenBtnDec2, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnDec2);

            g_hRenEditNum2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | ES_CENTER | ES_NUMBER | WS_TABSTOP, 681, 137, 40, 24, hWnd, (HMENU)IDC_REN_EDIT_NUM2, NULL, NULL);
            set_font(g_hRenEditNum2, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditNum2);

            g_hRenBtnInc2 = CreateWindowExW(0, L"BUTTON", L"+",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 724, 137, 28, 24, hWnd, (HMENU)IDC_REN_BTN_INC2, NULL, NULL);
            set_font(g_hRenBtnInc2, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnInc2);

            g_hRenSpin2 = CreateWindowExW(0, UPDOWN_CLASSW, NULL,
                WS_CHILD | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                0, 0, 0, 0, hWnd, (HMENU)IDC_REN_SPIN2, NULL, NULL);
            SendMessage(g_hRenSpin2, UDM_SETBUDDY, (WPARAM)g_hRenEditNum2, 0);
            SendMessage(g_hRenSpin2, UDM_SETRANGE32, 0, 100);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenSpin2);

            /* Group 3: Table Live Preview (ListView) */
            g_hRenList = CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | LVS_REPORT | LVS_SINGLESEL | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
                25, 205, 795, 385, hWnd, (HMENU)IDC_REN_LISTVIEW, NULL, NULL
            );
            set_font(g_hRenList, g_hFont);
            ListView_SetExtendedListViewStyle(g_hRenList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

            LVCOLUMNW lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.pszText = L"#";
            lvc.cx = 40;
            ListView_InsertColumn(g_hRenList, 0, &lvc);

            lvc.pszText = L"Nama File Asli";
            lvc.cx = 260;
            ListView_InsertColumn(g_hRenList, 1, &lvc);

            lvc.pszText = L"Nama File Baru (Live Preview)";
            lvc.cx = 260;
            ListView_InsertColumn(g_hRenList, 2, &lvc);

            lvc.pszText = L"Ukuran";
            lvc.cx = 85;
            ListView_InsertColumn(g_hRenList, 3, &lvc);

            lvc.pszText = L"Status / Validasi";
            lvc.cx = 140;
            ListView_InsertColumn(g_hRenList, 4, &lvc);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenList);

            /* Group 4: Renamer Bottom Buttons */
            g_hRenBtnStart = CreateWindowExW(0, L"BUTTON", L"JALANKAN BATCH RENAME",
                WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 25, 600, 510, 36, hWnd, (HMENU)IDC_REN_BTN_START, NULL, NULL);
            set_font(g_hRenBtnStart, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnStart);

            g_hRenBtnUndo = CreateWindowExW(0, L"BUTTON", L"BATALKAN (UNDO)",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 545, 600, 135, 36, hWnd, (HMENU)IDC_REN_BTN_UNDO, NULL, NULL);
            set_font(g_hRenBtnUndo, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnUndo);

            g_hRenBtnPreview = CreateWindowExW(0, L"BUTTON", L"REFRESH",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 690, 600, 130, 36, hWnd, (HMENU)IDC_REN_BTN_PREVIEW, NULL, NULL);
            set_font(g_hRenBtnPreview, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnPreview);

            g_hRenStaticStatus = CreateWindowExW(0, L"STATIC", L"Pilih folder untuk memindai berkas secara otomatis.",
                WS_CHILD, 25, 642, 795, 18, hWnd, (HMENU)IDC_REN_STATIC_STATUS, NULL, NULL);
            set_font(g_hRenStaticStatus, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenStaticStatus);

            /* Show Initial Page 0 */
            show_page(0);

            /* If remembered folder is valid, auto-scan on startup for renamer */
            if (g_bRememberFolder && g_szLastFolder[0] != L'\0') {
                DWORD attr = GetFileAttributesW(g_szLastFolder);
                if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
                    renamer_scan_folder(g_pRenameList, g_szLastFolder, true);
                    trigger_live_preview();
                    populate_renamer_listview();
                }
            }
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            if (lpnm->hwndFrom == g_hTab && lpnm->code == TCN_SELCHANGE) {
                int cur_tab = TabCtrl_GetCurSel(g_hTab);
                show_page(cur_tab);
            }
            break;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            /* Menu Actions */
            if (wmId == IDM_FILE_OPEN) {
                int cur_tab = TabCtrl_GetCurSel(g_hTab);
                if (cur_tab == 0) browse_folder_to_edit(hWnd, g_hEditFolder);
                else browse_folder_to_edit(hWnd, g_hRenEditFolder);
            } else if (wmId == IDM_FILE_EXIT) {
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            } else if (wmId == IDM_VIEW_ENCRYPT) {
                TabCtrl_SetCurSel(g_hTab, 0);
                show_page(0);
            } else if (wmId == IDM_VIEW_RENAME) {
                TabCtrl_SetCurSel(g_hTab, 1);
                show_page(1);
            } else if (wmId == IDM_HELP_GUIDE) {
                MessageBoxW(hWnd,
                    L"PANDUAN PENGGUNAAN:\n\n"
                    L"1. MODA ENKRIPSI & SHREDDER:\n"
                    L"   - Navigasi input kini mendukung tombol Tab / Shift+Tab.\n"
                    L"   - File dienkripsi dengan standar AES-256-GCM AEAD militer.\n"
                    L"   - File asli langsung dimusnahkan secara aman (secure shred).\n\n"
                    L"2. MODA BULK FILE RENAMER:\n"
                    L"   - Pilih folder (otomatis langsung dipindai tanpa perlu klik tombol scan).\n"
                    L"   - Checklist subfolder juga langsung memicu pemindaian otomatis.\n"
                    L"   - Tombol [-] dan [+] mengatur angka pangkas (trim) & penomoran secara langsung (+/- 1 step).\n"
                    L"   - Kolom 'Nama Baru' terupdate secara langsung (Live Preview).\n"
                    L"   - Pilihan 'Ingat folder' akan menyimpan lokasi folder terakhir Anda.",
                    L"Panduan Penggunaan", MB_OK | MB_ICONINFORMATION);
            } else if (wmId == IDM_HELP_ABOUT) {
                MessageBoxW(hWnd,
                    L"Win10 File Mod Tool Suite v1.2\n"
                    L"Pure Native C (C99) - Zero External Dependencies\n\n"
                    L"Fitur: Enkripsi Militer AES-256-GCM + Advanced Bulk Renamer",
                    L"Tentang Aplikasi", MB_OK | MB_ICONINFORMATION);
            }

            /* Page 1 (Encryption) Controls */
            else if (wmId == IDC_BTN_BROWSE && wmEvent == BN_CLICKED) {
                browse_folder_to_edit(hWnd, g_hEditFolder);
            } else if (wmId == IDC_BTN_START && wmEvent == BN_CLICKED) {
                start_encryption_processing(hWnd);
            } else if (wmId == IDC_BTN_CANCEL && wmEvent == BN_CLICKED) {
                cancel_processing(hWnd);
            } else if (wmId == IDC_CHK_REMEMBER && wmEvent == BN_CLICKED) {
                g_bRememberFolder = (SendMessage(g_hChkRemember1, BM_GETCHECK, 0, 0) == BST_CHECKED);
                SendMessage(g_hRenChkRemember2, BM_SETCHECK, g_bRememberFolder ? BST_CHECKED : BST_UNCHECKED, 0);
                save_settings(g_szLastFolder);
            } else if (wmId == IDC_CHK_SHOW_PASS && wmEvent == BN_CLICKED) {
                bool show = (SendMessage(g_hChkShowPass, BM_GETCHECK, 0, 0) == BST_CHECKED);
                SendMessage(g_hEditPass, EM_SETPASSWORDCHAR, show ? 0 : L'*', 0);
                SendMessage(g_hEditPassConfirm, EM_SETPASSWORDCHAR, show ? 0 : L'*', 0);
                InvalidateRect(g_hEditPass, NULL, TRUE);
                InvalidateRect(g_hEditPassConfirm, NULL, TRUE);
            } else if (wmId == IDC_CHK_CUSTOM_SALT && wmEvent == BN_CLICKED) {
                bool custom = (SendMessage(g_hChkCustomSalt, BM_GETCHECK, 0, 0) == BST_CHECKED);
                EnableWindow(g_hEditSalt, custom);
            } else if ((wmId == IDC_RADIO_ENCRYPT || wmId == IDC_RADIO_DECRYPT) && wmEvent == BN_CLICKED) {
                bool is_enc = (SendMessage(g_hRadioEncrypt, BM_GETCHECK, 0, 0) == BST_CHECKED);
                EnableWindow(g_hEditPassConfirm, is_enc);
                SetWindowTextW(g_hBtnStart, is_enc ? L"MULAI ENKRIPSI & HAPUS ASLI" : L"MULAI DEKRIPSI");
            }

            /* Page 2 (Renamer) Controls */
            else if (wmId == IDC_REN_BTN_BROWSE && wmEvent == BN_CLICKED) {
                browse_folder_to_edit(hWnd, g_hRenEditFolder);
            } else if (wmId == IDC_REN_CHK_RECURSIVE && wmEvent == BN_CLICKED) {
                scan_renamer_folder(hWnd);
            } else if (wmId == IDC_REN_CHK_REMEMBER && wmEvent == BN_CLICKED) {
                g_bRememberFolder = (SendMessage(g_hRenChkRemember2, BM_GETCHECK, 0, 0) == BST_CHECKED);
                SendMessage(g_hChkRemember1, BM_SETCHECK, g_bRememberFolder ? BST_CHECKED : BST_UNCHECKED, 0);
                save_settings(g_szLastFolder);
            } else if (wmId == IDC_REN_EDIT_FOLDER && (wmEvent == EN_KILLFOCUS)) {
                scan_renamer_folder(hWnd);
            } else if (wmId == IDC_REN_COMBO_METHOD && wmEvent == CBN_SELCHANGE) {
                int midx = (int)SendMessage(g_hRenComboMethod, CB_GETCURSEL, 0, 0);
                update_method_ui_labels(midx);
                trigger_live_preview();
            } else if ((wmId == IDC_REN_EDIT_P1 || wmId == IDC_REN_EDIT_P2 ||
                        wmId == IDC_REN_EDIT_NUM1 || wmId == IDC_REN_EDIT_NUM2) && wmEvent == EN_CHANGE) {
                trigger_live_preview();
            } else if (wmId == IDC_REN_COMBO_P3 && wmEvent == CBN_SELCHANGE) {
                trigger_live_preview();
            } else if (wmId == IDC_REN_BTN_DEC1 && wmEvent == BN_CLICKED) {
                step_number_edit(g_hRenEditNum1, -1, 0, 100);
            } else if (wmId == IDC_REN_BTN_INC1 && wmEvent == BN_CLICKED) {
                step_number_edit(g_hRenEditNum1, +1, 0, 100);
            } else if (wmId == IDC_REN_BTN_DEC2 && wmEvent == BN_CLICKED) {
                int midx = (int)SendMessage(g_hRenComboMethod, CB_GETCURSEL, 0, 0);
                int min_val = (midx == 2) ? 1 : 0;
                step_number_edit(g_hRenEditNum2, -1, min_val, 99999);
            } else if (wmId == IDC_REN_BTN_INC2 && wmEvent == BN_CLICKED) {
                step_number_edit(g_hRenEditNum2, +1, 0, 99999);
            } else if (wmId == IDC_REN_BTN_PREVIEW && wmEvent == BN_CLICKED) {
                scan_renamer_folder(hWnd);
                trigger_live_preview();
            } else if (wmId == IDC_REN_BTN_START && wmEvent == BN_CLICKED) {
                start_batch_rename(hWnd);
            } else if (wmId == IDC_REN_BTN_UNDO && wmEvent == BN_CLICKED) {
                undo_batch_rename(hWnd);
            }
            break;
        }

        case WM_APP_LOG_MSG: {
            wchar_t *msg = (wchar_t *)lParam;
            if (msg) {
                append_log(g_hLogBox, msg);
                free(msg);
            }
            break;
        }

        case WM_APP_PROGRESS: {
            int cur = (int)wParam;
            int total = (int)lParam;
            if (total > 0) {
                int pct = (cur * 100) / total;
                SendMessage(g_hProgress, PBM_SETPOS, pct, 0);

                wchar_t status_txt[256];
                swprintf(status_txt, 256, L"Memproses: %d / %d berkas (%d%%)...", cur, total, pct);
                SetWindowTextW(g_hStaticStatus, status_txt);
            }
            break;
        }

        case WM_APP_FINISHED: {
            on_finished(hWnd, (size_t)wParam, (size_t)lParam);
            break;
        }

        case WM_APP_REN_FINISHED: {
            on_renamer_finished(hWnd, (size_t)wParam);
            break;
        }

        case WM_CLOSE: {
            if (g_bIsWorking) {
                int res = MessageBoxW(hWnd, L"Proses sedang berjalan. Apakah Anda yakin ingin membatalkan dan keluar?",
                    L"Konfirmasi Keluar", MB_YESNO | MB_ICONQUESTION);
                if (res != IDYES) return 0;
                cancel_processing(hWnd);
            }
            if (g_bRememberFolder) {
                wchar_t current_folder[MAX_PATH * 2] = {0};
                GetWindowTextW(g_hEditFolder, current_folder, MAX_PATH * 2);
                if (wcslen(current_folder) == 0) {
                    GetWindowTextW(g_hRenEditFolder, current_folder, MAX_PATH * 2);
                }
                wchar_t clean[MAX_PATH * 2] = {0};
                sanitize_path(current_folder, clean, MAX_PATH * 2);
                if (clean[0] != L'\0') {
                    save_settings(clean);
                }
            }
            DestroyWindow(hWnd);
            break;
        }

        case WM_DESTROY: {
            if (g_hFont) DeleteObject(g_hFont);
            if (g_hFontBold) DeleteObject(g_hFontBold);
            if (g_pRenameList) renamer_free_list(g_pRenameList);
            if (g_pUndoHistory) renamer_free_undo(g_pUndoHistory);
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}
