#ifndef RENAMER_H
#define RENAMER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RENAME_METHOD_ADD = 0,         /* Add Prefix / Suffix */
    RENAME_METHOD_REPLACE,         /* Find & Replace */
    RENAME_METHOD_PATTERN,         /* Pattern with tags: <name>, <inc:1>, <inc:001> */
    RENAME_METHOD_CASE,            /* Change case: Lower, Upper, Title */
    RENAME_METHOD_TRIM             /* Trim count from start/end or trim whitespace */
} RenameMethodType;

typedef enum {
    CASE_LOWERCASE = 0,
    CASE_UPPERCASE,
    CASE_TITLECASE,
    CASE_FIRST_LETTER_UPPER
} CaseChangeType;

typedef enum {
    ADD_POS_PREFIX = 0,
    ADD_POS_SUFFIX
} AddPositionType;

typedef struct {
    RenameMethodType method;

    /* Add options */
    AddPositionType add_pos;
    wchar_t add_text[128];

    /* Replace options */
    wchar_t find_text[128];
    wchar_t replace_text[128];
    bool case_sensitive;

    /* Pattern options */
    wchar_t pattern[256];
    int start_number;
    int number_step;

    /* Case options */
    CaseChangeType case_type;
    bool apply_to_extension;

    /* Trim options */
    int trim_start_count;
    int trim_end_count;
    bool trim_whitespace;
} RenameConfig;

typedef struct {
    wchar_t full_path[MAX_PATH * 2];
    wchar_t dir_path[MAX_PATH * 2];
    wchar_t orig_name[MAX_PATH];
    wchar_t new_name[MAX_PATH];
    uint64_t file_size;
    bool has_collision;
    bool is_renamed;
    wchar_t status_text[128];
} RenameItem;

typedef struct {
    RenameItem *items;
    size_t count;
    size_t capacity;
} RenameItemList;

typedef struct {
    wchar_t old_full_path[MAX_PATH * 2];
    wchar_t new_full_path[MAX_PATH * 2];
} UndoEntry;

typedef struct {
    UndoEntry *entries;
    size_t count;
    size_t capacity;
} UndoHistory;

/* Create and manage RenameItemList */
RenameItemList* renamer_create_list(void);
void renamer_free_list(RenameItemList *list);
void renamer_clear_list(RenameItemList *list);
bool renamer_add_item(RenameItemList *list, const wchar_t *full_path, uint64_t size);

/* Populate list from folder scanning */
bool renamer_scan_folder(RenameItemList *list, const wchar_t *root_dir, bool recursive);

/* Calculate new names for all items based on config and detect collisions (Live Preview) */
void renamer_compute_preview(RenameItemList *list, const RenameConfig *cfg);

/* Apply the rename operation in background thread */
bool renamer_execute_batch(
    RenameItemList *list,
    UndoHistory *undo,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
);

/* Undo the last executed batch rename */
bool renamer_undo_batch(
    UndoHistory *undo,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
);

/* Undo history management */
UndoHistory* renamer_create_undo(void);
void renamer_free_undo(UndoHistory *undo);
void renamer_clear_undo(UndoHistory *undo);

#ifdef __cplusplus
}
#endif

#endif /* RENAMER_H */
