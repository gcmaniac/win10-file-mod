#ifndef DUPLICATE_H
#define DUPLICATE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DuplicateItem {
    wchar_t *path;
    wchar_t *filename;
    uint64_t size;
    FILETIME mod_time;
    size_t group_id;
    bool is_original;
    bool checked;
    struct DuplicateItem *next;
} DuplicateItem;

typedef struct DuplicateGroup {
    size_t group_id;
    uint64_t file_size;
    uint8_t hash[32];      /* SHA-256 binary hash */
    wchar_t hash_str[65];  /* SHA-256 hexadecimal string */
    size_t count;
    DuplicateItem *items;
    struct DuplicateGroup *next;
} DuplicateGroup;

typedef struct DuplicateResults {
    DuplicateGroup *groups;
    size_t group_count;
    size_t total_duplicates;    /* Count of copies (excluding the 1 original per group) */
    uint64_t wasted_bytes;       /* Total size of duplicate copies */
    size_t total_scanned_files;
} DuplicateResults;

typedef void (*DuplicateProgressCallback)(
    int phase,           /* 1: Scanning, 2: Matching/Hashing */
    size_t current,
    size_t total,
    const wchar_t *current_file,
    void *user_data
);

/* Scan directory and identify exact duplicate files by size and SHA-256 hash */
DuplicateResults* duplicate_find(
    const wchar_t *folder_path,
    bool recursive,
    DuplicateProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
);

/* Free allocated duplicate results tree */
void duplicate_free_results(DuplicateResults *res);

/* Auto-check duplicate copies (keeps 1 original file per group unchecked) */
void duplicate_auto_select_copies(DuplicateResults *res);

/* Safely delete checked items (moves files to Windows Recycle Bin) */
size_t duplicate_delete_checked(
    DuplicateResults *res,
    HWND hWndParent,
    uint64_t *out_deleted_bytes
);

/* Format bytes into human readable string (B, KB, MB, GB) */
void format_file_size_w(uint64_t bytes, wchar_t *out_buf, size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif /* DUPLICATE_H */
