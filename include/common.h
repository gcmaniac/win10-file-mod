#ifndef COMMON_H
#define COMMON_H

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <bcrypt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define APP_NAME            L"Win10 File Encryptor & Secure Shredder"
#define APP_VERSION         L"1.0.0"
#define HEADER_MAGIC        "WENC"
#define HEADER_VERSION      1

#define CIPHER_AES_256_GCM      1
#define CIPHER_AES_256_CBC      2

#define PBKDF2_ITERATIONS       100000
#define KEY_SIZE_AES256         32
#define SALT_SIZE               16
#define GCM_NONCE_SIZE          12
#define GCM_TAG_SIZE            16
#define CHUNK_SIZE              65536   /* 64 KB streaming buffer */

#define ENC_FILE_EXTENSION      L".enc"
#define TMP_FILE_EXTENSION      L".enc.tmp"

#pragma pack(push, 1)
typedef struct {
    char magic[4];              /* "WENC" */
    uint8_t version;            /* 0x01 */
    uint8_t cipher_id;          /* 1 = GCM, 2 = CBC */
    uint16_t flags;             /* Reserved */
    uint8_t salt[SALT_SIZE];    /* PBKDF2 Salt */
    uint8_t iv[GCM_NONCE_SIZE]; /* GCM Nonce (12 bytes) or CBC IV prefix */
    char orig_ext[16];          /* Extension, e.g. ".pdf\0" */
    uint8_t reserved[12];       /* Padding to align header to 64 bytes */
} FileHeader;
#pragma pack(pop)

typedef enum {
    OP_MODE_ENCRYPT_AND_SHRED = 1,
    OP_MODE_DECRYPT = 2
} OperationMode;

typedef struct {
    wchar_t folder_path[MAX_PATH * 2];
    bool recursive;
    int cipher_id;
    wchar_t password[256];
    bool use_custom_salt;
    wchar_t custom_salt[128];
    OperationMode op_mode;
} JobConfig;

typedef void (*ProgressCallback)(
    const wchar_t *current_file,
    int current_index,
    int total_files,
    const wchar_t *status_msg,
    void *user_data
);

#endif /* COMMON_H */
