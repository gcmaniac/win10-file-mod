#include "scanner.h"

FileList* scanner_create(void) {
    FileList *list = (FileList *)malloc(sizeof(FileList));
    if (!list) return NULL;
    list->capacity = 64;
    list->count = 0;
    list->total_bytes = 0;
    list->paths = (wchar_t **)malloc(sizeof(wchar_t *) * list->capacity);
    if (!list->paths) {
        free(list);
        return NULL;
    }
    return list;
}

void scanner_free(FileList *list) {
    if (!list) return;
    if (list->paths) {
        for (size_t i = 0; i < list->count; i++) {
            free(list->paths[i]);
        }
        free(list->paths);
    }
    free(list);
}

static bool scanner_add_file(FileList *list, const wchar_t *path, uint64_t size) {
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity * 2;
        wchar_t **new_paths = (wchar_t **)realloc(list->paths, sizeof(wchar_t *) * new_cap);
        if (!new_paths) return false;
        list->paths = new_paths;
        list->capacity = new_cap;
    }

    list->paths[list->count] = _wcsdup(path);
    if (!list->paths[list->count]) return false;
    list->count++;
    list->total_bytes += size;
    return true;
}

static bool ends_with(const wchar_t *str, const wchar_t *suffix) {
    size_t str_len = wcslen(str);
    size_t suffix_len = wcslen(suffix);
    if (str_len < suffix_len) return false;
    return (_wcsicmp(str + str_len - suffix_len, suffix) == 0);
}

static void scan_recursive_internal(
    const wchar_t *dir_path,
    bool recursive,
    OperationMode mode,
    FileList *list
) {
    if (!dir_path) return;

    /* Normalize dir_path: trim quotes and trailing slashes */
    wchar_t clean_dir[MAX_PATH * 2];
    const wchar_t *start = dir_path;
    while (*start == L' ' || *start == L'\t' || *start == L'\"') start++;
    wcsncpy(clean_dir, start, MAX_PATH * 2 - 1);
    clean_dir[MAX_PATH * 2 - 1] = L'\0';

    size_t dlen = wcslen(clean_dir);
    while (dlen > 0 && (clean_dir[dlen - 1] == L' ' || clean_dir[dlen - 1] == L'\t' || clean_dir[dlen - 1] == L'\"')) {
        clean_dir[--dlen] = L'\0';
    }
    if (dlen > 3 && (clean_dir[dlen - 1] == L'\\' || clean_dir[dlen - 1] == L'/')) {
        clean_dir[--dlen] = L'\0';
    }

    wchar_t search_pattern[MAX_PATH * 2];
    swprintf(search_pattern, MAX_PATH * 2, L"%ls\\*", clean_dir);

    WIN32_FIND_DATAW find_data;
    HANDLE hFind = FindFirstFileW(search_pattern, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0) {
            continue;
        }

        wchar_t full_path[MAX_PATH * 2];
        swprintf(full_path, MAX_PATH * 2, L"%ls\\%ls", clean_dir, find_data.cFileName);

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                scan_recursive_internal(full_path, recursive, mode, list);
            }
        } else {
            /* Skip temporary files */
            if (ends_with(find_data.cFileName, TMP_FILE_EXTENSION)) {
                continue;
            }

            /* Skip common Windows system files */
            if (_wcsicmp(find_data.cFileName, L"desktop.ini") == 0 ||
                _wcsicmp(find_data.cFileName, L"thumbs.db") == 0) {
                continue;
            }

            ULARGE_INTEGER fSize;
            fSize.LowPart = find_data.nFileSizeLow;
            fSize.HighPart = find_data.nFileSizeHigh;

            if (mode == OP_MODE_ENCRYPT_AND_SHRED) {
                /* When encrypting, only include files that are NOT already .enc */
                if (!ends_with(find_data.cFileName, ENC_FILE_EXTENSION)) {
                    scanner_add_file(list, full_path, fSize.QuadPart);
                }
            } else if (mode == OP_MODE_DECRYPT) {
                /* When decrypting, only include files that ARE .enc */
                if (ends_with(find_data.cFileName, ENC_FILE_EXTENSION)) {
                    scanner_add_file(list, full_path, fSize.QuadPart);
                }
            }
        }
    } while (FindNextFileW(hFind, &find_data));

    FindClose(hFind);
}

bool scanner_scan(
    const wchar_t *root_dir,
    bool recursive,
    OperationMode mode,
    FileList *out_list
) {
    if (!root_dir || !out_list) return false;
    scan_recursive_internal(root_dir, recursive, mode, out_list);
    return true;
}
