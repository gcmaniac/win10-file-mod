#include "ui.h"
#include "crypto.h"
#include "vault.h"
#include "scanner.h"
#include "shredder.h"
#include "renamer.h"
#include "i18n.h"
#include "service.h"
#include "duplicate.h"
#include "kiosapi_key.h"
#include "model_tester.h"
#include "vscode_model_config.h"
#include <shlobj.h>
#include <shellapi.h>
#include <sqlite3.h>
#include <errno.h>
#include <math.h>

/* Menu IDs */
#define IDM_FILE_OPEN           2001
#define IDM_FILE_EXIT           2002
#define IDM_VIEW_ENCRYPT        2003
#define IDM_VIEW_RENAME         2004
#define IDM_VIEW_DUPLICATES     2007
#define IDM_VIEW_MODEL_TESTER   2008
#define IDM_HELP_GUIDE          2005
#define IDM_HELP_ABOUT          2006
#define IDM_LANG_EN             2010
#define IDM_LANG_ID             2011
#define IDM_SERVICE_TOGGLE      2020
#define IDM_SERVICE_INSTALL     2021
#define IDM_SERVICE_UNINSTALL   2022

/* Explorer Context Menu IDs */
#define IDC_MENU_EXPLORER_ENCRYPT_SEL   5001
#define IDC_MENU_EXPLORER_DECRYPT_SEL   5002
#define IDC_MENU_EXPLORER_SELECT_ALL    5003
#define IDC_MENU_EXPLORER_ENCRYPT_ALL   5004

/* Duplicate Context Menu IDs */
#define IDC_MENU_DUP_OPEN               5101
#define IDC_MENU_DUP_EXPLORER           5102
#define IDC_MENU_DUP_TOGGLE             5103
#define IDC_MENU_DUP_DELETE             5104

/* Main Window System Tray IDs */
#define WM_APP_MAINTRAY                 (WM_APP + 30)

/* Inactivity Auto-Encryption Timer ID */
#define TIMER_AUTO_ENCRYPT_ID           7001

/* Tab Control ID */
#define IDC_MAIN_TAB            3000

/* Setup / Registration View IDs */
#define IDC_SETUP_GRP           1101
#define IDC_SETUP_LBL_INFO      1102
#define IDC_SETUP_LBL_PASS      1103
#define IDC_SETUP_EDIT_PASS     1104
#define IDC_SETUP_LBL_CONFIRM   1105
#define IDC_SETUP_EDIT_CONFIRM  1106
#define IDC_SETUP_LBL_EMAIL     1107
#define IDC_SETUP_EDIT_EMAIL    1108
#define IDC_SETUP_BTN_SAVE      1109

/* Login View IDs */
#define IDC_LOGIN_GRP           1201
#define IDC_LOGIN_LBL_INFO      1202
#define IDC_LOGIN_LBL_PASS      1203
#define IDC_LOGIN_EDIT_PASS     1204
#define IDC_LOGIN_CHK_SHOW      1205
#define IDC_LOGIN_BTN_MASUK     1206
#define IDC_LOGIN_BTN_LUPA      1207
#define IDC_LOGIN_LBL_ATTEMPTS  1208
#define IDC_LOGIN_LBL_STATUS    1209

/* Authenticated Explorer View IDs */
#define IDC_ENC_LBL_FOLDER      1001
#define IDC_EDIT_FOLDER         1002
#define IDC_BTN_BROWSE          1003
#define IDC_CHK_RECURSIVE       1004
#define IDC_CHK_REMEMBER        1005
#define IDC_ENC_BTN_RUN         1006
#define IDC_ENC_TREE            1010
#define IDC_ENC_EXPLORER_LIST   1011
#define IDC_ENC_FILE_TYPE       1012
#define IDC_ENC_TYPE_LABEL      1013
#define IDC_ENC_EXTENSIONS      1014
#define IDC_ENC_MAX_SIZE_LABEL  1015
#define IDC_ENC_MAX_SIZE        1016
#define IDC_ENC_MAX_SIZE_UNIT   1017
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

#define IDC_REN_BTN_DEC1        3114
#define IDC_REN_EDIT_NUM1       3115
#define IDC_REN_BTN_INC1        3116
#define IDC_REN_BTN_DEC2        3118
#define IDC_REN_EDIT_NUM2       3119
#define IDC_REN_BTN_INC2        3120

#define IDC_REN_LISTVIEW        3122
#define IDC_REN_BTN_START       3123
#define IDC_REN_BTN_UNDO        3124
#define IDC_REN_BTN_PREVIEW     3125
#define IDC_REN_STATIC_STATUS   3126
#define IDC_REN_FILE_TYPE_LABEL 3127
#define IDC_REN_FILE_TYPE       3128
#define IDC_REN_EXTENSIONS      3129
#define IDC_REN_MAX_SIZE_LABEL  3130
#define IDC_REN_MAX_SIZE        3131
#define IDC_REN_MAX_SIZE_UNIT   3132

/* Page 3 (Duplicate File Finder) IDs */
#define IDC_DUP_GRP_FOLDER      3201
#define IDC_DUP_EDIT_FOLDER     3202
#define IDC_DUP_BTN_BROWSE      3203
#define IDC_DUP_CHK_RECURSIVE   3204
#define IDC_DUP_BTN_SCAN        3205
#define IDC_DUP_BTN_STOP        3206
#define IDC_DUP_BTN_AUTOSEL     3207
#define IDC_DUP_BTN_DELETE      3208
#define IDC_DUP_BTN_SEL_ALL     3209
#define IDC_DUP_BTN_DESEL_ALL   3210
#define IDC_DUP_LISTVIEW        3211
#define IDC_DUP_PROGRESS        3212
#define IDC_DUP_STATIC_STATUS   3213
#define IDC_DUP_FILE_TYPE_LABEL 3214
#define IDC_DUP_FILE_TYPE       3215
#define IDC_DUP_EXTENSIONS      3216
#define IDC_DUP_MAX_SIZE_LABEL  3217
#define IDC_DUP_MAX_SIZE        3218
#define IDC_DUP_MAX_SIZE_UNIT   3219

/* Page 4 (Password-protected KiosAPI Model Tester) IDs */
#define IDC_MODEL_LOCK_INFO     3301
#define IDC_MODEL_LOCK_LOGIN    3302
#define IDC_MODEL_BTN_TEST      3303
#define IDC_MODEL_BTN_STOP      3304
#define IDC_MODEL_PROGRESS      3305
#define IDC_MODEL_STATUS        3306
#define IDC_MODEL_LIST          3307
#define IDM_MODEL_SCAN_SELECTED 3308
#define IDC_MODEL_MAX_INPUT_LABEL 3309
#define IDC_MODEL_MAX_INPUT      3310
#define IDC_MODEL_MAX_OUTPUT_LABEL 3311
#define IDC_MODEL_MAX_OUTPUT     3312
#define IDM_MODEL_ADD_TO_VSCODE  3313

/* Custom Thread Messages */
#define WM_APP_LOG_MSG          (WM_APP + 1)
#define WM_APP_PROGRESS         (WM_APP + 2)
#define WM_APP_FINISHED         (WM_APP + 3)
#define WM_APP_REN_FINISHED     (WM_APP + 5)
#define WM_APP_DUP_PROGRESS     (WM_APP + 10)
#define WM_APP_DUP_FINISHED     (WM_APP + 11)
#define WM_APP_MODEL_PROGRESS   (WM_APP + 12)
#define WM_APP_MODEL_FINISHED   (WM_APP + 13)

/* Global Window Handles */
static HWND g_hWndMain = NULL;
static HWND g_hTab = NULL;
static HFONT g_hFont = NULL;
static HFONT g_hFontBold = NULL;
static HFONT g_hFontTitle = NULL;

/* Authentication & State */
static bool g_bAuthenticated = false;
static char g_kiosapiKey[KIOSAPI_KEY_CAPACITY] = {0};
static size_t g_kiosapiKeyLen = 0;
static bool g_bKiosapiKeyReady = false;
static wchar_t g_kiosapiKeyError[256] = {0};

/* Setup Controls */
static HWND g_hSetupCtrls[16] = {0};
static int  g_nSetupCtrls = 0;
static HWND g_hSetupGrp = NULL;
static HWND g_hSetupLblInfo = NULL;
static HWND g_hSetupLblPass = NULL;
static HWND g_hSetupEditPass = NULL;
static HWND g_hSetupLblConfirm = NULL;
static HWND g_hSetupEditConfirm = NULL;
static HWND g_hSetupLblEmail = NULL;
static HWND g_hSetupEditEmail = NULL;
static HWND g_hSetupBtnSave = NULL;

/* Login Controls */
static HWND g_hLoginCtrls[16] = {0};
static int  g_nLoginCtrls = 0;
static HWND g_hLoginGrp = NULL;
static HWND g_hLoginLblInfo = NULL;
static HWND g_hLoginLblPass = NULL;
static HWND g_hLoginEditPass = NULL;
static HWND g_hLoginChkShow = NULL;
static HWND g_hLoginBtnMasuk = NULL;
static HWND g_hLoginBtnLupa = NULL;
static HWND g_hLoginLblAttempts = NULL;
static HWND g_hLoginLblStatus = NULL;
static WNDPROC g_pfnOrigLoginPassProc = NULL;

static LRESULT CALLBACK login_pass_edit_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_RETURN) {
            HWND hParent = GetParent(hWnd);
            if (hParent && g_hLoginBtnMasuk && IsWindowEnabled(g_hLoginBtnMasuk)) {
                SendMessage(hParent, WM_COMMAND, MAKEWPARAM(IDC_LOGIN_BTN_MASUK, BN_CLICKED), (LPARAM)g_hLoginBtnMasuk);
            }
            return 0;
        }
    } else if (uMsg == WM_CHAR) {
        if (wParam == VK_RETURN) {
            return 0; /* Suppress beep */
        }
    }
    return CallWindowProcW(g_pfnOrigLoginPassProc, hWnd, uMsg, wParam, lParam);
}

/* Authenticated Explorer Controls */
static HWND g_hExplorerCtrls[32] = {0};
static int  g_nExplorerCtrls = 0;
static HWND g_hEncLblFolder = NULL;
static HWND g_hEditFolder = NULL;
static HWND g_hBtnBrowse = NULL;
static HWND g_hChkRecursive = NULL;
static HWND g_hChkRemember1 = NULL;
static HWND g_hEncTree = NULL;
static HWND g_hEncExplorerList = NULL;
static HWND g_hEncFileTypeLabel = NULL;
static HWND g_hEncFileType = NULL;
static HWND g_hEncFileExtensions = NULL;
static HWND g_hEncMaxSizeLabel = NULL;
static HWND g_hEncMaxSizeEdit = NULL;
static HWND g_hEncMaxSizeUnit = NULL;
static HWND g_hProgress = NULL;
static HWND g_hStaticStatus = NULL;
static HWND g_hLogBox = NULL;

/* Page 2 (Renamer) Handles */
static HWND g_hRenCtrls[64] = {0};
static int  g_nRenCtrls = 0;
static HWND g_hRenGrp1 = NULL;
static HWND g_hRenEditFolder = NULL;
static HWND g_hRenBtnBrowse = NULL;
static HWND g_hRenChkRecursive = NULL;
static HWND g_hRenChkRemember2 = NULL;
static HWND g_hRenGrp2 = NULL;
static HWND g_hRenLblMethod = NULL;
static HWND g_hRenComboMethod = NULL;
static HWND g_hRenLblP1 = NULL;
static HWND g_hRenEditP1 = NULL;
static HWND g_hRenLblP2 = NULL;
static HWND g_hRenEditP2 = NULL;
static HWND g_hRenComboP3 = NULL;

static HWND g_hRenBtnDec1 = NULL;
static HWND g_hRenEditNum1 = NULL;
static HWND g_hRenBtnInc1 = NULL;
static HWND g_hRenBtnDec2 = NULL;
static HWND g_hRenEditNum2 = NULL;
static HWND g_hRenBtnInc2 = NULL;

static HWND g_hRenList = NULL;
static HWND g_hRenBtnStart = NULL;
static HWND g_hRenBtnUndo = NULL;
static HWND g_hRenBtnPreview = NULL;
static HWND g_hRenStaticStatus = NULL;
static HWND g_hRenFileTypeLabel = NULL;
static HWND g_hRenFileType = NULL;
static HWND g_hRenFileExtensions = NULL;
static HWND g_hRenMaxSizeLabel = NULL;
static HWND g_hRenMaxSizeEdit = NULL;
static HWND g_hRenMaxSizeUnit = NULL;

/* Worker Threads */
static HANDLE g_hWorkerThread = NULL;
static volatile bool g_bCancelRequested = false;
static bool g_bIsWorking = false;

/* Settings State */
static bool g_bRememberFolder = true;
static wchar_t g_szLastFolder[MAX_PATH * 2] = {0};
static wchar_t g_szCurrentExplorerFolder[MAX_PATH * 2] = {0};

/* Renamer State */
static RenameItemList *g_pRenameList = NULL;
static UndoHistory    *g_pUndoHistory = NULL;

/* Page 3 (Duplicate Finder) Handles */
static HWND g_hDupCtrls[32] = {0};
static int  g_nDupCtrls = 0;
static HWND g_hDupGrpFolder = NULL;
static HWND g_hDupEditFolder = NULL;
static HWND g_hDupBtnBrowse = NULL;
static HWND g_hDupChkRecursive = NULL;
static HWND g_hDupBtnScan = NULL;
static HWND g_hDupList = NULL;
static HWND g_hDupProgress = NULL;
static HWND g_hDupStaticStatus = NULL;
static HWND g_hDupFileTypeLabel = NULL;
static HWND g_hDupFileType = NULL;
static HWND g_hDupFileExtensions = NULL;
static HWND g_hDupMaxSizeLabel = NULL;
static HWND g_hDupMaxSizeEdit = NULL;
static HWND g_hDupMaxSizeUnit = NULL;

/* Page 4 (KiosAPI Model Tester) Handles */
static HWND g_hModelCtrls[10] = {0};
static int g_nModelCtrls = 0;
static HWND g_hModelLockCtrls[2] = {0};
static int g_nModelLockCtrls = 0;
static HWND g_hModelLockInfo = NULL;
static HWND g_hModelLockLogin = NULL;
static HWND g_hModelBtnTest = NULL;
static HWND g_hModelBtnStop = NULL;
static HWND g_hModelProgress = NULL;
static HWND g_hModelStatus = NULL;
static HWND g_hModelList = NULL;
static HWND g_hModelMaxInputLabel = NULL;
static HWND g_hModelMaxInput = NULL;
static HWND g_hModelMaxOutputLabel = NULL;
static HWND g_hModelMaxOutput = NULL;
static ModelSortColumn g_modelSortColumn = MODEL_SORT_UPTIME;
static bool g_modelSortDescending = true;
static bool g_bModelDbReady = false;
static ModelTestRow *g_modelRows = NULL;
static size_t g_modelRowCount = 0;
static size_t g_modelVisibleCount = 0;
static HANDLE g_hModelThread = NULL;
static volatile bool g_bModelCancel = false;
static bool g_bModelTesting = false;

typedef struct {
    HWND hWnd;
    wchar_t database_path[MAX_PATH * 2];
    char api_key[KIOSAPI_KEY_CAPACITY];
    const char **selected_model_names;
    size_t selected_model_count;
} ModelWorkerParams;

typedef struct {
    int completed;
    int total;
    bool succeeded;
    wchar_t model_name[MODEL_TEST_NAME_CAPACITY];
    wchar_t result[MODEL_TEST_RESULT_CAPACITY];
} ModelProgressPayload;

typedef struct {
    bool success;
    bool cancelled;
    int completed;
    int succeeded;
    int failed;
    wchar_t error[256];
} ModelFinishedPayload;

static bool get_app_database_path(wchar_t *out_path, size_t capacity);
static void refresh_model_listview(void);
static void update_model_ready_status(void);
static void start_model_tests(HWND hWnd, bool selected_only);
static void clear_kiosapi_key(void);
static HWND g_hDupBtnAutoSel = NULL;
static HWND g_hDupBtnSelAll = NULL;
static HWND g_hDupBtnDeselAll = NULL;
static HWND g_hDupBtnDelete = NULL;
static HWND g_hDupBtnStop = NULL;

static DuplicateResults *g_pDupResults = NULL;
static volatile bool g_bDupCancel = false;
static HANDLE g_hDupWorkerThread = NULL;

typedef struct {
    int phase;
    size_t current;
    size_t total;
    wchar_t current_file[MAX_PATH * 2];
} DupProgressPayload;

typedef struct {
    HWND hWnd;
    wchar_t folder[MAX_PATH * 2];
    bool recursive;
    ScannerFileType file_type_filter;
    uint64_t max_file_size_bytes;
} DupWorkerParams;

typedef struct {
    JobConfig config;
    HWND hWnd;
    FileList *custom_files;
    ScannerFileType file_type_filter;
    uint64_t max_file_size_bytes;
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

static void populate_file_type_combo(HWND combo) {
    if (!combo) return;
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_ENC_FILE_TYPE_ALL));
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_ENC_FILE_TYPE_DOCUMENTS));
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_ENC_FILE_TYPE_IMAGES));
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_ENC_FILE_TYPE_VIDEOS));
    SendMessageW(combo, CB_SETCURSEL, 0, 0);
}

static void update_file_filter_extensions(HWND combo, HWND extensions_label) {
    if (!combo || !extensions_label) return;
    int selected = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
    StringId extension_string = STR_ENC_EXT_ALL;
    if (selected == 1) extension_string = STR_ENC_EXT_DOCUMENTS;
    else if (selected == 2) extension_string = STR_ENC_EXT_IMAGES;
    else if (selected == 3) extension_string = STR_ENC_EXT_VIDEOS;
    SetWindowTextW(extensions_label, i18n_get(extension_string));
}

static void refresh_file_filter_controls(HWND combo, HWND extensions_label, HWND max_label, HWND unit) {
    if (combo) {
        int selected = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
        populate_file_type_combo(combo);
        SendMessageW(combo, CB_SETCURSEL, selected >= 0 && selected <= 3 ? selected : 0, 0);
    }
    update_file_filter_extensions(combo, extensions_label);
    if (max_label) SetWindowTextW(max_label, i18n_get(STR_ENC_MAX_SIZE_LABEL));
    if (unit) SetWindowTextW(unit, i18n_get(STR_ENC_MAX_SIZE_UNIT));
}

static void create_file_filter_controls(
    HWND parent,
    int label_id,
    int combo_id,
    int extensions_id,
    int max_label_id,
    int max_edit_id,
    int unit_id,
    HWND *label,
    HWND *combo,
    HWND *extensions,
    HWND *max_label,
    HWND *max_edit,
    HWND *unit,
    HWND *control_array,
    int *control_count,
    int control_capacity
) {
    *label = CreateWindowExW(0, L"STATIC", i18n_get(STR_ENC_FILE_TYPE_LABEL),
        WS_CHILD, 0, 0, 65, 22, parent, (HMENU)(INT_PTR)label_id, NULL, NULL);
    set_font(*label, g_hFontBold);
    register_ctrl(control_array, control_count, control_capacity, *label);

    *combo = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0, 0, 140, 120,
        parent, (HMENU)(INT_PTR)combo_id, NULL, NULL);
    set_font(*combo, g_hFont);
    populate_file_type_combo(*combo);
    register_ctrl(control_array, control_count, control_capacity, *combo);

    *extensions = CreateWindowExW(0, L"STATIC", i18n_get(STR_ENC_EXT_ALL),
        WS_CHILD | SS_LEFT, 0, 0, 385, 22, parent, (HMENU)(INT_PTR)extensions_id, NULL, NULL);
    set_font(*extensions, g_hFont);
    register_ctrl(control_array, control_count, control_capacity, *extensions);

    *max_label = CreateWindowExW(0, L"STATIC", i18n_get(STR_ENC_MAX_SIZE_LABEL),
        WS_CHILD, 0, 0, 95, 22, parent, (HMENU)(INT_PTR)max_label_id, NULL, NULL);
    set_font(*max_label, g_hFontBold);
    register_ctrl(control_array, control_count, control_capacity, *max_label);

    *max_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | ES_NUMBER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 55, 24,
        parent, (HMENU)(INT_PTR)max_edit_id, NULL, NULL);
    set_font(*max_edit, g_hFont);
    register_ctrl(control_array, control_count, control_capacity, *max_edit);

    *unit = CreateWindowExW(0, L"STATIC", i18n_get(STR_ENC_MAX_SIZE_UNIT),
        WS_CHILD, 0, 0, 70, 22, parent, (HMENU)(INT_PTR)unit_id, NULL, NULL);
    set_font(*unit, g_hFont);
    register_ctrl(control_array, control_count, control_capacity, *unit);
}

static bool read_file_filters(
    HWND hWnd,
    HWND combo,
    HWND max_size_edit,
    ScannerFileType *out_file_type_filter,
    uint64_t *out_max_file_size_bytes
) {
    wchar_t size_text[32] = {0};
    GetWindowTextW(max_size_edit, size_text, (int)(sizeof(size_text) / sizeof(size_text[0])));
    wchar_t *end = NULL;
    errno = 0;
    unsigned long long size_mb = wcstoull(size_text, &end, 10);
    if (errno == ERANGE || end == size_text || *end != L'\0' ||
        size_mb > UINT64_MAX / (1024ULL * 1024ULL)) {
        MessageBoxW(hWnd, i18n_get(STR_ENC_MAX_SIZE_INVALID),
            i18n_get_language() == LANG_ID ? L"Ukuran tidak valid" : L"Invalid size",
            MB_OK | MB_ICONWARNING);
        SetFocus(max_size_edit);
        return false;
    }

    int selected = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (selected < SCANNER_FILE_TYPE_ALL || selected > SCANNER_FILE_TYPE_VIDEOS) {
        selected = SCANNER_FILE_TYPE_ALL;
    }
    *out_file_type_filter = (ScannerFileType)selected;
    *out_max_file_size_bytes = size_mb == 0 ? 0 : (uint64_t)size_mb * 1024ULL * 1024ULL;
    return true;
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

    wchar_t lang_str[16] = {0};
    GetPrivateProfileStringW(L"Settings", L"Language", L"en", lang_str, 16, ini);
    if (_wcsicmp(lang_str, L"id") == 0) {
        i18n_init(LANG_ID);
    } else {
        i18n_init(LANG_EN); /* Default = English */
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

    AppLanguage cur_lang = i18n_get_language();
    WritePrivateProfileStringW(L"Settings", L"Language", (cur_lang == LANG_ID) ? L"id" : L"en", ini);
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

/* Forward declarations */
static void update_method_ui_labels(int method_idx);
static void trigger_live_preview(void);
static void scan_renamer_folder(HWND hWnd);
static void populate_tree_drives(HWND hTree);
static void populate_explorer_list(HWND hList, const wchar_t *folder_path);
static void show_forgot_password_dialog(HWND hWndParent);
static void layout_controls(HWND hWnd, int cx, int cy);

static void append_log(HWND hLog, const wchar_t *text) {
    if (!hLog || !text) return;
    int len = GetWindowTextLengthW(hLog);
    SendMessageW(hLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hLog, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(hLog, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    SendMessageW(hLog, EM_SCROLLCARET, 0, 0);
}

static void update_login_attempt_ui(void) {
    int attempts = vault_get_failed_attempts();
    int rem = MAX_FAILED_ATTEMPTS - attempts;
    if (rem < 0) rem = 0;

    bool is_id = (i18n_get_language() == LANG_ID);
    wchar_t txt[128];
    if (is_id) {
        swprintf(txt, 128, L"Percobaan gagal: %d / %d (Sisa: %d)", attempts, MAX_FAILED_ATTEMPTS, rem);
    } else {
        swprintf(txt, 128, L"Failed attempts: %d / %d (Remaining: %d)", attempts, MAX_FAILED_ATTEMPTS, rem);
    }
    SetWindowTextW(g_hLoginLblAttempts, txt);

    if (vault_is_locked()) {
        SetWindowTextW(g_hLoginLblStatus, is_id ?
            L"❌ AKUN TERKUNCI! Anda telah gagal 5 kali.\nWajib klik 'Lupa Password' untuk reset via email!" :
            L"❌ ACCOUNT LOCKED! You have failed 5 times.\nYou must click 'Forgot Password' to reset via email!");
        EnableWindow(g_hLoginEditPass, FALSE);
        EnableWindow(g_hLoginBtnMasuk, FALSE);
    } else if (attempts > 0) {
        if (is_id) {
            swprintf(txt, 128, L"⚠️ Password salah! Tersisa %d kesempatan sebelum akun terkunci.", rem);
        } else {
            swprintf(txt, 128, L"⚠️ Incorrect password! %d attempt(s) remaining before account lockout.", rem);
        }
        SetWindowTextW(g_hLoginLblStatus, txt);
        EnableWindow(g_hLoginEditPass, TRUE);
        EnableWindow(g_hLoginBtnMasuk, TRUE);
    } else {
        SetWindowTextW(g_hLoginLblStatus, is_id ?
            L"Silakan masukkan Master Password Anda untuk mengakses folder." :
            L"Please enter your Master Password to unlock folder access & encryption.");
        EnableWindow(g_hLoginEditPass, TRUE);
        EnableWindow(g_hLoginBtnMasuk, TRUE);
    }
}

static void show_page(int page_idx) {
    bool on_enc = (page_idx == 0);
    bool on_ren = (page_idx == 1);
    bool on_dup = (page_idx == 2);
    bool on_model = (page_idx == 3);

    bool is_cfg = vault_is_configured();

    /* Setup Controls */
    for (int i = 0; i < g_nSetupCtrls; i++) {
        if (g_hSetupCtrls[i]) ShowWindow(g_hSetupCtrls[i], (on_enc && !is_cfg) ? SW_SHOW : SW_HIDE);
    }

    /* Login Controls */
    for (int i = 0; i < g_nLoginCtrls; i++) {
        if (g_hLoginCtrls[i]) ShowWindow(g_hLoginCtrls[i], (on_enc && is_cfg && !g_bAuthenticated) ? SW_SHOW : SW_HIDE);
    }

    /* Authenticated Explorer Controls */
    for (int i = 0; i < g_nExplorerCtrls; i++) {
        if (g_hExplorerCtrls[i]) ShowWindow(g_hExplorerCtrls[i], (on_enc && is_cfg && g_bAuthenticated) ? SW_SHOW : SW_HIDE);
    }

    /* Page 2 Renamer Controls */
    for (int i = 0; i < g_nRenCtrls; i++) {
        if (g_hRenCtrls[i]) ShowWindow(g_hRenCtrls[i], on_ren ? SW_SHOW : SW_HIDE);
    }

    /* Page 3 Duplicate Controls */
    for (int i = 0; i < g_nDupCtrls; i++) {
        if (g_hDupCtrls[i]) ShowWindow(g_hDupCtrls[i], on_dup ? SW_SHOW : SW_HIDE);
    }

    for (int i = 0; i < g_nModelCtrls; i++) {
        if (g_hModelCtrls[i]) ShowWindow(g_hModelCtrls[i], (on_model && g_bAuthenticated) ? SW_SHOW : SW_HIDE);
    }
    for (int i = 0; i < g_nModelLockCtrls; i++) {
        if (g_hModelLockCtrls[i]) ShowWindow(g_hModelLockCtrls[i], (on_model && !g_bAuthenticated) ? SW_SHOW : SW_HIDE);
    }

    if (on_enc) {
        if (is_cfg && !g_bAuthenticated) {
            update_login_attempt_ui();
            if (g_hLoginEditPass && IsWindowVisible(g_hLoginEditPass) && IsWindowEnabled(g_hLoginEditPass)) {
                SetFocus(g_hLoginEditPass);
            }
        } else if (!is_cfg) {
            if (g_hSetupEditPass && IsWindowVisible(g_hSetupEditPass)) {
                SetFocus(g_hSetupEditPass);
            }
        } else if (is_cfg && g_bAuthenticated) {
            if (g_szCurrentExplorerFolder[0] != L'\0') {
                populate_explorer_list(g_hEncExplorerList, g_szCurrentExplorerFolder);
            }
        }
    } else if (on_ren) {
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
    } else if (on_dup) {
        wchar_t f1[MAX_PATH * 2] = {0};
        wchar_t f3[MAX_PATH * 2] = {0};
        GetWindowTextW(g_hEditFolder, f1, MAX_PATH * 2);
        GetWindowTextW(g_hDupEditFolder, f3, MAX_PATH * 2);
        if (wcslen(f3) == 0) {
            if (wcslen(f1) > 0) {
                SetWindowTextW(g_hDupEditFolder, f1);
            } else if (g_szLastFolder[0] != L'\0') {
                SetWindowTextW(g_hDupEditFolder, g_szLastFolder);
            }
        }
    } else if (on_model) {
        if (g_bAuthenticated) {
            refresh_model_listview();
        } else {
            SetWindowTextW(g_hModelLockInfo, i18n_get(STR_MODEL_LOCKED));
            SetFocus(g_hModelLockLogin);
        }
    }

    RECT rc;
    GetClientRect(g_hWndMain, &rc);
    layout_controls(g_hWndMain, rc.right - rc.left, rc.bottom - rc.top);

    InvalidateRect(g_hWndMain, NULL, TRUE);
    UpdateWindow(g_hWndMain);
}

/* ========================================================================= */
/* Explorer Tree & List Helper Functions                                     */
/* ========================================================================= */

static void populate_tree_drives(HWND hTree) {
    if (!hTree) return;
    TreeView_DeleteAllItems(hTree);

    wchar_t drives[512] = {0};
    DWORD len = GetLogicalDriveStringsW(511, drives);
    if (len == 0) return;

    wchar_t *p = drives;
    while (*p) {
        TVINSERTSTRUCTW tvis = {0};
        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
        tvis.item.pszText = p;
        tvis.item.cChildren = 1;

        SHFILEINFOW sfi = {0};
        SHGetFileInfoW(p, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
        tvis.item.iImage = sfi.iIcon;
        tvis.item.iSelectedImage = sfi.iIcon;
        tvis.item.lParam = (LPARAM)_wcsdup(p);

        TreeView_InsertItem(hTree, &tvis);
        p += wcslen(p) + 1;
    }
}

static void handle_tree_expanding(HWND hTree, NMTREEVIEWW *pnmtv) {
    if (pnmtv->action != TVE_EXPAND) return;

    HTREEITEM hItem = pnmtv->itemNew.hItem;
    HTREEITEM hChild = TreeView_GetChild(hTree, hItem);
    if (hChild && TreeView_GetNextSibling(hTree, hChild)) {
        return;
    }

    wchar_t *parent_path = (wchar_t *)pnmtv->itemNew.lParam;
    if (!parent_path) return;

    wchar_t search_pattern[MAX_PATH * 2];
    swprintf(search_pattern, MAX_PATH * 2, L"%ls%ls*", parent_path,
        (parent_path[wcslen(parent_path) - 1] == L'\\') ? L"" : L"\\");

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search_pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    if (hChild) TreeView_DeleteItem(hTree, hChild);

    do {
        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(ffd.cFileName, L".") != 0 &&
            wcscmp(ffd.cFileName, L"..") != 0) {

            wchar_t full_sub[MAX_PATH * 2];
            swprintf(full_sub, MAX_PATH * 2, L"%ls%ls%ls", parent_path,
                (parent_path[wcslen(parent_path) - 1] == L'\\') ? L"" : L"\\", ffd.cFileName);

            TVINSERTSTRUCTW tvis = {0};
            tvis.hParent = hItem;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
            tvis.item.pszText = ffd.cFileName;
            tvis.item.cChildren = 1;

            SHFILEINFOW sfi = {0};
            SHGetFileInfoW(full_sub, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
            tvis.item.iImage = sfi.iIcon;
            tvis.item.iSelectedImage = sfi.iIcon;
            tvis.item.lParam = (LPARAM)_wcsdup(full_sub);

            TreeView_InsertItem(hTree, &tvis);
        }
    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);
}

/* ========================================================================= */
/* Explorer Listview Item Data & Sorting Helpers                            */
/* ========================================================================= */

typedef struct {
    wchar_t name[MAX_PATH];
    wchar_t type[80];
    uint64_t size;
    bool is_directory;
    FILETIME ftModified;
    wchar_t status[64];
} ExplorerItemData;

static int g_nExplorerSortCol = -1;
static bool g_bExplorerSortAsc = true;

static void clear_explorer_list_items(HWND hList) {
    if (!hList) return;
    int count = ListView_GetItemCount(hList);
    for (int i = 0; i < count; i++) {
        LVITEMW lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        if (ListView_GetItem(hList, &lvi) && lvi.lParam) {
            free((void *)lvi.lParam);
            lvi.lParam = 0;
            ListView_SetItem(hList, &lvi);
        }
    }
    ListView_DeleteAllItems(hList);
}

static void update_explorer_header_sort_arrow(HWND hList, int sortCol, bool asc) {
    if (!hList) return;
    HWND hHeader = ListView_GetHeader(hList);
    if (!hHeader) return;

    int colCount = Header_GetItemCount(hHeader);
    for (int i = 0; i < colCount; i++) {
        HDITEMW hdi = {0};
        hdi.mask = HDI_FORMAT;
        if (Header_GetItem(hHeader, i, &hdi)) {
            hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            if (i == sortCol) {
                hdi.fmt |= (asc ? HDF_SORTUP : HDF_SORTDOWN);
            }
            Header_SetItem(hHeader, i, &hdi);
        }
    }
}

static int CALLBACK explorer_list_compare_func(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
    const ExplorerItemData *item1 = (const ExplorerItemData *)lParam1;
    const ExplorerItemData *item2 = (const ExplorerItemData *)lParam2;
    if (!item1 || !item2) {
        if (!item1 && !item2) return 0;
        return (!item1) ? -1 : 1;
    }

    int col = (int)(lParamSort & 0xFFFF);
    bool asc = ((lParamSort >> 16) & 1) != 0;

    /* Folders always stay grouped at the top regardless of sort direction */
    if (item1->is_directory != item2->is_directory) {
        return item1->is_directory ? -1 : 1;
    }

    int result = 0;
    switch (col) {
        case 0: /* Nama Berkas / Folder */
            result = _wcsicmp(item1->name, item2->name);
            break;

        case 1: /* Tipe */
            result = _wcsicmp(item1->type, item2->type);
            if (result == 0) {
                result = _wcsicmp(item1->name, item2->name);
            }
            break;

        case 2: /* Ukuran */
            if (item1->is_directory && item2->is_directory) {
                result = _wcsicmp(item1->name, item2->name);
            } else if (item1->size < item2->size) {
                result = -1;
            } else if (item1->size > item2->size) {
                result = 1;
            } else {
                result = _wcsicmp(item1->name, item2->name);
            }
            break;

        case 3: /* Tanggal Dimodifikasi */
            result = CompareFileTime(&item1->ftModified, &item2->ftModified);
            if (result == 0) {
                result = _wcsicmp(item1->name, item2->name);
            }
            break;

        case 4: /* Status Enkripsi */
            result = _wcsicmp(item1->status, item2->status);
            if (result == 0) {
                result = _wcsicmp(item1->name, item2->name);
            }
            break;

        default:
            result = _wcsicmp(item1->name, item2->name);
            break;
    }

    return asc ? result : -result;
}

static void sort_explorer_list(HWND hList, int col) {
    if (!hList || col < 0 || col > 4) return;

    if (g_nExplorerSortCol == col) {
        g_bExplorerSortAsc = !g_bExplorerSortAsc;
    } else {
        g_nExplorerSortCol = col;
        g_bExplorerSortAsc = true;
    }

    LPARAM lParamSort = (LPARAM)((col & 0xFFFF) | ((g_bExplorerSortAsc ? 1 : 0) << 16));
    ListView_SortItems(hList, explorer_list_compare_func, lParamSort);
    update_explorer_header_sort_arrow(hList, g_nExplorerSortCol, g_bExplorerSortAsc);
}

static void populate_explorer_list(HWND hList, const wchar_t *folder_path) {
    if (!hList || !folder_path || wcslen(folder_path) == 0) return;

    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    clear_explorer_list_items(hList);

    wchar_t search[MAX_PATH * 2];
    swprintf(search, MAX_PATH * 2, L"%ls%ls*", folder_path,
        (folder_path[wcslen(folder_path) - 1] == L'\\') ? L"" : L"\\");

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        SendMessage(hList, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hList, NULL, TRUE);
        return;
    }

    bool is_id = (i18n_get_language() == LANG_ID);
    int item_idx = 0;
    /* 1. Folders */
    do {
        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(ffd.cFileName, L".") != 0 &&
            wcscmp(ffd.cFileName, L"..") != 0) {

            wchar_t full_item[MAX_PATH * 2];
            swprintf(full_item, MAX_PATH * 2, L"%ls%ls%ls", folder_path,
                (folder_path[wcslen(folder_path) - 1] == L'\\') ? L"" : L"\\", ffd.cFileName);

            SHFILEINFOW sfi = {0};
            SHGetFileInfoW(full_item, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_TYPENAME);

            ExplorerItemData *data = (ExplorerItemData *)calloc(1, sizeof(ExplorerItemData));
            if (data) {
                wcsncpy(data->name, ffd.cFileName, MAX_PATH - 1);
                wcsncpy(data->type, sfi.szTypeName[0] ? sfi.szTypeName : (is_id ? L"Folder Berkas" : L"File folder"), 79);
                data->size = 0;
                data->is_directory = true;
                data->ftModified = ffd.ftLastWriteTime;
                wcsncpy(data->status, is_id ? L"Direktori" : L"Directory", 63);
            }

            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvi.iItem = item_idx;
            lvi.iSubItem = 0;
            lvi.pszText = ffd.cFileName;
            lvi.iImage = sfi.iIcon;
            lvi.lParam = (LPARAM)data;
            ListView_InsertItem(hList, &lvi);

            ListView_SetItemText(hList, item_idx, 1, sfi.szTypeName[0] ? sfi.szTypeName : (is_id ? L"Folder Berkas" : L"File folder"));
            ListView_SetItemText(hList, item_idx, 2, is_id ? L"<Folder>" : L"<Folder>");

            SYSTEMTIME st;
            FileTimeToSystemTime(&ffd.ftLastWriteTime, &st);
            wchar_t date_str[64];
            swprintf(date_str, 64, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
            ListView_SetItemText(hList, item_idx, 3, date_str);
            ListView_SetItemText(hList, item_idx, 4, is_id ? L"Direktori" : L"Directory");

            item_idx++;
        }
    } while (FindNextFileW(hFind, &ffd));
    FindClose(hFind);

    /* 2. Files */
    hFind = FindFirstFileW(search, &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                wchar_t full_item[MAX_PATH * 2];
                swprintf(full_item, MAX_PATH * 2, L"%ls%ls%ls", folder_path,
                    (folder_path[wcslen(folder_path) - 1] == L'\\') ? L"" : L"\\", ffd.cFileName);

                SHFILEINFOW sfi = {0};
                SHGetFileInfoW(full_item, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_TYPENAME);

                ULARGE_INTEGER uli;
                uli.LowPart = ffd.nFileSizeLow;
                uli.HighPart = ffd.nFileSizeHigh;

                ExplorerItemData *data = (ExplorerItemData *)calloc(1, sizeof(ExplorerItemData));
                if (data) {
                    wcsncpy(data->name, ffd.cFileName, MAX_PATH - 1);
                    wcsncpy(data->type, sfi.szTypeName[0] ? sfi.szTypeName : (is_id ? L"Berkas" : L"File"), 79);
                    data->size = uli.QuadPart;
                    data->is_directory = false;
                    data->ftModified = ffd.ftLastWriteTime;
                    const wchar_t *ext = wcsrchr(ffd.cFileName, L'.');
                    if (ext && _wcsicmp(ext, ENC_FILE_EXTENSION) == 0) {
                        wcsncpy(data->status, i18n_get(STR_STATUS_ENCRYPTED_ENC), 63);
                    } else if (tracker_is_file_decrypted(full_item)) {
                        wcsncpy(data->status, i18n_get(STR_STATUS_DECRYPTED_LOCK), 63);
                    } else {
                        wcsncpy(data->status, i18n_get(STR_STATUS_ORIGINAL_FILE), 63);
                    }
                }

                LVITEMW lvi = {0};
                lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                lvi.iItem = item_idx;
                lvi.iSubItem = 0;
                lvi.pszText = ffd.cFileName;
                lvi.iImage = sfi.iIcon;
                lvi.lParam = (LPARAM)data;
                ListView_InsertItem(hList, &lvi);

                ListView_SetItemText(hList, item_idx, 1, sfi.szTypeName[0] ? sfi.szTypeName : (is_id ? L"Berkas" : L"File"));

                wchar_t sz_str[32];
                if (uli.QuadPart >= 1024 * 1024) {
                    swprintf(sz_str, 32, L"%.2f MB", (double)uli.QuadPart / (1024.0 * 1024.0));
                } else {
                    swprintf(sz_str, 32, L"%.1f KB", (double)uli.QuadPart / 1024.0);
                }
                ListView_SetItemText(hList, item_idx, 2, sz_str);

                SYSTEMTIME st;
                FileTimeToSystemTime(&ffd.ftLastWriteTime, &st);
                wchar_t date_str[64];
                swprintf(date_str, 64, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
                ListView_SetItemText(hList, item_idx, 3, date_str);

                ListView_SetItemText(hList, item_idx, 4, data ? data->status : (LPWSTR)i18n_get(STR_STATUS_ORIGINAL_FILE));

                item_idx++;
            }
        } while (FindNextFileW(hFind, &ffd));
        FindClose(hFind);
    }

    if (g_nExplorerSortCol >= 0) {
        LPARAM lParamSort = (LPARAM)((g_nExplorerSortCol & 0xFFFF) | ((g_bExplorerSortAsc ? 1 : 0) << 16));
        ListView_SortItems(hList, explorer_list_compare_func, lParamSort);
        update_explorer_header_sort_arrow(hList, g_nExplorerSortCol, g_bExplorerSortAsc);
    }

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hList, NULL, TRUE);

    wchar_t status[256];
    swprintf(status, 256, L"Memuat folder: %ls (%d item)", folder_path, item_idx);
    SetWindowTextW(g_hStaticStatus, status);
}

/* ========================================================================= */
/* Encryption Worker & Auto-Encryption                                       */
/* ========================================================================= */

static DWORD WINAPI encrypt_worker_thread(LPVOID lpParam) {
    WorkerParams *wp = (WorkerParams *)lpParam;
    if (!wp) return 1;

    JobConfig cfg = wp->config;
    HWND hWnd = wp->hWnd;

    PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(L"--- Memulai Proses Enkripsi Militer AES-256-GCM ---"));

    FileList *files = NULL;
    if (wp->custom_files) {
        files = wp->custom_files;
    } else {
        files = scanner_create();
        if (!files || !scanner_scan(cfg.folder_path, cfg.recursive, cfg.op_mode, files)) {
            PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(L"[INFO] Tidak ditemukan file yang dapat diproses di dalam folder ini."));
            if (files) scanner_free(files);
            SecureZeroMemory(&cfg, sizeof(cfg));
            free(wp);
            PostMessageW(hWnd, WM_APP_FINISHED, 0, 0);
            return 0;
        }
    }

    size_t skipped_by_filter = scanner_filter_files(files, wp->file_type_filter, wp->max_file_size_bytes);
    if (skipped_by_filter > 0) {
        wchar_t filter_log[192];
        swprintf(filter_log, 192, L"[INFO] %zu berkas dilewati karena filter tipe atau ukuran.", skipped_by_filter);
        PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(filter_log));
    }

    if (!files || files->count == 0) {
        PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(L"[INFO] Tidak ada file yang diproses."));
        if (files) scanner_free(files);
        SecureZeroMemory(&cfg, sizeof(cfg));
        free(wp);
        PostMessageW(hWnd, WM_APP_FINISHED, 0, 0);
        return 0;
    }

    size_t success_count = 0;
    size_t fail_count = 0;
    size_t total = files->count;

    for (size_t i = 0; i < total; i++) {
        if (g_bCancelRequested) {
            PostMessageW(hWnd, WM_APP_LOG_MSG, 0, (LPARAM)_wcsdup(L"[BATAL] Operasi dihentikan oleh pengguna."));
            break;
        }

        const wchar_t *src_file = files->paths[i];
        PostMessageW(hWnd, WM_APP_PROGRESS, (WPARAM)(i + 1), (LPARAM)total);

        wchar_t msg[512];

        /* Skip if already .enc */
        size_t slen = wcslen(src_file);
        if (slen > 4 && _wcsicmp(src_file + slen - 4, ENC_FILE_EXTENSION) == 0) {
            fail_count++;
            continue;
        }

        uint8_t file_salt[SALT_SIZE];
        crypto_generate_random(file_salt, sizeof(file_salt));

        uint8_t derived_key[KEY_SIZE_AES256];
        crypto_derive_key(
            cfg.email,
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
                tracker_remove_file(src_file);
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

    ScannerFileType file_type_filter = SCANNER_FILE_TYPE_ALL;
    uint64_t max_file_size_bytes = 0;
    if (!read_file_filters(hWnd, g_hEncFileType, g_hEncMaxSizeEdit, &file_type_filter, &max_file_size_bytes)) return;

    save_settings(folder);

    const wchar_t *session_pass = vault_get_session_password();
    if (!session_pass || wcslen(session_pass) == 0) {
        MessageBoxW(hWnd, L"Sesi autentikasi telah berakhir! Silakan masuk kembali.", L"Peringatan", MB_OK | MB_ICONWARNING);
        g_bAuthenticated = false;
        clear_kiosapi_key();
        show_page(0);
        return;
    }

    WorkerParams *wp = (WorkerParams *)malloc(sizeof(WorkerParams));
    if (!wp) return;

    wcsncpy(wp->config.folder_path, folder, MAX_PATH * 2 - 1);
    wp->config.recursive = (SendMessage(g_hChkRecursive, BM_GETCHECK, 0, 0) == BST_CHECKED);
    wp->config.op_mode = OP_MODE_ENCRYPT_AND_SHRED;
    wp->config.cipher_id = CIPHER_AES_256_GCM;
    vault_get_recovery_email(wp->config.email, 255);
    wp->hWnd = hWnd;
    wp->custom_files = NULL;
    wp->file_type_filter = file_type_filter;
    wp->max_file_size_bytes = max_file_size_bytes;

    g_bIsWorking = true;
    g_bCancelRequested = false;

    SendMessage(g_hProgress, PBM_SETRANGE32, 0, 100);
    SendMessage(g_hProgress, PBM_SETPOS, 0, 0);
    SetWindowTextW(g_hStaticStatus, L"Memproses enkripsi & pemusnahan file asli...");

    g_hWorkerThread = CreateThread(NULL, 0, encrypt_worker_thread, wp, 0, NULL);
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

            if (hEdit == g_hEditFolder) {
                wcsncpy(g_szCurrentExplorerFolder, path, MAX_PATH * 2 - 1);
                populate_explorer_list(g_hEncExplorerList, path);
            } else if (hEdit == g_hRenEditFolder) {
                scan_renamer_folder(hWnd);
            }
        }
        CoTaskMemFree(pidl);
    }
}

static WNDPROC g_pfnOrigExplorerListProc = NULL;

static LRESULT CALLBACK explorer_list_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN) {
        if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            ListView_SetItemState(hWnd, -1, LVIS_SELECTED, LVIS_SELECTED);
            return 0;
        }
    }
    return CallWindowProcW(g_pfnOrigExplorerListProc, hWnd, uMsg, wParam, lParam);
}

static void encrypt_selected_files(HWND hWnd) {
    if (g_bIsWorking) return;
    if (!g_bAuthenticated) {
        MessageBoxW(hWnd, i18n_get(STR_SESSION_EXPIRED), L"Peringatan", MB_OK | MB_ICONWARNING);
        return;
    }

    int sel_count = ListView_GetSelectedCount(g_hEncExplorerList);
    if (sel_count == 0) return;

    ScannerFileType file_type_filter = SCANNER_FILE_TYPE_ALL;
    uint64_t max_file_size_bytes = 0;
    if (!read_file_filters(hWnd, g_hEncFileType, g_hEncMaxSizeEdit, &file_type_filter, &max_file_size_bytes)) return;

    FileList *custom_files = scanner_create();
    if (!custom_files) return;

    int item = -1;
    while ((item = ListView_GetNextItem(g_hEncExplorerList, item, LVNI_SELECTED)) != -1) {
        wchar_t name[MAX_PATH] = {0};
        ListView_GetItemText(g_hEncExplorerList, item, 0, name, MAX_PATH - 1);
        wchar_t sz_type[64] = {0};
        ListView_GetItemText(g_hEncExplorerList, item, 2, sz_type, 63);

        wchar_t full_path[MAX_PATH * 2];
        swprintf(full_path, MAX_PATH * 2, L"%ls%ls%ls",
            g_szCurrentExplorerFolder,
            (g_szCurrentExplorerFolder[wcslen(g_szCurrentExplorerFolder)-1] == L'\\') ? L"" : L"\\",
            name);

        if (wcscmp(sz_type, L"<Folder>") == 0) {
            scanner_scan(full_path, SendMessage(g_hChkRecursive, BM_GETCHECK, 0, 0) == BST_CHECKED, OP_MODE_ENCRYPT_AND_SHRED, custom_files);
        } else {
            const wchar_t *ext = wcsrchr(name, L'.');
            if (!ext || _wcsicmp(ext, ENC_FILE_EXTENSION) != 0) {
                scanner_add_file(custom_files, full_path, 0);
            }
        }
    }

    scanner_filter_files(custom_files, file_type_filter, max_file_size_bytes);
    if (custom_files->count == 0) {
        MessageBoxW(
            hWnd,
            (i18n_get_language() == LANG_ID) ?
            L"Tidak ada berkas yang cocok dengan filter tipe file dan ukuran maksimum." :
            L"No files match the selected file-type and maximum-size filters.",
            i18n_get(STR_APP_TITLE),
            MB_OK | MB_ICONINFORMATION
        );
        scanner_free(custom_files);
        return;
    }

    wchar_t prompt[512];
    if (i18n_get_language() == LANG_ID) {
        swprintf(prompt, 512,
            L"Apakah Anda yakin ingin mengenkripsi %zu berkas yang dipilih?\n\n"
            L"(Berkas asli akan dimusnahkan secara aman setelah dienkripsi AES-256-GCM)",
            custom_files->count);
    } else {
        swprintf(prompt, 512,
            L"Are you sure you want to encrypt %zu selected file(s)?\n\n"
            L"(Original files will be securely shredded after AES-256-GCM encryption)",
            custom_files->count);
    }

    int res = MessageBoxW(
        hWnd,
        prompt,
        (i18n_get_language() == LANG_ID) ? L"Konfirmasi Enkripsi Berkas" : L"Confirm File Encryption",
        MB_YESNO | MB_ICONQUESTION
    );

    if (res != IDYES) {
        scanner_free(custom_files);
        return;
    }

    WorkerParams *wp = (WorkerParams *)malloc(sizeof(WorkerParams));
    if (!wp) {
        scanner_free(custom_files);
        return;
    }

    wcsncpy(wp->config.folder_path, g_szCurrentExplorerFolder, MAX_PATH * 2 - 1);
    wp->config.recursive = false;
    wp->config.op_mode = OP_MODE_ENCRYPT_AND_SHRED;
    wp->config.cipher_id = CIPHER_AES_256_GCM;
    vault_get_recovery_email(wp->config.email, 255);
    wp->hWnd = hWnd;
    wp->custom_files = custom_files;
    wp->file_type_filter = file_type_filter;
    wp->max_file_size_bytes = max_file_size_bytes;

    g_bIsWorking = true;
    g_bCancelRequested = false;

    SendMessage(g_hProgress, PBM_SETRANGE32, 0, (LPARAM)custom_files->count);
    SendMessage(g_hProgress, PBM_SETPOS, 0, 0);
    SetWindowTextW(g_hStaticStatus,
        (i18n_get_language() == LANG_ID) ? L"Memproses enkripsi berkas yang dipilih..." : L"Encrypting selected files...");

    g_hWorkerThread = CreateThread(NULL, 0, encrypt_worker_thread, wp, 0, NULL);
}

static void decrypt_selected_files(HWND hWnd) {
    if (g_bIsWorking) return;
    if (!g_bAuthenticated) {
        MessageBoxW(hWnd, i18n_get(STR_SESSION_EXPIRED), L"Peringatan", MB_OK | MB_ICONWARNING);
        return;
    }

    int sel_count = ListView_GetSelectedCount(g_hEncExplorerList);
    if (sel_count == 0) return;

    int decrypted = 0;
    int item = -1;
    while ((item = ListView_GetNextItem(g_hEncExplorerList, item, LVNI_SELECTED)) != -1) {
        wchar_t name[MAX_PATH] = {0};
        ListView_GetItemText(g_hEncExplorerList, item, 0, name, MAX_PATH - 1);

        const wchar_t *ext = wcsrchr(name, L'.');
        if (ext && _wcsicmp(ext, ENC_FILE_EXTENSION) == 0) {
            wchar_t full_path[MAX_PATH * 2];
            swprintf(full_path, MAX_PATH * 2, L"%ls%ls%ls",
                g_szCurrentExplorerFolder,
                (g_szCurrentExplorerFolder[wcslen(g_szCurrentExplorerFolder)-1] == L'\\') ? L"" : L"\\",
                name);

            if (service_auto_decrypt_single_file(full_path, false)) {
                decrypted++;
            }
        }
    }

    if (g_szCurrentExplorerFolder[0] != L'\0') {
        populate_explorer_list(g_hEncExplorerList, g_szCurrentExplorerFolder);
    }

    wchar_t done_msg[256];
    if (i18n_get_language() == LANG_ID) {
        swprintf(done_msg, 256, L"Berhasil mendekripsi %d berkas terpilih.", decrypted);
    } else {
        swprintf(done_msg, 256, L"Successfully decrypted %d selected file(s).", decrypted);
    }
    MessageBoxW(hWnd, done_msg, (i18n_get_language() == LANG_ID) ? L"Informasi" : L"Information", MB_OK | MB_ICONINFORMATION);
}

static void handle_explorer_context_menu(HWND hWnd, int x, int y) {
    if (!g_hEncExplorerList || !g_bAuthenticated) return;

    POINT pt = { x, y };
    if (x == -1 && y == -1) {
        int sel = ListView_GetNextItem(g_hEncExplorerList, -1, LVNI_SELECTED);
        if (sel >= 0) {
            RECT rc;
            ListView_GetItemRect(g_hEncExplorerList, sel, &rc, LVIR_LABEL);
            pt.x = (rc.left + rc.right) / 2;
            pt.y = (rc.top + rc.bottom) / 2;
            ClientToScreen(g_hEncExplorerList, &pt);
        } else {
            RECT rc;
            GetClientRect(g_hEncExplorerList, &rc);
            pt.x = (rc.left + rc.right) / 2;
            pt.y = (rc.top + rc.bottom) / 2;
            ClientToScreen(g_hEncExplorerList, &pt);
        }
    } else {
        POINT clientPt = pt;
        ScreenToClient(g_hEncExplorerList, &clientPt);
        LVHITTESTINFO lvhti = {0};
        lvhti.pt = clientPt;
        int hit = ListView_HitTest(g_hEncExplorerList, &lvhti);
        if (hit >= 0) {
            UINT state = ListView_GetItemState(g_hEncExplorerList, hit, LVIS_SELECTED);
            if (!(state & LVIS_SELECTED)) {
                ListView_SetItemState(g_hEncExplorerList, -1, 0, LVIS_SELECTED);
                ListView_SetItemState(g_hEncExplorerList, hit, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            }
        }
    }

    int sel_count = ListView_GetSelectedCount(g_hEncExplorerList);
    int enc_count = 0;

    int item = -1;
    while ((item = ListView_GetNextItem(g_hEncExplorerList, item, LVNI_SELECTED)) != -1) {
        wchar_t name[MAX_PATH] = {0};
        ListView_GetItemText(g_hEncExplorerList, item, 0, name, MAX_PATH - 1);
        wchar_t sz_type[64] = {0};
        ListView_GetItemText(g_hEncExplorerList, item, 2, sz_type, 63);

        if (wcscmp(sz_type, L"<Folder>") != 0) {
            const wchar_t *ext = wcsrchr(name, L'.');
            if (ext && _wcsicmp(ext, ENC_FILE_EXTENSION) == 0) {
                enc_count++;
            }
        }
    }

    HMENU hMenu = CreatePopupMenu();
    if (sel_count > 0) {
        wchar_t enc_label[128];
        swprintf(enc_label, 128, L"%ls (%d)", i18n_get(STR_EXPLORER_MENU_ENCRYPT_SEL), sel_count);
        AppendMenuW(hMenu, MF_STRING, IDC_MENU_EXPLORER_ENCRYPT_SEL, enc_label);

        if (enc_count > 0) {
            wchar_t dec_label[128];
            swprintf(dec_label, 128, L"%ls (%d)", i18n_get(STR_EXPLORER_MENU_DECRYPT_SEL), enc_count);
            AppendMenuW(hMenu, MF_STRING, IDC_MENU_EXPLORER_DECRYPT_SEL, dec_label);
        }
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    }

    AppendMenuW(hMenu, MF_STRING, IDC_MENU_EXPLORER_SELECT_ALL, i18n_get(STR_EXPLORER_MENU_SELECT_ALL));
    if (g_szCurrentExplorerFolder[0] != L'\0') {
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, IDC_MENU_EXPLORER_ENCRYPT_ALL,
            (i18n_get_language() == LANG_ID) ? L"📁 Enkripsi seluruh folder ini" : L"📁 Encrypt entire folder");
    }

    SetForegroundWindow(hWnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);

    if (cmd == IDC_MENU_EXPLORER_ENCRYPT_SEL) {
        encrypt_selected_files(hWnd);
    } else if (cmd == IDC_MENU_EXPLORER_DECRYPT_SEL) {
        decrypt_selected_files(hWnd);
    } else if (cmd == IDC_MENU_EXPLORER_SELECT_ALL) {
        ListView_SetItemState(g_hEncExplorerList, -1, LVIS_SELECTED, LVIS_SELECTED);
    } else if (cmd == IDC_MENU_EXPLORER_ENCRYPT_ALL) {
        start_encryption_processing(hWnd);
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
        case 0:
            cfg->method = RENAME_METHOD_REPLACE;
            wcsncpy(cfg->find_text, p1, 127);
            wcsncpy(cfg->replace_text, p2, 127);
            cfg->case_sensitive = false;
            break;
        case 1:
            cfg->method = RENAME_METHOD_ADD;
            wcsncpy(cfg->add_text, p1, 127);
            cfg->add_pos = (SendMessage(g_hRenComboP3, CB_GETCURSEL, 0, 0) == 1) ? ADD_POS_SUFFIX : ADD_POS_PREFIX;
            break;
        case 2:
            cfg->method = RENAME_METHOD_PATTERN;
            wcsncpy(cfg->pattern, p1, 255);
            cfg->start_number = _wtoi(n2);
            if (cfg->start_number <= 0) cfg->start_number = 1;
            cfg->number_step = 1;
            break;
        case 3:
            cfg->method = RENAME_METHOD_CASE;
            cfg->case_type = (CaseChangeType)SendMessage(g_hRenComboP3, CB_GETCURSEL, 0, 0);
            cfg->apply_to_extension = false;
            break;
        case 4:
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
    ShowWindow(g_hRenLblP1, SW_HIDE);
    ShowWindow(g_hRenEditP1, SW_HIDE);
    ShowWindow(g_hRenLblP2, SW_HIDE);
    ShowWindow(g_hRenEditP2, SW_HIDE);
    ShowWindow(g_hRenComboP3, SW_HIDE);

    ShowWindow(g_hRenBtnDec1, SW_HIDE);
    ShowWindow(g_hRenEditNum1, SW_HIDE);
    ShowWindow(g_hRenBtnInc1, SW_HIDE);

    ShowWindow(g_hRenBtnDec2, SW_HIDE);
    ShowWindow(g_hRenEditNum2, SW_HIDE);
    ShowWindow(g_hRenBtnInc2, SW_HIDE);

    switch (method_idx) {
        case 0: /* Replace */
            SetWindowTextW(g_hRenLblP1, i18n_get(STR_REN_LBL_FIND));
            SetWindowTextW(g_hRenLblP2, i18n_get(STR_REN_LBL_REPLACE));
            SetWindowPos(g_hRenLblP1, NULL, 300, 170, 110, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenEditP1, NULL, 415, 167, 150, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenLblP2, NULL, 575, 170, 135, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenEditP2, NULL, 715, 167, 95, 24, SWP_NOZORDER);

            ShowWindow(g_hRenLblP1, SW_SHOW);
            ShowWindow(g_hRenEditP1, SW_SHOW);
            ShowWindow(g_hRenLblP2, SW_SHOW);
            ShowWindow(g_hRenEditP2, SW_SHOW);
            break;

        case 1: /* Add */
            SetWindowTextW(g_hRenLblP1, i18n_get(STR_REN_LBL_INSERT));
            SetWindowTextW(g_hRenLblP2, i18n_get(STR_REN_LBL_POS));
            SetWindowPos(g_hRenLblP1, NULL, 300, 170, 100, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenEditP1, NULL, 405, 167, 180, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenLblP2, NULL, 595, 170, 60, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenComboP3, NULL, 660, 167, 150, 120, SWP_NOZORDER);

            SendMessage(g_hRenComboP3, CB_RESETCONTENT, 0, 0);
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_POS_PREFIX));
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_POS_SUFFIX));
            SendMessage(g_hRenComboP3, CB_SETCURSEL, 0, 0);

            ShowWindow(g_hRenLblP1, SW_SHOW);
            ShowWindow(g_hRenEditP1, SW_SHOW);
            ShowWindow(g_hRenLblP2, SW_SHOW);
            ShowWindow(g_hRenComboP3, SW_SHOW);
            break;

        case 2: /* Pattern */
            SetWindowTextW(g_hRenLblP1, i18n_get(STR_REN_LBL_PATTERN));
            SetWindowTextW(g_hRenLblP2, i18n_get(STR_REN_LBL_STARTNUM));
            SetWindowPos(g_hRenLblP1, NULL, 300, 170, 85, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenEditP1, NULL, 390, 167, 185, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenLblP2, NULL, 585, 170, 85, 20, SWP_NOZORDER);

            SetWindowPos(g_hRenBtnDec2, NULL, 675, 167, 28, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenEditNum2, NULL, 706, 167, 45, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnInc2, NULL, 754, 167, 28, 24, SWP_NOZORDER);

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
            break;

        case 3: /* Case */
            SetWindowTextW(g_hRenLblP1, i18n_get(STR_REN_LBL_CASERULE));
            SetWindowPos(g_hRenLblP1, NULL, 300, 170, 100, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenComboP3, NULL, 405, 167, 250, 120, SWP_NOZORDER);

            SendMessage(g_hRenComboP3, CB_RESETCONTENT, 0, 0);
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_CASE_LOWER));
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_CASE_UPPER));
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_CASE_TITLE));
            SendMessageW(g_hRenComboP3, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_CASE_SENTENCE));
            SendMessage(g_hRenComboP3, CB_SETCURSEL, 2, 0);

            ShowWindow(g_hRenLblP1, SW_SHOW);
            ShowWindow(g_hRenComboP3, SW_SHOW);
            break;

        case 4: /* Trim */
            SetWindowTextW(g_hRenLblP1, i18n_get(STR_REN_TRIM_START));
            SetWindowTextW(g_hRenLblP2, i18n_get(STR_REN_TRIM_END));
            SetWindowPos(g_hRenLblP1, NULL, 300, 170, 105, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnDec1, NULL, 410, 167, 28, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenEditNum1, NULL, 441, 167, 40, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnInc1, NULL, 484, 167, 28, 24, SWP_NOZORDER);

            SetWindowPos(g_hRenLblP2, NULL, 525, 170, 120, 20, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnDec2, NULL, 650, 167, 28, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenEditNum2, NULL, 681, 167, 40, 24, SWP_NOZORDER);
            SetWindowPos(g_hRenBtnInc2, NULL, 724, 167, 28, 24, SWP_NOZORDER);

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

            ShowWindow(g_hRenLblP2, SW_SHOW);
            ShowWindow(g_hRenBtnDec2, SW_SHOW);
            ShowWindow(g_hRenEditNum2, SW_SHOW);
            ShowWindow(g_hRenBtnInc2, SW_SHOW);
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

    for (size_t i = 0; i < g_pRenameList->count; i++) {
        ListView_SetItemText(g_hRenList, (int)i, 2, g_pRenameList->items[i].new_name);
        ListView_SetItemText(g_hRenList, (int)i, 4, g_pRenameList->items[i].status_text);
    }
}

static void scan_renamer_folder(HWND hWnd) {
    wchar_t raw[MAX_PATH * 2] = {0};
    GetWindowTextW(g_hRenEditFolder, raw, MAX_PATH * 2);

    wchar_t folder[MAX_PATH * 2] = {0};
    sanitize_path(raw, folder, MAX_PATH * 2);
    SetWindowTextW(g_hRenEditFolder, folder);

    if (wcslen(folder) == 0) return;

    DWORD attr = GetFileAttributesW(folder);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) return;

    save_settings(folder);

    bool recursive = (SendMessage(g_hRenChkRecursive, BM_GETCHECK, 0, 0) == BST_CHECKED);
    ScannerFileType file_type_filter = SCANNER_FILE_TYPE_ALL;
    uint64_t max_file_size_bytes = 0;
    if (!read_file_filters(hWnd, g_hRenFileType, g_hRenMaxSizeEdit, &file_type_filter, &max_file_size_bytes)) return;
    renamer_scan_folder_filtered(g_pRenameList, folder, recursive, file_type_filter, max_file_size_bytes);

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
    bool is_id = (i18n_get_language() == LANG_ID);
    if (!g_pRenameList || g_pRenameList->count == 0) {
        MessageBoxW(hWnd, is_id ? L"Tidak ada berkas untuk diubah namanya!" : L"No files available to rename!",
            is_id ? L"Peringatan" : L"Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    trigger_live_preview();

    bool has_collision = false;
    for (size_t i = 0; i < g_pRenameList->count; i++) {
        if (g_pRenameList->items[i].has_collision) {
            has_collision = true;
            break;
        }
    }

    if (has_collision) {
        int res = MessageBoxW(hWnd,
            is_id ?
            L"Terdeteksi potensi duplikasi/tabrakan nama berkas (Collision)!\nApakah Anda yakin tetap ingin melanjutkan?" :
            L"Potential filename collision/duplicate detected!\nAre you sure you want to proceed?",
            is_id ? L"Peringatan Konflik Nama" : L"Name Collision Warning",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        if (res != IDYES) return;
    }

    g_bIsWorking = true;
    g_bCancelRequested = false;
    EnableWindow(g_hRenBtnStart, FALSE);
    EnableWindow(g_hRenBtnUndo, FALSE);

    SetWindowTextW(g_hRenStaticStatus, is_id ? L"Sedang menjalankan batch rename..." : L"Executing batch rename...");
    g_hWorkerThread = CreateThread(NULL, 0, renamer_worker_thread, hWnd, 0, NULL);
}

static void undo_batch_rename(HWND hWnd) {
    if (g_bIsWorking) return;
    bool is_id = (i18n_get_language() == LANG_ID);
    if (!g_pUndoHistory || g_pUndoHistory->count == 0) {
        MessageBoxW(hWnd, is_id ? L"Tidak ada riwayat rename sebelumnya yang dapat dibatalkan (Undo)." : L"No previous rename history available to undo.",
            is_id ? L"Informasi" : L"Information", MB_OK | MB_ICONINFORMATION);
        return;
    }

    int res = MessageBoxW(hWnd, is_id ? L"Apakah Anda ingin membatalkan (Undo) proses rename terakhir?" : L"Are you sure you want to undo the last rename operation?",
        is_id ? L"Konfirmasi Undo" : L"Confirm Undo", MB_YESNO | MB_ICONQUESTION);
    if (res != IDYES) return;

    size_t prev_count = g_pUndoHistory->count;
    renamer_undo_batch(g_pUndoHistory, NULL, NULL, &g_bCancelRequested);

    wchar_t msg[256];
    if (is_id) {
        swprintf(msg, 256, L"Berhasil mengembalikan %zu berkas ke nama semula!", prev_count);
    } else {
        swprintf(msg, 256, L"Successfully restored %zu files to their original names!", prev_count);
    }
    MessageBoxW(hWnd, msg, is_id ? L"Undo Selesai" : L"Undo Complete", MB_OK | MB_ICONINFORMATION);

    scan_renamer_folder(hWnd);
}

/* ========================================================================= */
/* Forgot Password & OTP Reset Modal Dialog                                  */
/* ========================================================================= */

#define IDD_RESET_OTP       4001
#define IDD_RESET_NEWPASS   4002
#define IDD_RESET_CONFIRM   4003
#define IDD_RESET_SUBMIT    4004
#define IDD_RESET_RESEND    4005
#define IDD_RESET_CANCEL    4006
#define IDD_RESET_STATUS    4007


static void show_forgot_password_dialog(HWND hWndParent) {
    if (!vault_is_configured()) {
        MessageBoxW(hWndParent, L"Master password dan email belum didaftarkan!", L"Peringatan", MB_OK | MB_ICONWARNING);
        return;
    }

    char otp[MAX_OTP_LEN] = {0};
    if (!vault_generate_otp(otp, sizeof(otp))) {
        MessageBoxW(hWndParent, L"Gagal menghasilkan kode verifikasi acak!", L"Kesalahan", MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t send_status[512] = {0};
    vault_send_reset_email(otp, send_status, 512);

    bool is_id = (i18n_get_language() == LANG_ID);

    /* Construct popup dialog dynamically */
    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770",
        is_id ? L"Pemulihan & Reset Password via Email" : L"Password Recovery & Reset via Email",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 500, 380, hWndParent, NULL, GetModuleHandle(NULL), NULL
    );

    if (!hDlg) return;

    RECT rcParent;
    GetWindowRect(hWndParent, &rcParent);
    int dlgW = 500, dlgH = 380;
    int dlgX = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int dlgY = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;
    SetWindowPos(hDlg, NULL, dlgX, dlgY, dlgW, dlgH, SWP_NOZORDER);

    EnableWindow(hWndParent, FALSE);

    wchar_t masked[256] = {0};
    vault_get_masked_email(masked, 256);

    wchar_t header_txt[512];
    if (is_id) {
        swprintf(header_txt, 512,
            L"KODE OTP TELAH DIKIRIM KE EMAIL PEMULIHAN:\n%ls\n(Kode 6-digit berlaku selama 15 menit)", masked);
    } else {
        swprintf(header_txt, 512,
            L"OTP CODE DISPATCHED TO RECOVERY EMAIL:\n%ls\n(6-digit code valid for 15 minutes)", masked);
    }

    HWND hLblHeader = CreateWindowExW(0, L"STATIC", header_txt,
        WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 15, 445, 45, hDlg, (HMENU)100, NULL, NULL);
    set_font(hLblHeader, g_hFontBold);

    HWND hLblOtp = CreateWindowExW(0, L"STATIC",
        is_id ? L"Masukkan Kode OTP 6-Digit:" : L"Enter 6-Digit OTP Code:",
        WS_CHILD | WS_VISIBLE, 30, 75, 200, 20, hDlg, NULL, NULL, NULL);
    set_font(hLblOtp, g_hFont);

    HWND hEditOtp = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_CENTER | ES_NUMBER | WS_TABSTOP, 240, 72, 120, 25, hDlg, (HMENU)IDD_RESET_OTP, NULL, NULL);
    set_font(hEditOtp, g_hFontBold);
    SendMessage(hEditOtp, EM_SETLIMITTEXT, 6, 0);

    HWND hLblNewPass = CreateWindowExW(0, L"STATIC",
        is_id ? L"Master Password Baru:" : L"New Master Password:",
        WS_CHILD | WS_VISIBLE, 30, 115, 180, 20, hDlg, NULL, NULL, NULL);
    set_font(hLblNewPass, g_hFont);

    HWND hEditNewPass = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_PASSWORD | WS_TABSTOP, 220, 112, 230, 24, hDlg, (HMENU)IDD_RESET_NEWPASS, NULL, NULL);
    set_font(hEditNewPass, g_hFont);

    HWND hLblConfirm = CreateWindowExW(0, L"STATIC",
        is_id ? L"Konfirmasi Password Baru:" : L"Confirm New Password:",
        WS_CHILD | WS_VISIBLE, 30, 155, 180, 20, hDlg, NULL, NULL, NULL);
    set_font(hLblConfirm, g_hFont);

    HWND hEditConfirm = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_PASSWORD | WS_TABSTOP, 220, 152, 230, 24, hDlg, (HMENU)IDD_RESET_CONFIRM, NULL, NULL);
    set_font(hEditConfirm, g_hFont);

    HWND hLblStatus = CreateWindowExW(0, L"STATIC",
        is_id ? L"Periksa inbox atau log audit pengiriman email (email_reset_dispatch_audit.log)." :
                L"Check inbox or dispatch audit log (email_reset_dispatch_audit.log).",
        WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 195, 445, 40, hDlg, (HMENU)IDD_RESET_STATUS, NULL, NULL);
    set_font(hLblStatus, g_hFont);

    HWND hBtnSubmit = CreateWindowExW(0, L"BUTTON",
        is_id ? L"Verifikasi Kode & Set Password Baru" : L"Verify Code & Set New Password",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, 30, 250, 260, 32, hDlg, (HMENU)IDD_RESET_SUBMIT, NULL, NULL);
    set_font(hBtnSubmit, g_hFontBold);

    HWND hBtnResend = CreateWindowExW(0, L"BUTTON",
        is_id ? L"Kirim Ulang Kode" : L"Resend Code",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 300, 250, 150, 32, hDlg, (HMENU)IDD_RESET_RESEND, NULL, NULL);
    set_font(hBtnResend, g_hFont);

    HWND hBtnCancel = CreateWindowExW(0, L"BUTTON",
        is_id ? L"Tutup" : L"Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 180, 295, 120, 28, hDlg, (HMENU)IDD_RESET_CANCEL, NULL, NULL);
    set_font(hBtnCancel, g_hFont);

    /* Modal loop */
    MSG msg;
    bool bRunning = true;
    while (bRunning && GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_COMMAND && (HWND)msg.lParam == hBtnCancel) {
            bRunning = false;
        } else if (msg.message == WM_COMMAND && (HWND)msg.lParam == hBtnResend) {
            char new_otp[MAX_OTP_LEN] = {0};
            if (vault_generate_otp(new_otp, sizeof(new_otp))) {
                wchar_t st[512] = {0};
                vault_send_reset_email(new_otp, st, 512);
                SetWindowTextW(hLblStatus, is_id ?
                    L"✅ Kode baru berhasil dikirim ulang ke email!" :
                    L"✅ New verification code sent to recovery email!");
            }
        } else if (msg.message == WM_COMMAND && (HWND)msg.lParam == hBtnSubmit) {
            wchar_t otp_w[32] = {0};
            wchar_t p1[256] = {0};
            wchar_t p2[256] = {0};

            GetWindowTextW(hEditOtp, otp_w, 31);
            GetWindowTextW(hEditNewPass, p1, 255);
            GetWindowTextW(hEditConfirm, p2, 255);

            char otp_a[32] = {0};
            WideCharToMultiByte(CP_UTF8, 0, otp_w, -1, otp_a, sizeof(otp_a) - 1, NULL, NULL);

            if (strlen(otp_a) != 6) {
                MessageBoxW(hDlg, is_id ? L"Kode OTP harus terdiri dari 6 angka!" : L"OTP code must be 6 digits!",
                    is_id ? L"Peringatan" : L"Warning", MB_OK | MB_ICONWARNING);
            } else if (wcslen(p1) < 4) {
                MessageBoxW(hDlg, is_id ? L"Password baru minimal 4 karakter!" : L"New password must be at least 4 characters!",
                    is_id ? L"Peringatan" : L"Warning", MB_OK | MB_ICONWARNING);
            } else if (wcscmp(p1, p2) != 0) {
                MessageBoxW(hDlg, is_id ? L"Konfirmasi password baru tidak cocok!" : L"New password confirmation does not match!",
                    is_id ? L"Peringatan" : L"Warning", MB_OK | MB_ICONWARNING);
            } else {
                if (vault_verify_otp_and_reset(otp_a, p1)) {
                    MessageBoxW(hDlg, is_id ?
                        L"Selamat! Master Password Anda berhasil diperbarui.\nAkun telah dibuka kembali (Unlocked). Silakan masuk dengan password baru." :
                        L"Congratulations! Master Password has been reset.\nAccount is now unlocked. Please log in with your new password.",
                        is_id ? L"Reset Berhasil" : L"Reset Successful", MB_OK | MB_ICONINFORMATION);
                    bRunning = false;
                } else {
                    MessageBoxW(hDlg, is_id ?
                        L"Kode OTP salah atau telah kadaluarsa (lebih dari 15 menit)!\nSilakan periksa kembali atau klik 'Kirim Ulang Kode'." :
                        L"Incorrect OTP code or expired (more than 15 minutes)!\nPlease check again or click 'Resend Code'.",
                        is_id ? L"Verifikasi Gagal" : L"Verification Failed", MB_OK | MB_ICONERROR);
                }
            }
        } else if (msg.message == WM_CLOSE && msg.hwnd == hDlg) {
            bRunning = false;
        }

        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(hWndParent, TRUE);
    DestroyWindow(hDlg);
    SetActiveWindow(hWndParent);

    update_login_attempt_ui();
    if (g_hLoginEditPass && IsWindowVisible(g_hLoginEditPass) && IsWindowEnabled(g_hLoginEditPass)) {
        SetFocus(g_hLoginEditPass);
    }
}

/* ========================================================================= */
/* Dynamic Window Layout & Resizing                                          */
/* ========================================================================= */

static void layout_controls(HWND hWnd, int cx, int cy) {
    (void)hWnd;
    if (cx <= 0 || cy <= 0) return;

    if (g_hTab) {
        SetWindowPos(g_hTab, NULL, 0, 0, cx, cy, SWP_NOZORDER);
    }

    int cur_tab = (g_hTab) ? TabCtrl_GetCurSel(g_hTab) : 0;

    if (cur_tab == 0) {
        if (!vault_is_configured()) {
            /* 1. Setup Card: Centered */
            int cardW = 500;
            int cardH = 380;
            int cardX = (cx - cardW) / 2;
            int cardY = (cy - cardH) / 2;
            if (cardY < 35) cardY = 35;

            SetWindowPos(g_hSetupGrp, NULL, cardX, cardY, cardW, cardH, SWP_NOZORDER);
            SetWindowPos(g_hSetupLblInfo, NULL, cardX + 25, cardY + 30, cardW - 50, 45, SWP_NOZORDER);

            SetWindowPos(g_hSetupLblPass, NULL, cardX + 25, cardY + 90, cardW - 50, 20, SWP_NOZORDER);
            SetWindowPos(g_hSetupEditPass, NULL, cardX + 25, cardY + 112, cardW - 50, 24, SWP_NOZORDER);

            SetWindowPos(g_hSetupLblConfirm, NULL, cardX + 25, cardY + 145, cardW - 50, 20, SWP_NOZORDER);
            SetWindowPos(g_hSetupEditConfirm, NULL, cardX + 25, cardY + 167, cardW - 50, 24, SWP_NOZORDER);

            SetWindowPos(g_hSetupLblEmail, NULL, cardX + 25, cardY + 200, cardW - 50, 20, SWP_NOZORDER);
            SetWindowPos(g_hSetupEditEmail, NULL, cardX + 25, cardY + 222, cardW - 50, 24, SWP_NOZORDER);

            SetWindowPos(g_hSetupBtnSave, NULL, cardX + 25, cardY + 285, cardW - 50, 36, SWP_NOZORDER);
        } else if (!g_bAuthenticated) {
            /* 2. Login Card: Centered */
            int cardW = 460;
            int cardH = 330;
            int cardX = (cx - cardW) / 2;
            int cardY = (cy - cardH) / 2;
            if (cardY < 40) cardY = 40;

            SetWindowPos(g_hLoginGrp, NULL, cardX, cardY, cardW, cardH, SWP_NOZORDER);
            SetWindowPos(g_hLoginLblInfo, NULL, cardX + 25, cardY + 30, cardW - 50, 25, SWP_NOZORDER);

            SetWindowPos(g_hLoginLblPass, NULL, cardX + 25, cardY + 68, cardW - 50, 20, SWP_NOZORDER);
            SetWindowPos(g_hLoginEditPass, NULL, cardX + 25, cardY + 90, cardW - 50, 25, SWP_NOZORDER);
            SetWindowPos(g_hLoginChkShow, NULL, cardX + 25, cardY + 120, 200, 20, SWP_NOZORDER);

            SetWindowPos(g_hLoginLblAttempts, NULL, cardX + 25, cardY + 145, cardW - 50, 20, SWP_NOZORDER);
            SetWindowPos(g_hLoginLblStatus, NULL, cardX + 25, cardY + 170, cardW - 50, 40, SWP_NOZORDER);

            SetWindowPos(g_hLoginBtnMasuk, NULL, cardX + 25, cardY + 245, 230, 36, SWP_NOZORDER);
            SetWindowPos(g_hLoginBtnLupa, NULL, cardX + 265, cardY + 245, 170, 36, SWP_NOZORDER);
        } else {
            /* 3. Authenticated Explorer View */
            int topY = 38;
            SetWindowPos(g_hEncLblFolder, NULL, 15, topY + 2, 90, 20, SWP_NOZORDER);
            SetWindowPos(g_hEditFolder, NULL, 105, topY, cx - 470, 24, SWP_NOZORDER);
            SetWindowPos(g_hBtnBrowse, NULL, cx - 355, topY - 1, 80, 26, SWP_NOZORDER);
            SetWindowPos(g_hChkRecursive, NULL, cx - 265, topY + 1, 140, 22, SWP_NOZORDER);
            SetWindowPos(g_hChkRemember1, NULL, cx - 115, topY + 1, 100, 22, SWP_NOZORDER);

            SetWindowPos(g_hEncFileTypeLabel, NULL, 15, 72, 65, 22, SWP_NOZORDER);
            SetWindowPos(g_hEncFileType, NULL, 82, 69, 140, 120, SWP_NOZORDER);
            SetWindowPos(g_hEncFileExtensions, NULL, 232, 72, cx - 532, 22, SWP_NOZORDER);
            SetWindowPos(g_hEncMaxSizeLabel, NULL, cx - 290, 72, 95, 22, SWP_NOZORDER);
            SetWindowPos(g_hEncMaxSizeEdit, NULL, cx - 190, 69, 55, 24, SWP_NOZORDER);
            SetWindowPos(g_hEncMaxSizeUnit, NULL, cx - 130, 72, 115, 22, SWP_NOZORDER);

            int paneY = 104;
            int bottomAreaH = 175;
            int paneH = cy - paneY - bottomAreaH;
            if (paneH < 150) paneH = 150;

            int treeW = 250;
            SetWindowPos(g_hEncTree, NULL, 15, paneY, treeW, paneH, SWP_NOZORDER);

            int listX = 15 + treeW + 8;
            int listW = cx - listX - 15;
            SetWindowPos(g_hEncExplorerList, NULL, listX, paneY, listW, paneH, SWP_NOZORDER);

            int bY = paneY + paneH + 8;
            SetWindowPos(g_hProgress, NULL, 15, bY, cx - 30, 16, SWP_NOZORDER);
            SetWindowPos(g_hStaticStatus, NULL, 15, bY + 22, cx - 30, 20, SWP_NOZORDER);
            SetWindowPos(g_hLogBox, NULL, 15, bY + 45, cx - 30, cy - (bY + 45) - 10, SWP_NOZORDER);
        }
    } else if (cur_tab == 1) {
        /* Page 2 (Renamer) Layout */
        SetWindowPos(g_hRenGrp1, NULL, 15, 35, cx - 30, 95, SWP_NOZORDER);
        SetWindowPos(g_hRenEditFolder, NULL, 25, 60, cx - 365, 24, SWP_NOZORDER);
        SetWindowPos(g_hRenBtnBrowse, NULL, cx - 330, 59, 80, 26, SWP_NOZORDER);
        SetWindowPos(g_hRenChkRecursive, NULL, cx - 240, 61, 95, 22, SWP_NOZORDER);
        SetWindowPos(g_hRenChkRemember2, NULL, cx - 135, 61, 110, 22, SWP_NOZORDER);
        SetWindowPos(g_hRenFileTypeLabel, NULL, 25, 96, 40, 22, SWP_NOZORDER);
        SetWindowPos(g_hRenFileType, NULL, 68, 93, 130, 120, SWP_NOZORDER);
        SetWindowPos(g_hRenFileExtensions, NULL, 205, 96, cx - 500, 22, SWP_NOZORDER);
        SetWindowPos(g_hRenMaxSizeLabel, NULL, cx - 285, 96, 95, 22, SWP_NOZORDER);
        SetWindowPos(g_hRenMaxSizeEdit, NULL, cx - 185, 93, 55, 24, SWP_NOZORDER);
        SetWindowPos(g_hRenMaxSizeUnit, NULL, cx - 125, 96, 105, 22, SWP_NOZORDER);

        SetWindowPos(g_hRenGrp2, NULL, 15, 138, cx - 30, 75, SWP_NOZORDER);
        SetWindowPos(g_hRenLblMethod, NULL, 25, 163, 55, 20, SWP_NOZORDER);
        SetWindowPos(g_hRenComboMethod, NULL, 85, 160, 195, 140, SWP_NOZORDER);

        int listY = 222;
        int listH = cy - listY - 50;
        if (listH < 120) listH = 120;
        SetWindowPos(g_hRenList, NULL, 15, listY, cx - 30, listH, SWP_NOZORDER);

        int btnY = cy - 40;
        SetWindowPos(g_hRenBtnPreview, NULL, 15, btnY, 130, 28, SWP_NOZORDER);
        SetWindowPos(g_hRenBtnStart, NULL, 155, btnY, 190, 28, SWP_NOZORDER);
        SetWindowPos(g_hRenBtnUndo, NULL, 355, btnY, 150, 28, SWP_NOZORDER);
        SetWindowPos(g_hRenStaticStatus, NULL, 515, btnY + 4, cx - 530, 20, SWP_NOZORDER);
    } else if (cur_tab == 2) {
        /* Page 3 (Duplicate Finder) Layout */
        SetWindowPos(g_hDupGrpFolder, NULL, 15, 35, cx - 30, 95, SWP_NOZORDER);
        SetWindowPos(g_hDupEditFolder, NULL, 25, 60, cx - 395, 24, SWP_NOZORDER);
        SetWindowPos(g_hDupBtnBrowse, NULL, cx - 360, 59, 80, 26, SWP_NOZORDER);
        SetWindowPos(g_hDupChkRecursive, NULL, cx - 270, 61, 130, 22, SWP_NOZORDER);
        SetWindowPos(g_hDupBtnScan, NULL, cx - 130, 59, 115, 26, SWP_NOZORDER);
        SetWindowPos(g_hDupFileTypeLabel, NULL, 25, 96, 40, 22, SWP_NOZORDER);
        SetWindowPos(g_hDupFileType, NULL, 68, 93, 130, 120, SWP_NOZORDER);
        SetWindowPos(g_hDupFileExtensions, NULL, 205, 96, cx - 500, 22, SWP_NOZORDER);
        SetWindowPos(g_hDupMaxSizeLabel, NULL, cx - 285, 96, 95, 22, SWP_NOZORDER);
        SetWindowPos(g_hDupMaxSizeEdit, NULL, cx - 185, 93, 55, 24, SWP_NOZORDER);
        SetWindowPos(g_hDupMaxSizeUnit, NULL, cx - 125, 96, 105, 22, SWP_NOZORDER);

        int listY = 138;
        int bottomAreaH = 80;
        int listH = cy - listY - bottomAreaH;
        if (listH < 120) listH = 120;
        SetWindowPos(g_hDupList, NULL, 15, listY, cx - 30, listH, SWP_NOZORDER);

        int bY = listY + listH + 8;
        SetWindowPos(g_hDupProgress, NULL, 15, bY, cx - 30, 16, SWP_NOZORDER);
        SetWindowPos(g_hDupStaticStatus, NULL, 15, bY + 20, cx - 30, 20, SWP_NOZORDER);

        int btnY = bY + 44;
        SetWindowPos(g_hDupBtnAutoSel, NULL, 15, btnY, 180, 28, SWP_NOZORDER);
        SetWindowPos(g_hDupBtnSelAll, NULL, 205, btnY, 100, 28, SWP_NOZORDER);
        SetWindowPos(g_hDupBtnDeselAll, NULL, 315, btnY, 100, 28, SWP_NOZORDER);
        SetWindowPos(g_hDupBtnDelete, NULL, 425, btnY, 200, 28, SWP_NOZORDER);
        SetWindowPos(g_hDupBtnStop, NULL, cx - 115, btnY, 100, 28, SWP_NOZORDER);
    } else if (cur_tab == 3) {
        if (g_bAuthenticated) {
            SetWindowPos(g_hModelBtnTest, NULL, 15, 38, 160, 30, SWP_NOZORDER);
            SetWindowPos(g_hModelBtnStop, NULL, 185, 38, 100, 30, SWP_NOZORDER);
            SetWindowPos(g_hModelMaxInputLabel, NULL, 300, 40, 90, 22, SWP_NOZORDER);
            SetWindowPos(g_hModelMaxInput, NULL, 390, 38, 82, 24, SWP_NOZORDER);
            SetWindowPos(g_hModelMaxOutputLabel, NULL, 485, 40, 100, 22, SWP_NOZORDER);
            SetWindowPos(g_hModelMaxOutput, NULL, 585, 38, 82, 24, SWP_NOZORDER);
            SetWindowPos(g_hModelStatus, NULL, 15, 68, cx - 30, 20, SWP_NOZORDER);
            SetWindowPos(g_hModelProgress, NULL, 15, 91, cx - 30, 12, SWP_NOZORDER);

            int listY = 111;
            int listH = cy - listY - 15;
            if (listH < 120) listH = 120;
            SetWindowPos(g_hModelList, NULL, 15, listY, cx - 30, listH, SWP_NOZORDER);
        } else {
            int cardW = 520;
            int cardH = 150;
            int cardX = (cx - cardW) / 2;
            int cardY = (cy - cardH) / 2;
            if (cardY < 40) cardY = 40;
            SetWindowPos(g_hModelLockInfo, NULL, cardX + 20, cardY + 20, cardW - 40, 45, SWP_NOZORDER);
            SetWindowPos(g_hModelLockLogin, NULL, cardX + 150, cardY + 85, cardW - 300, 36, SWP_NOZORDER);
        }
    }
}

/* ========================================================================= */
/* App Menu Creation                                                         */
/* ========================================================================= */

static HMENU create_app_menu(void) {
    HMENU hMenuBar = CreateMenu();

    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_OPEN, i18n_get(STR_MENU_OPEN_FOLDER));
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, i18n_get(STR_MENU_EXIT));
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, i18n_get(STR_MENU_FILE));

    HMENU hViewMenu = CreatePopupMenu();
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_ENCRYPT, i18n_get(STR_TAB_ENCRYPT));
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_RENAME, i18n_get(STR_TAB_RENAME));
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_DUPLICATES, i18n_get(STR_TAB_DUPLICATES));
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_MODEL_TESTER, i18n_get(STR_TAB_MODEL_TESTER));
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, i18n_get(STR_MENU_MODES));

    HMENU hLangMenu = CreatePopupMenu();
    UINT enFlags = (i18n_get_language() == LANG_EN) ? (MF_STRING | MF_CHECKED) : MF_STRING;
    UINT idFlags = (i18n_get_language() == LANG_ID) ? (MF_STRING | MF_CHECKED) : MF_STRING;
    AppendMenuW(hLangMenu, enFlags, IDM_LANG_EN, i18n_get(STR_MENU_LANG_EN));
    AppendMenuW(hLangMenu, idFlags, IDM_LANG_ID, i18n_get(STR_MENU_LANG_ID));
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hLangMenu, i18n_get(STR_MENU_LANGUAGE));

    HMENU hServiceMenu = CreatePopupMenu();
    bool bRunning = service_is_running();
    AppendMenuW(hServiceMenu, MF_STRING | MF_DISABLED, 0,
        bRunning ? i18n_get(STR_SERVICE_STATUS_RUNNING) : i18n_get(STR_SERVICE_STATUS_STOPPED));
    AppendMenuW(hServiceMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hServiceMenu, MF_STRING, IDM_SERVICE_TOGGLE,
        bRunning ? i18n_get(STR_SERVICE_DISABLE) : i18n_get(STR_SERVICE_ENABLE));
    AppendMenuW(hServiceMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hServiceMenu, MF_STRING, IDM_SERVICE_INSTALL, i18n_get(STR_SERVICE_INSTALL_INTEGRATION));
    AppendMenuW(hServiceMenu, MF_STRING, IDM_SERVICE_UNINSTALL, i18n_get(STR_SERVICE_UNINSTALL_INTEGRATION));
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hServiceMenu, i18n_get(STR_MENU_SERVICE));

    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_GUIDE, i18n_get(STR_MENU_GUIDE));
    AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, i18n_get(STR_MENU_ABOUT));
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, i18n_get(STR_MENU_HELP));

    return hMenuBar;
}

static void update_ui_texts(HWND hWnd) {
    if (!hWnd) return;

    SetWindowTextW(hWnd, i18n_get(STR_APP_TITLE));

    /* Refresh App Menu */
    HMENU hOldMenu = GetMenu(hWnd);
    HMENU hNewMenu = create_app_menu();
    SetMenu(hWnd, hNewMenu);
    if (hOldMenu) DestroyMenu(hOldMenu);
    DrawMenuBar(hWnd);

    /* Refresh Tab text */
    if (g_hTab) {
        TCITEMW tie = {0};
        tie.mask = TCIF_TEXT;
        tie.pszText = (LPWSTR)i18n_get(STR_TAB_ENCRYPT);
        TabCtrl_SetItem(g_hTab, 0, &tie);
        tie.pszText = (LPWSTR)i18n_get(STR_TAB_RENAME);
        TabCtrl_SetItem(g_hTab, 1, &tie);
        tie.pszText = (LPWSTR)i18n_get(STR_TAB_DUPLICATES);
        TabCtrl_SetItem(g_hTab, 2, &tie);
        tie.pszText = (LPWSTR)i18n_get(STR_TAB_MODEL_TESTER);
        TabCtrl_SetItem(g_hTab, 3, &tie);
    }

    /* Refresh Setup View */
    if (g_hSetupGrp) SetWindowTextW(g_hSetupGrp, i18n_get(STR_SETUP_GRP));
    if (g_hSetupLblInfo) SetWindowTextW(g_hSetupLblInfo, i18n_get(STR_SETUP_INFO));
    if (g_hSetupLblPass) SetWindowTextW(g_hSetupLblPass, i18n_get(STR_SETUP_LBL_PASS));
    if (g_hSetupLblConfirm) SetWindowTextW(g_hSetupLblConfirm, i18n_get(STR_SETUP_LBL_CONFIRM));
    if (g_hSetupLblEmail) SetWindowTextW(g_hSetupLblEmail, i18n_get(STR_SETUP_LBL_EMAIL));
    if (g_hSetupBtnSave) SetWindowTextW(g_hSetupBtnSave, i18n_get(STR_SETUP_BTN_SAVE));

    /* Refresh Login View */
    if (g_hLoginGrp) SetWindowTextW(g_hLoginGrp, i18n_get(STR_LOGIN_GRP));
    if (g_hLoginLblInfo) SetWindowTextW(g_hLoginLblInfo, i18n_get(STR_LOGIN_INFO));
    if (g_hLoginLblPass) SetWindowTextW(g_hLoginLblPass, i18n_get(STR_LOGIN_LBL_PASS));
    if (g_hLoginChkShow) SetWindowTextW(g_hLoginChkShow, i18n_get(STR_LOGIN_CHK_SHOW));
    if (g_hLoginBtnMasuk) SetWindowTextW(g_hLoginBtnMasuk, i18n_get(STR_LOGIN_BTN_MASUK));
    if (g_hLoginBtnLupa) SetWindowTextW(g_hLoginBtnLupa, i18n_get(STR_LOGIN_BTN_LUPA));
    update_login_attempt_ui();

    /* Refresh Explorer View */
    if (g_hEncLblFolder) SetWindowTextW(g_hEncLblFolder, i18n_get(STR_ENC_LBL_FOLDER));
    if (g_hBtnBrowse) SetWindowTextW(g_hBtnBrowse, i18n_get(STR_BTN_BROWSE));
    if (g_hChkRecursive) SetWindowTextW(g_hChkRecursive, i18n_get(STR_CHK_RECURSIVE));
    if (g_hChkRemember1) SetWindowTextW(g_hChkRemember1, i18n_get(STR_CHK_REMEMBER));
    if (g_hEncFileTypeLabel) SetWindowTextW(g_hEncFileTypeLabel, i18n_get(STR_ENC_FILE_TYPE_LABEL));
    refresh_file_filter_controls(g_hEncFileType, g_hEncFileExtensions, g_hEncMaxSizeLabel, g_hEncMaxSizeUnit);

    if (g_hEncExplorerList) {
        LVCOLUMNW col = {0};
        col.mask = LVCF_TEXT;
        col.pszText = (LPWSTR)i18n_get(STR_COL_NAME);
        ListView_SetColumn(g_hEncExplorerList, 0, &col);
        col.pszText = (LPWSTR)i18n_get(STR_COL_TYPE);
        ListView_SetColumn(g_hEncExplorerList, 1, &col);
        col.pszText = (LPWSTR)i18n_get(STR_COL_SIZE);
        ListView_SetColumn(g_hEncExplorerList, 2, &col);
        col.pszText = (LPWSTR)i18n_get(STR_COL_MODIFIED);
        ListView_SetColumn(g_hEncExplorerList, 3, &col);
        col.pszText = (LPWSTR)i18n_get(STR_COL_STATUS);
        ListView_SetColumn(g_hEncExplorerList, 4, &col);

        if (g_szCurrentExplorerFolder[0] != L'\0') {
            populate_explorer_list(g_hEncExplorerList, g_szCurrentExplorerFolder);
        }
    }

    /* Refresh Renamer View */
    if (g_hRenGrp1) SetWindowTextW(g_hRenGrp1, i18n_get(STR_REN_GRP_FOLDER));
    if (g_hRenBtnBrowse) SetWindowTextW(g_hRenBtnBrowse, i18n_get(STR_BTN_BROWSE));
    if (g_hRenChkRecursive) SetWindowTextW(g_hRenChkRecursive, i18n_get(STR_CHK_RECURSIVE));
    if (g_hRenChkRemember2) SetWindowTextW(g_hRenChkRemember2, i18n_get(STR_CHK_REMEMBER));
    if (g_hRenFileTypeLabel) SetWindowTextW(g_hRenFileTypeLabel, i18n_get(STR_ENC_FILE_TYPE_LABEL));
    refresh_file_filter_controls(g_hRenFileType, g_hRenFileExtensions, g_hRenMaxSizeLabel, g_hRenMaxSizeUnit);
    if (g_hRenGrp2) SetWindowTextW(g_hRenGrp2, i18n_get(STR_REN_GRP_METHOD));
    if (g_hRenLblMethod) SetWindowTextW(g_hRenLblMethod, i18n_get(STR_REN_LBL_METHOD));

    if (g_hRenComboMethod) {
        int cur_m = (int)SendMessage(g_hRenComboMethod, CB_GETCURSEL, 0, 0);
        SendMessage(g_hRenComboMethod, CB_RESETCONTENT, 0, 0);
        SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_METHOD_REPLACE));
        SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_METHOD_ADD));
        SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_METHOD_PATTERN));
        SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_METHOD_CASE));
        SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_REN_METHOD_TRIM));
        SendMessage(g_hRenComboMethod, CB_SETCURSEL, (cur_m >= 0 ? cur_m : 0), 0);
        update_method_ui_labels(cur_m >= 0 ? cur_m : 0);
    }

    if (g_hRenList) {
        LVCOLUMNW c = {0};
        c.mask = LVCF_TEXT;
        c.pszText = (LPWSTR)i18n_get(STR_REN_COL_ORIGINAL);
        ListView_SetColumn(g_hRenList, 1, &c);
        c.pszText = (LPWSTR)i18n_get(STR_REN_COL_PROSPECTIVE);
        ListView_SetColumn(g_hRenList, 2, &c);
        c.pszText = (LPWSTR)i18n_get(STR_COL_SIZE);
        ListView_SetColumn(g_hRenList, 3, &c);
        c.pszText = (LPWSTR)i18n_get(STR_COL_STATUS);
        ListView_SetColumn(g_hRenList, 4, &c);
    }

    if (g_hRenBtnPreview) SetWindowTextW(g_hRenBtnPreview, (i18n_get_language() == LANG_ID) ? L"🔄 Segarkan Pratinjau" : L"🔄 Refresh Preview");
    if (g_hRenBtnStart) SetWindowTextW(g_hRenBtnStart, i18n_get(STR_REN_BTN_APPLY));
    if (g_hRenBtnUndo) SetWindowTextW(g_hRenBtnUndo, i18n_get(STR_REN_BTN_UNDO));

    /* Refresh Duplicate Finder View */
    if (g_hDupGrpFolder) SetWindowTextW(g_hDupGrpFolder, i18n_get(STR_DUP_GRP_TARGET));
    if (g_hDupBtnBrowse) SetWindowTextW(g_hDupBtnBrowse, i18n_get(STR_BTN_BROWSE));
    if (g_hDupChkRecursive) SetWindowTextW(g_hDupChkRecursive, i18n_get(STR_CHK_RECURSIVE));
    if (g_hDupBtnScan) SetWindowTextW(g_hDupBtnScan, i18n_get(STR_DUP_BTN_SCAN));
    if (g_hDupFileTypeLabel) SetWindowTextW(g_hDupFileTypeLabel, i18n_get(STR_ENC_FILE_TYPE_LABEL));
    refresh_file_filter_controls(g_hDupFileType, g_hDupFileExtensions, g_hDupMaxSizeLabel, g_hDupMaxSizeUnit);
    if (g_hDupBtnStop) SetWindowTextW(g_hDupBtnStop, i18n_get(STR_DUP_BTN_STOP));
    if (g_hDupBtnAutoSel) SetWindowTextW(g_hDupBtnAutoSel, i18n_get(STR_DUP_BTN_AUTOSEL));
    if (g_hDupBtnSelAll) SetWindowTextW(g_hDupBtnSelAll, i18n_get(STR_DUP_BTN_SEL_ALL));
    if (g_hDupBtnDeselAll) SetWindowTextW(g_hDupBtnDeselAll, i18n_get(STR_DUP_BTN_DESEL_ALL));
    if (g_hDupBtnDelete) SetWindowTextW(g_hDupBtnDelete, i18n_get(STR_DUP_BTN_DELETE));

    if (g_hDupList) {
        LVCOLUMNW c = {0};
        c.mask = LVCF_TEXT;
        c.pszText = (LPWSTR)i18n_get(STR_DUP_COL_FILENAME);
        ListView_SetColumn(g_hDupList, 0, &c);
        c.pszText = (LPWSTR)i18n_get(STR_DUP_COL_STATUS);
        ListView_SetColumn(g_hDupList, 1, &c);
        c.pszText = (LPWSTR)i18n_get(STR_DUP_COL_SIZE);
        ListView_SetColumn(g_hDupList, 2, &c);
        c.pszText = (LPWSTR)i18n_get(STR_DUP_COL_MODIFIED);
        ListView_SetColumn(g_hDupList, 3, &c);
        c.pszText = (LPWSTR)i18n_get(STR_DUP_COL_PATH);
        ListView_SetColumn(g_hDupList, 4, &c);
    }

    if (g_hModelLockInfo) SetWindowTextW(g_hModelLockInfo, i18n_get(STR_MODEL_LOCKED));
    if (g_hModelLockLogin) SetWindowTextW(g_hModelLockLogin, i18n_get(STR_LOGIN_BTN_MASUK));
    if (g_hModelBtnTest) SetWindowTextW(g_hModelBtnTest, i18n_get(STR_MODEL_BTN_TEST));
    if (g_hModelBtnStop) SetWindowTextW(g_hModelBtnStop, i18n_get(STR_MODEL_BTN_STOP));
    if (g_hModelMaxInputLabel) SetWindowTextW(g_hModelMaxInputLabel, i18n_get(STR_MODEL_MAX_INPUT_PRICE));
    if (g_hModelMaxOutputLabel) SetWindowTextW(g_hModelMaxOutputLabel, i18n_get(STR_MODEL_MAX_OUTPUT_PRICE));
    if (g_hModelList) {
        LVCOLUMNW c = {0};
        c.mask = LVCF_TEXT;
        c.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_NAME);
        ListView_SetColumn(g_hModelList, 0, &c);
        c.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_PROVIDER);
        ListView_SetColumn(g_hModelList, 1, &c);
        c.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_UPTIME);
        ListView_SetColumn(g_hModelList, 2, &c);
        c.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_OUTPUT);
        ListView_SetColumn(g_hModelList, 3, &c);
        c.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_INPUT);
        ListView_SetColumn(g_hModelList, 4, &c);
        c.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_LATENCY);
        ListView_SetColumn(g_hModelList, 5, &c);
        c.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_RESULT);
        ListView_SetColumn(g_hModelList, 6, &c);
    }

    RECT rc;
    GetClientRect(hWnd, &rc);
    layout_controls(hWnd, rc.right - rc.left, rc.bottom - rc.top);
    InvalidateRect(hWnd, NULL, TRUE);
}

static bool get_app_database_path(wchar_t *out_path, size_t capacity) {
    if (!out_path || capacity == 0) return false;
    wchar_t module_path[MAX_PATH * 2] = {0};
    DWORD length = GetModuleFileNameW(NULL, module_path, (DWORD)(sizeof(module_path) / sizeof(module_path[0])));
    if (length == 0 || length >= sizeof(module_path) / sizeof(module_path[0])) return false;
    wchar_t *separator = wcsrchr(module_path, L'\\');
    if (!separator) return false;
    *separator = L'\0';
    int result = swprintf(out_path, capacity, L"%ls\\kiosapi-models.db", module_path);
    return result >= 0 && (size_t)result < capacity;
}

static bool get_vscode_model_config_path(wchar_t *out_path, size_t capacity) {
    if (!out_path || capacity == 0) return false;
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, out_path))) return false;
    size_t used = wcslen(out_path);
    const wchar_t suffix[] = L"\\Code\\User\\chatLanguageModels.json";
    size_t suffix_length = sizeof(suffix) / sizeof(suffix[0]);
    if (used + suffix_length > capacity) return false;
    wcscat(out_path, suffix);
    return true;
}

static void copy_utf8_to_wide(const char *source, wchar_t *destination, size_t capacity) {
    if (!destination || capacity == 0) return;
    destination[0] = L'\0';
    if (!source || !source[0]) return;
    int converted = MultiByteToWideChar(CP_UTF8, 0, source, -1, destination, (int)capacity);
    if (converted == 0) destination[0] = L'\0';
    destination[capacity - 1] = L'\0';
}

static void set_list_text(int row, int column, const wchar_t *text) {
    if (!g_hModelList) return;
    LVITEMW item = {0};
    item.iSubItem = column;
    item.pszText = (LPWSTR)(text ? text : L"");
    ListView_SetItemText(g_hModelList, row, column, item.pszText);
}

static bool parse_model_price_filter(HWND edit, bool *out_has_limit, double *out_limit) {
    wchar_t text[64] = {0};
    if (!edit || !out_has_limit || !out_limit) return false;
    if (GetWindowTextLengthW(edit) >= (int)(sizeof(text) / sizeof(text[0]))) return false;
    GetWindowTextW(edit, text, (int)(sizeof(text) / sizeof(text[0])));
    if (!text[0]) {
        *out_has_limit = false;
        *out_limit = 0.0;
        return true;
    }

    bool has_digit = false;
    bool has_decimal = false;
    for (const wchar_t *it = text; *it; ++it) {
        if (*it >= L'0' && *it <= L'9') {
            has_digit = true;
        } else if (*it == L'.' && !has_decimal) {
            has_decimal = true;
        } else {
            return false;
        }
    }
    if (!has_digit) return false;

    errno = 0;
    wchar_t *end = NULL;
    double value = wcstod(text, &end);
    if (errno == ERANGE || !end || *end || !isfinite(value) || value < 0.0) return false;
    *out_has_limit = true;
    *out_limit = value;
    return true;
}

static void refresh_model_listview(void) {
    if (!g_hModelList || g_bModelTesting) return;
    bool has_max_input = false;
    bool has_max_output = false;
    double max_input = 0.0;
    double max_output = 0.0;
    if (!parse_model_price_filter(g_hModelMaxInput, &has_max_input, &max_input) ||
        !parse_model_price_filter(g_hModelMaxOutput, &has_max_output, &max_output)) {
        ListView_DeleteAllItems(g_hModelList);
        g_modelVisibleCount = 0;
        EnableWindow(g_hModelBtnTest, FALSE);
        SetWindowTextW(g_hModelStatus, i18n_get(STR_MODEL_PRICE_FILTER_INVALID));
        return;
    }
    if (g_modelRows) {
        free(g_modelRows);
        g_modelRows = NULL;
        g_modelRowCount = 0;
    }
    g_modelVisibleCount = 0;
    ListView_DeleteAllItems(g_hModelList);
    g_bModelDbReady = false;

    wchar_t database_path[MAX_PATH * 2] = {0};
    char error[256] = {0};
    if (!get_app_database_path(database_path, MAX_PATH * 2) ||
        !model_tester_load_rows(
            database_path,
            g_modelSortColumn,
            g_modelSortDescending,
            &g_modelRows,
            &g_modelRowCount,
            error,
            sizeof(error)
        )) {
        wchar_t wide_error[256] = {0};
        copy_utf8_to_wide(error[0] ? error : "Could not locate kiosapi-models.db beside the application.", wide_error, 256);
        wchar_t status[320];
        swprintf(status, 320, L"Database error: %ls", wide_error);
        SetWindowTextW(g_hModelStatus, status);
        EnableWindow(g_hModelBtnTest, FALSE);
        return;
    }
    g_bModelDbReady = true;

    for (size_t i = 0; i < g_modelRowCount; i++) {
        ModelTestRow *row = &g_modelRows[i];
        if (!model_tester_row_matches_price_filter(
                row, has_max_input, max_input, has_max_output, max_output)) {
            continue;
        }
        wchar_t name[MODEL_TEST_NAME_CAPACITY] = {0};
        wchar_t provider[MODEL_TEST_PROVIDER_CAPACITY] = {0};
        copy_utf8_to_wide(row->model_name, name, MODEL_TEST_NAME_CAPACITY);
        copy_utf8_to_wide(row->provider, provider, MODEL_TEST_PROVIDER_CAPACITY);

        LVITEMW item = {0};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = (int)i;
        item.pszText = name;
        item.lParam = (LPARAM)i;
        int index = ListView_InsertItem(g_hModelList, &item);
        if (index < 0) continue;

        set_list_text(index, 1, provider);
        wchar_t value[128];
        if (row->has_uptime) {
            swprintf(value, 128, L"%.2f%% (%d/%d)", row->uptime_percent, row->success_count, row->test_count);
        } else {
            wcscpy(value, L"—");
        }
        set_list_text(index, 2, value);

        if (row->has_output_price) {
            wchar_t currency[8] = {0};
            copy_utf8_to_wide(row->currency, currency, 8);
            swprintf(value, 128, L"%ls%.6g", currency, row->output_price_per_1m);
        } else {
            wcscpy(value, L"N/A");
        }
        set_list_text(index, 3, value);

        if (row->has_input_price) {
            wchar_t currency[8] = {0};
            copy_utf8_to_wide(row->currency, currency, 8);
            swprintf(value, 128, L"%ls%.6g", currency, row->input_price_per_1m);
        } else {
            wcscpy(value, L"N/A");
        }
        set_list_text(index, 4, value);

        if (row->has_last_latency) {
            swprintf(value, 128, L"%.0f ms", row->last_latency_ms);
        } else {
            wcscpy(value, L"—");
        }
        set_list_text(index, 5, value);
        copy_utf8_to_wide(row->last_result, value, 128);
        set_list_text(index, 6, value[0] ? value : L"—");
        g_modelVisibleCount++;
    }

    SendMessageW(g_hModelProgress, PBM_SETRANGE32, 0, (LPARAM)g_modelVisibleCount);
    SendMessageW(g_hModelProgress, PBM_SETPOS, 0, 0);
    update_model_ready_status();
}

static void update_model_ready_status(void) {
    if (!g_bModelDbReady || !g_hModelStatus) return;
    bool has_limit = false;
    double limit = 0.0;
    if (!parse_model_price_filter(g_hModelMaxInput, &has_limit, &limit) ||
        !parse_model_price_filter(g_hModelMaxOutput, &has_limit, &limit)) {
        SetWindowTextW(g_hModelStatus, i18n_get(STR_MODEL_PRICE_FILTER_INVALID));
        EnableWindow(g_hModelBtnTest, FALSE);
        return;
    }
    if (g_modelRowCount > 0 && g_modelVisibleCount == 0) {
        SetWindowTextW(g_hModelStatus, i18n_get(STR_MODEL_PRICE_FILTER_NO_MATCH));
        EnableWindow(g_hModelBtnTest, FALSE);
        return;
    }
    if (g_bKiosapiKeyReady) {
        SetWindowTextW(g_hModelStatus, i18n_get(STR_MODEL_STATUS_KEY_READY));
    } else if (g_kiosapiKeyError[0]) {
        SetWindowTextW(g_hModelStatus, g_kiosapiKeyError);
    } else {
        SetWindowTextW(g_hModelStatus, i18n_get(STR_MODEL_STATUS_KEY_MISSING));
    }
    EnableWindow(g_hModelBtnTest,
        g_bAuthenticated && g_bKiosapiKeyReady && !g_bModelTesting && g_modelVisibleCount > 0);
}

static void clear_kiosapi_key(void) {
    SecureZeroMemory(g_kiosapiKey, sizeof(g_kiosapiKey));
    g_kiosapiKeyLen = 0;
    g_bKiosapiKeyReady = false;
}

static void unlock_kiosapi_key(const wchar_t *password) {
    clear_kiosapi_key();
    g_kiosapiKeyError[0] = L'\0';
    if (!kiosapi_key_load(
            password,
            g_kiosapiKey,
            sizeof(g_kiosapiKey),
            &g_kiosapiKeyLen,
            g_kiosapiKeyError,
            sizeof(g_kiosapiKeyError) / sizeof(g_kiosapiKeyError[0]))) {
        clear_kiosapi_key();
        return;
    }
    g_bKiosapiKeyReady = true;
}

static void model_test_progress(
    int completed,
    int total,
    const char *model_name,
    bool succeeded,
    const char *result,
    void *user_data
) {
    HWND hWnd = (HWND)user_data;
    if (!hWnd || !IsWindow(hWnd)) return;
    ModelProgressPayload *payload = (ModelProgressPayload *)calloc(1, sizeof(*payload));
    if (!payload) return;
    payload->completed = completed;
    payload->total = total;
    payload->succeeded = succeeded;
    copy_utf8_to_wide(model_name, payload->model_name, MODEL_TEST_NAME_CAPACITY);
    copy_utf8_to_wide(result, payload->result, MODEL_TEST_RESULT_CAPACITY);
    if (!PostMessageW(hWnd, WM_APP_MODEL_PROGRESS, 0, (LPARAM)payload)) free(payload);
}

static DWORD WINAPI model_test_worker_thread(LPVOID parameter) {
    ModelWorkerParams *params = (ModelWorkerParams *)parameter;
    if (!params) return 0;
    ModelFinishedPayload *finished = (ModelFinishedPayload *)calloc(1, sizeof(*finished));
    if (finished) {
        char error[256] = {0};
        finished->success = model_tester_run_selected(
            params->database_path, params->api_key, params->selected_model_names, params->selected_model_count,
            &g_bModelCancel, model_test_progress, params->hWnd, &finished->completed,
            &finished->succeeded, &finished->failed, &finished->cancelled, error, sizeof(error)
        );
        copy_utf8_to_wide(error, finished->error, sizeof(finished->error) / sizeof(finished->error[0]));
        if (!PostMessageW(params->hWnd, WM_APP_MODEL_FINISHED, 0, (LPARAM)finished)) free(finished);
    } else {
        PostMessageW(params->hWnd, WM_APP_MODEL_FINISHED, 0, 0);
    }

    free(params->selected_model_names);
    SecureZeroMemory(params, sizeof(*params));
    free(params);
    return 0;
}

static size_t get_model_names(const char ***out_names, bool selected_only) {
    *out_names = NULL;
    int expected_count = selected_only
        ? ListView_GetSelectedCount(g_hModelList)
        : ListView_GetItemCount(g_hModelList);
    if (expected_count <= 0 || !g_modelRows) return 0;

    const char **names = (const char **)calloc((size_t)expected_count, sizeof(*names));
    if (!names) return 0;
    size_t count = 0;
    int index = selected_only ? -1 : 0;
    while (selected_only
            ? ((index = ListView_GetNextItem(g_hModelList, index, LVNI_SELECTED)) >= 0)
            : index < ListView_GetItemCount(g_hModelList)) {
        LVITEMW item = {0};
        item.mask = LVIF_PARAM;
        item.iItem = index;
        if (ListView_GetItem(g_hModelList, &item) &&
            item.lParam >= 0 && (size_t)item.lParam < g_modelRowCount) {
            names[count++] = g_modelRows[item.lParam].model_name;
        }
        if (!selected_only) index++;
    }
    if (count == 0) {
        free(names);
        return 0;
    }
    *out_names = names;
    return count;
}

static void start_model_tests(HWND hWnd, bool selected_only) {
    if (!g_bAuthenticated || !g_bKiosapiKeyReady || g_bModelTesting) return;
    if (g_modelVisibleCount == 0) refresh_model_listview();
    if (g_modelVisibleCount == 0) {
        MessageBoxW(hWnd, i18n_get(STR_MODEL_PRICE_FILTER_NO_MATCH),
            i18n_get_language() == LANG_ID ? L"Tidak Ada Model" : L"No Models",
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    const char **selected_names = NULL;
    size_t test_count = get_model_names(&selected_names, selected_only);
    if (test_count == 0) {
        int selected_rows = ListView_GetSelectedCount(g_hModelList);
        MessageBoxW(hWnd,
            selected_only && selected_rows > 0
                ? (i18n_get_language() == LANG_ID ? L"Memori tidak cukup untuk menyiapkan model terpilih." : L"Not enough memory to prepare the selected models.")
                : (selected_only
                    ? (i18n_get_language() == LANG_ID ? L"Pilih satu atau beberapa model terlebih dahulu." : L"Select one or more models first.")
                    : (i18n_get_language() == LANG_ID ? L"Memori tidak cukup untuk menyiapkan model." : L"Not enough memory to prepare the models.")),
            selected_only && selected_rows > 0
                ? (i18n_get_language() == LANG_ID ? L"Kesalahan" : L"Error")
                : (selected_only
                    ? (i18n_get_language() == LANG_ID ? L"Tidak Ada Model Dipilih" : L"No Models Selected")
                    : (i18n_get_language() == LANG_ID ? L"Kesalahan" : L"Error")),
            MB_OK | ((selected_only && selected_rows == 0) ? MB_ICONINFORMATION : MB_ICONERROR));
        return;
    }

    wchar_t confirmation[512];
    swprintf(confirmation, 512, i18n_get(selected_only ? STR_MODEL_CONFIRM_SELECTED : STR_MODEL_CONFIRM), (int)test_count);
    if (MessageBoxW(
            hWnd,
            confirmation,
            selected_only
                ? (i18n_get_language() == LANG_ID ? L"Konfirmasi Pemindaian Model Terpilih" : L"Confirm Scan Selected Models")
                : (i18n_get_language() == LANG_ID ? L"Konfirmasi Pengujian Model" : L"Confirm Model Test"),
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
        free(selected_names);
        return;
    }

    ModelWorkerParams *params = (ModelWorkerParams *)calloc(1, sizeof(*params));
    if (!params) {
        free(selected_names);
        MessageBoxW(hWnd,
            (i18n_get_language() == LANG_ID) ? L"Memori tidak cukup untuk memulai pengujian." : L"Not enough memory to start the model tests.",
            (i18n_get_language() == LANG_ID) ? L"Kesalahan" : L"Error",
            MB_OK | MB_ICONERROR);
        return;
    }
    params->hWnd = hWnd;
    params->selected_model_names = selected_names;
    params->selected_model_count = test_count;
    if (!get_app_database_path(params->database_path, MAX_PATH * 2)) {
        free(params->selected_model_names);
        free(params);
        SetWindowTextW(g_hModelStatus, L"Could not locate kiosapi-models.db beside the application.");
        return;
    }
    memcpy(params->api_key, g_kiosapiKey, g_kiosapiKeyLen + 1);

    g_bModelCancel = false;
    g_bModelTesting = true;
    EnableWindow(g_hModelMaxInput, FALSE);
    EnableWindow(g_hModelMaxOutput, FALSE);
    EnableWindow(g_hModelBtnTest, FALSE);
    EnableWindow(g_hModelBtnStop, TRUE);
    SendMessageW(g_hModelProgress, PBM_SETRANGE32, 0, (LPARAM)test_count);
    SendMessageW(g_hModelProgress, PBM_SETPOS, 0, 0);
    SetWindowTextW(g_hModelStatus, i18n_get_language() == LANG_ID ? L"Memulai pengujian model..." : L"Starting model tests...");

    g_hModelThread = CreateThread(NULL, 0, model_test_worker_thread, params, 0, NULL);
    if (!g_hModelThread) {
        free(params->selected_model_names);
        SecureZeroMemory(params, sizeof(*params));
        free(params);
        g_bModelTesting = false;
        EnableWindow(g_hModelMaxInput, TRUE);
        EnableWindow(g_hModelMaxOutput, TRUE);
        EnableWindow(g_hModelBtnStop, FALSE);
        EnableWindow(g_hModelBtnTest, TRUE);
        DWORD error = GetLastError();
        wchar_t message[160];
        swprintf(message, 160, L"Could not start model test worker (Windows error %lu).", (unsigned long)error);
        SetWindowTextW(g_hModelStatus, message);
    }
}

static void handle_model_context_menu(HWND hWnd, int screen_x, int screen_y, bool from_mouse) {
    if (!g_hModelList) return;
    POINT point = {screen_x, screen_y};
    if (from_mouse) {
        POINT client_point = point;
        ScreenToClient(g_hModelList, &client_point);
        LVHITTESTINFO hit = {0};
        hit.pt = client_point;
        int index = ListView_HitTest(g_hModelList, &hit);
        if (index >= 0 && !(ListView_GetItemState(g_hModelList, index, LVIS_SELECTED) & LVIS_SELECTED)) {
            ListView_SetItemState(g_hModelList, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_SetItemState(g_hModelList, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }
    } else if (screen_x == -1 && screen_y == -1) {
        int index = ListView_GetNextItem(g_hModelList, -1, LVNI_SELECTED);
        if (index >= 0) {
            RECT item_rect;
            if (ListView_GetItemRect(g_hModelList, index, &item_rect, LVIR_BOUNDS)) {
                point.x = item_rect.left;
                point.y = item_rect.bottom;
                ClientToScreen(g_hModelList, &point);
            } else {
                GetCursorPos(&point);
            }
        } else {
            GetCursorPos(&point);
        }
    }

    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    bool can_scan = g_bAuthenticated && g_bKiosapiKeyReady && !g_bModelTesting &&
        ListView_GetSelectedCount(g_hModelList) > 0;
    AppendMenuW(menu, MF_STRING | (can_scan ? 0 : MF_GRAYED), IDM_MODEL_SCAN_SELECTED,
        i18n_get(STR_MODEL_SCAN_SELECTED));
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    bool can_add_model = g_bAuthenticated && !g_bModelTesting &&
        ListView_GetSelectedCount(g_hModelList) == 1;
    AppendMenuW(menu, MF_STRING | (can_add_model ? 0 : MF_GRAYED), IDM_MODEL_ADD_TO_VSCODE,
        i18n_get(STR_MODEL_ADD_TO_VSCODE));
    SetForegroundWindow(hWnd);
    int command = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, 0, hWnd, NULL);
    DestroyMenu(menu);
    if (command == IDM_MODEL_SCAN_SELECTED) start_model_tests(hWnd, true);
    else if (command == IDM_MODEL_ADD_TO_VSCODE) {
        const char **selected_names = NULL;
        size_t selected_count = get_model_names(&selected_names, true);
        if (selected_count != 1 || !selected_names) {
            free(selected_names);
            MessageBoxW(hWnd, i18n_get(STR_MODEL_ADD_TO_VSCODE_SELECT_ONE),
                i18n_get_language() == LANG_ID ? L"Pilih Satu Model" : L"Select One Model",
                MB_OK | MB_ICONINFORMATION);
            return;
        }
        char model_id[MODEL_TEST_NAME_CAPACITY] = {0};
        snprintf(model_id, sizeof(model_id), "%s", selected_names[0]);
        wchar_t model_name[MODEL_TEST_NAME_CAPACITY] = {0};
        copy_utf8_to_wide(model_id, model_name, MODEL_TEST_NAME_CAPACITY);
        free(selected_names);

        wchar_t config_path[MAX_PATH * 4] = {0};
        if (!get_vscode_model_config_path(config_path, MAX_PATH * 4)) {
            MessageBoxW(hWnd, i18n_get(STR_MODEL_ADD_TO_VSCODE_PATH_ERROR),
                i18n_get_language() == LANG_ID ? L"Kesalahan" : L"Error",
                MB_OK | MB_ICONERROR);
            return;
        }
        wchar_t confirmation[1024];
        swprintf(confirmation, 1024, i18n_get(STR_MODEL_ADD_TO_VSCODE_CONFIRM),
            model_name, config_path);
        if (MessageBoxW(hWnd, confirmation,
                i18n_get_language() == LANG_ID ? L"Tambahkan Model ke VS Code" : L"Add Model to VS Code",
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
            return;
        }

        char error[256] = {0};
        VscodeModelConfigResult result = vscode_model_config_add(
            config_path, model_id, error, sizeof(error));
        if (result == VSCODE_MODEL_CONFIG_ADDED) {
            wchar_t message[1024];
            swprintf(message, 1024, i18n_get(STR_MODEL_ADD_TO_VSCODE_SUCCESS), model_name);
            SetWindowTextW(g_hModelStatus, message);
            MessageBoxW(hWnd, message,
                i18n_get_language() == LANG_ID ? L"Model Ditambahkan" : L"Model Added",
                MB_OK | MB_ICONINFORMATION);
        } else if (result == VSCODE_MODEL_CONFIG_ALREADY_PRESENT) {
            wchar_t message[512];
            swprintf(message, 512, i18n_get(STR_MODEL_ADD_TO_VSCODE_EXISTS), model_name);
            MessageBoxW(hWnd, message,
                i18n_get_language() == LANG_ID ? L"Model Sudah Ada" : L"Model Already Exists",
                MB_OK | MB_ICONINFORMATION);
        } else {
            wchar_t wide_error[256] = {0};
            copy_utf8_to_wide(error, wide_error, 256);
            wchar_t message[512];
            swprintf(message, 512, i18n_get(STR_MODEL_ADD_TO_VSCODE_ERROR), wide_error);
            MessageBoxW(hWnd, message,
                i18n_get_language() == LANG_ID ? L"Kesalahan Konfigurasi VS Code" : L"VS Code Configuration Error",
                MB_OK | MB_ICONERROR);
        }
    }
}

/* ========================================================================= */
/* System Tray (Hide to Taskbar) Helpers                                     */
/* ========================================================================= */

static NOTIFYICONDATAW g_nidMain = {0};
static bool g_bMainTrayActive = false;

static void show_main_tray_icon(HWND hWnd, bool showBalloon) {
    if (!g_bMainTrayActive) {
        SecureZeroMemory(&g_nidMain, sizeof(NOTIFYICONDATAW));
        g_nidMain.cbSize = sizeof(NOTIFYICONDATAW);
        g_nidMain.hWnd = hWnd;
        g_nidMain.uID = 100;
        g_nidMain.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        g_nidMain.uCallbackMessage = WM_APP_MAINTRAY;
        g_nidMain.hIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1));
        if (!g_nidMain.hIcon) {
            g_nidMain.hIcon = LoadIconW(NULL, IDI_APPLICATION);
        }
        wcsncpy(g_nidMain.szTip, i18n_get(STR_TRAY_TOOLTIP), 127);
        Shell_NotifyIconW(NIM_ADD, &g_nidMain);
        g_bMainTrayActive = true;
    }

    if (showBalloon) {
        NOTIFYICONDATAW nid = {0};
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = hWnd;
        nid.uID = 100;
        nid.uFlags = NIF_INFO;
        nid.dwInfoFlags = NIIF_INFO;
        wcsncpy(nid.szInfoTitle, i18n_get(STR_TRAY_BALLOON_TITLE), 63);
        if (g_bIsWorking) {
            bool is_id = (i18n_get_language() == LANG_ID);
            wcsncpy(nid.szInfo, is_id ?
                L"Aplikasi tetap aktif di latar belakang. Proses yang sedang berjalan tetap diteruskan.\nKlik dua kali ikon ini untuk membuka kembali." :
                L"Application remains active in the background. Ongoing process continues running.\nDouble-click this icon to restore.", 255);
        } else {
            wcsncpy(nid.szInfo, i18n_get(STR_TRAY_BALLOON_MSG), 255);
        }
        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }
}

static void remove_main_tray_icon(void) {
    if (g_bMainTrayActive) {
        Shell_NotifyIconW(NIM_DELETE, &g_nidMain);
        g_bMainTrayActive = false;
    }
}

static void restore_main_window(HWND hWnd) {
    if (!hWnd) return;
    ShowWindow(hWnd, SW_SHOW);
    ShowWindow(hWnd, SW_RESTORE);
    SetForegroundWindow(hWnd);
    remove_main_tray_icon();
}

static void app_exit_completely(HWND hWnd) {
    bool is_id = (i18n_get_language() == LANG_ID);
    if (g_bIsWorking || g_bModelTesting) {
        int res = MessageBoxW(hWnd,
            is_id ? L"Proses sedang berjalan. Apakah Anda yakin ingin membatalkan dan keluar sepenuhnya?" :
                    L"A process is currently running. Are you sure you want to cancel and exit completely?",
            is_id ? L"Konfirmasi Keluar" : L"Confirm Exit", MB_YESNO | MB_ICONQUESTION);
        if (res != IDYES) return;
        g_bCancelRequested = true;
        g_bDupCancel = true;
        g_bModelCancel = true;
        if (g_bModelTesting && g_hModelThread) {
            WaitForSingleObject(g_hModelThread, INFINITE);
            CloseHandle(g_hModelThread);
            g_hModelThread = NULL;
            g_bModelTesting = false;
            MSG queued_message;
            while (PeekMessageW(&queued_message, hWnd, WM_APP_MODEL_PROGRESS, WM_APP_MODEL_FINISHED, PM_REMOVE)) {
                free((void *)queued_message.lParam);
            }
        }
    }

    if (g_bRememberFolder) {
        wchar_t current_folder[MAX_PATH * 2] = {0};
        GetWindowTextW(g_hEditFolder, current_folder, MAX_PATH * 2);
        if (wcslen(current_folder) == 0 && g_hRenEditFolder) {
            GetWindowTextW(g_hRenEditFolder, current_folder, MAX_PATH * 2);
        }
        if (wcslen(current_folder) == 0 && g_hDupEditFolder) {
            GetWindowTextW(g_hDupEditFolder, current_folder, MAX_PATH * 2);
        }
        wchar_t clean[MAX_PATH * 2] = {0};
        sanitize_path(current_folder, clean, MAX_PATH * 2);
        if (clean[0] != L'\0') {
            save_settings(clean);
        }
    }

    remove_main_tray_icon();
    clear_explorer_list_items(g_hEncExplorerList);
    tracker_encrypt_all_decrypted_files();
    clear_kiosapi_key();
    vault_logout();
    DestroyWindow(hWnd);
}

/* ========================================================================= */
/* Duplicate File Finder Functions                                           */
/* ========================================================================= */

static bool g_bUpdatingDupChecks = false;

static void update_duplicate_list_checks(void) {
    if (!g_hDupList) return;
    g_bUpdatingDupChecks = true;
    SendMessage(g_hDupList, WM_SETREDRAW, FALSE, 0);

    int count = ListView_GetItemCount(g_hDupList);
    for (int i = 0; i < count; i++) {
        LVITEMW lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        if (ListView_GetItem(g_hDupList, &lvi) && lvi.lParam) {
            DuplicateItem *pIt = (DuplicateItem *)lvi.lParam;
            ListView_SetCheckState(g_hDupList, i, pIt->checked);
        }
    }

    SendMessage(g_hDupList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hDupList, NULL, TRUE);
    UpdateWindow(g_hDupList);
    g_bUpdatingDupChecks = false;
}

static void populate_duplicate_listview(void) {
    if (!g_hDupList) return;
    g_bUpdatingDupChecks = true;
    SendMessage(g_hDupList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_hDupList);

    if (!g_pDupResults) {
        SendMessage(g_hDupList, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(g_hDupList, NULL, TRUE);
        UpdateWindow(g_hDupList);
        g_bUpdatingDupChecks = false;
        return;
    }

    int item_idx = 0;
    for (DuplicateGroup *g = g_pDupResults->groups; g; g = g->next) {
        for (DuplicateItem *it = g->items; it; it = it->next) {
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = item_idx;
            lvi.iSubItem = 0;
            lvi.pszText = it->filename;
            lvi.lParam = (LPARAM)it;
            ListView_InsertItem(g_hDupList, &lvi);

            /* SubItem 1: Status (Original vs Duplicate) */
            wchar_t status_buf[64];
            if (it->is_original) {
                swprintf(status_buf, 64, L"⭐ %ls (Grup #%zu)", i18n_get(STR_DUP_ORIGINAL), it->group_id);
            } else {
                swprintf(status_buf, 64, L"📑 %ls (Grup #%zu)", i18n_get(STR_DUP_DUPLICATE), it->group_id);
            }
            ListView_SetItemText(g_hDupList, item_idx, 1, status_buf);

            /* SubItem 2: Size */
            wchar_t size_str[64];
            format_file_size_w(it->size, size_str, 64);
            ListView_SetItemText(g_hDupList, item_idx, 2, size_str);

            /* SubItem 3: Modified Time */
            SYSTEMTIME stUTC, stLocal;
            FileTimeToSystemTime(&it->mod_time, &stUTC);
            SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
            wchar_t date_str[64];
            swprintf(date_str, 64, L"%04d-%02d-%02d %02d:%02d",
                stLocal.wYear, stLocal.wMonth, stLocal.wDay,
                stLocal.wHour, stLocal.wMinute);
            ListView_SetItemText(g_hDupList, item_idx, 3, date_str);

            /* SubItem 4: Full Path */
            ListView_SetItemText(g_hDupList, item_idx, 4, it->path);

            /* Set checkbox */
            ListView_SetCheckState(g_hDupList, item_idx, it->checked);

            item_idx++;
        }
    }

    SendMessage(g_hDupList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hDupList, NULL, TRUE);
    UpdateWindow(g_hDupList);
    g_bUpdatingDupChecks = false;
}

static void duplicate_progress_callback(
    int phase,
    size_t current,
    size_t total,
    const wchar_t *current_file,
    void *user_data
) {
    HWND hWnd = (HWND)user_data;
    if (!hWnd || !IsWindow(hWnd)) return;

    DupProgressPayload *payload = (DupProgressPayload *)malloc(sizeof(DupProgressPayload));
    if (payload) {
        payload->phase = phase;
        payload->current = current;
        payload->total = total;
        wcsncpy(payload->current_file, current_file ? current_file : L"", MAX_PATH * 2 - 1);
        payload->current_file[MAX_PATH * 2 - 1] = L'\0';
        PostMessageW(hWnd, WM_APP_DUP_PROGRESS, 0, (LPARAM)payload);
    }
}

static DWORD WINAPI duplicate_worker_thread(LPVOID lpParam) {
    DupWorkerParams *params = (DupWorkerParams *)lpParam;
    if (!params) return 0;

    DuplicateResults *results = duplicate_find_filtered(
        params->folder,
        params->recursive,
        params->file_type_filter,
        params->max_file_size_bytes,
        duplicate_progress_callback,
        (void *)params->hWnd,
        &g_bDupCancel
    );

    PostMessageW(params->hWnd, WM_APP_DUP_FINISHED, 0, (LPARAM)results);
    free(params);
    return 0;
}

static void start_duplicate_scan(HWND hWnd) {
    if (g_bIsWorking) return;

    wchar_t raw_folder[MAX_PATH * 2] = {0};
    GetWindowTextW(g_hDupEditFolder, raw_folder, MAX_PATH * 2);
    wchar_t folder[MAX_PATH * 2] = {0};
    sanitize_path(raw_folder, folder, MAX_PATH * 2);

    if (folder[0] == L'\0') {
        MessageBoxW(hWnd, i18n_get(STR_WARN_SELECT_FOLDER),
            (i18n_get_language() == LANG_ID) ? L"Peringatan" : L"Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    DWORD attr = GetFileAttributesW(folder);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        MessageBoxW(hWnd, i18n_get(STR_WARN_INVALID_FOLDER),
            (i18n_get_language() == LANG_ID) ? L"Kesalahan" : L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    bool recursive = (SendMessage(g_hDupChkRecursive, BM_GETCHECK, 0, 0) == BST_CHECKED);
    ScannerFileType file_type_filter = SCANNER_FILE_TYPE_ALL;
    uint64_t max_file_size_bytes = 0;
    if (!read_file_filters(hWnd, g_hDupFileType, g_hDupMaxSizeEdit, &file_type_filter, &max_file_size_bytes)) return;

    if (g_pDupResults) {
        duplicate_free_results(g_pDupResults);
        g_pDupResults = NULL;
    }
    ListView_DeleteAllItems(g_hDupList);

    g_bIsWorking = true;
    g_bDupCancel = false;

    EnableWindow(g_hDupBtnScan, FALSE);
    EnableWindow(g_hDupBtnAutoSel, FALSE);
    EnableWindow(g_hDupBtnDelete, FALSE);
    EnableWindow(g_hDupBtnStop, TRUE);

    SendMessage(g_hDupProgress, PBM_SETRANGE32, 0, 100);
    SendMessage(g_hDupProgress, PBM_SETPOS, 0, 0);

    bool is_id = (i18n_get_language() == LANG_ID);
    SetWindowTextW(g_hDupStaticStatus, is_id ? L"Memulai pemindaian direktori..." : L"Starting directory scan...");

    DupWorkerParams *params = (DupWorkerParams *)malloc(sizeof(DupWorkerParams));
    if (!params) {
        g_bIsWorking = false;
        EnableWindow(g_hDupBtnScan, TRUE);
        EnableWindow(g_hDupBtnStop, FALSE);
        return;
    }
    params->hWnd = hWnd;
    wcsncpy(params->folder, folder, MAX_PATH * 2 - 1);
    params->folder[MAX_PATH * 2 - 1] = L'\0';
    params->recursive = recursive;
    params->file_type_filter = file_type_filter;
    params->max_file_size_bytes = max_file_size_bytes;

    g_hDupWorkerThread = CreateThread(NULL, 0, duplicate_worker_thread, (LPVOID)params, 0, NULL);
    if (!g_hDupWorkerThread) {
        free(params);
        g_bIsWorking = false;
        EnableWindow(g_hDupBtnScan, TRUE);
        EnableWindow(g_hDupBtnStop, FALSE);
        SetWindowTextW(g_hDupStaticStatus, L"Could not start duplicate scan worker.");
    }
}

static void delete_selected_duplicates(HWND hWnd) {
    if (!g_pDupResults || g_bIsWorking) return;

    int item_count = ListView_GetItemCount(g_hDupList);
    size_t checked_count = 0;
    uint64_t checked_bytes = 0;

    for (int i = 0; i < item_count; i++) {
        BOOL is_checked = ListView_GetCheckState(g_hDupList, i);
        LVITEMW lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        if (ListView_GetItem(g_hDupList, &lvi)) {
            DuplicateItem *pIt = (DuplicateItem *)lvi.lParam;
            if (pIt) {
                pIt->checked = (is_checked != FALSE);
                if (pIt->checked) {
                    checked_count++;
                    checked_bytes += pIt->size;
                }
            }
        }
    }

    if (checked_count == 0) {
        MessageBoxW(hWnd, i18n_get(STR_DUP_WARN_NO_SELECTION),
            (i18n_get_language() == LANG_ID) ? L"Informasi" : L"Information", MB_OK | MB_ICONINFORMATION);
        return;
    }

    wchar_t size_str[64];
    format_file_size_w(checked_bytes, size_str, 64);

    wchar_t confirm_msg[512];
    swprintf(confirm_msg, 512, i18n_get(STR_DUP_CONFIRM_DEL_MSG), checked_count, size_str);

    int res = MessageBoxW(hWnd, confirm_msg, i18n_get(STR_DUP_CONFIRM_DEL_TITLE),
        MB_YESNO | MB_ICONQUESTION);
    if (res != IDYES) return;

    uint64_t deleted_bytes = 0;
    size_t deleted_count = duplicate_delete_checked(g_pDupResults, hWnd, &deleted_bytes);

    populate_duplicate_listview();

    wchar_t del_size_str[64];
    format_file_size_w(deleted_bytes, del_size_str, 64);

    wchar_t success_msg[512];
    swprintf(success_msg, 512, i18n_get(STR_DUP_DEL_SUCCESS), deleted_count, del_size_str);
    MessageBoxW(hWnd, success_msg,
        (i18n_get_language() == LANG_ID) ? L"Sukses" : L"Success", MB_OK | MB_ICONINFORMATION);

    wchar_t status_buf[256];
    bool is_id = (i18n_get_language() == LANG_ID);
    if (is_id) {
        swprintf(status_buf, 256, L"Berhasil menghapus %zu salinan. Ruang dihemat: %ls.", deleted_count, del_size_str);
    } else {
        swprintf(status_buf, 256, L"Successfully removed %zu copy file(s). Reclaimed: %ls.", deleted_count, del_size_str);
    }
    SetWindowTextW(g_hDupStaticStatus, status_buf);
}

static void handle_duplicate_context_menu(HWND hWnd, int x, int y) {
    if (!g_hDupList) return;

    int sel_idx = ListView_GetNextItem(g_hDupList, -1, LVNI_SELECTED);
    if (sel_idx < 0) return;

    LVITEMW lvi = {0};
    lvi.mask = LVIF_PARAM;
    lvi.iItem = sel_idx;
    if (!ListView_GetItem(g_hDupList, &lvi)) return;
    DuplicateItem *pIt = (DuplicateItem *)lvi.lParam;
    if (!pIt) return;

    POINT pt;
    if (x == -1 && y == -1) {
        RECT rc;
        ListView_GetItemRect(g_hDupList, sel_idx, &rc, LVIR_BOUNDS);
        pt.x = rc.left + 50;
        pt.y = rc.top + 10;
        ClientToScreen(g_hDupList, &pt);
    } else {
        pt.x = x;
        pt.y = y;
    }

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDC_MENU_DUP_OPEN, i18n_get(STR_DUP_MENU_OPEN));
    AppendMenuW(hMenu, MF_STRING, IDC_MENU_DUP_EXPLORER, i18n_get(STR_DUP_MENU_EXPLORER));
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    BOOL is_checked = ListView_GetCheckState(g_hDupList, sel_idx);
    AppendMenuW(hMenu, MF_STRING, IDC_MENU_DUP_TOGGLE, is_checked ? i18n_get(STR_DUP_MENU_UNCHECK) : i18n_get(STR_DUP_MENU_CHECK));
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDC_MENU_DUP_DELETE, i18n_get(STR_DUP_BTN_DELETE));

    SetForegroundWindow(hWnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);

    if (cmd == IDC_MENU_DUP_OPEN) {
        if (pIt->path) {
            ShellExecuteW(NULL, L"open", pIt->path, NULL, NULL, SW_SHOWNORMAL);
        }
    } else if (cmd == IDC_MENU_DUP_EXPLORER) {
        if (pIt->path) {
            wchar_t param[MAX_PATH * 2 + 32];
            swprintf(param, MAX_PATH * 2 + 32, L"/select,\"%ls\"", pIt->path);
            ShellExecuteW(NULL, L"open", L"explorer.exe", param, NULL, SW_SHOWNORMAL);
        }
    } else if (cmd == IDC_MENU_DUP_TOGGLE) {
        pIt->checked = !is_checked;
        ListView_SetCheckState(g_hDupList, sel_idx, pIt->checked);
    } else if (cmd == IDC_MENU_DUP_DELETE) {
        pIt->checked = true;
        ListView_SetCheckState(g_hDupList, sel_idx, TRUE);
        delete_selected_duplicates(hWnd);
    }
}

bool ui_init(HINSTANCE hInstance) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES |
                 ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    vault_init();
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
    int winWidth = 980;
    int winHeight = 720;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - winWidth) / 2;
    int y = (screenH - winHeight) / 2;

    HWND hWnd = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        L"Win10FileModMainClass",
        i18n_get(STR_APP_TITLE),
        WS_OVERLAPPEDWINDOW,
        x, y, winWidth, winHeight,
        NULL, create_app_menu(), hInstance, NULL
    );

    if (hWnd) {
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
        if (g_hLoginEditPass && IsWindowVisible(g_hLoginEditPass) && IsWindowEnabled(g_hLoginEditPass)) {
            SetFocus(g_hLoginEditPass);
        } else if (g_hSetupEditPass && IsWindowVisible(g_hSetupEditPass)) {
            SetFocus(g_hSetupEditPass);
        }
    }

    return hWnd;
}

/* ========================================================================= */
/* Main Window Procedure                                                     */
/* ========================================================================= */

LRESULT CALLBACK ui_wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SETFOCUS: {
            if (!g_bAuthenticated && vault_is_configured()) {
                if (g_hLoginEditPass && IsWindowVisible(g_hLoginEditPass) && IsWindowEnabled(g_hLoginEditPass)) {
                    SetFocus(g_hLoginEditPass);
                    return 0;
                }
            } else if (!vault_is_configured()) {
                if (g_hSetupEditPass && IsWindowVisible(g_hSetupEditPass)) {
                    SetFocus(g_hSetupEditPass);
                    return 0;
                }
            }
            break;
        }

        case DM_GETDEFID: {
            if (!g_bAuthenticated && vault_is_configured()) {
                return MAKELRESULT(IDC_LOGIN_BTN_MASUK, DC_HASDEFID);
            } else if (!vault_is_configured()) {
                return MAKELRESULT(IDC_SETUP_BTN_SAVE, DC_HASDEFID);
            }
            return 0;
        }

        case WM_CREATE: {
            g_hWndMain = hWnd;
            tracker_init();
            tracker_encrypt_all_decrypted_files();
            SetTimer(hWnd, TIMER_AUTO_ENCRYPT_ID, 15000, NULL);
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
            g_hFontTitle = CreateFontW(
                -15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );

            /* Main Tab Control */
            g_hTab = CreateWindowExW(
                0, WC_TABCONTROLW, L"",
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_TABSTOP,
                0, 0, 980, 720, hWnd, (HMENU)IDC_MAIN_TAB, NULL, NULL
            );
            set_font(g_hTab, g_hFontBold);

            TCITEMW tie = {0};
            tie.mask = TCIF_TEXT;
            tie.pszText = (LPWSTR)i18n_get(STR_TAB_ENCRYPT);
            TabCtrl_InsertItem(g_hTab, 0, &tie);

            tie.pszText = (LPWSTR)i18n_get(STR_TAB_RENAME);
            TabCtrl_InsertItem(g_hTab, 1, &tie);

            tie.pszText = (LPWSTR)i18n_get(STR_TAB_DUPLICATES);
            TabCtrl_InsertItem(g_hTab, 2, &tie);

            tie.pszText = (LPWSTR)i18n_get(STR_TAB_MODEL_TESTER);
            TabCtrl_InsertItem(g_hTab, 3, &tie);

            /* ================= VIEW 1A: SETUP / ONBOARDING ================= */
            g_hSetupGrp = CreateWindowExW(0, L"BUTTON", L" 🛡️ Pengaturan Keamanan Master & Email Pemulihan (Wajib) ",
                WS_CHILD | BS_GROUPBOX, 0, 0, 500, 380, hWnd, (HMENU)IDC_SETUP_GRP, NULL, NULL);
            set_font(g_hSetupGrp, g_hFontBold);
            register_ctrl(g_hSetupCtrls, &g_nSetupCtrls, 16, g_hSetupGrp);

            g_hSetupLblInfo = CreateWindowExW(0, L"STATIC",
                L"Untuk keamanan data militer, Anda diwajibkan membuat Master Password dan mendaftarkan Email Pemulihan. Email ini digunakan jika lupa password atau akun terkunci.",
                WS_CHILD, 0, 0, 450, 45, hWnd, (HMENU)IDC_SETUP_LBL_INFO, NULL, NULL);
            set_font(g_hSetupLblInfo, g_hFont);
            register_ctrl(g_hSetupCtrls, &g_nSetupCtrls, 16, g_hSetupLblInfo);

            g_hSetupLblPass = CreateWindowExW(0, L"STATIC", L"Master Password Baru (Min 4 karakter):",
                WS_CHILD, 0, 0, 450, 20, hWnd, (HMENU)IDC_SETUP_LBL_PASS, NULL, NULL);
            set_font(g_hSetupLblPass, g_hFont);
            register_ctrl(g_hSetupCtrls, &g_nSetupCtrls, 16, g_hSetupLblPass);

            g_hSetupEditPass = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 450, 24, hWnd, (HMENU)IDC_SETUP_EDIT_PASS, NULL, NULL);
            set_font(g_hSetupEditPass, g_hFont);
            register_ctrl(g_hSetupCtrls, &g_nSetupCtrls, 16, g_hSetupEditPass);

            g_hSetupLblConfirm = CreateWindowExW(0, L"STATIC", L"Konfirmasi Master Password:",
                WS_CHILD, 0, 0, 450, 20, hWnd, (HMENU)IDC_SETUP_LBL_CONFIRM, NULL, NULL);
            set_font(g_hSetupLblConfirm, g_hFont);
            register_ctrl(g_hSetupCtrls, &g_nSetupCtrls, 16, g_hSetupLblConfirm);

            g_hSetupEditConfirm = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 450, 24, hWnd, (HMENU)IDC_SETUP_EDIT_CONFIRM, NULL, NULL);
            set_font(g_hSetupEditConfirm, g_hFont);
            register_ctrl(g_hSetupCtrls, &g_nSetupCtrls, 16, g_hSetupEditConfirm);

            g_hSetupLblEmail = CreateWindowExW(0, L"STATIC", L"Alamat Email Pemulihan (Wajib untuk reset password):",
                WS_CHILD, 0, 0, 450, 20, hWnd, (HMENU)IDC_SETUP_LBL_EMAIL, NULL, NULL);
            set_font(g_hSetupLblEmail, g_hFont);
            register_ctrl(g_hSetupCtrls, &g_nSetupCtrls, 16, g_hSetupLblEmail);

            g_hSetupEditEmail = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 450, 24, hWnd, (HMENU)IDC_SETUP_EDIT_EMAIL, NULL, NULL);
            set_font(g_hSetupEditEmail, g_hFont);
            register_ctrl(g_hSetupCtrls, &g_nSetupCtrls, 16, g_hSetupEditEmail);

            g_hSetupBtnSave = CreateWindowExW(0, L"BUTTON", L"Daftarkan & Buka Aplikasi",
                WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 0, 0, 450, 36, hWnd, (HMENU)IDC_SETUP_BTN_SAVE, NULL, NULL);
            set_font(g_hSetupBtnSave, g_hFontBold);
            register_ctrl(g_hSetupCtrls, &g_nSetupCtrls, 16, g_hSetupBtnSave);

            /* ================= VIEW 1B: LOGIN SCREEN ================= */
            g_hLoginGrp = CreateWindowExW(0, L"BUTTON", L" 🔒 Autentikasi Master Password ",
                WS_CHILD | BS_GROUPBOX, 0, 0, 460, 330, hWnd, (HMENU)IDC_LOGIN_GRP, NULL, NULL);
            set_font(g_hLoginGrp, g_hFontBold);
            register_ctrl(g_hLoginCtrls, &g_nLoginCtrls, 16, g_hLoginGrp);

            g_hLoginLblInfo = CreateWindowExW(0, L"STATIC", L"Masukkan Master Password untuk membuka akses folder & enkripsi:",
                WS_CHILD, 0, 0, 410, 25, hWnd, (HMENU)IDC_LOGIN_LBL_INFO, NULL, NULL);
            set_font(g_hLoginLblInfo, g_hFont);
            register_ctrl(g_hLoginCtrls, &g_nLoginCtrls, 16, g_hLoginLblInfo);

            g_hLoginLblPass = CreateWindowExW(0, L"STATIC", L"Master Password:",
                WS_CHILD, 0, 0, 410, 20, hWnd, (HMENU)IDC_LOGIN_LBL_PASS, NULL, NULL);
            set_font(g_hLoginLblPass, g_hFont);
            register_ctrl(g_hLoginCtrls, &g_nLoginCtrls, 16, g_hLoginLblPass);

            g_hLoginEditPass = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 410, 25, hWnd, (HMENU)IDC_LOGIN_EDIT_PASS, NULL, NULL);
            set_font(g_hLoginEditPass, g_hFont);
            g_pfnOrigLoginPassProc = (WNDPROC)SetWindowLongPtrW(g_hLoginEditPass, GWLP_WNDPROC, (LONG_PTR)login_pass_edit_proc);
            register_ctrl(g_hLoginCtrls, &g_nLoginCtrls, 16, g_hLoginEditPass);

            g_hLoginChkShow = CreateWindowExW(0, L"BUTTON", L"Lihat Karakter Password",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 0, 0, 200, 20, hWnd, (HMENU)IDC_LOGIN_CHK_SHOW, NULL, NULL);
            set_font(g_hLoginChkShow, g_hFont);
            register_ctrl(g_hLoginCtrls, &g_nLoginCtrls, 16, g_hLoginChkShow);

            g_hLoginLblAttempts = CreateWindowExW(0, L"STATIC", L"Percobaan gagal: 0 / 5",
                WS_CHILD, 0, 0, 410, 20, hWnd, (HMENU)IDC_LOGIN_LBL_ATTEMPTS, NULL, NULL);
            set_font(g_hLoginLblAttempts, g_hFontBold);
            register_ctrl(g_hLoginCtrls, &g_nLoginCtrls, 16, g_hLoginLblAttempts);

            g_hLoginLblStatus = CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD, 0, 0, 410, 40, hWnd, (HMENU)IDC_LOGIN_LBL_STATUS, NULL, NULL);
            set_font(g_hLoginLblStatus, g_hFont);
            register_ctrl(g_hLoginCtrls, &g_nLoginCtrls, 16, g_hLoginLblStatus);

            g_hLoginBtnMasuk = CreateWindowExW(0, L"BUTTON", L"Masuk",
                WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 0, 0, 230, 36, hWnd, (HMENU)IDC_LOGIN_BTN_MASUK, NULL, NULL);
            set_font(g_hLoginBtnMasuk, g_hFontBold);
            register_ctrl(g_hLoginCtrls, &g_nLoginCtrls, 16, g_hLoginBtnMasuk);

            g_hLoginBtnLupa = CreateWindowExW(0, L"BUTTON", L"Lupa Password",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 170, 36, hWnd, (HMENU)IDC_LOGIN_BTN_LUPA, NULL, NULL);
            set_font(g_hLoginBtnLupa, g_hFontBold);
            register_ctrl(g_hLoginCtrls, &g_nLoginCtrls, 16, g_hLoginBtnLupa);

            /* ================= VIEW 1C: AUTHENTICATED EXPLORER ================= */
            g_hEncLblFolder = CreateWindowExW(0, L"STATIC", L"Folder Target:",
                WS_CHILD, 0, 0, 90, 20, hWnd, (HMENU)IDC_ENC_LBL_FOLDER, NULL, NULL);
            set_font(g_hEncLblFolder, g_hFontBold);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEncLblFolder);

            g_hEditFolder = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_szLastFolder,
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 500, 24, hWnd, (HMENU)IDC_EDIT_FOLDER, NULL, NULL);
            set_font(g_hEditFolder, g_hFont);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEditFolder);

            g_hBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 80, 26, hWnd, (HMENU)IDC_BTN_BROWSE, NULL, NULL);
            set_font(g_hBtnBrowse, g_hFont);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hBtnBrowse);

            g_hChkRecursive = CreateWindowExW(0, L"BUTTON", L"Pindai Sub-folder",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 0, 0, 140, 22, hWnd, (HMENU)IDC_CHK_RECURSIVE, NULL, NULL);
            set_font(g_hChkRecursive, g_hFont);
            SendMessage(g_hChkRecursive, BM_SETCHECK, BST_CHECKED, 0);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hChkRecursive);

            g_hChkRemember1 = CreateWindowExW(0, L"BUTTON", L"Ingat folder",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 0, 0, 100, 22, hWnd, (HMENU)IDC_CHK_REMEMBER, NULL, NULL);
            set_font(g_hChkRemember1, g_hFont);
            SendMessage(g_hChkRemember1, BM_SETCHECK, g_bRememberFolder ? BST_CHECKED : BST_UNCHECKED, 0);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hChkRemember1);

            g_hEncFileTypeLabel = CreateWindowExW(0, L"STATIC", i18n_get(STR_ENC_FILE_TYPE_LABEL),
                WS_CHILD, 0, 0, 65, 22, hWnd, (HMENU)IDC_ENC_TYPE_LABEL, NULL, NULL);
            set_font(g_hEncFileTypeLabel, g_hFontBold);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEncFileTypeLabel);

            g_hEncFileType = CreateWindowExW(0, L"COMBOBOX", L"",
                WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0, 0, 140, 120,
                hWnd, (HMENU)IDC_ENC_FILE_TYPE, NULL, NULL);
            set_font(g_hEncFileType, g_hFont);
            SendMessageW(g_hEncFileType, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_ENC_FILE_TYPE_ALL));
            SendMessageW(g_hEncFileType, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_ENC_FILE_TYPE_DOCUMENTS));
            SendMessageW(g_hEncFileType, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_ENC_FILE_TYPE_IMAGES));
            SendMessageW(g_hEncFileType, CB_ADDSTRING, 0, (LPARAM)i18n_get(STR_ENC_FILE_TYPE_VIDEOS));
            SendMessageW(g_hEncFileType, CB_SETCURSEL, 0, 0);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEncFileType);

            g_hEncFileExtensions = CreateWindowExW(0, L"STATIC", i18n_get(STR_ENC_EXT_ALL),
                WS_CHILD | SS_LEFT, 0, 0, 385, 22, hWnd, (HMENU)IDC_ENC_EXTENSIONS, NULL, NULL);
            set_font(g_hEncFileExtensions, g_hFont);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEncFileExtensions);

            g_hEncMaxSizeLabel = CreateWindowExW(0, L"STATIC", i18n_get(STR_ENC_MAX_SIZE_LABEL),
                WS_CHILD, 0, 0, 122, 22, hWnd, (HMENU)IDC_ENC_MAX_SIZE_LABEL, NULL, NULL);
            set_font(g_hEncMaxSizeLabel, g_hFontBold);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEncMaxSizeLabel);

            g_hEncMaxSizeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | ES_NUMBER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 58, 24,
                hWnd, (HMENU)IDC_ENC_MAX_SIZE, NULL, NULL);
            set_font(g_hEncMaxSizeEdit, g_hFont);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEncMaxSizeEdit);

            g_hEncMaxSizeUnit = CreateWindowExW(0, L"STATIC", i18n_get(STR_ENC_MAX_SIZE_UNIT),
                WS_CHILD, 0, 0, 75, 22, hWnd, (HMENU)IDC_ENC_MAX_SIZE_UNIT, NULL, NULL);
            set_font(g_hEncMaxSizeUnit, g_hFont);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEncMaxSizeUnit);
            update_file_filter_extensions(g_hEncFileType, g_hEncFileExtensions);

            /* TreeView (Directory Sidebar) */
            g_hEncTree = CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
                WS_CHILD | WS_BORDER | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
                0, 0, 250, 400, hWnd, (HMENU)IDC_ENC_TREE, NULL, NULL
            );
            set_font(g_hEncTree, g_hFont);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEncTree);

            /* Attach System Image List to TreeView */
            SHFILEINFOW sfiTree = {0};
            HIMAGELIST hSysImg = (HIMAGELIST)SHGetFileInfoW(L"C:\\", 0, &sfiTree, sizeof(sfiTree), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
            if (hSysImg) {
                SendMessage(g_hEncTree, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hSysImg);
            }
            populate_tree_drives(g_hEncTree);

            /* ListView (File & Folder Details Pane) */
            g_hEncExplorerList = CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
                0, 0, 600, 400, hWnd, (HMENU)IDC_ENC_EXPLORER_LIST, NULL, NULL
            );
            set_font(g_hEncExplorerList, g_hFont);
            ListView_SetExtendedListViewStyle(g_hEncExplorerList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
            g_pfnOrigExplorerListProc = (WNDPROC)SetWindowLongPtrW(g_hEncExplorerList, GWLP_WNDPROC, (LONG_PTR)explorer_list_proc);

            LVCOLUMNW col = {0};
            col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

            col.cx = 260; col.pszText = L"Nama Berkas / Folder";
            ListView_InsertColumn(g_hEncExplorerList, 0, &col);

            col.cx = 100; col.pszText = L"Tipe";
            ListView_InsertColumn(g_hEncExplorerList, 1, &col);

            col.cx = 95; col.pszText = L"Ukuran";
            ListView_InsertColumn(g_hEncExplorerList, 2, &col);

            col.cx = 135; col.pszText = L"Tanggal Dimodifikasi";
            ListView_InsertColumn(g_hEncExplorerList, 3, &col);

            col.cx = 145; col.pszText = L"Status Enkripsi";
            ListView_InsertColumn(g_hEncExplorerList, 4, &col);

            if (hSysImg) {
                SendMessage(g_hEncExplorerList, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hSysImg);
            }
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hEncExplorerList);

            g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                WS_CHILD | PBS_SMOOTH, 0, 0, 700, 16, hWnd, (HMENU)IDC_PROGRESS, NULL, NULL);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hProgress);

            g_hStaticStatus = CreateWindowExW(0, L"STATIC", L"Pilih folder dari pohon direktori atau tombol Browse...",
                WS_CHILD, 0, 0, 700, 20, hWnd, (HMENU)IDC_STATIC_STATUS, NULL, NULL);
            set_font(g_hStaticStatus, g_hFont);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hStaticStatus);

            g_hLogBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                0, 0, 700, 100, hWnd, (HMENU)IDC_LOG_BOX, NULL, NULL);
            set_font(g_hLogBox, g_hFont);
            register_ctrl(g_hExplorerCtrls, &g_nExplorerCtrls, 32, g_hLogBox);

            append_log(g_hLogBox, L"=== Win10 Native File Encryptor & Secure Shredder ===");
            append_log(g_hLogBox, L"Standar Kriptografi: Windows CNG AES-256-GCM AEAD + PBKDF2 Ginger.");

            /* ================= PAGE 2: BULK RENAMER ================= */
            g_hRenGrp1 = CreateWindowExW(0, L"BUTTON", L" 1. Pemilihan Folder Sumber (Langsung Dipindai Otomatis) ",
                WS_CHILD | BS_GROUPBOX, 0, 0, 700, 65, hWnd, (HMENU)IDC_REN_GRP_FOLDER, NULL, NULL);
            set_font(g_hRenGrp1, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenGrp1);

            g_hRenEditFolder = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_szLastFolder,
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 450, 24, hWnd, (HMENU)IDC_REN_EDIT_FOLDER, NULL, NULL);
            set_font(g_hRenEditFolder, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditFolder);

            g_hRenBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 80, 26, hWnd, (HMENU)IDC_REN_BTN_BROWSE, NULL, NULL);
            set_font(g_hRenBtnBrowse, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnBrowse);

            g_hRenChkRecursive = CreateWindowExW(0, L"BUTTON", L"Subfolder",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 0, 0, 85, 22, hWnd, (HMENU)IDC_REN_CHK_RECURSIVE, NULL, NULL);
            set_font(g_hRenChkRecursive, g_hFont);
            SendMessage(g_hRenChkRecursive, BM_SETCHECK, BST_CHECKED, 0);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenChkRecursive);

            g_hRenChkRemember2 = CreateWindowExW(0, L"BUTTON", L"Ingat folder",
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 0, 0, 110, 22, hWnd, (HMENU)IDC_REN_CHK_REMEMBER, NULL, NULL);
            set_font(g_hRenChkRemember2, g_hFont);
            SendMessage(g_hRenChkRemember2, BM_SETCHECK, g_bRememberFolder ? BST_CHECKED : BST_UNCHECKED, 0);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenChkRemember2);

            create_file_filter_controls(
                hWnd,
                IDC_REN_FILE_TYPE_LABEL, IDC_REN_FILE_TYPE, IDC_REN_EXTENSIONS,
                IDC_REN_MAX_SIZE_LABEL, IDC_REN_MAX_SIZE, IDC_REN_MAX_SIZE_UNIT,
                &g_hRenFileTypeLabel, &g_hRenFileType, &g_hRenFileExtensions,
                &g_hRenMaxSizeLabel, &g_hRenMaxSizeEdit, &g_hRenMaxSizeUnit,
                g_hRenCtrls, &g_nRenCtrls, 64
            );

            g_hRenGrp2 = CreateWindowExW(0, L"BUTTON", L" 2. Metode & Aturan Penamaan Baru (Advanced Renamer Style) ",
                WS_CHILD | BS_GROUPBOX, 0, 0, 700, 75, hWnd, (HMENU)IDC_REN_GRP_METHOD, NULL, NULL);
            set_font(g_hRenGrp2, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenGrp2);

            g_hRenLblMethod = CreateWindowExW(0, L"STATIC", L"Metode:",
                WS_CHILD, 0, 0, 55, 20, hWnd, (HMENU)IDC_REN_LBL_METHOD, NULL, NULL);
            set_font(g_hRenLblMethod, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenLblMethod);

            g_hRenComboMethod = CreateWindowExW(0, L"COMBOBOX", L"",
                WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0, 0, 195, 140, hWnd, (HMENU)IDC_REN_COMBO_METHOD, NULL, NULL);
            set_font(g_hRenComboMethod, g_hFont);
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"1. Ganti Teks (Find & Replace)");
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"2. Tambah Teks (Prefix/Suffix)");
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"3. Pola Tag (<name>, <inc:01>)");
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"4. Ubah Huruf Besar/Kecil");
            SendMessageW(g_hRenComboMethod, CB_ADDSTRING, 0, (LPARAM)L"5. Pangkas Karakter (Trim)");
            SendMessage(g_hRenComboMethod, CB_SETCURSEL, 0, 0);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenComboMethod);

            g_hRenLblP1 = CreateWindowExW(0, L"STATIC", L"Cari Teks (Find):",
                WS_CHILD, 0, 0, 110, 20, hWnd, (HMENU)IDC_REN_LBL_P1, NULL, NULL);
            set_font(g_hRenLblP1, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenLblP1);

            g_hRenEditP1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 150, 24, hWnd, (HMENU)IDC_REN_EDIT_P1, NULL, NULL);
            set_font(g_hRenEditP1, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditP1);

            g_hRenLblP2 = CreateWindowExW(0, L"STATIC", L"Ganti Dengan (Replace):",
                WS_CHILD, 0, 0, 135, 20, hWnd, (HMENU)IDC_REN_LBL_P2, NULL, NULL);
            set_font(g_hRenLblP2, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenLblP2);

            g_hRenEditP2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 95, 24, hWnd, (HMENU)IDC_REN_EDIT_P2, NULL, NULL);
            set_font(g_hRenEditP2, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditP2);

            g_hRenComboP3 = CreateWindowExW(0, L"COMBOBOX", L"",
                WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 0, 0, 150, 120, hWnd, (HMENU)IDC_REN_COMBO_P3, NULL, NULL);
            set_font(g_hRenComboP3, g_hFont);
            ShowWindow(g_hRenComboP3, SW_HIDE);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenComboP3);

            /* Stepper Controls Group 1 */
            g_hRenBtnDec1 = CreateWindowExW(0, L"BUTTON", L"-",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 28, 24, hWnd, (HMENU)IDC_REN_BTN_DEC1, NULL, NULL);
            set_font(g_hRenBtnDec1, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnDec1);

            g_hRenEditNum1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | ES_CENTER | ES_NUMBER | WS_TABSTOP, 0, 0, 40, 24, hWnd, (HMENU)IDC_REN_EDIT_NUM1, NULL, NULL);
            set_font(g_hRenEditNum1, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditNum1);

            g_hRenBtnInc1 = CreateWindowExW(0, L"BUTTON", L"+",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 28, 24, hWnd, (HMENU)IDC_REN_BTN_INC1, NULL, NULL);
            set_font(g_hRenBtnInc1, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnInc1);

            /* Stepper Controls Group 2 */
            g_hRenBtnDec2 = CreateWindowExW(0, L"BUTTON", L"-",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 28, 24, hWnd, (HMENU)IDC_REN_BTN_DEC2, NULL, NULL);
            set_font(g_hRenBtnDec2, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnDec2);

            g_hRenEditNum2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | ES_CENTER | ES_NUMBER | WS_TABSTOP, 0, 0, 40, 24, hWnd, (HMENU)IDC_REN_EDIT_NUM2, NULL, NULL);
            set_font(g_hRenEditNum2, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenEditNum2);

            g_hRenBtnInc2 = CreateWindowExW(0, L"BUTTON", L"+",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 28, 24, hWnd, (HMENU)IDC_REN_BTN_INC2, NULL, NULL);
            set_font(g_hRenBtnInc2, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnInc2);

            /* Renamer ListView */
            g_hRenList = CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
                0, 0, 700, 300, hWnd, (HMENU)IDC_REN_LISTVIEW, NULL, NULL
            );
            set_font(g_hRenList, g_hFont);
            ListView_SetExtendedListViewStyle(g_hRenList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

            LVCOLUMNW c = {0};
            c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            c.cx = 40; c.pszText = L"No."; ListView_InsertColumn(g_hRenList, 0, &c);
            c.cx = 260; c.pszText = L"Nama Asli Berkas"; ListView_InsertColumn(g_hRenList, 1, &c);
            c.cx = 260; c.pszText = L"Nama Baru (Live Preview)"; ListView_InsertColumn(g_hRenList, 2, &c);
            c.cx = 85; c.pszText = L"Ukuran"; ListView_InsertColumn(g_hRenList, 3, &c);
            c.cx = 120; c.pszText = L"Status"; ListView_InsertColumn(g_hRenList, 4, &c);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenList);

            g_hRenBtnPreview = CreateWindowExW(0, L"BUTTON", L"🔄 Segarkan Pratinjau",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 130, 28, hWnd, (HMENU)IDC_REN_BTN_PREVIEW, NULL, NULL);
            set_font(g_hRenBtnPreview, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnPreview);

            g_hRenBtnStart = CreateWindowExW(0, L"BUTTON", L"🚀 MULAI BATCH RENAME",
                WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 0, 0, 190, 28, hWnd, (HMENU)IDC_REN_BTN_START, NULL, NULL);
            set_font(g_hRenBtnStart, g_hFontBold);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnStart);

            g_hRenBtnUndo = CreateWindowExW(0, L"BUTTON", L"↩️ Batalkan Rename (Undo)",
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 150, 28, hWnd, (HMENU)IDC_REN_BTN_UNDO, NULL, NULL);
            set_font(g_hRenBtnUndo, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenBtnUndo);

            g_hRenStaticStatus = CreateWindowExW(0, L"STATIC", L"Pilih folder untuk memindai berkas secara otomatis.",
                WS_CHILD, 0, 0, 300, 20, hWnd, (HMENU)IDC_REN_STATIC_STATUS, NULL, NULL);
            set_font(g_hRenStaticStatus, g_hFont);
            register_ctrl(g_hRenCtrls, &g_nRenCtrls, 64, g_hRenStaticStatus);

            /* ================= VIEW 3: DUPLICATE FILE FINDER ================= */
            g_hDupGrpFolder = CreateWindowExW(0, L"BUTTON", i18n_get(STR_DUP_GRP_TARGET),
                WS_CHILD | BS_GROUPBOX, 0, 0, 700, 65, hWnd, (HMENU)IDC_DUP_GRP_FOLDER, NULL, NULL);
            set_font(g_hDupGrpFolder, g_hFontBold);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupGrpFolder);

            g_hDupEditFolder = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_szLastFolder,
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 300, 24, hWnd, (HMENU)IDC_DUP_EDIT_FOLDER, NULL, NULL);
            set_font(g_hDupEditFolder, g_hFont);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupEditFolder);

            g_hDupBtnBrowse = CreateWindowExW(0, L"BUTTON", i18n_get(STR_BTN_BROWSE),
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 80, 26, hWnd, (HMENU)IDC_DUP_BTN_BROWSE, NULL, NULL);
            set_font(g_hDupBtnBrowse, g_hFont);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupBtnBrowse);

            g_hDupChkRecursive = CreateWindowExW(0, L"BUTTON", i18n_get(STR_CHK_RECURSIVE),
                WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 0, 0, 130, 22, hWnd, (HMENU)IDC_DUP_CHK_RECURSIVE, NULL, NULL);
            set_font(g_hDupChkRecursive, g_hFont);
            SendMessage(g_hDupChkRecursive, BM_SETCHECK, BST_CHECKED, 0);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupChkRecursive);

            g_hDupBtnScan = CreateWindowExW(0, L"BUTTON", i18n_get(STR_DUP_BTN_SCAN),
                WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 0, 0, 115, 26, hWnd, (HMENU)IDC_DUP_BTN_SCAN, NULL, NULL);
            set_font(g_hDupBtnScan, g_hFontBold);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupBtnScan);

            create_file_filter_controls(
                hWnd,
                IDC_DUP_FILE_TYPE_LABEL, IDC_DUP_FILE_TYPE, IDC_DUP_EXTENSIONS,
                IDC_DUP_MAX_SIZE_LABEL, IDC_DUP_MAX_SIZE, IDC_DUP_MAX_SIZE_UNIT,
                &g_hDupFileTypeLabel, &g_hDupFileType, &g_hDupFileExtensions,
                &g_hDupMaxSizeLabel, &g_hDupMaxSizeEdit, &g_hDupMaxSizeUnit,
                g_hDupCtrls, &g_nDupCtrls, 32
            );

            g_hDupList = CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
                0, 0, 700, 300, hWnd, (HMENU)IDC_DUP_LISTVIEW, NULL, NULL
            );
            set_font(g_hDupList, g_hFont);
            ListView_SetExtendedListViewStyle(g_hDupList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);

            LVCOLUMNW dc = {0};
            dc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            dc.cx = 240; dc.pszText = (LPWSTR)i18n_get(STR_DUP_COL_FILENAME); ListView_InsertColumn(g_hDupList, 0, &dc);
            dc.cx = 170; dc.pszText = (LPWSTR)i18n_get(STR_DUP_COL_STATUS); ListView_InsertColumn(g_hDupList, 1, &dc);
            dc.cx = 95;  dc.pszText = (LPWSTR)i18n_get(STR_DUP_COL_SIZE); ListView_InsertColumn(g_hDupList, 2, &dc);
            dc.cx = 135; dc.pszText = (LPWSTR)i18n_get(STR_DUP_COL_MODIFIED); ListView_InsertColumn(g_hDupList, 3, &dc);
            dc.cx = 320; dc.pszText = (LPWSTR)i18n_get(STR_DUP_COL_PATH); ListView_InsertColumn(g_hDupList, 4, &dc);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupList);

            g_hDupProgress = CreateWindowExW(0, PROGRESS_CLASSW, L"",
                WS_CHILD | PBS_SMOOTH, 0, 0, 700, 16, hWnd, (HMENU)IDC_DUP_PROGRESS, NULL, NULL);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupProgress);

            g_hDupStaticStatus = CreateWindowExW(0, L"STATIC", i18n_get(STR_DUP_STATUS_READY),
                WS_CHILD, 0, 0, 700, 20, hWnd, (HMENU)IDC_DUP_STATIC_STATUS, NULL, NULL);
            set_font(g_hDupStaticStatus, g_hFont);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupStaticStatus);

            g_hDupBtnAutoSel = CreateWindowExW(0, L"BUTTON", i18n_get(STR_DUP_BTN_AUTOSEL),
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 180, 28, hWnd, (HMENU)IDC_DUP_BTN_AUTOSEL, NULL, NULL);
            set_font(g_hDupBtnAutoSel, g_hFont);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupBtnAutoSel);

            g_hDupBtnSelAll = CreateWindowExW(0, L"BUTTON", i18n_get(STR_DUP_BTN_SEL_ALL),
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 100, 28, hWnd, (HMENU)IDC_DUP_BTN_SEL_ALL, NULL, NULL);
            set_font(g_hDupBtnSelAll, g_hFont);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupBtnSelAll);

            g_hDupBtnDeselAll = CreateWindowExW(0, L"BUTTON", i18n_get(STR_DUP_BTN_DESEL_ALL),
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 100, 28, hWnd, (HMENU)IDC_DUP_BTN_DESEL_ALL, NULL, NULL);
            set_font(g_hDupBtnDeselAll, g_hFont);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupBtnDeselAll);

            g_hDupBtnDelete = CreateWindowExW(0, L"BUTTON", i18n_get(STR_DUP_BTN_DELETE),
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 200, 28, hWnd, (HMENU)IDC_DUP_BTN_DELETE, NULL, NULL);
            set_font(g_hDupBtnDelete, g_hFontBold);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupBtnDelete);

            g_hDupBtnStop = CreateWindowExW(0, L"BUTTON", i18n_get(STR_DUP_BTN_STOP),
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 100, 28, hWnd, (HMENU)IDC_DUP_BTN_STOP, NULL, NULL);
            set_font(g_hDupBtnStop, g_hFont);
            EnableWindow(g_hDupBtnStop, FALSE);
            register_ctrl(g_hDupCtrls, &g_nDupCtrls, 32, g_hDupBtnStop);

            /* ================= PAGE 4: PASSWORD-PROTECTED MODEL TESTER ================= */
            g_hModelLockInfo = CreateWindowExW(0, L"STATIC", i18n_get(STR_MODEL_LOCKED),
                WS_CHILD | SS_CENTER, 0, 0, 500, 45, hWnd, (HMENU)IDC_MODEL_LOCK_INFO, NULL, NULL);
            set_font(g_hModelLockInfo, g_hFontTitle);
            register_ctrl(g_hModelLockCtrls, &g_nModelLockCtrls, 2, g_hModelLockInfo);

            g_hModelLockLogin = CreateWindowExW(0, L"BUTTON",
                i18n_get(STR_LOGIN_BTN_MASUK),
                WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 0, 0, 220, 36, hWnd, (HMENU)IDC_MODEL_LOCK_LOGIN, NULL, NULL);
            set_font(g_hModelLockLogin, g_hFontBold);
            register_ctrl(g_hModelLockCtrls, &g_nModelLockCtrls, 2, g_hModelLockLogin);

            g_hModelBtnTest = CreateWindowExW(0, L"BUTTON", i18n_get(STR_MODEL_BTN_TEST),
                WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 0, 0, 160, 30, hWnd, (HMENU)IDC_MODEL_BTN_TEST, NULL, NULL);
            set_font(g_hModelBtnTest, g_hFontBold);
            register_ctrl(g_hModelCtrls, &g_nModelCtrls, 10, g_hModelBtnTest);

            g_hModelBtnStop = CreateWindowExW(0, L"BUTTON", i18n_get(STR_MODEL_BTN_STOP),
                WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 100, 30, hWnd, (HMENU)IDC_MODEL_BTN_STOP, NULL, NULL);
            set_font(g_hModelBtnStop, g_hFont);
            EnableWindow(g_hModelBtnStop, FALSE);
            register_ctrl(g_hModelCtrls, &g_nModelCtrls, 10, g_hModelBtnStop);

            g_hModelMaxInputLabel = CreateWindowExW(0, L"STATIC", i18n_get(STR_MODEL_MAX_INPUT_PRICE),
                WS_CHILD, 0, 0, 90, 22, hWnd, (HMENU)IDC_MODEL_MAX_INPUT_LABEL, NULL, NULL);
            set_font(g_hModelMaxInputLabel, g_hFontBold);
            register_ctrl(g_hModelCtrls, &g_nModelCtrls, 10, g_hModelMaxInputLabel);

            g_hModelMaxInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 82, 24,
                hWnd, (HMENU)IDC_MODEL_MAX_INPUT, NULL, NULL);
            set_font(g_hModelMaxInput, g_hFont);
            register_ctrl(g_hModelCtrls, &g_nModelCtrls, 10, g_hModelMaxInput);

            g_hModelMaxOutputLabel = CreateWindowExW(0, L"STATIC", i18n_get(STR_MODEL_MAX_OUTPUT_PRICE),
                WS_CHILD, 0, 0, 100, 22, hWnd, (HMENU)IDC_MODEL_MAX_OUTPUT_LABEL, NULL, NULL);
            set_font(g_hModelMaxOutputLabel, g_hFontBold);
            register_ctrl(g_hModelCtrls, &g_nModelCtrls, 10, g_hModelMaxOutputLabel);

            g_hModelMaxOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 0, 82, 24,
                hWnd, (HMENU)IDC_MODEL_MAX_OUTPUT, NULL, NULL);
            set_font(g_hModelMaxOutput, g_hFont);
            register_ctrl(g_hModelCtrls, &g_nModelCtrls, 10, g_hModelMaxOutput);

            g_hModelStatus = CreateWindowExW(0, L"STATIC", i18n_get(STR_MODEL_STATUS_READY),
                WS_CHILD, 0, 0, 700, 22, hWnd, (HMENU)IDC_MODEL_STATUS, NULL, NULL);
            set_font(g_hModelStatus, g_hFont);
            register_ctrl(g_hModelCtrls, &g_nModelCtrls, 10, g_hModelStatus);

            g_hModelProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                WS_CHILD | PBS_SMOOTH, 0, 0, 700, 16, hWnd, (HMENU)IDC_MODEL_PROGRESS, NULL, NULL);
            register_ctrl(g_hModelCtrls, &g_nModelCtrls, 10, g_hModelProgress);

            g_hModelList = CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
                0, 0, 700, 400, hWnd, (HMENU)IDC_MODEL_LIST, NULL, NULL
            );
            set_font(g_hModelList, g_hFont);
            ListView_SetExtendedListViewStyle(g_hModelList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
            register_ctrl(g_hModelCtrls, &g_nModelCtrls, 10, g_hModelList);
            LVCOLUMNW mc = {0};
            mc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            mc.cx = 190; mc.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_NAME); ListView_InsertColumn(g_hModelList, 0, &mc);
            mc.cx = 120; mc.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_PROVIDER); ListView_InsertColumn(g_hModelList, 1, &mc);
            mc.cx = 100; mc.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_UPTIME); ListView_InsertColumn(g_hModelList, 2, &mc);
            mc.cx = 115; mc.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_OUTPUT); ListView_InsertColumn(g_hModelList, 3, &mc);
            mc.cx = 110; mc.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_INPUT); ListView_InsertColumn(g_hModelList, 4, &mc);
            mc.cx = 105; mc.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_LATENCY); ListView_InsertColumn(g_hModelList, 5, &mc);
            mc.cx = 130; mc.pszText = (LPWSTR)i18n_get(STR_MODEL_COL_RESULT); ListView_InsertColumn(g_hModelList, 6, &mc);
            /* Initialize localized texts & show initial page */
            update_ui_texts(hWnd);
            show_page(0);
            break;
        }

        case WM_SIZE: {
            int cx = LOWORD(lParam);
            int cy = HIWORD(lParam);
            layout_controls(hWnd, cx, cy);
            break;
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO *mmi = (MINMAXINFO *)lParam;
            mmi->ptMinTrackSize.x = 900;
            mmi->ptMinTrackSize.y = 650;
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            if (lpnm->hwndFrom == g_hTab && lpnm->code == TCN_SELCHANGE) {
                int cur_tab = TabCtrl_GetCurSel(g_hTab);
                show_page(cur_tab);
            } else if (lpnm->hwndFrom == g_hEncTree) {
                if (lpnm->code == TVN_ITEMEXPANDINGW) {
                    handle_tree_expanding(g_hEncTree, (NMTREEVIEWW *)lParam);
                } else if (lpnm->code == TVN_SELCHANGEDW) {
                    NMTREEVIEWW *pnmtv = (NMTREEVIEWW *)lParam;
                    wchar_t *path = (wchar_t *)pnmtv->itemNew.lParam;
                    if (path && wcslen(path) > 0) {
                        wcsncpy(g_szCurrentExplorerFolder, path, MAX_PATH * 2 - 1);
                        SetWindowTextW(g_hEditFolder, path);
                        save_settings(path);
                        populate_explorer_list(g_hEncExplorerList, path);
                    }
                }
            } else if (lpnm->hwndFrom == g_hEncExplorerList) {
                if (lpnm->code == LVN_COLUMNCLICK) {
                    NMLISTVIEW *pnmlv = (NMLISTVIEW *)lParam;
                    sort_explorer_list(g_hEncExplorerList, pnmlv->iSubItem);
                    return 0;
                } else if (lpnm->code == NM_DBLCLK) {
                    int sel = ListView_GetNextItem(g_hEncExplorerList, -1, LVNI_SELECTED);
                    if (sel >= 0) {
                        wchar_t name[MAX_PATH] = {0};
                        ListView_GetItemText(g_hEncExplorerList, sel, 0, name, MAX_PATH - 1);
                        wchar_t sz_type[64] = {0};
                        ListView_GetItemText(g_hEncExplorerList, sel, 2, sz_type, 63);
                        if (wcscmp(sz_type, L"<Folder>") == 0) {
                            wchar_t next_path[MAX_PATH * 2];
                            swprintf(next_path, MAX_PATH * 2, L"%ls%ls%ls",
                                g_szCurrentExplorerFolder,
                                (g_szCurrentExplorerFolder[wcslen(g_szCurrentExplorerFolder)-1] == L'\\') ? L"" : L"\\",
                                name);
                            wcsncpy(g_szCurrentExplorerFolder, next_path, MAX_PATH * 2 - 1);
                            SetWindowTextW(g_hEditFolder, next_path);
                            save_settings(next_path);
                            populate_explorer_list(g_hEncExplorerList, next_path);
                        } else {
                            wchar_t full_file[MAX_PATH * 2];
                            swprintf(full_file, MAX_PATH * 2, L"%ls%ls%ls",
                                g_szCurrentExplorerFolder,
                                (g_szCurrentExplorerFolder[wcslen(g_szCurrentExplorerFolder)-1] == L'\\') ? L"" : L"\\",
                                name);

                            const wchar_t *ext = wcsrchr(name, L'.');
                            if (ext && _wcsicmp(ext, ENC_FILE_EXTENSION) == 0) {
                                if (service_auto_decrypt_single_file(full_file, true)) {
                                    populate_explorer_list(g_hEncExplorerList, g_szCurrentExplorerFolder);
                                }
                            } else {
                                tracker_touch_file(full_file);
                                ShellExecuteW(NULL, L"open", full_file, NULL, NULL, SW_SHOWNORMAL);
                            }
                        }
                    }
                } else if (lpnm->code == NM_RCLICK) {
                    POINT pt;
                    GetCursorPos(&pt);
                    handle_explorer_context_menu(hWnd, pt.x, pt.y);
                    return 0;
                }
            } else if (lpnm->hwndFrom == g_hDupList) {
                if (lpnm->code == NM_DBLCLK) {
                    int sel_idx = ListView_GetNextItem(g_hDupList, -1, LVNI_SELECTED);
                    if (sel_idx >= 0) {
                        LVITEMW lvi = {0};
                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = sel_idx;
                        if (ListView_GetItem(g_hDupList, &lvi) && lvi.lParam) {
                            DuplicateItem *pIt = (DuplicateItem *)lvi.lParam;
                            if (pIt && pIt->path) {
                                ShellExecuteW(NULL, L"open", pIt->path, NULL, NULL, SW_SHOWNORMAL);
                            }
                        }
                    }
                    return 0;
                } else if (lpnm->code == NM_RCLICK) {
                    POINT pt;
                    GetCursorPos(&pt);
                    handle_duplicate_context_menu(hWnd, pt.x, pt.y);
                    return 0;
                } else if (lpnm->code == LVN_ITEMCHANGED) {
                    if (g_bUpdatingDupChecks) return 0;
                    NMLISTVIEW *nmlv = (NMLISTVIEW *)lParam;
                    if ((nmlv->uNewState & LVIS_STATEIMAGEMASK) != (nmlv->uOldState & LVIS_STATEIMAGEMASK)) {
                        int state = (nmlv->uNewState & LVIS_STATEIMAGEMASK) >> 12;
                        if (state == 1 || state == 2) {
                            bool is_checked = (state == 2);
                            LVITEMW lvi = {0};
                            lvi.mask = LVIF_PARAM;
                            lvi.iItem = nmlv->iItem;
                            if (ListView_GetItem(g_hDupList, &lvi) && lvi.lParam) {
                                DuplicateItem *pIt = (DuplicateItem *)lvi.lParam;
                                if (pIt) {
                                    pIt->checked = is_checked;
                                }
                            }
                        }
                    }
                    return 0;
                }
            } else if (lpnm->hwndFrom == g_hModelList) {
                if (lpnm->code == LVN_COLUMNCLICK && g_bAuthenticated) {
                    NMLISTVIEW *nmlv = (NMLISTVIEW *)lParam;
                    ModelSortColumn selected;
                    bool default_descending;
                    if (nmlv->iSubItem == 2) {
                        selected = MODEL_SORT_UPTIME;
                        default_descending = true;
                    } else if (nmlv->iSubItem == 3) {
                        selected = MODEL_SORT_OUTPUT_PRICE;
                        default_descending = false;
                    } else if (nmlv->iSubItem == 4) {
                        selected = MODEL_SORT_INPUT_PRICE;
                        default_descending = false;
                    } else {
                        break;
                    }
                    if (g_modelSortColumn == selected) {
                        g_modelSortDescending = !g_modelSortDescending;
                    } else {
                        g_modelSortColumn = selected;
                        g_modelSortDescending = default_descending;
                    }
                    refresh_model_listview();
                } else if (lpnm->code == NM_RCLICK) {
                    POINT pt;
                    GetCursorPos(&pt);
                    handle_model_context_menu(hWnd, pt.x, pt.y, true);
                    return 0;
                }
            }
            break;
        }

        case WM_CONTEXTMENU: {
            HWND hTarget = (HWND)wParam;
            if (hTarget == g_hEncExplorerList) {
                handle_explorer_context_menu(hWnd, (short)LOWORD(lParam), (short)HIWORD(lParam));
                return 0;
            } else if (hTarget == g_hDupList) {
                handle_duplicate_context_menu(hWnd, (short)LOWORD(lParam), (short)HIWORD(lParam));
                return 0;
            } else if (hTarget == g_hModelList &&
                       (short)LOWORD(lParam) == -1 && (short)HIWORD(lParam) == -1) {
                handle_model_context_menu(hWnd, -1, -1, false);
                return 0;
            }
            break;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            /* Menu Actions */
            if (wmId == IDM_FILE_OPEN) {
                int cur_tab = TabCtrl_GetCurSel(g_hTab);
                if (cur_tab == 0 && g_bAuthenticated) browse_folder_to_edit(hWnd, g_hEditFolder);
                else if (cur_tab == 1) browse_folder_to_edit(hWnd, g_hRenEditFolder);
                else if (cur_tab == 2) browse_folder_to_edit(hWnd, g_hDupEditFolder);
            } else if (wmId == IDM_FILE_EXIT || wmId == ID_MAINTRAY_EXIT) {
                app_exit_completely(hWnd);
                return 0;
            } else if (wmId == ID_MAINTRAY_RESTORE) {
                restore_main_window(hWnd);
                return 0;
            } else if (wmId == IDM_VIEW_ENCRYPT) {
                TabCtrl_SetCurSel(g_hTab, 0);
                show_page(0);
            } else if (wmId == IDM_VIEW_RENAME) {
                TabCtrl_SetCurSel(g_hTab, 1);
                show_page(1);
            } else if (wmId == IDM_VIEW_DUPLICATES) {
                TabCtrl_SetCurSel(g_hTab, 2);
                show_page(2);
            } else if (wmId == IDM_VIEW_MODEL_TESTER) {
                TabCtrl_SetCurSel(g_hTab, 3);
                show_page(3);
            } else if (wmId == IDM_LANG_EN) {
                if (i18n_get_language() != LANG_EN) {
                    i18n_set_language(LANG_EN);
                    save_settings(g_szLastFolder);
                    update_ui_texts(hWnd);
                }
            } else if (wmId == IDM_LANG_ID) {
                if (i18n_get_language() != LANG_ID) {
                    i18n_set_language(LANG_ID);
                    save_settings(g_szLastFolder);
                    update_ui_texts(hWnd);
                }
            } else if (wmId == IDM_SERVICE_TOGGLE) {
                if (service_is_running()) {
                    service_stop_process();
                } else {
                    service_start_process();
                }
                Sleep(200);
                update_ui_texts(hWnd);
            } else if (wmId == IDM_SERVICE_INSTALL) {
                service_install();
                update_ui_texts(hWnd);
                MessageBoxW(hWnd, i18n_get(STR_SERVICE_INTEGRATION_INSTALLED), L"Win10 File Encryptor", MB_OK | MB_ICONINFORMATION);
            } else if (wmId == IDM_SERVICE_UNINSTALL) {
                service_uninstall();
                update_ui_texts(hWnd);
                MessageBoxW(hWnd, i18n_get(STR_SERVICE_INTEGRATION_REMOVED), L"Win10 File Encryptor", MB_OK | MB_ICONINFORMATION);
            } else if (wmId == IDM_HELP_GUIDE) {
                MessageBoxW(hWnd, i18n_get(STR_GUIDE_MSG), i18n_get(STR_GUIDE_TITLE), MB_OK | MB_ICONINFORMATION);
            } else if (wmId == IDM_HELP_ABOUT) {
                MessageBoxW(hWnd, i18n_get(STR_ABOUT_MSG), i18n_get(STR_ABOUT_TITLE), MB_OK | MB_ICONINFORMATION);
            }

            /* Setup Onboarding Actions */
            else if (wmId == IDC_SETUP_BTN_SAVE && wmEvent == BN_CLICKED) {
                wchar_t p1[256] = {0};
                wchar_t p2[256] = {0};
                wchar_t email[256] = {0};

                GetWindowTextW(g_hSetupEditPass, p1, 255);
                GetWindowTextW(g_hSetupEditConfirm, p2, 255);
                GetWindowTextW(g_hSetupEditEmail, email, 255);

                if (wcslen(p1) < 4) {
                    MessageBoxW(hWnd, i18n_get(STR_SETUP_ERR_PASS_LEN), (i18n_get_language() == LANG_ID) ? L"Peringatan" : L"Warning", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                if (wcscmp(p1, p2) != 0) {
                    MessageBoxW(hWnd, i18n_get(STR_SETUP_ERR_PASS_MATCH), (i18n_get_language() == LANG_ID) ? L"Peringatan" : L"Warning", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                if (wcslen(email) < 5 || wcschr(email, L'@') == NULL || wcschr(email, L'.') == NULL) {
                    MessageBoxW(hWnd, i18n_get(STR_SETUP_ERR_EMAIL_INVALID),
                        (i18n_get_language() == LANG_ID) ? L"Validasi Email Wajib" : L"Email Validation", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                if (vault_setup(p1, email)) {
                    unlock_kiosapi_key(p1);
                    SecureZeroMemory(p1, sizeof(p1));
                    SecureZeroMemory(p2, sizeof(p2));
                    SecureZeroMemory(email, sizeof(email));
                    /* Automatically register explorer integration & start service */
                    service_install();
                    MessageBoxW(hWnd, i18n_get(STR_SETUP_SUCCESS),
                        (i18n_get_language() == LANG_ID) ? L"Sukses" : L"Success", MB_OK | MB_ICONINFORMATION);
                    g_bAuthenticated = true;
                    show_page(0);
                    update_ui_texts(hWnd);
                } else {
                    MessageBoxW(hWnd,
                        (i18n_get_language() == LANG_ID) ? L"Gagal menyimpan konfigurasi ke dalam vault terenkripsi!" : L"Failed to save configuration to encrypted vault!",
                        (i18n_get_language() == LANG_ID) ? L"Kesalahan" : L"Error", MB_OK | MB_ICONERROR);
                }
            }

            /* Login Actions */
            else if (wmId == IDC_LOGIN_CHK_SHOW && wmEvent == BN_CLICKED) {
                bool show = (SendMessage(g_hLoginChkShow, BM_GETCHECK, 0, 0) == BST_CHECKED);
                SendMessage(g_hLoginEditPass, EM_SETPASSWORDCHAR, show ? 0 : L'*', 0);
                InvalidateRect(g_hLoginEditPass, NULL, TRUE);
            } else if (wmId == IDC_LOGIN_BTN_MASUK && wmEvent == BN_CLICKED) {
                wchar_t pass[256] = {0};
                GetWindowTextW(g_hLoginEditPass, pass, 255);

                int rem = 0;
                AuthResult res = vault_authenticate(pass, &rem);
                if (res == AUTH_SUCCESS) unlock_kiosapi_key(pass);
                SecureZeroMemory(pass, sizeof(pass));

                if (res == AUTH_SUCCESS) {
                    g_bAuthenticated = true;
                    SetWindowTextW(g_hLoginEditPass, L"");
                    show_page(0);

                    if (g_bRememberFolder && g_szLastFolder[0] != L'\0') {
                        wcsncpy(g_szCurrentExplorerFolder, g_szLastFolder, MAX_PATH * 2 - 1);
                        populate_explorer_list(g_hEncExplorerList, g_szLastFolder);
                    }
                } else {
                    update_login_attempt_ui();
                    SetWindowTextW(g_hLoginEditPass, L"");
                    if (res == AUTH_LOCKED) {
                        MessageBoxW(hWnd, i18n_get(STR_LOGIN_LOCKED_MSG), i18n_get(STR_LOGIN_LOCKED_TITLE), MB_OK | MB_ICONERROR);
                    } else {
                        SetFocus(g_hLoginEditPass);
                    }
                }
            } else if (wmId == IDC_LOGIN_BTN_LUPA && wmEvent == BN_CLICKED) {
                show_forgot_password_dialog(hWnd);
            }

            /* Authenticated Explorer Actions */
            else if (wmId == IDC_ENC_FILE_TYPE && wmEvent == CBN_SELCHANGE) {
                update_file_filter_extensions(g_hEncFileType, g_hEncFileExtensions);
            } else if (wmId == IDC_REN_FILE_TYPE && wmEvent == CBN_SELCHANGE) {
                update_file_filter_extensions(g_hRenFileType, g_hRenFileExtensions);
                scan_renamer_folder(hWnd);
            } else if (wmId == IDC_DUP_FILE_TYPE && wmEvent == CBN_SELCHANGE) {
                update_file_filter_extensions(g_hDupFileType, g_hDupFileExtensions);
            } else if (wmId == IDC_REN_MAX_SIZE && wmEvent == EN_KILLFOCUS) {
                scan_renamer_folder(hWnd);
            }
            else if (wmId == IDC_BTN_BROWSE && wmEvent == BN_CLICKED) {
                browse_folder_to_edit(hWnd, g_hEditFolder);
            } else if (wmId == IDC_CHK_REMEMBER && wmEvent == BN_CLICKED) {
                g_bRememberFolder = (SendMessage(g_hChkRemember1, BM_GETCHECK, 0, 0) == BST_CHECKED);
                SendMessage(g_hRenChkRemember2, BM_SETCHECK, g_bRememberFolder ? BST_CHECKED : BST_UNCHECKED, 0);
                save_settings(g_szLastFolder);
            }

            /* Page 2 Renamer Actions */
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

            /* Page 3 Duplicate Finder Actions */
            else if (wmId == IDC_DUP_BTN_BROWSE && wmEvent == BN_CLICKED) {
                browse_folder_to_edit(hWnd, g_hDupEditFolder);
            } else if (wmId == IDC_DUP_BTN_SCAN && wmEvent == BN_CLICKED) {
                start_duplicate_scan(hWnd);
            } else if (wmId == IDC_DUP_BTN_STOP && wmEvent == BN_CLICKED) {
                g_bDupCancel = true;
                bool is_id = (i18n_get_language() == LANG_ID);
                SetWindowTextW(g_hDupStaticStatus, is_id ? L"Membatalkan pemindaian..." : L"Cancelling duplicate scan...");
            } else if (wmId == IDC_DUP_BTN_AUTOSEL && wmEvent == BN_CLICKED) {
                if (g_pDupResults) {
                    duplicate_auto_select_copies(g_pDupResults);
                    update_duplicate_list_checks();
                }
            } else if (wmId == IDC_DUP_BTN_SEL_ALL && wmEvent == BN_CLICKED) {
                if (g_pDupResults) {
                    for (DuplicateGroup *g = g_pDupResults->groups; g; g = g->next) {
                        for (DuplicateItem *it = g->items; it; it = it->next) {
                            it->checked = true;
                        }
                    }
                    update_duplicate_list_checks();
                }
            } else if (wmId == IDC_DUP_BTN_DESEL_ALL && wmEvent == BN_CLICKED) {
                if (g_pDupResults) {
                    for (DuplicateGroup *g = g_pDupResults->groups; g; g = g->next) {
                        for (DuplicateItem *it = g->items; it; it = it->next) {
                            it->checked = false;
                        }
                    }
                    update_duplicate_list_checks();
                }
            } else if (wmId == IDC_DUP_BTN_DELETE && wmEvent == BN_CLICKED) {
                delete_selected_duplicates(hWnd);
            } else if (wmId == IDC_MODEL_LOCK_LOGIN && wmEvent == BN_CLICKED) {
                TabCtrl_SetCurSel(g_hTab, 0);
                show_page(0);
            } else if ((wmId == IDC_MODEL_MAX_INPUT || wmId == IDC_MODEL_MAX_OUTPUT) &&
                       wmEvent == EN_CHANGE) {
                refresh_model_listview();
            } else if (wmId == IDC_MODEL_BTN_TEST && wmEvent == BN_CLICKED) {
                start_model_tests(hWnd, false);
            } else if (wmId == IDC_MODEL_BTN_STOP && wmEvent == BN_CLICKED) {
                g_bModelCancel = true;
                EnableWindow(g_hModelBtnStop, FALSE);
                SetWindowTextW(g_hModelStatus,
                    i18n_get_language() == LANG_ID ? L"Menyelesaikan permintaan saat ini lalu menghentikan pengujian..." :
                                                   L"Finishing the current request, then stopping...");
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

        case WM_APP_MODEL_PROGRESS: {
            ModelProgressPayload *payload = (ModelProgressPayload *)lParam;
            if (payload) {
                SendMessageW(g_hModelProgress, PBM_SETPOS, payload->completed, 0);
                wchar_t status[384];
                swprintf(
                    status,
                    384,
                    L"%ls (%d/%d): %ls",
                    payload->model_name,
                    payload->completed,
                    payload->total,
                    payload->result
                );
                SetWindowTextW(g_hModelStatus, status);
                free(payload);
            }
            break;
        }

        case WM_APP_MODEL_FINISHED: {
            ModelFinishedPayload *payload = (ModelFinishedPayload *)lParam;
            g_bModelTesting = false;
            if (g_hModelThread) {
                CloseHandle(g_hModelThread);
                g_hModelThread = NULL;
            }
            EnableWindow(g_hModelBtnStop, FALSE);
            EnableWindow(g_hModelMaxInput, TRUE);
            EnableWindow(g_hModelMaxOutput, TRUE);
            refresh_model_listview();

            wchar_t status[512];
            if (!payload) {
                wcscpy(status, L"Model test worker could not allocate a result.");
            } else if (!payload->success) {
                swprintf(status, 512, L"Model test stopped: %ls", payload->error);
            } else if (payload->cancelled) {
                swprintf(
                    status,
                    512,
                    i18n_get_language() == LANG_ID
                        ? L"Pengujian dibatalkan. Selesai: %d | Berhasil: %d | Gagal: %d"
                        : L"Test cancelled. Completed: %d | Passed: %d | Failed: %d",
                    payload->completed,
                    payload->succeeded,
                    payload->failed
                );
            } else {
                swprintf(
                    status,
                    512,
                    i18n_get_language() == LANG_ID
                        ? L"Pengujian selesai. Total: %d | Berhasil: %d | Gagal: %d"
                        : L"Test complete. Total: %d | Passed: %d | Failed: %d",
                    payload->completed,
                    payload->succeeded,
                    payload->failed
                );
            }
            SetWindowTextW(g_hModelStatus, status);
            EnableWindow(g_hModelBtnTest, g_bAuthenticated && g_bKiosapiKeyReady && g_modelVisibleCount > 0);
            free(payload);
            break;
        }

        case WM_APP_PROGRESS: {
            int cur = (int)wParam;
            int total = (int)lParam;
            if (total > 0) {
                int pct = (cur * 100) / total;
                SendMessage(g_hProgress, PBM_SETPOS, pct, 0);
                wchar_t status_txt[256];
                swprintf(status_txt, 256, L"Memproses enkripsi: %d / %d berkas (%d%%)...", cur, total, pct);
                SetWindowTextW(g_hStaticStatus, status_txt);
            }
            break;
        }

        case WM_APP_FINISHED: {
            g_bIsWorking = false;
            if (g_hWorkerThread) {
                CloseHandle(g_hWorkerThread);
                g_hWorkerThread = NULL;
            }

            size_t success_count = (size_t)wParam;
            size_t fail_count = (size_t)lParam;

            bool is_id = (i18n_get_language() == LANG_ID);
            wchar_t status_txt[256];
            if (is_id) {
                swprintf(status_txt, 256, L"Selesai! Berhasil: %zu | Gagal/Dilewati: %zu", success_count, fail_count);
            } else {
                swprintf(status_txt, 256, L"Completed! Succeeded: %zu | Failed/Skipped: %zu", success_count, fail_count);
            }
            SetWindowTextW(g_hStaticStatus, status_txt);

            if (g_szCurrentExplorerFolder[0] != L'\0') {
                populate_explorer_list(g_hEncExplorerList, g_szCurrentExplorerFolder);
            }

            wchar_t done_msg[512];
            if (is_id) {
                swprintf(done_msg, 512, L"Proses enkripsi selesai.\n\nBerhasil dienkripsi & dimusnahkan: %zu berkas\nGagal / Dilewati: %zu berkas", success_count, fail_count);
            } else {
                swprintf(done_msg, 512, L"Encryption process completed.\n\nSuccessfully encrypted & shredded: %zu files\nFailed / Skipped: %zu files", success_count, fail_count);
            }
            if (g_bMainTrayActive) {
                NOTIFYICONDATAW nid = {0};
                nid.cbSize = sizeof(NOTIFYICONDATAW);
                nid.hWnd = hWnd;
                nid.uID = 100;
                nid.uFlags = NIF_INFO;
                nid.dwInfoFlags = NIIF_INFO;
                wcsncpy(nid.szInfoTitle, i18n_get(STR_APP_TITLE), 63);
                wcsncpy(nid.szInfo, status_txt, 255);
                Shell_NotifyIconW(NIM_MODIFY, &nid);
            } else {
                MessageBoxW(hWnd, done_msg, is_id ? L"Informasi" : L"Information", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }

        case WM_APP_REN_FINISHED: {
            g_bIsWorking = false;
            if (g_hWorkerThread) {
                CloseHandle(g_hWorkerThread);
                g_hWorkerThread = NULL;
            }
            EnableWindow(g_hRenBtnStart, TRUE);
            EnableWindow(g_hRenBtnUndo, TRUE);
            populate_renamer_listview();
            bool is_id = (i18n_get_language() == LANG_ID);
            SetWindowTextW(g_hRenStaticStatus, is_id ? L"Batch Rename Selesai!" : L"Batch Rename Complete!");

            if (g_bMainTrayActive) {
                NOTIFYICONDATAW nid = {0};
                nid.cbSize = sizeof(NOTIFYICONDATAW);
                nid.hWnd = hWnd;
                nid.uID = 100;
                nid.uFlags = NIF_INFO;
                nid.dwInfoFlags = NIIF_INFO;
                wcsncpy(nid.szInfoTitle, i18n_get(STR_APP_TITLE), 63);
                wcsncpy(nid.szInfo, is_id ? L"Proses Batch Rename berhasil selesai!" : L"Batch rename operation completed successfully!", 255);
                Shell_NotifyIconW(NIM_MODIFY, &nid);
            } else {
                MessageBoxW(hWnd,
                    is_id ? L"Proses Batch Rename berhasil selesai!" : L"Batch rename operation completed successfully!",
                    is_id ? L"Informasi" : L"Information", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }

        case WM_APP_DUP_PROGRESS: {
            DupProgressPayload *payload = (DupProgressPayload *)lParam;
            if (payload) {
                if (payload->phase == 1) {
                    wchar_t msg[256];
                    swprintf(msg, 256, i18n_get(STR_DUP_STATUS_SCANNING), payload->current);
                    SetWindowTextW(g_hDupStaticStatus, msg);
                } else if (payload->phase == 2) {
                    if (payload->total > 0) {
                        int pct = (int)((payload->current * 100) / payload->total);
                        SendMessage(g_hDupProgress, PBM_SETPOS, pct, 0);
                    }
                    wchar_t msg[256];
                    swprintf(msg, 256, i18n_get(STR_DUP_STATUS_HASHING), payload->current, payload->total);
                    SetWindowTextW(g_hDupStaticStatus, msg);
                }
                free(payload);
            }
            return 0;
        }

        case WM_APP_DUP_FINISHED: {
            g_bIsWorking = false;
            if (g_hDupWorkerThread) {
                CloseHandle(g_hDupWorkerThread);
                g_hDupWorkerThread = NULL;
            }

            EnableWindow(g_hDupBtnScan, TRUE);
            EnableWindow(g_hDupBtnAutoSel, TRUE);
            EnableWindow(g_hDupBtnDelete, TRUE);
            EnableWindow(g_hDupBtnStop, FALSE);

            SendMessage(g_hDupProgress, PBM_SETPOS, 100, 0);

            g_pDupResults = (DuplicateResults *)lParam;
            if (g_pDupResults) {
                duplicate_auto_select_copies(g_pDupResults);
                populate_duplicate_listview();

                if (g_pDupResults->group_count > 0) {
                    wchar_t size_str[64];
                    format_file_size_w(g_pDupResults->wasted_bytes, size_str, 64);
                    wchar_t status_msg[512];
                    swprintf(status_msg, 512, i18n_get(STR_DUP_STATUS_FOUND),
                        g_pDupResults->total_duplicates, g_pDupResults->group_count, size_str);
                    SetWindowTextW(g_hDupStaticStatus, status_msg);

                    if (g_bMainTrayActive) {
                        NOTIFYICONDATAW nid = {0};
                        nid.cbSize = sizeof(NOTIFYICONDATAW);
                        nid.hWnd = hWnd;
                        nid.uID = 100;
                        nid.uFlags = NIF_INFO;
                        nid.dwInfoFlags = NIIF_INFO;
                        wcsncpy(nid.szInfoTitle, i18n_get(STR_APP_TITLE), 63);
                        wcsncpy(nid.szInfo, status_msg, 255);
                        Shell_NotifyIconW(NIM_MODIFY, &nid);
                    }
                } else {
                    SetWindowTextW(g_hDupStaticStatus, i18n_get(STR_DUP_STATUS_NONE));
                    if (g_bMainTrayActive) {
                        NOTIFYICONDATAW nid = {0};
                        nid.cbSize = sizeof(NOTIFYICONDATAW);
                        nid.hWnd = hWnd;
                        nid.uID = 100;
                        nid.uFlags = NIF_INFO;
                        nid.dwInfoFlags = NIIF_INFO;
                        wcsncpy(nid.szInfoTitle, i18n_get(STR_APP_TITLE), 63);
                        wcsncpy(nid.szInfo, i18n_get(STR_DUP_STATUS_NONE), 255);
                        Shell_NotifyIconW(NIM_MODIFY, &nid);
                    }
                }
            }
            return 0;
        }

        case WM_TIMER: {
            if (wParam == TIMER_AUTO_ENCRYPT_ID) {
                int encCount = tracker_check_inactivity();
                if (encCount > 0) {
                    if (g_szCurrentExplorerFolder[0] != L'\0') {
                        populate_explorer_list(g_hEncExplorerList, g_szCurrentExplorerFolder);
                    }
                    bool is_id = (i18n_get_language() == LANG_ID);
                    wchar_t bmsg[256];
                    if (is_id) {
                        swprintf(bmsg, 256, L"%d berkas otomatis di-enkripsi kembali setelah 30 menit tidak aktif.", encCount);
                    } else {
                        swprintf(bmsg, 256, L"%d file(s) automatically re-encrypted after 30 minutes of inactivity.", encCount);
                    }
                    show_main_tray_icon(hWnd, false);
                    NOTIFYICONDATAW nid = {0};
                    nid.cbSize = sizeof(NOTIFYICONDATAW);
                    nid.hWnd = hWnd;
                    nid.uID = 100;
                    nid.uFlags = NIF_INFO;
                    nid.dwInfoFlags = NIIF_INFO;
                    wcsncpy(nid.szInfoTitle, i18n_get(STR_APP_TITLE), 63);
                    wcsncpy(nid.szInfo, bmsg, 255);
                    Shell_NotifyIconW(NIM_MODIFY, &nid);
                }
                return 0;
            }
            break;
        }

        case WM_APP_MAINTRAY: {
            if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP) {
                restore_main_window(hWnd);
            } else if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, ID_MAINTRAY_RESTORE, i18n_get(STR_TRAY_MENU_RESTORE));
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, ID_MAINTRAY_EXIT, i18n_get(STR_TRAY_MENU_EXIT));

                SetForegroundWindow(hWnd);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
                DestroyMenu(hMenu);
            }
            return 0;
        }

        case WM_CLOSE: {
            if (g_bRememberFolder) {
                wchar_t current_folder[MAX_PATH * 2] = {0};
                GetWindowTextW(g_hEditFolder, current_folder, MAX_PATH * 2);
                if (wcslen(current_folder) == 0 && g_hRenEditFolder) {
                    GetWindowTextW(g_hRenEditFolder, current_folder, MAX_PATH * 2);
                }
                if (wcslen(current_folder) == 0 && g_hDupEditFolder) {
                    GetWindowTextW(g_hDupEditFolder, current_folder, MAX_PATH * 2);
                }
                wchar_t clean[MAX_PATH * 2] = {0};
                sanitize_path(current_folder, clean, MAX_PATH * 2);
                if (clean[0] != L'\0') {
                    save_settings(clean);
                }
            }

            /* Hide window and show icon in taskbar / system tray.
             * All ongoing processes across all pages continue in the background.
             * Show balloon notification only once upon first close after start. */
            ShowWindow(hWnd, SW_HIDE);
            static bool s_bCloseBalloonShown = false;
            bool showBalloon = !s_bCloseBalloonShown;
            show_main_tray_icon(hWnd, showBalloon);
            s_bCloseBalloonShown = true;
            return 0;
        }

        case WM_DESTROY: {
            KillTimer(hWnd, TIMER_AUTO_ENCRYPT_ID);
            remove_main_tray_icon();
            g_bModelCancel = true;
            clear_kiosapi_key();
            clear_explorer_list_items(g_hEncExplorerList);
            tracker_encrypt_all_decrypted_files();
            if (g_pDupResults) {
                duplicate_free_results(g_pDupResults);
                g_pDupResults = NULL;
            }
            if (g_hDupWorkerThread) {
                CloseHandle(g_hDupWorkerThread);
                g_hDupWorkerThread = NULL;
            }
            if (g_hFont) DeleteObject(g_hFont);
            if (g_hFontBold) DeleteObject(g_hFontBold);
            if (g_hFontTitle) DeleteObject(g_hFontTitle);
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
