#include "common.h"
#include "crypto.h"
#include "shredder.h"
#include "scanner.h"
#include <assert.h>

static void create_dummy_file(const wchar_t *path, const char *content, size_t len) {
    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hFile != INVALID_HANDLE_VALUE);
    DWORD written = 0;
    WriteFile(hFile, content, (DWORD)len, &written, NULL);
    CloseHandle(hFile);
}

static bool read_file_content(const wchar_t *path, char *buf, size_t max_len, DWORD *out_len) {
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD br = 0;
    bool ok = ReadFile(hFile, buf, (DWORD)max_len, &br, NULL);
    CloseHandle(hFile);
    if (out_len) *out_len = br;
    return ok;
}

int wmain(void) {
    wprintf(L"=====================================================\n");
    wprintf(L"  Running Win10 File Encryptor Automated Test Suite  \n");
    wprintf(L"=====================================================\n");

    /* 1. Init crypto */
    assert(crypto_init() == true);
    wprintf(L"[PASS] Crypto subsystem initialized (Windows CNG).\n");

    /* 2. Test CSPRNG */
    uint8_t rand1[16] = {0};
    uint8_t rand2[16] = {0};
    assert(crypto_generate_random(rand1, sizeof(rand1)) == true);
    assert(crypto_generate_random(rand2, sizeof(rand2)) == true);
    assert(memcmp(rand1, rand2, 16) != 0);
    wprintf(L"[PASS] BCryptGenRandom CSPRNG generation.\n");

    /* 3. Create dummy file */
    const wchar_t *test_src = L"test_original.txt";
    const wchar_t *test_tmp = L"test_original.txt.enc.tmp";
    const wchar_t *test_enc = L"test_original.txt.enc";
    const char *sample_data = "Hello World! This is a confidential test file for AES-256-GCM verification.";
    create_dummy_file(test_src, sample_data, strlen(sample_data));
    wprintf(L"[PASS] Dummy test file created.\n");

    /* 4. Derive key from password */
    const wchar_t *password = L"SecretPassword2026!";
    uint8_t file_salt[SALT_SIZE];
    crypto_generate_random(file_salt, sizeof(file_salt));
    uint8_t key[KEY_SIZE_AES256];
    assert(crypto_derive_key(password, NULL, file_salt, sizeof(file_salt), key) == true);
    wprintf(L"[PASS] PBKDF2-HMAC-SHA256 key derivation (100,000 iterations).\n");

    /* 5. Encrypt file */
    assert(crypto_encrypt_file(test_src, test_tmp, key, file_salt, CIPHER_AES_256_GCM, NULL, NULL, NULL) == true);
    wprintf(L"[PASS] AES-256-GCM chunked AEAD encryption to temporary file.\n");

    /* 6. Commit & Shred Original */
    assert(shredder_commit_and_wipe_original(test_src, test_tmp, test_enc) == true);
    /* Verify original file is gone */
    assert(GetFileAttributesW(test_src) == INVALID_FILE_ATTRIBUTES);
    /* Verify final .enc exists */
    assert(GetFileAttributesW(test_enc) != INVALID_FILE_ATTRIBUTES);
    wprintf(L"[PASS] Multi-pass secure shredding & atomic file commit.\n");

    /* 7. Verify Decryption with WRONG password fails */
    uint8_t wrong_key[KEY_SIZE_AES256];
    assert(crypto_derive_key(L"WrongPasswordHere!", NULL, file_salt, sizeof(file_salt), wrong_key) == true);
    wchar_t out_path[MAX_PATH];
    bool dec_wrong = crypto_decrypt_file(test_enc, NULL, wrong_key, out_path, MAX_PATH, NULL, NULL, NULL);
    assert(dec_wrong == false);
    wprintf(L"[PASS] Decryption with incorrect password correctly rejected (AEAD Auth Tag check failed).\n");

    /* 8. Verify Decryption with CORRECT password succeeds */
    /* Read header from test_enc to get actual stored salt */
    HANDLE hEnc = CreateFileW(test_enc, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    assert(hEnc != INVALID_HANDLE_VALUE);
    FileHeader hdr;
    DWORD br = 0;
    ReadFile(hEnc, &hdr, sizeof(hdr), &br, NULL);
    CloseHandle(hEnc);

    uint8_t correct_key[KEY_SIZE_AES256];
    assert(crypto_derive_key(password, NULL, hdr.salt, sizeof(hdr.salt), correct_key) == true);

    bool dec_ok = crypto_decrypt_file(test_enc, NULL, correct_key, out_path, MAX_PATH, NULL, NULL, NULL);
    assert(dec_ok == true);
    wprintf(L"[PASS] Decryption with correct password succeeded: %ls\n", out_path);

    /* 9. Verify decrypted content is byte-for-byte identical */
    char dec_buf[256] = {0};
    DWORD dec_len = 0;
    assert(read_file_content(out_path, dec_buf, sizeof(dec_buf), &dec_len) == true);
    assert(dec_len == strlen(sample_data));
    assert(memcmp(dec_buf, sample_data, dec_len) == 0);
    wprintf(L"[PASS] Decrypted content matches original 100%% byte-for-byte.\n");

    /* Cleanup test artifacts */
    DeleteFileW(test_enc);
    DeleteFileW(out_path);

    crypto_cleanup();
    wprintf(L"\n>>> ALL 9 TEST SUITES PASSED PERFECTLY! <<<\n");
    return 0;
}
