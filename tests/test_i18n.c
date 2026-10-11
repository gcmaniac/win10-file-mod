#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include "i18n.h"

int wmain(void) {
    wprintf(L"=====================================================\n");
    wprintf(L"  Running Win10 Multi-Language (i18n) Test Suite     \n");
    wprintf(L"=====================================================\n");

    /* 1. Test Default Language is English */
    i18n_init(LANG_EN);
    assert(i18n_get_language() == LANG_EN);
    wprintf(L"[PASS] Default language initialized to English (LANG_EN).\n");

    /* 2. Verify Key English Strings */
    const wchar_t *title_en = i18n_get(STR_APP_TITLE);
    assert(wcslen(title_en) > 0);
    assert(wcsstr(title_en, L"Encryptor") != NULL);

    const wchar_t *login_en = i18n_get(STR_LOGIN_BTN_MASUK);
    assert(wcscmp(login_en, L"Log In") == 0);

    const wchar_t *rename_en = i18n_get(STR_TAB_RENAME);
    assert(wcsstr(rename_en, L"Renamer") != NULL);
    wprintf(L"[PASS] English strings verified (Title, Log In, Renamer).\n");

    /* 3. Switch to Bahasa Indonesia */
    i18n_set_language(LANG_ID);
    assert(i18n_get_language() == LANG_ID);
    wprintf(L"[PASS] Language switched to Bahasa Indonesia (LANG_ID).\n");

    /* 4. Verify Key Indonesian Strings */
    const wchar_t *title_id = i18n_get(STR_APP_TITLE);
    assert(wcslen(title_id) > 0);
    assert(wcsstr(title_id, L"Enkripsi") != NULL);

    const wchar_t *login_id = i18n_get(STR_LOGIN_BTN_MASUK);
    assert(wcscmp(login_id, L"Masuk") == 0);

    const wchar_t *menu_file_id = i18n_get(STR_MENU_FILE);
    assert(wcscmp(menu_file_id, L"&Berkas") == 0);
    wprintf(L"[PASS] Indonesian strings verified (Enkripsi, Masuk, &Berkas).\n");

    /* 5. Switch back to English */
    i18n_set_language(LANG_EN);
    assert(i18n_get_language() == LANG_EN);
    assert(wcscmp(i18n_get(STR_LOGIN_BTN_MASUK), L"Log In") == 0);
    assert(wcscmp(i18n_get(STR_MENU_FILE), L"&File") == 0);
    wprintf(L"[PASS] Language switched back to English and confirmed.\n");

    /* 6. Verify Complete Coverage of String IDs */
    for (int i = 0; i < STR_COUNT; i++) {
        i18n_set_language(LANG_EN);
        const wchar_t *s_en = i18n_get((StringId)i);
        assert(s_en != NULL && wcslen(s_en) > 0);

        i18n_set_language(LANG_ID);
        const wchar_t *s_id = i18n_get((StringId)i);
        assert(s_id != NULL && wcslen(s_id) > 0);
    }
    wprintf(L"[PASS] Complete 100%% translation coverage across all %d String IDs.\n", STR_COUNT);

    wprintf(L"\n>>> ALL I18N TESTS PASSED PERFECTLY! <<<\n\n");
    return 0;
}
