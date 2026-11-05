#include "common.h"
#include "crypto.h"
#include "scanner.h"
#include "shredder.h"
#include <assert.h>
#include <stdio.h>

static void create_file(const wchar_t *path, const char *data, size_t len) {
    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hFile != INVALID_HANDLE_VALUE);
    DWORD written = 0;
    WriteFile(hFile, data, (DWORD)len, &written, NULL);
    CloseHandle(hFile);
}

int wmain(void) {
    wprintf(L"==========================================================\n");
    wprintf(L"  Running Folder Encryption & Already Encrypted Skip Test \n");
    wprintf(L"==========================================================\n");

    assert(crypto_init() == true);

    wchar_t temp_dir[MAX_PATH];
    wchar_t temp_path[MAX_PATH];
    assert(GetTempPathW(MAX_PATH, temp_path) > 0);
    assert(GetTempFileNameW(temp_path, L"enc_test", 0, temp_dir) != 0);
    assert(DeleteFileW(temp_dir));
    assert(CreateDirectoryW(temp_dir, NULL));

    wchar_t file_plain1[MAX_PATH];
    wchar_t file_plain2[MAX_PATH];
    wchar_t file_already_enc_ext[MAX_PATH];
    wchar_t file_already_enc_header[MAX_PATH];

    swprintf(file_plain1, MAX_PATH, L"%ls\\plain1.txt", temp_dir);
    swprintf(file_plain2, MAX_PATH, L"%ls\\plain2.docx", temp_dir);
    swprintf(file_already_enc_ext, MAX_PATH, L"%ls\\secret.txt.enc", temp_dir);
    swprintf(file_already_enc_header, MAX_PATH, L"%ls\\disguised_encrypted.bin", temp_dir);

    /* 1. Create plain files */
    create_file(file_plain1, "Plain data 1", 12);
    create_file(file_plain2, "Plain data 2", 12);

    /* 2. Create actual encrypted files */
    const wchar_t *email = L"user@test.com";
    uint8_t salt[SALT_SIZE];
    crypto_generate_random(salt, sizeof(salt));
    uint8_t key[KEY_SIZE_AES256];
    assert(crypto_derive_key(email, salt, sizeof(salt), key));

    wchar_t tmp_enc[MAX_PATH];
    swprintf(tmp_enc, MAX_PATH, L"%ls.enc.tmp", file_plain1);
    assert(crypto_encrypt_file(file_plain1, tmp_enc, key, salt, CIPHER_AES_256_GCM, NULL, NULL, NULL));
    assert(shredder_commit_and_wipe_original(file_plain1, tmp_enc, file_already_enc_ext));

    /* Recreate plain1.txt so we have 2 plain files and 1 .enc file */
    create_file(file_plain1, "Plain data 1 regenerated", 24);

    /* Encrypt another file into disguised_encrypted.bin (WENC header, but no .enc extension) */
    wchar_t tmp_enc2[MAX_PATH];
    swprintf(tmp_enc2, MAX_PATH, L"%ls.enc.tmp", file_plain2);
    assert(crypto_encrypt_file(file_plain2, tmp_enc2, key, salt, CIPHER_AES_256_GCM, NULL, NULL, NULL));
    assert(shredder_commit_and_wipe_original(file_plain2, tmp_enc2, file_already_enc_header));

    /* Recreate plain2.docx */
    create_file(file_plain2, "Plain data 2 regenerated", 24);

    /* Now temp_dir contains:
       - plain1.txt (plain)
       - plain2.docx (plain)
       - secret.txt.enc (encrypted by extension and header)
       - disguised_encrypted.bin (encrypted by WENC header only)
    */

    /* Test crypto_is_file_encrypted */
    assert(crypto_is_file_encrypted(file_plain1) == false);
    assert(crypto_is_file_encrypted(file_plain2) == false);
    assert(crypto_is_file_encrypted(file_already_enc_ext) == true);
    assert(crypto_is_file_encrypted(file_already_enc_header) == true);
    wprintf(L"[PASS] crypto_is_file_encrypted correctly identifies plain vs encrypted files.\n");

    /* Test crypto_encrypt_file refuses to encrypt already encrypted files */
    wchar_t dummy_out[MAX_PATH];
    swprintf(dummy_out, MAX_PATH, L"%ls\\dummy.enc.tmp", temp_dir);
    assert(crypto_encrypt_file(file_already_enc_ext, dummy_out, key, salt, CIPHER_AES_256_GCM, NULL, NULL, NULL) == false);
    assert(crypto_encrypt_file(file_already_enc_header, dummy_out, key, salt, CIPHER_AES_256_GCM, NULL, NULL, NULL) == false);
    wprintf(L"[PASS] crypto_encrypt_file refuses to encrypt already encrypted files.\n");

    /* Test scanner_scan in OP_MODE_ENCRYPT_AND_SHRED: must only scan unencrypted files */
    FileList *list = scanner_create();
    assert(list != NULL);
    assert(scanner_scan(temp_dir, false, OP_MODE_ENCRYPT_AND_SHRED, list) == true);

    wprintf(L"Scanned file count for encryption: %zu\n", list->count);
    for (size_t i = 0; i < list->count; i++) {
        wprintf(L" - %ls\n", list->paths[i]);
    }

    assert(list->count == 2);
    /* Verify only plain1.txt and plain2.docx were included */
    for (size_t i = 0; i < list->count; i++) {
        assert(crypto_is_file_encrypted(list->paths[i]) == false);
    }
    wprintf(L"[PASS] scanner_scan cleanly ignored both .enc and WENC header files.\n");
    scanner_free(list);

    /* Clean up */
    DeleteFileW(file_plain1);
    DeleteFileW(file_plain2);
    DeleteFileW(file_already_enc_ext);
    DeleteFileW(file_already_enc_header);
    RemoveDirectoryW(temp_dir);

    wprintf(L"\n>>> ALL TESTS PASSED! <<<\n");
    return 0;
}

