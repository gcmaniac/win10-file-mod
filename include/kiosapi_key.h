#ifndef KIOSAPI_KEY_H
#define KIOSAPI_KEY_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KIOSAPI_KEY_CAPACITY 4096

/* Load the encrypted API key, protecting the plaintext source on first use. */
bool kiosapi_key_load(
    const wchar_t *password,
    char *out_key,
    size_t out_capacity,
    size_t *out_len,
    wchar_t *out_error,
    size_t error_capacity
);

#ifdef __cplusplus
}
#endif

#endif
