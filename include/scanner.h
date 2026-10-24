#ifndef SCANNER_H
#define SCANNER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    wchar_t **paths;
    size_t count;
    size_t capacity;
    uint64_t total_bytes;
} FileList;

typedef enum {
    SCANNER_FILE_TYPE_ALL = 0,
    SCANNER_FILE_TYPE_DOCUMENTS,
    SCANNER_FILE_TYPE_IMAGES,
    SCANNER_FILE_TYPE_VIDEOS
} ScannerFileType;

/* Create an empty FileList container */
FileList* scanner_create(void);

/* Free FileList and all its allocated strings */
void scanner_free(FileList *list);

/* Add a file path manually to FileList */
bool scanner_add_file(FileList *list, const wchar_t *path, uint64_t size);

/* Scan directory for target files based on operation mode and recursive option */
bool scanner_scan(
    const wchar_t *root_dir,
    bool recursive,
    OperationMode mode,
    FileList *out_list
);

/* Remove files outside the selected type and size limits; returns the skipped count. */
size_t scanner_filter_files(FileList *list, ScannerFileType file_type, uint64_t max_file_size_bytes);

bool scanner_file_matches_filter(
    const wchar_t *path,
    uint64_t file_size_bytes,
    ScannerFileType file_type,
    uint64_t max_file_size_bytes
);

#ifdef __cplusplus
}
#endif

#endif /* SCANNER_H */
