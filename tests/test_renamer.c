#include "renamer.h"
#include <assert.h>
#include <stdio.h>
#include <windows.h>

static void create_test_file(const wchar_t *path, DWORD size) {
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    char data[8] = "test";
    DWORD written = 0;
    assert(WriteFile(file, data, size, &written, NULL));
    assert(written == size);
    CloseHandle(file);
}

int wmain(void) {
    wprintf(L"=====================================================\n");
    wprintf(L"  Running Win10 Bulk Renamer Automated Test Suite     \n");
    wprintf(L"=====================================================\n");

    RenameItemList *list = renamer_create_list();
    assert(list != NULL);

    renamer_add_item(list, L"C:\\Games\\breath-of-fire-aura-cave.webp", 100);
    renamer_add_item(list, L"C:\\Games\\breath-of-fire-camlon.webp", 200);
    renamer_add_item(list, L"C:\\Games\\breath-of-fire-drogen.webp", 300);
    assert(list->count == 3);
    wprintf(L"[PASS] Items added to renamer list.\n");

    /* 1. Test Replace Method: replace "breath-of-fire-" with "bof1_" */
    RenameConfig cfg;
    SecureZeroMemory(&cfg, sizeof(cfg));
    cfg.method = RENAME_METHOD_REPLACE;
    wcscpy(cfg.find_text, L"breath-of-fire-");
    wcscpy(cfg.replace_text, L"bof1_");
    renamer_compute_preview(list, &cfg);

    assert(wcscmp(list->items[0].new_name, L"bof1_aura-cave.webp") == 0);
    assert(wcscmp(list->items[1].new_name, L"bof1_camlon.webp") == 0);
    assert(wcscmp(list->items[2].new_name, L"bof1_drogen.webp") == 0);
    wprintf(L"[PASS] Method Replace: 'breath-of-fire-' -> 'bof1_'.\n");

    /* 2. Test Add Method: Prefix "SNES_" */
    SecureZeroMemory(&cfg, sizeof(cfg));
    cfg.method = RENAME_METHOD_ADD;
    cfg.add_pos = ADD_POS_PREFIX;
    wcscpy(cfg.add_text, L"SNES_");
    renamer_compute_preview(list, &cfg);

    assert(wcscmp(list->items[0].new_name, L"SNES_breath-of-fire-aura-cave.webp") == 0);
    wprintf(L"[PASS] Method Add Prefix: 'SNES_'.\n");

    /* 3. Test Pattern with sequential zero-padded tag: "Map_<inc:01>" */
    SecureZeroMemory(&cfg, sizeof(cfg));
    cfg.method = RENAME_METHOD_PATTERN;
    wcscpy(cfg.pattern, L"Map_<inc:01>");
    cfg.start_number = 1;
    cfg.number_step = 1;
    renamer_compute_preview(list, &cfg);

    assert(wcscmp(list->items[0].new_name, L"Map_01.webp") == 0);
    assert(wcscmp(list->items[1].new_name, L"Map_02.webp") == 0);
    assert(wcscmp(list->items[2].new_name, L"Map_03.webp") == 0);
    wprintf(L"[PASS] Method Pattern: 'Map_<inc:01>' -> 'Map_01.webp', 'Map_02.webp', 'Map_03.webp'.\n");

    /* 4. Test Case change: Title Case */
    SecureZeroMemory(&cfg, sizeof(cfg));
    cfg.method = RENAME_METHOD_CASE;
    cfg.case_type = CASE_TITLECASE;
    renamer_compute_preview(list, &cfg);
    assert(wcscmp(list->items[1].new_name, L"Breath-Of-Fire-Camlon.webp") == 0);
    wprintf(L"[PASS] Method Case: Title Case conversion.\n");

    /* 5. Test Collision detection */
    SecureZeroMemory(&cfg, sizeof(cfg));
    cfg.method = RENAME_METHOD_PATTERN;
    wcscpy(cfg.pattern, L"StaticName"); // All 3 will get StaticName.webp
    renamer_compute_preview(list, &cfg);
    assert(list->items[0].has_collision == true);
    assert(list->items[1].has_collision == true);
    assert(list->items[2].has_collision == true);
    wprintf(L"[PASS] Collision detection successfully flagged duplicate prospective names.\n");

    wchar_t temp_path[MAX_PATH];
    wchar_t temp_dir[MAX_PATH];
    assert(GetTempPathW(MAX_PATH, temp_path) > 0);
    assert(GetTempFileNameW(temp_path, L"wfm", 0, temp_dir) != 0);
    assert(DeleteFileW(temp_dir));
    assert(CreateDirectoryW(temp_dir, NULL));
    wchar_t document_path[MAX_PATH];
    wchar_t image_path[MAX_PATH];
    swprintf(document_path, MAX_PATH, L"%ls\\document.txt", temp_dir);
    swprintf(image_path, MAX_PATH, L"%ls\\image.jpg", temp_dir);
    create_test_file(document_path, 4);
    create_test_file(image_path, 5);
    assert(renamer_scan_folder_filtered(list, temp_dir, false, SCANNER_FILE_TYPE_DOCUMENTS, 4));
    assert(list->count == 1);
    assert(wcscmp(list->items[0].orig_name, L"document.txt") == 0);
    assert(list->items[0].file_size == 4);
    DeleteFileW(document_path);
    DeleteFileW(image_path);
    RemoveDirectoryW(temp_dir);
    wprintf(L"[PASS] Folder scan respects file-type and maximum-size filters.\n");

    renamer_free_list(list);
    wprintf(L"\n>>> ALL RENAMER TESTS PASSED PERFECTLY! <<<\n");
    return 0;
}
