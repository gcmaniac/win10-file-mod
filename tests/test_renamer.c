#include "renamer.h"
#include <assert.h>
#include <stdio.h>

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

    renamer_free_list(list);
    wprintf(L"\n>>> ALL RENAMER TESTS PASSED PERFECTLY! <<<\n");
    return 0;
}
