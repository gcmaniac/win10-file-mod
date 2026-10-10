#ifndef CRYPTO_H
#define CRYPTO_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize CNG algorithm providers */
bool crypto_init(void);

/* Free CNG algorithm providers */
void crypto_cleanup(void);

/* Generate cryptographically secure random bytes */
bool crypto_generate_random(uint8_t *buf, size_t len);

/* Derive a 256-bit encryption key using PBKDF2-HMAC-SHA256 */
bool crypto_derive_key(
    const wchar_t *password,
    const wchar_t *custom_salt,
    const uint8_t *file_salt,
    size_t salt_len,
    uint8_t *out_key
);

/* Encrypt a file from src_path to dst_path using chunked AEAD (AES-256-GCM) */
bool crypto_encrypt_file(
    const wchar_t *src_path,
    const wchar_t *dst_path,
    const uint8_t *key,
    const uint8_t *file_salt,
    int cipher_id,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
);

/* Decrypt a file from src_path to destination folder, returns decrypted file path */
bool crypto_decrypt_file(
    const wchar_t *src_path,
    const wchar_t *dst_dir,
    const uint8_t *key,
    wchar_t *out_decrypted_path,
    size_t max_out_len,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_H */
