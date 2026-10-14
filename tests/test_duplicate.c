#include "duplicate.h"
#include <assert.h>
#include <stdio.h>
#include <windows.h>

static void create_test_file(const wchar_t *path, const char *content) {
    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hFile != INVALID_HANDLE_VALUE);
    DWORD written = 0;
    WriteFile(hFile, content, (DWORD)strlen(content), &written, NULL);
    CloseHandle(hFile);
}

int wmain(void) {
    wprintf(L"=====================================================\n");
    wprintf(L"  Running Duplicate File Finder Automated Test Suite  \n");
    wprintf(L"=====================================================\n");

    /* 1. Test size formatting */
    wchar_t size_buf[64];
    format_file_size_w(512, size_buf, 64);
    assert(wcscmp(size_buf, L"512 B") == 0);

    format_file_size_w(2048, size_buf, 64);
    assert(wcscmp(size_buf, L"2.00 KB") == 0);

    format_file_size_w(1024 * 1024 * 5, size_buf, 64);
    assert(wcscmp(size_buf, L"5.00 MB") == 0);
    wprintf(L"[PASS] format_file_size_w works accurately.\n");

    /* 2. Setup temporary test directory structure */
    wchar_t test_dir[MAX_PATH] = L"test_dup_dir";
    wchar_t test_sub[MAX_PATH] = L"test_dup_dir\\sub";
    CreateDirectoryW(test_dir, NULL);
    CreateDirectoryW(test_sub, NULL);

    wchar_t f1[MAX_PATH] = L"test_dup_dir\\original.txt";
    wchar_t f2[MAX_PATH] = L"test_dup_dir\\copy1.txt";
    wchar_t f3[MAX_PATH] = L"test_dup_dir\\sub\\copy2.txt";
    wchar_t f4[MAX_PATH] = L"test_dup_dir\\same_size_diff_content.txt";
    wchar_t f5[MAX_PATH] = L"test_dup_dir\\different_size.txt";

    /* Content of 15 bytes */
    create_test_file(f1, "1234567890ABCDE");
    create_test_file(f2, "1234567890ABCDE"); /* Duplicate of f1 */
    create_test_file(f3, "1234567890ABCDE"); /* Duplicate of f1 in subfolder */
    create_test_file(f4, "ABCDE1234567890"); /* Same length (15 bytes), different content */
    create_test_file(f5, "Unique longer text content here!");

    /* 3. Run duplicate_find with recursive = true */
    volatile bool cancel = false;
    DuplicateResults *res = duplicate_find(test_dir, true, NULL, NULL, &cancel);
    assert(res != NULL);
    assert(res->total_scanned_files == 5);
    wprintf(L"[PASS] Scanned 5 files in test directory tree.\n");

    /* Exactly 1 duplicate group should be found */
    assert(res->group_count == 1);
    assert(res->total_duplicates == 2);
    assert(res->wasted_bytes == 15 * 2);
    assert(res->groups != NULL);
    assert(res->groups->count == 3);
    assert(res->groups->file_size == 15);
    wprintf(L"[PASS] Duplicate grouping detected exactly 1 group of 3 matching files (2 wasted copies).\n");

    /* 4. Test auto-select copies */
    duplicate_auto_select_copies(res);
    int original_count = 0;
    int checked_count = 0;
    for (DuplicateItem *it = res->groups->items; it; it = it->next) {
        if (it->is_original) {
            original_count++;
            assert(it->checked == false); /* Original must not be checked */
        } else {
            assert(it->checked == true);  /* Duplicate copy must be checked */
            checked_count++;
        }
    }
    assert(original_count == 1);
    assert(checked_count == 2);
    wprintf(L"[PASS] duplicate_auto_select_copies preserved the original and checked both duplicate copies.\n");

    /* 5. Clean up */
    duplicate_free_results(res);
    DeleteFileW(f1);
    DeleteFileW(f2);
    DeleteFileW(f3);
    DeleteFileW(f4);
    DeleteFileW(f5);
    RemoveDirectoryW(test_sub);
    RemoveDirectoryW(test_dir);

    wprintf(L"\n>>> ALL DUPLICATE FINDER TESTS PASSED PERFECTLY! <<<\n");
    return 0;
}
