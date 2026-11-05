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

bool scanner_add_file(FileList *list, const wchar_t *path, uint64_t size) {
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

static bool is_file_already_encrypted(const wchar_t *file_name, const wchar_t *full_path) {
    if (!file_name || !full_path) return false;
    if (ends_with(file_name, ENC_FILE_EXTENSION) || ends_with(file_name, TMP_FILE_EXTENSION)) {
        return true;
    }
    HANDLE hFile = CreateFileW(full_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    FileHeader header;
    DWORD bytesRead = 0;
    BOOL readOk = ReadFile(hFile, &header, sizeof(header), &bytesRead, NULL);
    CloseHandle(hFile);

    if (readOk && bytesRead >= sizeof(FileHeader)) {
        if (memcmp(header.magic, HEADER_MAGIC, 4) == 0 && header.version == HEADER_VERSION) {
            return true;
        }
    }
    return false;
}

static bool extension_in_list(const wchar_t *extension, const wchar_t *const *extensions, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (_wcsicmp(extension, extensions[i]) == 0) return true;
    }
    return false;
}

static bool matches_file_type(const wchar_t *path, ScannerFileType file_type) {
    static const wchar_t *const document_extensions[] = {
        L".pdf", L".doc", L".docx", L".odt", L".rtf", L".txt",
        L".xls", L".xlsx", L".ods", L".ppt", L".pptx", L".odp", L".csv"
    };
    static const wchar_t *const image_extensions[] = {
        L".jpg", L".jpeg", L".png", L".gif", L".bmp", L".tif",
        L".tiff", L".webp", L".svg", L".ico", L".heic", L".heif"
    };
    static const wchar_t *const video_extensions[] = {
        L".mp4", L".mov", L".avi", L".mkv", L".wmv", L".flv",
        L".webm", L".m4v", L".mpeg", L".mpg", L".3gp", L".ts"
    };

    const wchar_t *name = wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    const wchar_t *extension = wcsrchr(name, L'.');
    if (!extension) extension = L"";

    switch (file_type) {
        case SCANNER_FILE_TYPE_DOCUMENTS:
            return extension_in_list(extension, document_extensions, sizeof(document_extensions) / sizeof(document_extensions[0]));
        case SCANNER_FILE_TYPE_IMAGES:
            return extension_in_list(extension, image_extensions, sizeof(image_extensions) / sizeof(image_extensions[0]));
        case SCANNER_FILE_TYPE_VIDEOS:
            return extension_in_list(extension, video_extensions, sizeof(video_extensions) / sizeof(video_extensions[0]));
        case SCANNER_FILE_TYPE_ALL:
        default:
            return true;
    }
}

bool scanner_file_matches_filter(
    const wchar_t *path,
    uint64_t file_size_bytes,
    ScannerFileType file_type,
    uint64_t max_file_size_bytes
) {
    return path && matches_file_type(path, file_type) &&
        (max_file_size_bytes == 0 || file_size_bytes <= max_file_size_bytes);
}

size_t scanner_filter_files(FileList *list, ScannerFileType file_type, uint64_t max_file_size_bytes) {
    if (!list) return 0;
    size_t kept = 0;
    size_t skipped = 0;
    uint64_t total_bytes = 0;

    for (size_t i = 0; i < list->count; i++) {
        const wchar_t *path = list->paths[i];
        WIN32_FILE_ATTRIBUTE_DATA attributes;
        bool include = path && matches_file_type(path, file_type) &&
            GetFileAttributesExW(path, GetFileExInfoStandard, &attributes) &&
            !(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        ULARGE_INTEGER file_size;
        file_size.LowPart = include ? attributes.nFileSizeLow : 0;
        file_size.HighPart = include ? attributes.nFileSizeHigh : 0;
        if (include && !scanner_file_matches_filter(path, file_size.QuadPart, file_type, max_file_size_bytes)) include = false;

        if (include) {
            list->paths[kept++] = list->paths[i];
            total_bytes += file_size.QuadPart;
        } else {
            free(list->paths[i]);
            skipped++;
        }
    }
    list->count = kept;
    list->total_bytes = total_bytes;
    return skipped;
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
                /* When encrypting, exclude files that have already been encrypted */
                if (!is_file_already_encrypted(find_data.cFileName, full_path)) {
                    scanner_add_file(list, full_path, fSize.QuadPart);
                }
            } else if (mode == OP_MODE_DECRYPT) {
                /* When decrypting, only include files that are encrypted */
                if (is_file_already_encrypted(find_data.cFileName, full_path)) {
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
