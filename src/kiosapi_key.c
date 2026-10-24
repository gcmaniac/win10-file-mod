#include "kiosapi_key.h"
#include "crypto.h"

#include <shlwapi.h>

static bool set_error(wchar_t *out_error, size_t capacity, const wchar_t *message) {
    if (out_error && capacity > 0) {
        wcsncpy(out_error, message, capacity - 1);
        out_error[capacity - 1] = L'\0';
    }
    return false;
}

static bool build_key_paths(
    wchar_t *plain_path,
    wchar_t *encrypted_path,
    wchar_t *temp_path,
    size_t capacity
) {
    wchar_t module_path[MAX_PATH * 2] = {0};
    DWORD length = GetModuleFileNameW(NULL, module_path, (DWORD)(sizeof(module_path) / sizeof(module_path[0])));
    if (length == 0 || length >= sizeof(module_path) / sizeof(module_path[0])) return false;

    wchar_t *separator = wcsrchr(module_path, L'\\');
    if (!separator) return false;
    *separator = L'\0';

    if (swprintf(plain_path, capacity, L"%ls\\kiosapi-key.txt", module_path) < 0 ||
        swprintf(encrypted_path, capacity, L"%ls\\kiosapi-key.wkey", module_path) < 0 ||
        swprintf(temp_path, capacity, L"%ls\\kiosapi-key.wkey.tmp", module_path) < 0) {
        return false;
    }
    return true;
}

static bool file_exists(const wchar_t *path) {
    DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static bool normalize_key(uint8_t *buffer, size_t capacity, size_t *length) {
    size_t begin = 0;
    size_t end = *length;
    if (end >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) begin = 3;
    while (begin < end && (buffer[begin] == ' ' || buffer[begin] == '\t' || buffer[begin] == '\r' || buffer[begin] == '\n')) begin++;
    while (end > begin && (buffer[end - 1] == ' ' || buffer[end - 1] == '\t' || buffer[end - 1] == '\r' || buffer[end - 1] == '\n')) end--;
    if (begin == end || end >= capacity) return false;

    size_t key_len = end - begin;
    for (size_t i = 0; i < key_len; i++) {
        uint8_t ch = buffer[begin + i];
        if (ch < 0x21 || ch > 0x7E) return false;
    }
    if (begin > 0) memmove(buffer, buffer + begin, key_len);
    buffer[key_len] = '\0';
    *length = key_len;
    return true;
}

static bool read_plain_key(const wchar_t *path, uint8_t *buffer, size_t capacity, size_t *out_len) {
    *out_len = 0;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size;
    bool success = GetFileSizeEx(file, &size) &&
        size.QuadPart > 0 &&
        size.QuadPart < (LONGLONG)capacity;
    DWORD bytes_read = 0;
    if (success) {
        success = ReadFile(file, buffer, (DWORD)size.QuadPart, &bytes_read, NULL) &&
            bytes_read == (DWORD)size.QuadPart;
    }
    CloseHandle(file);
    if (!success) return false;
    *out_len = bytes_read;
    return normalize_key(buffer, capacity, out_len);
}

static bool secure_remove_plaintext(const wchar_t *path) {
    DWORD attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) return GetLastError() == ERROR_FILE_NOT_FOUND;
    if (attributes & FILE_ATTRIBUTE_READONLY) return false;

    HANDLE file = CreateFileW(
        path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_WRITE_THROUGH,
        NULL
    );
    if (file == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size;
    bool success = GetFileSizeEx(file, &size) && size.QuadPart >= 0;
    uint8_t wipe_buffer[CHUNK_SIZE];
    LARGE_INTEGER zero = {0};

    for (int pass = 0; success && pass < 2; pass++) {
        if (!SetFilePointerEx(file, zero, NULL, FILE_BEGIN)) {
            success = false;
            break;
        }
        if (pass == 0) {
            SecureZeroMemory(wipe_buffer, sizeof(wipe_buffer));
        } else if (!crypto_generate_random(wipe_buffer, sizeof(wipe_buffer))) {
            success = false;
            break;
        }

        uint64_t remaining = (uint64_t)size.QuadPart;
        while (remaining > 0) {
            DWORD to_write = remaining > sizeof(wipe_buffer) ? (DWORD)sizeof(wipe_buffer) : (DWORD)remaining;
            DWORD bytes_written = 0;
            if (!WriteFile(file, wipe_buffer, to_write, &bytes_written, NULL) || bytes_written != to_write) {
                success = false;
                break;
            }
            remaining -= bytes_written;
        }
        if (success) success = FlushFileBuffers(file) != 0;
    }

    if (success) {
        if (!SetFilePointerEx(file, zero, NULL, FILE_BEGIN) || !SetEndOfFile(file)) {
            success = false;
        } else {
            success = FlushFileBuffers(file) != 0;
        }
    }

    SecureZeroMemory(wipe_buffer, sizeof(wipe_buffer));
    CloseHandle(file);
    if (!success) return false;
    return DeleteFileW(path) != 0;
}

static bool constant_time_equal(const uint8_t *left, const uint8_t *right, size_t len) {
    uint8_t difference = 0;
    for (size_t i = 0; i < len; i++) difference |= left[i] ^ right[i];
    return difference == 0;
}

bool kiosapi_key_load(
    const wchar_t *password,
    char *out_key,
    size_t out_capacity,
    size_t *out_len,
    wchar_t *out_error,
    size_t error_capacity
) {
    if (out_len) *out_len = 0;
    if (out_error && error_capacity > 0) out_error[0] = L'\0';
    if (!password || !password[0] || !out_key || out_capacity < KIOSAPI_KEY_CAPACITY ||
        !out_len || !crypto_init()) {
        return set_error(out_error, error_capacity, L"Invalid key-unlock request.");
    }

    wchar_t plain_path[MAX_PATH * 2] = {0};
    wchar_t encrypted_path[MAX_PATH * 2] = {0};
    wchar_t temp_path[MAX_PATH * 2] = {0};
    if (!build_key_paths(plain_path, encrypted_path, temp_path, MAX_PATH * 2)) {
        return set_error(out_error, error_capacity, L"Could not locate the application folder.");
    }

    bool has_plain = file_exists(plain_path);
    bool has_encrypted = file_exists(encrypted_path);
    if (!has_plain && !has_encrypted) {
        return set_error(out_error, error_capacity, L"kiosapi-key.txt or kiosapi-key.wkey was not found beside the application.");
    }
    if (file_exists(temp_path)) {
        return set_error(out_error, error_capacity, L"A stale kiosapi-key.wkey.tmp file exists. Remove it before continuing.");
    }

    uint8_t plaintext[KIOSAPI_KEY_CAPACITY] = {0};
    uint8_t decrypted[KIOSAPI_KEY_CAPACITY] = {0};
    uint8_t file_salt[SALT_SIZE] = {0};
    uint8_t file_key[KEY_SIZE_AES256] = {0};
    size_t plaintext_len = 0;
    size_t decrypted_len = 0;
    bool success = false;

    if (has_plain && !read_plain_key(plain_path, plaintext, sizeof(plaintext), &plaintext_len)) {
        set_error(out_error, error_capacity, L"The plaintext API key file is empty, too large, or contains invalid characters.");
        goto cleanup;
    }

    if (has_plain && !has_encrypted) {
        if (!crypto_generate_random(file_salt, sizeof(file_salt)) ||
            !crypto_derive_key(password, file_salt, sizeof(file_salt), file_key)) {
            set_error(out_error, error_capacity, L"Could not derive the encryption key.");
            goto cleanup;
        }

        if (!crypto_encrypt_file(plain_path, temp_path, file_key, file_salt, CIPHER_AES_256_GCM, NULL, NULL, NULL) ||
            !crypto_decrypt_file_to_memory(temp_path, file_key, decrypted, sizeof(decrypted), &decrypted_len) ||
            !normalize_key(decrypted, sizeof(decrypted), &decrypted_len) ||
            decrypted_len != plaintext_len ||
            !constant_time_equal(plaintext, decrypted, plaintext_len)) {
            DeleteFileW(temp_path);
            set_error(out_error, error_capacity, L"Could not verify the encrypted API key; the plaintext file was left unchanged.");
            goto cleanup;
        }

        if (!MoveFileExW(temp_path, encrypted_path, MOVEFILE_WRITE_THROUGH)) {
            DeleteFileW(temp_path);
            set_error(out_error, error_capacity, L"Could not commit the encrypted API key; the plaintext file was left unchanged.");
            goto cleanup;
        }
        has_encrypted = true;
    }

    if (has_plain && has_encrypted) {
        FileHeader header;
        HANDLE encrypted_file = CreateFileW(encrypted_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        DWORD bytes_read = 0;
        bool header_ok = encrypted_file != INVALID_HANDLE_VALUE &&
            ReadFile(encrypted_file, &header, sizeof(header), &bytes_read, NULL) &&
            bytes_read == sizeof(header) &&
            memcmp(header.magic, HEADER_MAGIC, sizeof(header.magic)) == 0 &&
            header.version == HEADER_VERSION;
        if (encrypted_file != INVALID_HANDLE_VALUE) CloseHandle(encrypted_file);
        if (!header_ok || !crypto_derive_key(password, header.salt, sizeof(header.salt), file_key) ||
            !crypto_decrypt_file_to_memory(encrypted_path, file_key, decrypted, sizeof(decrypted), &decrypted_len) ||
            !normalize_key(decrypted, sizeof(decrypted), &decrypted_len) ||
            decrypted_len != plaintext_len ||
            !constant_time_equal(plaintext, decrypted, plaintext_len)) {
            set_error(out_error, error_capacity, L"Both key files exist but do not match or the password is incorrect. Neither file was changed.");
            goto cleanup;
        }
        if (!secure_remove_plaintext(plain_path)) {
            set_error(out_error, error_capacity, L"The encrypted key is valid, but the plaintext file could not be removed. The API key remains locked.");
            goto cleanup;
        }
    } else if (has_encrypted) {
        FileHeader header;
        HANDLE encrypted_file = CreateFileW(encrypted_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        DWORD bytes_read = 0;
        bool header_ok = encrypted_file != INVALID_HANDLE_VALUE &&
            ReadFile(encrypted_file, &header, sizeof(header), &bytes_read, NULL) &&
            bytes_read == sizeof(header) &&
            memcmp(header.magic, HEADER_MAGIC, sizeof(header.magic)) == 0 &&
            header.version == HEADER_VERSION;
        if (encrypted_file != INVALID_HANDLE_VALUE) CloseHandle(encrypted_file);
        if (!header_ok || !crypto_derive_key(password, header.salt, sizeof(header.salt), file_key) ||
            !crypto_decrypt_file_to_memory(encrypted_path, file_key, decrypted, sizeof(decrypted), &decrypted_len) ||
            !normalize_key(decrypted, sizeof(decrypted), &decrypted_len)) {
            set_error(out_error, error_capacity, L"Could not unlock the encrypted API key. Check the master password and key file.");
            goto cleanup;
        }
        plaintext_len = decrypted_len;
        memcpy(plaintext, decrypted, decrypted_len);
    }

    if (plaintext_len == 0 || plaintext_len >= out_capacity) {
        set_error(out_error, error_capacity, L"The API key is empty or exceeds the supported size.");
        goto cleanup;
    }
    memcpy(out_key, plaintext, plaintext_len);
    out_key[plaintext_len] = '\0';
    *out_len = plaintext_len;
    success = true;

cleanup:
    if (!success) {
        SecureZeroMemory(out_key, out_capacity);
        *out_len = 0;
    }
    SecureZeroMemory(plaintext, sizeof(plaintext));
    SecureZeroMemory(decrypted, sizeof(decrypted));
    SecureZeroMemory(file_salt, sizeof(file_salt));
    SecureZeroMemory(file_key, sizeof(file_key));
    return success;
}
