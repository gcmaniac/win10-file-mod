#ifndef SHREDDER_H
#define SHREDDER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Overwrite file contents with zeroes/random data, flush buffers, and delete file */
bool shredder_wipe_and_delete(const wchar_t *file_path);

/* Atomically commit the temporary encrypted file to final .enc and shred the original file */
bool shredder_commit_and_wipe_original(
    const wchar_t *original_path,
    const wchar_t *temp_enc_path,
    const wchar_t *final_enc_path
);

#ifdef __cplusplus
}
#endif

#endif /* SHREDDER_H */
