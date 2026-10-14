#include "duplicate.h"
#include <bcrypt.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void format_file_size_w(uint64_t bytes, wchar_t *out_buf, size_t buf_len) {
    if (!out_buf || buf_len == 0) return;
    const double KB = 1024.0;
    const double MB = 1024.0 * 1024.0;
    const double GB = 1024.0 * 1024.0 * 1024.0;

    double dbytes = (double)bytes;
    if (dbytes < KB) {
        swprintf(out_buf, buf_len, L"%llu B", bytes);
    } else if (dbytes < MB) {
        swprintf(out_buf, buf_len, L"%.2f KB", dbytes / KB);
    } else if (dbytes < GB) {
        swprintf(out_buf, buf_len, L"%.2f MB", dbytes / MB);
    } else {
        swprintf(out_buf, buf_len, L"%.2f GB", dbytes / GB);
    }
}

typedef struct {
    wchar_t *path;
    wchar_t *filename;
    uint64_t size;
    FILETIME mod_time;
    uint8_t hash[32];
    bool hash_computed;
} ScannedFile;

static int compare_files_by_size(const void *a, const void *b) {
    const ScannedFile *fa = (const ScannedFile *)a;
    const ScannedFile *fb = (const ScannedFile *)b;
    if (fa->size < fb->size) return -1;
    if (fa->size > fb->size) return 1;
    return _wcsicmp(fa->path, fb->path);
}

static bool compute_file_sha256(
    const wchar_t *path,
    uint8_t out_hash[32],
    volatile bool *cancel_flag
) {
    if (!path || !out_hash) return false;

    HANDLE hFile = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    BCRYPT_ALG_HANDLE hAlg = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &hAlg,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        CloseHandle(hFile);
        return false;
    }

    BCRYPT_HASH_HANDLE hHash = NULL;
    status = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
    if (!BCRYPT_SUCCESS(status)) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        CloseHandle(hFile);
        return false;
    }

    const DWORD buf_size = 64 * 1024;
    BYTE *buffer = (BYTE *)malloc(buf_size);
    if (!buffer) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        CloseHandle(hFile);
        return false;
    }

    DWORD bytes_read = 0;
    bool success = true;

    while (ReadFile(hFile, buffer, buf_size, &bytes_read, NULL) && bytes_read > 0) {
        if (cancel_flag && *cancel_flag) {
            success = false;
            break;
        }
        status = BCryptHashData(hHash, buffer, bytes_read, 0);
        if (!BCRYPT_SUCCESS(status)) {
            success = false;
            break;
        }
    }

    if (success && (!cancel_flag || !*cancel_flag)) {
        status = BCryptFinishHash(hHash, out_hash, 32, 0);
        if (!BCRYPT_SUCCESS(status)) {
            success = false;
        }
    } else {
        success = false;
    }

    free(buffer);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    CloseHandle(hFile);

    return success;
}

static void scan_directory_recursive(
    const wchar_t *dir,
    bool recursive,
    ScannedFile **pFiles,
    size_t *pCount,
    size_t *pCap,
    DuplicateProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
) {
    if (!dir || (cancel_flag && *cancel_flag)) return;

    wchar_t pattern[MAX_PATH * 2];
    swprintf(pattern, MAX_PATH * 2, L"%ls\\*", dir);

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (cancel_flag && *cancel_flag) break;

        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) {
            continue;
        }

        wchar_t full_path[MAX_PATH * 2];
        swprintf(full_path, MAX_PATH * 2, L"%ls\\%ls", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                scan_directory_recursive(
                    full_path, recursive, pFiles, pCount, pCap, cb, user_data, cancel_flag
                );
            }
        } else {
            /* Skip hidden/system files and temporary files */
            if (_wcsicmp(fd.cFileName, L"desktop.ini") == 0 ||
                _wcsicmp(fd.cFileName, L"thumbs.db") == 0) {
                continue;
            }
            size_t name_len = wcslen(fd.cFileName);
            if (name_len > 4 && _wcsicmp(fd.cFileName + name_len - 4, L".tmp") == 0) {
                continue;
            }

            ULARGE_INTEGER uli;
            uli.LowPart = fd.nFileSizeLow;
            uli.HighPart = fd.nFileSizeHigh;

            /* Expand capacity if necessary */
            if (*pCount >= *pCap) {
                size_t new_cap = (*pCap == 0) ? 256 : (*pCap * 2);
                ScannedFile *new_arr = (ScannedFile *)realloc(*pFiles, sizeof(ScannedFile) * new_cap);
                if (!new_arr) break;
                *pFiles = new_arr;
                *pCap = new_cap;
            }

            ScannedFile *sf = &((*pFiles)[*pCount]);
            sf->path = _wcsdup(full_path);
            sf->filename = _wcsdup(fd.cFileName);
            sf->size = uli.QuadPart;
            sf->mod_time = fd.ftLastWriteTime;
            sf->hash_computed = false;
            memset(sf->hash, 0, sizeof(sf->hash));
            (*pCount)++;

            if (cb && (*pCount % 50 == 0)) {
                cb(1, *pCount, 0, full_path, user_data);
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
}

DuplicateResults* duplicate_find(
    const wchar_t *folder_path,
    bool recursive,
    DuplicateProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
) {
    if (!folder_path || folder_path[0] == L'\0') return NULL;

    DuplicateResults *results = (DuplicateResults *)calloc(1, sizeof(DuplicateResults));
    if (!results) return NULL;

    ScannedFile *files = NULL;
    size_t file_count = 0;
    size_t file_cap = 0;

    /* Normalize target folder path */
    wchar_t clean_dir[MAX_PATH * 2];
    const wchar_t *p = folder_path;
    while (*p == L' ' || *p == L'\t' || *p == L'\"') p++;
    wcsncpy(clean_dir, p, MAX_PATH * 2 - 1);
    clean_dir[MAX_PATH * 2 - 1] = L'\0';

    size_t dlen = wcslen(clean_dir);
    while (dlen > 0 && (clean_dir[dlen - 1] == L' ' || clean_dir[dlen - 1] == L'\t' || clean_dir[dlen - 1] == L'\"')) {
        clean_dir[--dlen] = L'\0';
    }
    if (dlen > 3 && (clean_dir[dlen - 1] == L'\\' || clean_dir[dlen - 1] == L'/')) {
        clean_dir[--dlen] = L'\0';
    }

    /* Phase 1: Scan filesystem */
    scan_directory_recursive(clean_dir, recursive, &files, &file_count, &file_cap, cb, user_data, cancel_flag);
    results->total_scanned_files = file_count;

    if (cb) {
        cb(1, file_count, file_count, L"", user_data);
    }

    if (file_count < 2 || (cancel_flag && *cancel_flag)) {
        for (size_t i = 0; i < file_count; i++) {
            free(files[i].path);
            free(files[i].filename);
        }
        free(files);
        return results;
    }

    /* Phase 2: Sort files by size to identify size candidates */
    qsort(files, file_count, sizeof(ScannedFile), compare_files_by_size);

    /* Count candidates sharing identical sizes */
    size_t total_candidates = 0;
    for (size_t i = 0; i < file_count; ) {
        size_t j = i + 1;
        while (j < file_count && files[j].size == files[i].size) {
            j++;
        }
        if (j - i > 1) {
            total_candidates += (j - i);
        }
        i = j;
    }

    /* Phase 3: Hash matching candidates and build duplicate groups */
    size_t hashed_candidates = 0;
    size_t next_group_id = 1;

    for (size_t i = 0; i < file_count; ) {
        if (cancel_flag && *cancel_flag) break;

        size_t j = i + 1;
        while (j < file_count && files[j].size == files[i].size) {
            j++;
        }

        size_t group_size = j - i;
        if (group_size > 1) {
            /* Files in [i .. j-1] have identical size. Hash and cluster them. */
            for (size_t k = i; k < j; k++) {
                if (cancel_flag && *cancel_flag) break;

                if (!files[k].hash_computed) {
                    files[k].hash_computed = compute_file_sha256(files[k].path, files[k].hash, cancel_flag);
                    hashed_candidates++;
                    if (cb) {
                        cb(2, hashed_candidates, total_candidates, files[k].path, user_data);
                    }
                }
            }

            /* Cluster by identical hash */
            bool *matched = (bool *)calloc(group_size, sizeof(bool));
            if (matched) {
                for (size_t m = 0; m < group_size; m++) {
                    if (matched[m]) continue;
                    size_t k1 = i + m;
                    if (!files[k1].hash_computed) continue;

                    /* Find all other files with the exact same hash */
                    size_t sub_count = 1;
                    for (size_t n = m + 1; n < group_size; n++) {
                        size_t k2 = i + n;
                        if (!matched[n] && files[k2].hash_computed &&
                            memcmp(files[k1].hash, files[k2].hash, 32) == 0) {
                            sub_count++;
                        }
                    }

                    if (sub_count > 1) {
                        /* Duplicate cluster identified! Create DuplicateGroup */
                        DuplicateGroup *grp = (DuplicateGroup *)calloc(1, sizeof(DuplicateGroup));
                        if (grp) {
                            grp->group_id = next_group_id++;
                            grp->file_size = files[k1].size;
                            memcpy(grp->hash, files[k1].hash, 32);

                            /* Generate hex string */
                            for (int h = 0; h < 32; h++) {
                                swprintf(&grp->hash_str[h * 2], 3, L"%02x", grp->hash[h]);
                            }

                            DuplicateItem *last_item = NULL;
                            bool is_first = true;

                            for (size_t n = m; n < group_size; n++) {
                                size_t kn = i + n;
                                if (!matched[n] && files[kn].hash_computed &&
                                    memcmp(files[k1].hash, files[kn].hash, 32) == 0) {
                                    matched[n] = true;

                                    DuplicateItem *item = (DuplicateItem *)calloc(1, sizeof(DuplicateItem));
                                    if (item) {
                                        item->path = _wcsdup(files[kn].path);
                                        item->filename = _wcsdup(files[kn].filename);
                                        item->size = files[kn].size;
                                        item->mod_time = files[kn].mod_time;
                                        item->group_id = grp->group_id;
                                        item->is_original = is_first;
                                        item->checked = false; /* Unchecked initially */
                                        item->next = NULL;

                                        if (!is_first) {
                                            results->total_duplicates++;
                                            results->wasted_bytes += files[kn].size;
                                        }
                                        is_first = false;

                                        if (!grp->items) {
                                            grp->items = item;
                                        } else {
                                            last_item->next = item;
                                        }
                                        last_item = item;
                                        grp->count++;
                                    }
                                }
                            }

                            /* Append group to results list */
                            grp->next = results->groups;
                            results->groups = grp;
                            results->group_count++;
                        }
                    }
                }
                free(matched);
            }
        }

        i = j;
    }

    /* Reverse groups list so group 1 is first */
    DuplicateGroup *prev = NULL;
    DuplicateGroup *curr = results->groups;
    while (curr) {
        DuplicateGroup *next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }
    results->groups = prev;

    /* Free scanned items */
    for (size_t i = 0; i < file_count; i++) {
        free(files[i].path);
        free(files[i].filename);
    }
    free(files);

    return results;
}

void duplicate_free_results(DuplicateResults *res) {
    if (!res) return;

    DuplicateGroup *g = res->groups;
    while (g) {
        DuplicateGroup *next_g = g->next;
        DuplicateItem *item = g->items;
        while (item) {
            DuplicateItem *next_item = item->next;
            if (item->path) free(item->path);
            if (item->filename) free(item->filename);
            free(item);
            item = next_item;
        }
        free(g);
        g = next_g;
    }
    free(res);
}

void duplicate_auto_select_copies(DuplicateResults *res) {
    if (!res) return;
    for (DuplicateGroup *g = res->groups; g; g = g->next) {
        for (DuplicateItem *it = g->items; it; it = it->next) {
            /* Keep original unchecked, check duplicates */
            it->checked = !it->is_original;
        }
    }
}

size_t duplicate_delete_checked(
    DuplicateResults *res,
    HWND hWndParent,
    uint64_t *out_deleted_bytes
) {
    if (!res) return 0;
    if (out_deleted_bytes) *out_deleted_bytes = 0;

    size_t deleted_count = 0;

    for (DuplicateGroup *g = res->groups; g; g = g->next) {
        DuplicateItem *prev = NULL;
        DuplicateItem *it = g->items;

        while (it) {
            if (it->checked) {
                /* Safe delete: move to Recycle Bin via SHFileOperationW */
                wchar_t double_null_path[MAX_PATH * 2 + 2];
                wcsncpy(double_null_path, it->path, MAX_PATH * 2);
                size_t len = wcslen(double_null_path);
                double_null_path[len] = L'\0';
                double_null_path[len + 1] = L'\0'; /* Double null terminated */

                SHFILEOPSTRUCTW op = {0};
                op.hwnd = hWndParent;
                op.wFunc = FO_DELETE;
                op.pFrom = double_null_path;
                op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;

                int ret = SHFileOperationW(&op);
                if (ret == 0 && !op.fAnyOperationsAborted) {
                    deleted_count++;
                    if (out_deleted_bytes) {
                        *out_deleted_bytes += it->size;
                    }

                    /* Remove from group items */
                    DuplicateItem *to_delete = it;
                    if (prev) {
                        prev->next = it->next;
                        it = it->next;
                    } else {
                        g->items = it->next;
                        it = it->next;
                    }

                    if (to_delete->path) free(to_delete->path);
                    if (to_delete->filename) free(to_delete->filename);
                    free(to_delete);
                    g->count--;
                    continue;
                }
            }
            prev = it;
            it = it->next;
        }
    }

    return deleted_count;
}
