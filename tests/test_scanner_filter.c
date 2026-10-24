#include "scanner.h"
#include <assert.h>

static void write_test_file(const wchar_t *path, size_t size) {
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    uint8_t data[32];
    memset(data, 'x', sizeof(data));
    DWORD written = 0;
    assert(size <= sizeof(data));
    assert(WriteFile(file, data, (DWORD)size, &written, NULL));
    assert(written == size);
    CloseHandle(file);
}

static FileList *create_test_list(const wchar_t *const *paths, size_t count) {
    FileList *list = scanner_create();
    assert(list != NULL);
    for (size_t i = 0; i < count; i++) {
        assert(scanner_add_file(list, paths[i], 0));
    }
    return list;
}

int wmain(void) {
    wchar_t temp_path[MAX_PATH];
    wchar_t temp_dir[MAX_PATH];
    assert(GetTempPathW(MAX_PATH, temp_path) > 0);
    assert(GetTempFileNameW(temp_path, L"wfm", 0, temp_dir) != 0);
    assert(DeleteFileW(temp_dir));
    assert(CreateDirectoryW(temp_dir, NULL));

    wchar_t paths[6][MAX_PATH];
    const wchar_t *names[] = {
        L"document.PDF", L"notes.docx", L"photo.jpg",
        L"movie.mp4", L"archive.zip", L"extensionless"
    };
    const wchar_t *file_paths[6];
    const size_t sizes[] = {4, 8, 6, 9, 7, 3};
    for (size_t i = 0; i < 6; i++) {
        swprintf(paths[i], MAX_PATH, L"%ls\\%ls", temp_dir, names[i]);
        file_paths[i] = paths[i];
        write_test_file(paths[i], sizes[i]);
    }

    FileList *list = create_test_list(file_paths, 6);
    assert(scanner_filter_files(list, SCANNER_FILE_TYPE_DOCUMENTS, 6) == 5);
    assert(list->count == 1 && wcscmp(list->paths[0], paths[0]) == 0);
    assert(list->total_bytes == 4);
    scanner_free(list);

    list = create_test_list(file_paths, 6);
    assert(scanner_filter_files(list, SCANNER_FILE_TYPE_IMAGES, 6) == 5);
    assert(list->count == 1 && wcscmp(list->paths[0], paths[2]) == 0);
    assert(list->total_bytes == 6);
    scanner_free(list);

    list = create_test_list(file_paths, 6);
    assert(scanner_filter_files(list, SCANNER_FILE_TYPE_VIDEOS, 8) == 6);
    assert(list->count == 0 && list->total_bytes == 0);
    scanner_free(list);

    list = create_test_list(file_paths, 6);
    assert(scanner_filter_files(list, SCANNER_FILE_TYPE_ALL, 7) == 2);
    assert(list->count == 4 && list->total_bytes == 20);
    scanner_free(list);

    list = create_test_list(file_paths, 6);
    assert(scanner_filter_files(list, SCANNER_FILE_TYPE_ALL, 0) == 0);
    assert(list->count == 6 && list->total_bytes == 37);
    scanner_free(list);

    for (size_t i = 0; i < 6; i++) assert(DeleteFileW(paths[i]));
    assert(RemoveDirectoryW(temp_dir));
    wprintf(L"Scanner file-type and maximum-size filter tests passed.\n");
    return 0;
}
