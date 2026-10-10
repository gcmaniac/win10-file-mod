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

/* Create an empty FileList container */
FileList* scanner_create(void);

/* Free FileList and all its allocated strings */
void scanner_free(FileList *list);

/* Scan directory for target files based on operation mode and recursive option */
bool scanner_scan(
    const wchar_t *root_dir,
    bool recursive,
    OperationMode mode,
    FileList *out_list
);

#ifdef __cplusplus
}
#endif

#endif /* SCANNER_H */
