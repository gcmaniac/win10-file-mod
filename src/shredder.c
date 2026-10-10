#include "shredder.h"
#include "crypto.h"

bool shredder_wipe_and_delete(const wchar_t *file_path) {
    if (!file_path || wcslen(file_path) == 0) return false;

    /* Ensure file is not read-only */
    SetFileAttributesW(file_path, FILE_ATTRIBUTE_NORMAL);

    HANDLE hFile = CreateFileW(
        file_path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_WRITE_THROUGH,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        /* If file cannot be opened for write, try deleting directly */
        return DeleteFileW(file_path) ? true : false;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return DeleteFileW(file_path) ? true : false;
    }

    if (fileSize.QuadPart > 0) {
        uint8_t *zero_buf = (uint8_t *)calloc(1, CHUNK_SIZE);
        uint8_t *rand_buf = (uint8_t *)malloc(CHUNK_SIZE);

        if (zero_buf) {
            /* Pass 1: Overwrite with 0x00 */
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
            uint64_t remaining = (uint64_t)fileSize.QuadPart;
            while (remaining > 0) {
                DWORD to_write = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : (DWORD)remaining;
                DWORD written = 0;
                if (!WriteFile(hFile, zero_buf, to_write, &written, NULL) || written == 0) {
                    break;
                }
                remaining -= written;
            }
        }

        if (rand_buf && crypto_generate_random(rand_buf, CHUNK_SIZE)) {
            /* Pass 2: Overwrite with pseudorandom data */
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
            uint64_t remaining = (uint64_t)fileSize.QuadPart;
            while (remaining > 0) {
                DWORD to_write = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : (DWORD)remaining;
                DWORD written = 0;
                if (!WriteFile(hFile, rand_buf, to_write, &written, NULL) || written == 0) {
                    break;
                }
                remaining -= written;
            }
        }

        free(zero_buf);
        free(rand_buf);

        /* Flush hardware write buffers */
        FlushFileBuffers(hFile);

        /* Truncate file to 0 bytes */
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        SetEndOfFile(hFile);
    }

    CloseHandle(hFile);

    /* Delete file from filesystem */
    return DeleteFileW(file_path) ? true : false;
}

bool shredder_commit_and_wipe_original(
    const wchar_t *original_path,
    const wchar_t *temp_enc_path,
    const wchar_t *final_enc_path
) {
    if (!original_path || !temp_enc_path || !final_enc_path) return false;

    /* Atomically rename temporary file to final encrypted file */
    if (!MoveFileExW(temp_enc_path, final_enc_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        return false;
    }

    /* Securely wipe and remove the original unencrypted file */
    return shredder_wipe_and_delete(original_path);
}
