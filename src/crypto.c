#include "crypto.h"
#include <shlwapi.h>

static BCRYPT_ALG_HANDLE g_hHashAlg = NULL;
static BCRYPT_ALG_HANDLE g_hAesAlg = NULL;

#pragma pack(push, 1)
typedef struct {
    uint32_t chunk_index;
    uint8_t is_last;
} ChunkAad;

typedef struct {
    uint32_t payload_len;
    uint8_t is_last;
    uint8_t tag[GCM_TAG_SIZE];
} ChunkHeader;
#pragma pack(pop)

bool crypto_init(void) {
    NTSTATUS status;

    if (g_hHashAlg == NULL) {
        status = BCryptOpenAlgorithmProvider(
            &g_hHashAlg,
            BCRYPT_SHA256_ALGORITHM,
            NULL,
            BCRYPT_ALG_HANDLE_HMAC_FLAG
        );
        if (!BCRYPT_SUCCESS(status)) {
            return false;
        }
    }

    if (g_hAesAlg == NULL) {
        status = BCryptOpenAlgorithmProvider(
            &g_hAesAlg,
            BCRYPT_AES_ALGORITHM,
            NULL,
            0
        );
        if (!BCRYPT_SUCCESS(status)) {
            BCryptCloseAlgorithmProvider(g_hHashAlg, 0);
            g_hHashAlg = NULL;
            return false;
        }

        status = BCryptSetProperty(
            g_hAesAlg,
            BCRYPT_CHAINING_MODE,
            (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
            sizeof(BCRYPT_CHAIN_MODE_GCM),
            0
        );
        if (!BCRYPT_SUCCESS(status)) {
            crypto_cleanup();
            return false;
        }
    }

    return true;
}

void crypto_cleanup(void) {
    if (g_hHashAlg) {
        BCryptCloseAlgorithmProvider(g_hHashAlg, 0);
        g_hHashAlg = NULL;
    }
    if (g_hAesAlg) {
        BCryptCloseAlgorithmProvider(g_hAesAlg, 0);
        g_hAesAlg = NULL;
    }
}

bool crypto_generate_random(uint8_t *buf, size_t len) {
    if (!buf || len == 0) return false;
    NTSTATUS status = BCryptGenRandom(NULL, (PUCHAR)buf, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return BCRYPT_SUCCESS(status);
}

bool crypto_derive_key(
    const wchar_t *password,
    const wchar_t *custom_salt,
    const uint8_t *file_salt,
    size_t salt_len,
    uint8_t *out_key
) {
    if (!password || !file_salt || !out_key) return false;
    if (!g_hHashAlg && !crypto_init()) return false;

    /* Combine password + custom salt into UTF-8 representation */
    wchar_t combined_pass[512] = {0};
    wcsncpy(combined_pass, password, 255);
    if (custom_salt && wcslen(custom_salt) > 0) {
        wcsncat(combined_pass, L"::", 2);
        wcsncat(combined_pass, custom_salt, 250);
    }

    char pass_utf8[1024] = {0};
    int pass_len = WideCharToMultiByte(CP_UTF8, 0, combined_pass, -1, pass_utf8, sizeof(pass_utf8) - 1, NULL, NULL);
    SecureZeroMemory(combined_pass, sizeof(combined_pass));

    if (pass_len <= 1) {
        /* Empty password */
        pass_utf8[0] = '\0';
        pass_len = 0;
    } else {
        pass_len -= 1; /* Exclude null terminator from byte count */
    }

    NTSTATUS status = BCryptDeriveKeyPBKDF2(
        g_hHashAlg,
        (PUCHAR)pass_utf8,
        (ULONG)pass_len,
        (PUCHAR)file_salt,
        (ULONG)salt_len,
        PBKDF2_ITERATIONS,
        (PUCHAR)out_key,
        KEY_SIZE_AES256,
        0
    );

    SecureZeroMemory(pass_utf8, sizeof(pass_utf8));
    return BCRYPT_SUCCESS(status);
}

static void build_chunk_nonce(const uint8_t base_nonce[GCM_NONCE_SIZE], uint32_t chunk_idx, uint8_t out_nonce[GCM_NONCE_SIZE]) {
    memcpy(out_nonce, base_nonce, GCM_NONCE_SIZE);
    uint32_t *p = (uint32_t *)(out_nonce + 8);
    *p ^= chunk_idx;
}

bool crypto_encrypt_file(
    const wchar_t *src_path,
    const wchar_t *dst_path,
    const uint8_t *key,
    const uint8_t *file_salt,
    int cipher_id,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
) {
    (void)cipher_id;
    (void)cb;
    (void)user_data;
    if (!crypto_init()) return false;

    HANDLE hSrc = CreateFileW(src_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hSrc == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hSrc, &fileSize)) {
        CloseHandle(hSrc);
        return false;
    }

    HANDLE hDst = CreateFileW(dst_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDst == INVALID_HANDLE_VALUE) {
        CloseHandle(hSrc);
        return false;
    }

    /* Initialize Header */
    FileHeader header;
    SecureZeroMemory(&header, sizeof(header));
    memcpy(header.magic, HEADER_MAGIC, 4);
    header.version = HEADER_VERSION;
    header.cipher_id = CIPHER_AES_256_GCM;

    memcpy(header.salt, file_salt, sizeof(header.salt));
    crypto_generate_random(header.iv, sizeof(header.iv));

    /* Extract original extension */
    const wchar_t *ext = wcsrchr(src_path, L'.');
    if (ext && wcslen(ext) < 16) {
        WideCharToMultiByte(CP_UTF8, 0, ext, -1, header.orig_ext, sizeof(header.orig_ext), NULL, NULL);
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(hDst, &header, sizeof(header), &bytesWritten, NULL) || bytesWritten != sizeof(header)) {
        CloseHandle(hSrc);
        CloseHandle(hDst);
        DeleteFileW(dst_path);
        return false;
    }

    /* Create CNG Symmetric Key Object */
    BCRYPT_KEY_HANDLE hKey = NULL;
    NTSTATUS status = BCryptGenerateSymmetricKey(g_hAesAlg, &hKey, NULL, 0, (PUCHAR)key, KEY_SIZE_AES256, 0);
    if (!BCRYPT_SUCCESS(status)) {
        CloseHandle(hSrc);
        CloseHandle(hDst);
        DeleteFileW(dst_path);
        return false;
    }

    uint8_t *plain_buf = (uint8_t *)malloc(CHUNK_SIZE);
    uint8_t *cipher_buf = (uint8_t *)malloc(CHUNK_SIZE);
    if (!plain_buf || !cipher_buf) {
        free(plain_buf);
        free(cipher_buf);
        BCryptDestroyKey(hKey);
        CloseHandle(hSrc);
        CloseHandle(hDst);
        DeleteFileW(dst_path);
        return false;
    }

    uint32_t chunk_idx = 0;
    uint64_t total_processed = 0;
    bool success = true;

    while (true) {
        if (cancel_flag && *cancel_flag) {
            success = false;
            break;
        }

        DWORD bytesRead = 0;
        if (!ReadFile(hSrc, plain_buf, CHUNK_SIZE, &bytesRead, NULL)) {
            success = false;
            break;
        }

        total_processed += bytesRead;
        uint8_t is_last = (total_processed >= (uint64_t)fileSize.QuadPart) ? 1 : 0;

        uint8_t chunk_nonce[GCM_NONCE_SIZE];
        build_chunk_nonce(header.iv, chunk_idx, chunk_nonce);

        ChunkAad aad;
        aad.chunk_index = chunk_idx;
        aad.is_last = is_last;

        ChunkHeader ch;
        ch.payload_len = bytesRead;
        ch.is_last = is_last;
        SecureZeroMemory(ch.tag, sizeof(ch.tag));

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
        BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
        authInfo.pbNonce = chunk_nonce;
        authInfo.cbNonce = sizeof(chunk_nonce);
        authInfo.pbAuthData = (PUCHAR)&aad;
        authInfo.cbAuthData = sizeof(aad);
        authInfo.pbTag = ch.tag;
        authInfo.cbTag = sizeof(ch.tag);

        ULONG cbResult = 0;
        status = BCryptEncrypt(
            hKey,
            plain_buf,
            bytesRead,
            &authInfo,
            NULL,
            0,
            cipher_buf,
            bytesRead,
            &cbResult,
            0
        );

        if (!BCRYPT_SUCCESS(status)) {
            success = false;
            break;
        }

        /* Write ChunkHeader then Ciphertext */
        if (!WriteFile(hDst, &ch, sizeof(ch), &bytesWritten, NULL) || bytesWritten != sizeof(ch)) {
            success = false;
            break;
        }

        if (bytesRead > 0) {
            if (!WriteFile(hDst, cipher_buf, bytesRead, &bytesWritten, NULL) || bytesWritten != bytesRead) {
                success = false;
                break;
            }
        }

        chunk_idx++;
        if (is_last) {
            break;
        }
    }

    SecureZeroMemory(plain_buf, CHUNK_SIZE);
    free(plain_buf);
    free(cipher_buf);
    BCryptDestroyKey(hKey);
    CloseHandle(hSrc);
    CloseHandle(hDst);

    if (!success) {
        DeleteFileW(dst_path);
    }

    return success;
}

bool crypto_decrypt_file(
    const wchar_t *src_path,
    const wchar_t *dst_dir,
    const uint8_t *key,
    wchar_t *out_decrypted_path,
    size_t max_out_len,
    ProgressCallback cb,
    void *user_data,
    volatile bool *cancel_flag
) {
    (void)cb;
    (void)user_data;
    if (!crypto_init()) return false;

    HANDLE hSrc = CreateFileW(src_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hSrc == INVALID_HANDLE_VALUE) return false;

    FileHeader header;
    DWORD bytesRead = 0;
    if (!ReadFile(hSrc, &header, sizeof(header), &bytesRead, NULL) || bytesRead != sizeof(header)) {
        CloseHandle(hSrc);
        return false;
    }

    if (memcmp(header.magic, HEADER_MAGIC, 4) != 0 || header.version != HEADER_VERSION) {
        CloseHandle(hSrc);
        return false;
    }

    /* Build destination filename */
    wchar_t dst_file[MAX_PATH * 2] = {0};
    const wchar_t *src_filename = wcsrchr(src_path, L'\\');
    src_filename = src_filename ? src_filename + 1 : src_path;

    if (dst_dir && wcslen(dst_dir) > 0) {
        swprintf(dst_file, MAX_PATH * 2, L"%ls\\%ls", dst_dir, src_filename);
    } else {
        wcsncpy(dst_file, src_path, MAX_PATH * 2 - 1);
    }

    /* Strip .enc extension if present */
    size_t len = wcslen(dst_file);
    if (len > 4 && _wcsicmp(dst_file + len - 4, ENC_FILE_EXTENSION) == 0) {
        dst_file[len - 4] = L'\0';
    } else {
        /* If not ending in .enc, restore original saved extension */
        if (header.orig_ext[0] != '\0') {
            wchar_t wExt[16] = {0};
            MultiByteToWideChar(CP_UTF8, 0, header.orig_ext, -1, wExt, 15);
            wcsncat(dst_file, L"_decrypted", MAX_PATH * 2 - wcslen(dst_file) - 1);
            wcsncat(dst_file, wExt, MAX_PATH * 2 - wcslen(dst_file) - 1);
        } else {
            wcsncat(dst_file, L".decrypted", MAX_PATH * 2 - wcslen(dst_file) - 1);
        }
    }

    if (out_decrypted_path) {
        wcsncpy(out_decrypted_path, dst_file, max_out_len - 1);
    }

    HANDLE hDst = CreateFileW(dst_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDst == INVALID_HANDLE_VALUE) {
        CloseHandle(hSrc);
        return false;
    }

    BCRYPT_KEY_HANDLE hKey = NULL;
    NTSTATUS status = BCryptGenerateSymmetricKey(g_hAesAlg, &hKey, NULL, 0, (PUCHAR)key, KEY_SIZE_AES256, 0);
    if (!BCRYPT_SUCCESS(status)) {
        CloseHandle(hSrc);
        CloseHandle(hDst);
        DeleteFileW(dst_file);
        return false;
    }

    uint8_t *cipher_buf = (uint8_t *)malloc(CHUNK_SIZE);
    uint8_t *plain_buf = (uint8_t *)malloc(CHUNK_SIZE);
    if (!cipher_buf || !plain_buf) {
        free(cipher_buf);
        free(plain_buf);
        BCryptDestroyKey(hKey);
        CloseHandle(hSrc);
        CloseHandle(hDst);
        DeleteFileW(dst_file);
        return false;
    }

    uint32_t chunk_idx = 0;
    bool success = true;

    while (true) {
        if (cancel_flag && *cancel_flag) {
            success = false;
            break;
        }

        ChunkHeader ch;
        if (!ReadFile(hSrc, &ch, sizeof(ch), &bytesRead, NULL)) {
            success = false;
            break;
        }

        if (bytesRead == 0) {
            /* Unexpected EOF */
            break;
        }

        if (bytesRead != sizeof(ch) || ch.payload_len > CHUNK_SIZE) {
            success = false;
            break;
        }

        if (ch.payload_len > 0) {
            if (!ReadFile(hSrc, cipher_buf, ch.payload_len, &bytesRead, NULL) || bytesRead != ch.payload_len) {
                success = false;
                break;
            }
        }

        uint8_t chunk_nonce[GCM_NONCE_SIZE];
        build_chunk_nonce(header.iv, chunk_idx, chunk_nonce);

        ChunkAad aad;
        aad.chunk_index = chunk_idx;
        aad.is_last = ch.is_last;

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
        BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
        authInfo.pbNonce = chunk_nonce;
        authInfo.cbNonce = sizeof(chunk_nonce);
        authInfo.pbAuthData = (PUCHAR)&aad;
        authInfo.cbAuthData = sizeof(aad);
        authInfo.pbTag = ch.tag;
        authInfo.cbTag = sizeof(ch.tag);

        ULONG cbResult = 0;
        status = BCryptDecrypt(
            hKey,
            cipher_buf,
            ch.payload_len,
            &authInfo,
            NULL,
            0,
            plain_buf,
            ch.payload_len,
            &cbResult,
            0
        );

        if (!BCRYPT_SUCCESS(status)) {
            /* Decryption / Tag verification failed! (Wrong password or tampered file) */
            success = false;
            break;
        }

        DWORD bytesWritten = 0;
        if (ch.payload_len > 0) {
            if (!WriteFile(hDst, plain_buf, ch.payload_len, &bytesWritten, NULL) || bytesWritten != ch.payload_len) {
                success = false;
                break;
            }
        }

        chunk_idx++;
        if (ch.is_last) {
            break;
        }
    }

    SecureZeroMemory(plain_buf, CHUNK_SIZE);
    free(cipher_buf);
    free(plain_buf);
    BCryptDestroyKey(hKey);
    CloseHandle(hSrc);
    CloseHandle(hDst);

    if (!success) {
        DeleteFileW(dst_file);
    }

    return success;
}
