#include "renamer.h"
#include <wctype.h>

RenameItemList* renamer_create_list(void) {
    RenameItemList *list = (RenameItemList *)malloc(sizeof(RenameItemList));
    if (!list) return NULL;
    list->capacity = 128;
    list->count = 0;
    list->items = (RenameItem *)calloc(list->capacity, sizeof(RenameItem));
    if (!list->items) {
        free(list);
        return NULL;
    }
    return list;
}

void renamer_free_list(RenameItemList *list) {
    if (!list) return;
    if (list->items) free(list->items);
    free(list);
}

void renamer_clear_list(RenameItemList *list) {
    if (!list) return;
    list->count = 0;
}

bool renamer_add_item(RenameItemList *list, const wchar_t *full_path, uint64_t size) {
    if (!list || !full_path) return false;

    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity * 2;
        RenameItem *new_items = (RenameItem *)realloc(list->items, new_cap * sizeof(RenameItem));
        if (!new_items) return false;
        list->items = new_items;
        list->capacity = new_cap;
    }

    RenameItem *item = &list->items[list->count];
    SecureZeroMemory(item, sizeof(RenameItem));

    wcsncpy(item->full_path, full_path, MAX_PATH * 2 - 1);
    item->file_size = size;

    /* Extract directory and filename */
    const wchar_t *last_sep = wcsrchr(full_path, L'\\');
    if (last_sep) {
        size_t dir_len = (size_t)(last_sep - full_path);
        wcsncpy(item->dir_path, full_path, dir_len);
        item->dir_path[dir_len] = L'\0';
        wcsncpy(item->orig_name, last_sep + 1, MAX_PATH - 1);
    } else {
        item->dir_path[0] = L'\0';
        wcsncpy(item->orig_name, full_path, MAX_PATH - 1);
    }

    wcsncpy(item->new_name, item->orig_name, MAX_PATH - 1);
    wcscpy(item->status_text, L"Siap");
    list->count++;
    return true;
}

static void scan_recursive_renamer(RenameItemList *list, const wchar_t *dir_path, bool recursive) {
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

    wchar_t pattern[MAX_PATH * 2];
    swprintf(pattern, MAX_PATH * 2, L"%ls\\*", clean_dir);

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;

        wchar_t item_path[MAX_PATH * 2];
        swprintf(item_path, MAX_PATH * 2, L"%ls\\%ls", clean_dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                scan_recursive_renamer(list, item_path, recursive);
            }
        } else {
            /* Ignore desktop.ini, thumbs.db */
            if (_wcsicmp(fd.cFileName, L"desktop.ini") == 0 || _wcsicmp(fd.cFileName, L"thumbs.db") == 0) continue;

            ULARGE_INTEGER sz;
            sz.LowPart = fd.nFileSizeLow;
            sz.HighPart = fd.nFileSizeHigh;
            renamer_add_item(list, item_path, sz.QuadPart);
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
}

bool renamer_scan_folder(RenameItemList *list, const wchar_t *root_dir, bool recursive) {
    if (!list || !root_dir) return false;
    renamer_clear_list(list);
    scan_recursive_renamer(list, root_dir, recursive);
    return true;
}

static void split_filename(const wchar_t *filename, wchar_t *out_base, size_t max_base, wchar_t *out_ext, size_t max_ext) {
    const wchar_t *dot = wcsrchr(filename, L'.');
    if (dot && dot != filename) {
        size_t base_len = (size_t)(dot - filename);
        if (base_len >= max_base) base_len = max_base - 1;
        wcsncpy(out_base, filename, base_len);
        out_base[base_len] = L'\0';
        wcsncpy(out_ext, dot, max_ext - 1);
        out_ext[max_ext - 1] = L'\0';
    } else {
        wcsncpy(out_base, filename, max_base - 1);
        out_base[max_base - 1] = L'\0';
        out_ext[0] = L'\0';
    }
}

static void to_title_case(wchar_t *str) {
    bool new_word = true;
    for (size_t i = 0; str[i] != L'\0'; i++) {
        if (iswspace(str[i]) || str[i] == L'_' || str[i] == L'-' || str[i] == L'.') {
            new_word = true;
        } else if (new_word) {
            str[i] = towupper(str[i]);
            new_word = false;
        } else {
            str[i] = towlower(str[i]);
        }
    }
}

static void apply_method_to_item(RenameItem *item, size_t index, const RenameConfig *cfg) {
    wchar_t base[MAX_PATH] = {0};
    wchar_t ext[MAX_PATH] = {0};
    split_filename(item->orig_name, base, MAX_PATH, ext, MAX_PATH);

    wchar_t new_base[MAX_PATH] = {0};
    wchar_t new_ext[MAX_PATH] = {0};
    wcsncpy(new_base, base, MAX_PATH - 1);
    wcsncpy(new_ext, ext, MAX_PATH - 1);

    switch (cfg->method) {
        case RENAME_METHOD_ADD: {
            if (cfg->add_text[0] != L'\0') {
                if (cfg->add_pos == ADD_POS_PREFIX) {
                    swprintf(new_base, MAX_PATH, L"%ls%ls", cfg->add_text, base);
                } else {
                    swprintf(new_base, MAX_PATH, L"%ls%ls", base, cfg->add_text);
                }
            }
            break;
        }

        case RENAME_METHOD_REPLACE: {
            if (cfg->find_text[0] != L'\0') {
                const wchar_t *src_ptr = base;
                wchar_t out_buf[MAX_PATH] = {0};
                size_t out_len = 0;
                size_t find_len = wcslen(cfg->find_text);
                size_t rep_len = wcslen(cfg->replace_text);

                while (*src_ptr != L'\0' && out_len < MAX_PATH - 1) {
                    bool match = false;
                    if (cfg->case_sensitive) {
                        match = (wcsncmp(src_ptr, cfg->find_text, find_len) == 0);
                    } else {
                        match = (_wcsnicmp(src_ptr, cfg->find_text, find_len) == 0);
                    }

                    if (match) {
                        if (out_len + rep_len < MAX_PATH - 1) {
                            wcscpy(out_buf + out_len, cfg->replace_text);
                            out_len += rep_len;
                            src_ptr += find_len;
                        } else {
                            break;
                        }
                    } else {
                        out_buf[out_len++] = *src_ptr++;
                    }
                }
                out_buf[out_len] = L'\0';
                wcsncpy(new_base, out_buf, MAX_PATH - 1);
            }
            break;
        }

        case RENAME_METHOD_PATTERN: {
            if (cfg->pattern[0] != L'\0') {
                wchar_t out_buf[MAX_PATH] = {0};
                size_t out_len = 0;
                const wchar_t *p = cfg->pattern;

                int current_num = cfg->start_number + (int)index * cfg->number_step;

                while (*p != L'\0' && out_len < MAX_PATH - 1) {
                    if (*p == L'<' && wcschr(p, L'>') != NULL) {
                        const wchar_t *close_tag = wcschr(p, L'>');
                        size_t tag_len = (size_t)(close_tag - p + 1);
                        wchar_t tag[64] = {0};
                        if (tag_len < 64) {
                            wcsncpy(tag, p, tag_len);
                            tag[tag_len] = L'\0';
                        }

                        if (_wcsicmp(tag, L"<name>") == 0) {
                            size_t blen = wcslen(base);
                            if (out_len + blen < MAX_PATH - 1) {
                                wcscpy(out_buf + out_len, base);
                                out_len += blen;
                            }
                        } else if (_wcsicmp(tag, L"<ext>") == 0) {
                            const wchar_t *e = (ext[0] == L'.') ? ext + 1 : ext;
                            size_t elen = wcslen(e);
                            if (out_len + elen < MAX_PATH - 1) {
                                wcscpy(out_buf + out_len, e);
                                out_len += elen;
                            }
                        } else if (_wcsnicmp(tag, L"<inc", 4) == 0) {
                            /* Parse padding from <inc:01>, <inc:001>, or <inc> */
                            int pad = 1;
                            const wchar_t *colon = wcschr(tag, L':');
                            if (colon) {
                                const wchar_t *digits = colon + 1;
                                size_t dcount = 0;
                                while (digits[dcount] >= L'0' && digits[dcount] <= L'9') dcount++;
                                if (dcount > 0) pad = (int)dcount;
                            }
                            wchar_t num_str[32];
                            wchar_t fmt[16];
                            swprintf(fmt, 16, L"%%0%dld", pad);
                            swprintf(num_str, 32, fmt, (long)current_num);

                            size_t nlen = wcslen(num_str);
                            if (out_len + nlen < MAX_PATH - 1) {
                                wcscpy(out_buf + out_len, num_str);
                                out_len += nlen;
                            }
                        } else {
                            /* Unknown tag, copy verbatim */
                            if (out_len + tag_len < MAX_PATH - 1) {
                                wcscpy(out_buf + out_len, tag);
                                out_len += tag_len;
                            }
                        }
                        p = close_tag + 1;
                    } else {
                        out_buf[out_len++] = *p++;
                    }
                }
                out_buf[out_len] = L'\0';
                wcsncpy(new_base, out_buf, MAX_PATH - 1);
            }
            break;
        }

        case RENAME_METHOD_CASE: {
            switch (cfg->case_type) {
                case CASE_LOWERCASE:
                    for (size_t i = 0; new_base[i] != L'\0'; i++) new_base[i] = towlower(new_base[i]);
                    if (cfg->apply_to_extension) {
                        for (size_t i = 0; new_ext[i] != L'\0'; i++) new_ext[i] = towlower(new_ext[i]);
                    }
                    break;
                case CASE_UPPERCASE:
                    for (size_t i = 0; new_base[i] != L'\0'; i++) new_base[i] = towupper(new_base[i]);
                    if (cfg->apply_to_extension) {
                        for (size_t i = 0; new_ext[i] != L'\0'; i++) new_ext[i] = towupper(new_ext[i]);
                    }
                    break;
                case CASE_TITLECASE:
                    to_title_case(new_base);
                    if (cfg->apply_to_extension) {
                        for (size_t i = 0; new_ext[i] != L'\0'; i++) new_ext[i] = towlower(new_ext[i]);
                    }
                    break;
                case CASE_FIRST_LETTER_UPPER:
                    if (new_base[0] != L'\0') new_base[0] = towupper(new_base[0]);
                    break;
            }
            break;
        }

        case RENAME_METHOD_TRIM: {
            size_t blen = wcslen(new_base);
            size_t start_cut = (size_t)cfg->trim_start_count;
            size_t end_cut = (size_t)cfg->trim_end_count;

            if (start_cut + end_cut < blen) {
                wchar_t trimmed[MAX_PATH] = {0};
                size_t remaining_len = blen - start_cut - end_cut;
                wcsncpy(trimmed, new_base + start_cut, remaining_len);
                trimmed[remaining_len] = L'\0';

                if (cfg->trim_whitespace) {
                    wchar_t *s = trimmed;
                    while (*s == L' ' || *s == L'\t') s++;
                    size_t slen = wcslen(s);
                    while (slen > 0 && (s[slen - 1] == L' ' || s[slen - 1] == L'\t')) s[--slen] = L'\0';
                    wcsncpy(new_base, s, MAX_PATH - 1);
                } else {
                    wcsncpy(new_base, trimmed, MAX_PATH - 1);
                }
            }
            break;
        }
    }

    /* Recombine new_base + new_ext */
    if (new_base[0] == L'\0') {
        /* Prevent completely empty name */
        wcsncpy(item->new_name, item->orig_name, MAX_PATH - 1);
    } else {
        swprintf(item->new_name, MAX_PATH, L"%ls%ls", new_base, new_ext);
    }
}

void renamer_compute_preview(RenameItemList *list, const RenameConfig *cfg) {
    if (!list) return;

    /* Step 1: Compute prospective new name for each item */
    for (size_t i = 0; i < list->count; i++) {
        list->items[i].has_collision = false;
        apply_method_to_item(&list->items[i], i, cfg);
    }

    /* Step 2: Collision detection between items in same folder */
    for (size_t i = 0; i < list->count; i++) {
        for (size_t j = i + 1; j < list->count; j++) {
            if (_wcsicmp(list->items[i].dir_path, list->items[j].dir_path) == 0 &&
                _wcsicmp(list->items[i].new_name, list->items[j].new_name) == 0) {
                list->items[i].has_collision = true;
                list->items[j].has_collision = true;
                wcscpy(list->items[i].status_text, L"Konflik: Nama Duplikat!");
                wcscpy(list->items[j].status_text, L"Konflik: Nama Duplikat!");
            }
        }
    }

    /* Step 3: Check for collisions against existing files on disk */
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].has_collision) continue;

        if (wcscmp(list->items[i].orig_name, list->items[i].new_name) == 0) {
            wcscpy(list->items[i].status_text, L"Tidak Berubah");
        } else {
            wchar_t target_path[MAX_PATH * 2];
            swprintf(target_path, MAX_PATH * 2, L"%ls\\%ls", list->items[i].dir_path, list->items[i].new_name);

            DWORD attr = GetFileAttributesW(target_path);
            if (attr != INVALID_FILE_ATTRIBUTES) {
                list->items[i].has_collision = true;
                wcscpy(list->items[i].status_text, L"Konflik: File Sudah Ada di Disk!");
            } else {
                wcscpy(list->items[i].status_text, L"Siap Diubah");
            }
        }
    }
}

UndoHistory* renamer_create_undo(void) {
    UndoHistory *undo = (UndoHistory *)malloc(sizeof(UndoHistory));
    if (!undo) return NULL;
    undo->capacity = 128;
    undo->count = 0;
    undo->entries = (UndoEntry *)malloc(undo->capacity * sizeof(UndoEntry));
    if (!undo->entries) {
        free(undo);
        return NULL;
    }
    return undo;
}

void renamer_free_undo(UndoHistory *undo) {
    if (!undo) return;
    if (undo->entries) free(undo->entries);
    free(undo);
}

void renamer_clear_undo(UndoHistory *undo) {
    if (!undo) return;
    undo->count = 0;
}

static void undo_add_entry(UndoHistory *undo, const wchar_t *old_path, const wchar_t *new_path) {
    if (!undo) return;
    if (undo->count >= undo->capacity) {
        size_t ncap = undo->capacity * 2;
        UndoEntry *ne = (UndoEntry *)realloc(undo->entries, ncap * sizeof(UndoEntry));
        if (!ne) return;
        undo->entries = ne;
        undo->capacity = ncap;
    }
    wcsncpy(undo->entries[undo->count].old_full_path, old_path, MAX_PATH * 2 - 1);
    wcsncpy(undo->entries[undo->count].new_full_path, new_path, MAX_PATH * 2 - 1);
    undo->count++;
}

bool renamer_execute_batch(
    RenameItemList *list,
    UndoHistory *undo,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
) {
    if (!list) return false;
    if (undo) renamer_clear_undo(undo);

    for (size_t i = 0; i < list->count; i++) {
        if (cancel_flag && *cancel_flag) break;

        RenameItem *item = &list->items[i];
        if (item->has_collision || wcscmp(item->orig_name, item->new_name) == 0) {
            continue;
        }

        wchar_t dest_path[MAX_PATH * 2];
        swprintf(dest_path, MAX_PATH * 2, L"%ls\\%ls", item->dir_path, item->new_name);

        if (MoveFileW(item->full_path, dest_path)) {
            if (undo) {
                undo_add_entry(undo, item->full_path, dest_path);
            }
            wcsncpy(item->full_path, dest_path, MAX_PATH * 2 - 1);
            wcsncpy(item->orig_name, item->new_name, MAX_PATH - 1);
            item->is_renamed = true;
            wcscpy(item->status_text, L"Sukses Diubah");
        } else {
            swprintf(item->status_text, 128, L"Gagal (Error %lu)", GetLastError());
        }

        if (cb) {
            cb(item->orig_name, (int)(i + 1), (int)list->count, item->status_text, user_data);
        }
    }

    return true;
}

bool renamer_undo_batch(
    UndoHistory *undo,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
) {
    if (!undo || undo->count == 0) return false;

    /* Revert in reverse order */
    for (size_t i = undo->count; i > 0; i--) {
        if (cancel_flag && *cancel_flag) break;

        size_t idx = i - 1;
        UndoEntry *e = &undo->entries[idx];
        MoveFileW(e->new_full_path, e->old_full_path);

        if (cb) {
            cb(e->old_full_path, (int)(undo->count - idx), (int)undo->count, L"Reverted", user_data);
        }
    }

    renamer_clear_undo(undo);
    return true;
}
