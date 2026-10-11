#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "common.h"
#include "crypto.h"
#include "vault.h"
#include "shredder.h"
#include "scanner.h"
#include "service.h"
#include <assert.h>
#include <stdio.h>

static void create_file(const wchar_t *path, const char *content) {
    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(hFile != INVALID_HANDLE_VALUE);
    DWORD written = 0;
    WriteFile(hFile, content, (DWORD)strlen(content), &written, NULL);
    CloseHandle(hFile);
}

static bool check_file_content(const wchar_t *path, const char *expected) {
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    char buf[1024] = {0};
    DWORD br = 0;
    ReadFile(hFile, buf, sizeof(buf) - 1, &br, NULL);
    CloseHandle(hFile);
    return (strcmp(buf, expected) == 0);
}

int wmain(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    wprintf(L"=====================================================\n");
    wprintf(L"  Running Service & Explorer Auto-Decrypt Tests      \n");
    wprintf(L"=====================================================\n");
    fflush(stdout);

    /* 1. Initialize subsystems */
    assert(crypto_init() == true);
    assert(vault_init() == true);
    wprintf(L"[PASS] Crypto & Vault subsystems initialized.\n");
    fflush(stdout);

    /* Ensure vault has configured email */
    if (!vault_is_configured()) {
        assert(vault_setup(L"MasterPass123", L"user@example.com") == true);
        wprintf(L"[INFO] Vault was setup with test credentials.\n");
    }

    wchar_t email[256] = {0};
    assert(vault_get_recovery_email(email, 255) == true);
    wprintf(L"[PASS] Recovery email retrieved for decryption: %ls\n", email);

    /* 2. Test Single File Auto-Decrypt */
    const wchar_t *test_src = L"test_auto_file.txt";
    const wchar_t *test_tmp = L"test_auto_file.txt.enc.tmp";
    const wchar_t *test_enc = L"test_auto_file.txt.enc";
    const char *payload = "Confidential auto-decryption payload for Explorer integration.";

    create_file(test_src, payload);
    assert(GetFileAttributesW(test_src) != INVALID_FILE_ATTRIBUTES);

    uint8_t salt[SALT_SIZE];
    crypto_generate_random(salt, sizeof(salt));
    uint8_t key[KEY_SIZE_AES256];
    assert(crypto_derive_key(email, salt, sizeof(salt), key) == true);
    assert(crypto_encrypt_file(test_src, test_tmp, key, salt, CIPHER_AES_256_GCM, NULL, NULL, NULL) == true);
    assert(shredder_commit_and_wipe_original(test_src, test_tmp, test_enc) == true);

    /* Original must be gone, .enc must exist */
    assert(GetFileAttributesW(test_src) == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(test_enc) != INVALID_FILE_ATTRIBUTES);
    wprintf(L"[PASS] File encrypted to .enc and original securely shredded.\n");

    /* Now test service_auto_decrypt_single_file */
    bool dec_res = service_auto_decrypt_single_file(test_enc, false);
    assert(dec_res == true);

    /* .enc must be gone, original must be restored */
    assert(GetFileAttributesW(test_enc) == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(test_src) != INVALID_FILE_ATTRIBUTES);
    assert(check_file_content(test_src, payload) == true);
    wprintf(L"[PASS] service_auto_decrypt_single_file successfully decrypted and restored original file.\n");

    /* Clean up file */
    DeleteFileW(test_src);

    /* 3. Test Folder Auto-Decrypt */
    CreateDirectoryW(L"test_auto_folder", NULL);
    const wchar_t *f1 = L"test_auto_folder\\doc1.txt";
    const wchar_t *f1_tmp = L"test_auto_folder\\doc1.txt.enc.tmp";
    const wchar_t *f1_enc = L"test_auto_folder\\doc1.txt.enc";

    const wchar_t *f2 = L"test_auto_folder\\doc2.txt";
    const wchar_t *f2_tmp = L"test_auto_folder\\doc2.txt.enc.tmp";
    const wchar_t *f2_enc = L"test_auto_folder\\doc2.txt.enc";

    create_file(f1, "Folder document 1 content");
    create_file(f2, "Folder document 2 content");

    uint8_t salt1[SALT_SIZE], salt2[SALT_SIZE];
    crypto_generate_random(salt1, sizeof(salt1));
    crypto_generate_random(salt2, sizeof(salt2));
    uint8_t key1[KEY_SIZE_AES256], key2[KEY_SIZE_AES256];
    crypto_derive_key(email, salt1, sizeof(salt1), key1);
    crypto_derive_key(email, salt2, sizeof(salt2), key2);

    crypto_encrypt_file(f1, f1_tmp, key1, salt1, CIPHER_AES_256_GCM, NULL, NULL, NULL);
    shredder_commit_and_wipe_original(f1, f1_tmp, f1_enc);

    crypto_encrypt_file(f2, f2_tmp, key2, salt2, CIPHER_AES_256_GCM, NULL, NULL, NULL);
    shredder_commit_and_wipe_original(f2, f2_tmp, f2_enc);

    assert(GetFileAttributesW(f1_enc) != INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(f2_enc) != INVALID_FILE_ATTRIBUTES);
    wprintf(L"[PASS] Folder prepared with 2 encrypted protected files.\n");

    /* Auto decrypt entire folder */
    int count = service_auto_decrypt_folder(L"test_auto_folder");
    assert(count == 2);

    assert(GetFileAttributesW(f1_enc) == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(f2_enc) == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(f1) != INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(f2) != INVALID_FILE_ATTRIBUTES);
    assert(check_file_content(f1, "Folder document 1 content") == true);
    assert(check_file_content(f2, "Folder document 2 content") == true);
    wprintf(L"[PASS] service_auto_decrypt_folder decrypted all protected files in folder.\n");

    assert(tracker_is_file_decrypted(f1) == true);
    assert(tracker_is_file_decrypted(f2) == true);
    wprintf(L"[PASS] Decrypted files successfully tracked in 30-minute inactivity tracker.\n");

    /* Test auto-encryption of all decrypted files */
    int re_enc_count = tracker_encrypt_all_decrypted_files();
    assert(re_enc_count == 2);
    assert(GetFileAttributesW(f1) == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(f1_enc) != INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(f2) == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(f2_enc) != INVALID_FILE_ATTRIBUTES);
    wprintf(L"[PASS] tracker_encrypt_all_decrypted_files re-encrypted all files to .enc upon exit.\n");

    /* Clean up folder */
    DeleteFileW(f1_enc);
    DeleteFileW(f2_enc);
    RemoveDirectoryW(L"test_auto_folder");

    /* 4. Test Registry Integration Installation & Removal */
    assert(service_install() == true);
    assert(service_is_installed() == true);
    wprintf(L"[PASS] service_install registered Explorer shell associations and startup.\n");

    assert(service_uninstall() == true);
    assert(service_is_installed() == false);
    wprintf(L"[PASS] service_uninstall cleaned up registry associations.\n");

    /* Reinstall for user convenience */
    service_install();
    wprintf(L"[PASS] Service shell integration reinstated.\n");

    wprintf(L"\n>>> ALL SERVICE AND AUTO-DECRYPT TESTS PASSED SUCCESSFULLY! <<<\n");
    return 0;
}
