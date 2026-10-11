#include "i18n.h"
#include <stddef.h>

static AppLanguage g_CurrentLang = LANG_EN;

/* English translation table (Default) */
static const wchar_t *g_StringsEN[STR_COUNT] = {
    [STR_APP_TITLE]             = L"Win10 File Encryptor & Bulk Renamer",
    [STR_TAB_ENCRYPT]           = L"🔒 Encryption & Explorer",
    [STR_TAB_RENAME]            = L"🏷️ Bulk File Renamer",

    [STR_MENU_FILE]             = L"&File",
    [STR_MENU_OPEN_FOLDER]      = L"&Open Folder...\tCtrl+O",
    [STR_MENU_EXIT]             = L"E&xit\tAlt+F4",
    [STR_MENU_MODES]            = L"&Tools",
    [STR_MENU_LANGUAGE]         = L"🌐 &Language",
    [STR_MENU_LANG_EN]          = L"English (Default)",
    [STR_MENU_LANG_ID]          = L"Bahasa Indonesia",
    [STR_MENU_HELP]             = L"&Help",
    [STR_MENU_GUIDE]            = L"&User Guide",
    [STR_MENU_ABOUT]            = L"&About Application",

    [STR_SETUP_GRP]             = L" 🛡️ Master Security & Recovery Email Setup (Required) ",
    [STR_SETUP_INFO]            = L"For military-grade data security, you must set a Master Password and a Recovery Email. The email is used as your portable encryption key and for password reset.",
    [STR_SETUP_LBL_PASS]        = L"New Master Password (Min 4 chars):",
    [STR_SETUP_LBL_CONFIRM]     = L"Confirm Master Password:",
    [STR_SETUP_LBL_EMAIL]       = L"Recovery Email Address (Used for file encryption key & reset):",
    [STR_SETUP_BTN_SAVE]        = L"Register & Open Application",
    [STR_SETUP_ERR_PASS_LEN]    = L"Master password must be at least 4 characters long!",
    [STR_SETUP_ERR_PASS_MATCH]  = L"Master password confirmation does not match!",
    [STR_SETUP_ERR_EMAIL_INVALID] = L"Please enter a valid recovery email address (e.g. name@domain.com)!",
    [STR_SETUP_SUCCESS]         = L"Master Password and Recovery Email configured successfully!\nPlease remember your recovery email.",

    [STR_LOGIN_GRP]             = L" 🔒 Master Password Authentication ",
    [STR_LOGIN_INFO]            = L"Enter Master Password to unlock folder access & encryption:",
    [STR_LOGIN_LBL_PASS]        = L"Master Password:",
    [STR_LOGIN_CHK_SHOW]        = L"Show Password",
    [STR_LOGIN_LBL_ATTEMPTS]    = L"Failed attempts: %d / %d",
    [STR_LOGIN_BTN_MASUK]       = L"Log In",
    [STR_LOGIN_BTN_LUPA]        = L"Forgot Password",
    [STR_LOGIN_LOCKED_MSG]      = L"ACCOUNT LOCKED!\nYou have reached the limit of 5 failed attempts.\nPlease click 'Forgot Password' to reset via recovery email.",
    [STR_LOGIN_LOCKED_TITLE]    = L"Account Locked",
    [STR_LOGIN_ERR_TITLE]       = L"Authentication Failed",

    [STR_RESET_TITLE]           = L"Password Recovery & Reset via Email",
    [STR_RESET_INFO]            = L"A 6-digit verification code has been dispatched to %ls.\nEnter the code and your new password:",
    [STR_RESET_LBL_OTP]         = L"6-Digit Verification Code:",
    [STR_RESET_LBL_NEWPASS]     = L"New Master Password:",
    [STR_RESET_LBL_CONFIRM]     = L"Confirm New Password:",
    [STR_RESET_BTN_SUBMIT]      = L"Verify & Reset Password",
    [STR_RESET_BTN_RESEND]      = L"Resend Code",
    [STR_RESET_BTN_CANCEL]      = L"Cancel",
    [STR_RESET_SENT_MSG]        = L"A new 6-digit verification code has been generated and dispatched to %ls.",
    [STR_RESET_SUCCESS]         = L"Password successfully reset and account unlocked! You can now log in.",

    [STR_ENC_LBL_FOLDER]        = L"Target Folder:",
    [STR_BTN_BROWSE]            = L"Browse...",
    [STR_CHK_RECURSIVE]         = L"Scan Subfolders",
    [STR_CHK_REMEMBER]          = L"Remember folder",
    [STR_COL_NAME]              = L"Name",
    [STR_COL_SIZE]              = L"Size",
    [STR_COL_MODIFIED]          = L"Date Modified",
    [STR_COL_TYPE]              = L"Type",
    [STR_COL_STATUS]            = L"Status",
    [STR_TREE_THIS_PC]          = L"This PC",
    [STR_TREE_DRIVE_LOCAL]      = L"Local Disk",
    [STR_TREE_DRIVE_REMOVABLE]  = L"Removable Drive",
    [STR_STATUS_READY]          = L"Ready",
    [STR_STATUS_PROCESSING]     = L"Processing encryption & shredding original files...",
    [STR_STATUS_FINISHED]       = L"Completed: %d files encrypted, %d failed.",
    [STR_SESSION_EXPIRED]       = L"Authentication session expired! Please log in again.",
    [STR_WARN_SELECT_FOLDER]    = L"Please select a target folder first!",
    [STR_WARN_INVALID_FOLDER]   = L"Target folder is invalid or could not be found!",

    [STR_REN_GRP_FOLDER]        = L" 1. Target Folder & Scan ",
    [STR_REN_GRP_METHOD]        = L" 2. Renaming Method & Rules ",
    [STR_REN_LBL_METHOD]        = L"Method:",
    [STR_REN_METHOD_REPLACE]    = L"Search & Replace Text",
    [STR_REN_METHOD_ADD]        = L"Add Prefix / Suffix",
    [STR_REN_METHOD_PATTERN]    = L"Numbering Pattern (<inc:01>)",
    [STR_REN_METHOD_CASE]       = L"Change Letter Case",
    [STR_REN_METHOD_TRIM]       = L"Trim / Truncate Characters",
    [STR_REN_LBL_FIND]          = L"Find text:",
    [STR_REN_LBL_REPLACE]       = L"Replace with:",
    [STR_REN_LBL_INSERT]        = L"Insert text:",
    [STR_REN_LBL_POS]           = L"Position:",
    [STR_REN_POS_PREFIX]        = L"Prefix (Start)",
    [STR_REN_POS_SUFFIX]        = L"Suffix (End)",
    [STR_REN_POS_INDEX]         = L"Specific Index",
    [STR_REN_LBL_PATTERN]       = L"Pattern format:",
    [STR_REN_LBL_STARTNUM]      = L"Start number:",
    [STR_REN_LBL_STEP]          = L"Step increment:",
    [STR_REN_LBL_CASERULE]      = L"Case rule:",
    [STR_REN_CASE_UPPER]        = L"UPPERCASE",
    [STR_REN_CASE_LOWER]        = L"lowercase",
    [STR_REN_CASE_TITLE]        = L"Title Case",
    [STR_REN_CASE_SENTENCE]     = L"Sentence case",
    [STR_REN_LBL_TRIMLEN]       = L"Trim length:",
    [STR_REN_LBL_TRIMFROM]      = L"Trim from:",
    [STR_REN_TRIM_START]        = L"From Start (Left)",
    [STR_REN_TRIM_END]          = L"From End (Right)",
    [STR_REN_GRP_PREVIEW]       = L" 3. File Preview & Execution ",
    [STR_REN_COL_ORIGINAL]      = L"Original Filename",
    [STR_REN_COL_PROSPECTIVE]   = L"New Prospective Name",
    [STR_REN_COL_LOCATION]      = L"Location / Status",
    [STR_REN_BTN_APPLY]         = L"⚡ Apply Rename Batch",
    [STR_REN_BTN_UNDO]          = L"↩️ Undo Last Rename",
    [STR_REN_CONFIRM_TITLE]     = L"Confirm Batch Rename",
    [STR_REN_CONFIRM_MSG]       = L"Are you sure you want to rename %zu files according to the preview?",
    [STR_REN_SUCCESS_TITLE]     = L"Rename Complete",
    [STR_REN_SUCCESS_MSG]       = L"Successfully renamed %zu files!",
    [STR_REN_UNDO_NO_HISTORY]   = L"No previous rename history available to undo.",
    [STR_REN_UNDO_CONFIRM]      = L"Are you sure you want to undo the last rename operation?",
    [STR_REN_UNDO_SUCCESS]      = L"Successfully restored %zu files to their original names!",
    [STR_REN_STATUS_SCANNED]    = L"Folder scanned: %zu items found.",

    [STR_ABOUT_TITLE]           = L"About Application",
    [STR_ABOUT_MSG]             = L"Win10 File Encryptor & Bulk Renamer v1.0.0\n\n- AES-256-GCM chunked AEAD file encryption\n- Zero-hardware portable pure email key derivation\n- Multi-pass secure DOD 5220.22-M file shredding\n- Comprehensive bulk file renamer with undo history\n\nDeveloped natively for Windows 10/11.",
    [STR_GUIDE_TITLE]           = L"User Guide",
    [STR_GUIDE_MSG]             = L"User Guide:\n\n1. Encryption & Explorer:\n   - Log in with your Master Password.\n   - Select target directory in the Explorer pane.\n   - Files will be encrypted with AES-256-GCM using your recovery email as key.\n   - Original files are securely shredded.\n\n2. Bulk File Renamer:\n   - Switch to the Bulk Renamer tab.\n   - Choose directory and configure renaming rules.\n   - Review live preview and click 'Apply Rename Batch'.\n   - Click 'Undo' if you need to revert changes.\n\n3. Background Service & Auto-Decrypt:\n   - When registered, opening any .enc file or protected folder in Windows Explorer will automatically decrypt it on-the-fly!",

    /* Service & Explorer Integration */
    [STR_MENU_SERVICE]                  = L"⚡ &Service",
    [STR_SERVICE_STATUS_RUNNING]        = L"Explorer Auto-Decrypt Service: Active (Running)",
    [STR_SERVICE_STATUS_STOPPED]        = L"Explorer Auto-Decrypt Service: Stopped (Inactive)",
    [STR_SERVICE_ENABLE]                = L"▶️ Start Background Service",
    [STR_SERVICE_DISABLE]               = L"⏹️ Stop Background Service",
    [STR_SERVICE_INSTALL_INTEGRATION]   = L"📥 Register Explorer Integration (.enc & auto-decrypt)",
    [STR_SERVICE_UNINSTALL_INTEGRATION] = L"🗑️ Remove Explorer Integration",
    [STR_SERVICE_INTEGRATION_INSTALLED] = L"Explorer integration registered successfully!\nOpening .enc files or protected folders in Explorer will now automatically decrypt them.",
    [STR_SERVICE_INTEGRATION_REMOVED]   = L"Explorer integration has been removed.",
    [STR_SERVICE_VAULT_NOT_SETUP]       = L"Vault has not been configured yet. Please open the main app to configure your recovery email.",
    [STR_SERVICE_DEC_FAIL]              = L"Failed to decrypt protected file!\nInvalid key or data corrupted.",
    [STR_SERVICE_BALLOON_TITLE]         = L"Win10 File Encryptor Service",
    [STR_SERVICE_TRAY_TITLE]            = L"Win10 File Protection Service",
    [STR_SERVICE_AUTO_DECRYPTED_FOLDER] = L"Folder opened: %d protected file(s) automatically decrypted.",
    [STR_SERVICE_AUTO_DECRYPTED_FILE]   = L"Protected file automatically decrypted: %ls",

    /* Explorer Context Menu */
    [STR_EXPLORER_MENU_ENCRYPT_SEL]     = L"🔒 Encrypt Selected File(s)",
    [STR_EXPLORER_MENU_DECRYPT_SEL]     = L"🔓 Decrypt Selected File(s)",
    [STR_EXPLORER_MENU_SELECT_ALL]      = L"Select All\tCtrl+A",

    /* System Tray */
    [STR_TRAY_BALLOON_TITLE]            = L"Win10 File Encryptor",
    [STR_TRAY_BALLOON_MSG]              = L"Application minimized to taskbar/system tray.\nDouble-click this icon to restore the window.",
    [STR_TRAY_TOOLTIP]                  = L"Win10 File Encryptor (Running in Background)",
    [STR_TRAY_MENU_RESTORE]             = L"🖥️ Open Win10 File Encryptor",
    [STR_TRAY_MENU_EXIT]                = L"❌ Exit Application Completely",

    /* Status Labels */
    [STR_STATUS_ENCRYPTED_ENC]          = L"🔒 Encrypted (.enc)",
    [STR_STATUS_DECRYPTED_LOCK]         = L"🔓 Decrypted (Auto-lock 30m)",
    [STR_STATUS_ORIGINAL_FILE]          = L"📄 Original File"
};

/* Indonesian translation table */
static const wchar_t *g_StringsID[STR_COUNT] = {
    [STR_APP_TITLE]             = L"Win10 Enkripsi Berkas & Pengubah Nama Massal",
    [STR_TAB_ENCRYPT]           = L"🔒 Enkripsi & Explorer",
    [STR_TAB_RENAME]            = L"🏷️ Bulk File Renamer",

    [STR_MENU_FILE]             = L"&Berkas",
    [STR_MENU_OPEN_FOLDER]      = L"&Buka Folder...\tCtrl+O",
    [STR_MENU_EXIT]             = L"&Keluar\tAlt+F4",
    [STR_MENU_MODES]            = L"&Moda Alat",
    [STR_MENU_LANGUAGE]         = L"🌐 &Bahasa",
    [STR_MENU_LANG_EN]          = L"English (Default)",
    [STR_MENU_LANG_ID]          = L"Bahasa Indonesia",
    [STR_MENU_HELP]             = L"&Bantuan",
    [STR_MENU_GUIDE]            = L"&Panduan Penggunaan",
    [STR_MENU_ABOUT]            = L"&Tentang Aplikasi",

    [STR_SETUP_GRP]             = L" 🛡️ Pengaturan Keamanan Master & Email Pemulihan (Wajib) ",
    [STR_SETUP_INFO]            = L"Untuk keamanan data militer, Anda diwajibkan membuat Master Password dan mendaftarkan Email Pemulihan. Email ini digunakan untuk kunci enkripsi dan pemulihan password.",
    [STR_SETUP_LBL_PASS]        = L"Master Password Baru (Min 4 karakter):",
    [STR_SETUP_LBL_CONFIRM]     = L"Konfirmasi Master Password:",
    [STR_SETUP_LBL_EMAIL]       = L"Alamat Email Pemulihan (Kunci enkripsi berkas & reset):",
    [STR_SETUP_BTN_SAVE]        = L"Daftarkan & Buka Aplikasi",
    [STR_SETUP_ERR_PASS_LEN]    = L"Master password minimal harus 4 karakter!",
    [STR_SETUP_ERR_PASS_MATCH]  = L"Konfirmasi Master password tidak cocok!",
    [STR_SETUP_ERR_EMAIL_INVALID] = L"Format email tidak valid! Harap masukkan email yang benar (misal: nama@domain.com)",
    [STR_SETUP_SUCCESS]         = L"Master Password dan Email Pemulihan berhasil didaftarkan!\nHarap simpan email Anda dengan baik.",

    [STR_LOGIN_GRP]             = L" 🔒 Autentikasi Master Password ",
    [STR_LOGIN_INFO]            = L"Masukkan Master Password untuk membuka akses folder & enkripsi:",
    [STR_LOGIN_LBL_PASS]        = L"Master Password:",
    [STR_LOGIN_CHK_SHOW]        = L"Lihat Karakter Password",
    [STR_LOGIN_LBL_ATTEMPTS]    = L"Percobaan gagal: %d / %d",
    [STR_LOGIN_BTN_MASUK]       = L"Masuk",
    [STR_LOGIN_BTN_LUPA]        = L"Lupa Password",
    [STR_LOGIN_LOCKED_MSG]      = L"AKUN TERKUNCI!\nAnda telah mencapai batas 5 kali percobaan gagal.\nSilakan klik tombol 'Lupa Password' untuk mereset via email pemulihan.",
    [STR_LOGIN_LOCKED_TITLE]    = L"Akun Terkunci",
    [STR_LOGIN_ERR_TITLE]       = L"Autentikasi Gagal",

    [STR_RESET_TITLE]           = L"Pemulihan & Reset Password via Email",
    [STR_RESET_INFO]            = L"Kode verifikasi 6-digit telah dikirimkan ke %ls.\nMasukkan kode tersebut dan password baru:",
    [STR_RESET_LBL_OTP]         = L"Kode Verifikasi 6-Digit:",
    [STR_RESET_LBL_NEWPASS]     = L"Master Password Baru:",
    [STR_RESET_LBL_CONFIRM]     = L"Konfirmasi Master Password:",
    [STR_RESET_BTN_SUBMIT]      = L"Verifikasi & Reset Password",
    [STR_RESET_BTN_RESEND]      = L"Kirim Ulang Kode",
    [STR_RESET_BTN_CANCEL]      = L"Batal",
    [STR_RESET_SENT_MSG]        = L"Kode verifikasi 6-digit baru telah dibuat dan dikirimkan ke %ls.",
    [STR_RESET_SUCCESS]         = L"Password berhasil diubah dan akun telah dibuka kuncinya! Silakan masuk kembali.",

    [STR_ENC_LBL_FOLDER]        = L"Folder Target:",
    [STR_BTN_BROWSE]            = L"Pilih Folder...",
    [STR_CHK_RECURSIVE]         = L"Pindai Sub-folder",
    [STR_CHK_REMEMBER]          = L"Ingat folder",
    [STR_COL_NAME]              = L"Nama Berkas",
    [STR_COL_SIZE]              = L"Ukuran",
    [STR_COL_MODIFIED]          = L"Tanggal Modifikasi",
    [STR_COL_TYPE]              = L"Tipe",
    [STR_COL_STATUS]            = L"Status",
    [STR_TREE_THIS_PC]          = L"Komputer Ini",
    [STR_TREE_DRIVE_LOCAL]      = L"Diska Lokal",
    [STR_TREE_DRIVE_REMOVABLE]  = L"Diska Portabel",
    [STR_STATUS_READY]          = L"Siap",
    [STR_STATUS_PROCESSING]     = L"Memproses enkripsi & pemusnahan file asli...",
    [STR_STATUS_FINISHED]       = L"Proses selesai: %d file terenkripsi, %d gagal.",
    [STR_SESSION_EXPIRED]       = L"Sesi autentikasi telah berakhir! Silakan masuk kembali.",
    [STR_WARN_SELECT_FOLDER]    = L"Silakan pilih folder target terlebih dahulu!",
    [STR_WARN_INVALID_FOLDER]   = L"Folder target tidak valid atau tidak ditemukan!",

    [STR_REN_GRP_FOLDER]        = L" 1. Pemilihan Folder & Pemindaian ",
    [STR_REN_GRP_METHOD]        = L" 2. Metode & Aturan Rename ",
    [STR_REN_LBL_METHOD]        = L"Metode:",
    [STR_REN_METHOD_REPLACE]    = L"Cari & Ganti Teks",
    [STR_REN_METHOD_ADD]        = L"Tambah Teks Depan/Belakang",
    [STR_REN_METHOD_PATTERN]    = L"Pola Penomoran Urut (<inc:01>)",
    [STR_REN_METHOD_CASE]       = L"Ubah Huruf Besar/Kecil",
    [STR_REN_METHOD_TRIM]       = L"Pangkas Karakter (Trim)",
    [STR_REN_LBL_FIND]          = L"Teks yang Dicari:",
    [STR_REN_LBL_REPLACE]       = L"Ganti dengan:",
    [STR_REN_LBL_INSERT]        = L"Teks yang Ditambahkan:",
    [STR_REN_LBL_POS]           = L"Posisi:",
    [STR_REN_POS_PREFIX]        = L"Awalan (Depan)",
    [STR_REN_POS_SUFFIX]        = L"Akhiran (Belakang)",
    [STR_REN_POS_INDEX]         = L"Indeks Posisi",
    [STR_REN_LBL_PATTERN]       = L"Format Pola:",
    [STR_REN_LBL_STARTNUM]      = L"Mulai Angka:",
    [STR_REN_LBL_STEP]          = L"Kelipatan Langkah:",
    [STR_REN_LBL_CASERULE]      = L"Aturan Huruf:",
    [STR_REN_CASE_UPPER]        = L"HURUF BESAR SEMUA",
    [STR_REN_CASE_LOWER]        = L"huruf kecil semua",
    [STR_REN_CASE_TITLE]        = L"Huruf Besar Tiap Kata",
    [STR_REN_CASE_SENTENCE]     = L"Huruf Besar Awal Kalimat",
    [STR_REN_LBL_TRIMLEN]       = L"Jumlah Karakter Dipangkas:",
    [STR_REN_LBL_TRIMFROM]      = L"Pangkas Dari:",
    [STR_REN_TRIM_START]        = L"Dari Depan (Kiri)",
    [STR_REN_TRIM_END]          = L"Dari Belakang (Kanan)",
    [STR_REN_GRP_PREVIEW]       = L" 3. Pratinjau Berkas & Eksekusi ",
    [STR_REN_COL_ORIGINAL]      = L"Nama Berkas Asli",
    [STR_REN_COL_PROSPECTIVE]   = L"Nama Baru (Pratinjau)",
    [STR_REN_COL_LOCATION]      = L"Lokasi Direktori / Status",
    [STR_REN_BTN_APPLY]         = L"⚡ Terapkan Rename Massal",
    [STR_REN_BTN_UNDO]          = L"↩️ Batalkan Rename (Undo)",
    [STR_REN_CONFIRM_TITLE]     = L"Konfirmasi Eksekusi Rename",
    [STR_REN_CONFIRM_MSG]       = L"Apakah Anda yakin ingin mengubah nama %zu berkas sesuai pratinjau?",
    [STR_REN_SUCCESS_TITLE]     = L"Rename Selesai",
    [STR_REN_SUCCESS_MSG]       = L"Rename selesai! %zu berkas berhasil diubah namanya.",
    [STR_REN_UNDO_NO_HISTORY]   = L"Tidak ada riwayat rename sebelumnya yang dapat dibatalkan (Undo).",
    [STR_REN_UNDO_CONFIRM]      = L"Apakah Anda ingin membatalkan (Undo) proses rename terakhir?",
    [STR_REN_UNDO_SUCCESS]      = L"Berhasil mengembalikan %zu berkas ke nama semula!",
    [STR_REN_STATUS_SCANNED]    = L"Folder dipindai: %zu berkas ditemukan.",

    [STR_ABOUT_TITLE]           = L"Tentang Aplikasi",
    [STR_ABOUT_MSG]             = L"Win10 File Encryptor & Bulk Renamer v1.0.0\n\n- Enkripsi berkas AES-256-GCM chunked AEAD\n- Kunci portabel berbasis email tanpa ikatan hardware\n- Pemusnahan fisik data asli multi-pass (DOD 5220.22-M)\n- Bulk file renamer cerdas dengan riwayat pembatalan (Undo)\n\nDibuat secara native untuk Windows 10/11.",
    [STR_GUIDE_TITLE]           = L"Panduan Penggunaan",
    [STR_GUIDE_MSG]             = L"Panduan Penggunaan:\n\n1. Enkripsi & Explorer:\n   - Masuk menggunakan Master Password Anda.\n   - Pilih direktori target pada panel Explorer.\n   - Berkas akan dienkripsi dengan AES-256-GCM menggunakan email pemulihan sebagai kunci.\n   - Berkas asli dimusnahkan secara aman.\n\n2. Bulk File Renamer:\n   - Pindah ke tab Bulk File Renamer.\n   - Pilih direktori dan tentukan aturan pengubahan nama.\n   - Periksa pratinjau dan tekan 'Terapkan Rename Massal'.\n   - Tekan 'Batalkan Rename' jika ingin mengembalikan nama berkas.\n\n3. Layanan Latar Belakang & Auto-Decrypt:\n   - Jika integrasi diaktifkan, membuka berkas .enc atau folder terproteksi di Windows Explorer akan otomatis mendekripsinya!",

    /* Service & Explorer Integration */
    [STR_MENU_SERVICE]                  = L"⚡ &Layanan",
    [STR_SERVICE_STATUS_RUNNING]        = L"Layanan Auto-Decrypt Explorer: Aktif (Berjalan)",
    [STR_SERVICE_STATUS_STOPPED]        = L"Layanan Auto-Decrypt Explorer: Nonaktif (Berhenti)",
    [STR_SERVICE_ENABLE]                = L"▶️ Jalankan Layanan Latar Belakang",
    [STR_SERVICE_DISABLE]               = L"⏹️ Hentikan Layanan Latar Belakang",
    [STR_SERVICE_INSTALL_INTEGRATION]   = L"📥 Daftarkan Integrasi Explorer (.enc & auto-decrypt)",
    [STR_SERVICE_UNINSTALL_INTEGRATION] = L"🗑️ Hapus Integrasi Explorer",
    [STR_SERVICE_INTEGRATION_INSTALLED] = L"Integrasi Explorer berhasil didaftarkan!\nMembuka file .enc atau folder terproteksi di Explorer akan otomatis mendekripsinya.",
    [STR_SERVICE_INTEGRATION_REMOVED]   = L"Integrasi Explorer berhasil dihapus.",
    [STR_SERVICE_VAULT_NOT_SETUP]       = L"Vault belum dikonfigurasi. Silakan buka aplikasi utama untuk mengatur email pemulihan.",
    [STR_SERVICE_DEC_FAIL]              = L"Gagal mendekripsi berkas terproteksi!\nKunci tidak valid atau data rusak.",
    [STR_SERVICE_BALLOON_TITLE]         = L"Layanan Win10 File Encryptor",
    [STR_SERVICE_TRAY_TITLE]            = L"Layanan Proteksi Berkas Win10",
    [STR_SERVICE_AUTO_DECRYPTED_FOLDER] = L"Folder dibuka: %d berkas terproteksi berhasil didekripsi otomatis.",
    [STR_SERVICE_AUTO_DECRYPTED_FILE]   = L"Berkas terproteksi berhasil didekripsi otomatis: %ls",

    /* Explorer Context Menu */
    [STR_EXPLORER_MENU_ENCRYPT_SEL]     = L"🔒 Enkripsi berkas yang dipilih",
    [STR_EXPLORER_MENU_DECRYPT_SEL]     = L"🔓 Dekripsi berkas yang dipilih",
    [STR_EXPLORER_MENU_SELECT_ALL]      = L"Pilih Semua\tCtrl+A",

    /* System Tray */
    [STR_TRAY_BALLOON_TITLE]            = L"Win10 File Encryptor",
    [STR_TRAY_BALLOON_MSG]              = L"Aplikasi disembunyikan di taskbar/system tray.\nKlik dua kali ikon ini untuk menampilkan kembali.",
    [STR_TRAY_TOOLTIP]                  = L"Win10 File Encryptor (Berjalan di Latar Belakang)",
    [STR_TRAY_MENU_RESTORE]             = L"🖥️ Buka Win10 File Encryptor",
    [STR_TRAY_MENU_EXIT]                = L"❌ Keluar Sepenuhnya",

    /* Status Labels */
    [STR_STATUS_ENCRYPTED_ENC]          = L"🔒 Terenkripsi (.enc)",
    [STR_STATUS_DECRYPTED_LOCK]         = L"🔓 Didekripsi (Auto-lock 30m)",
    [STR_STATUS_ORIGINAL_FILE]          = L"📄 File Asli"
};

void i18n_init(AppLanguage default_lang) {
    if (default_lang < LANG_COUNT) {
        g_CurrentLang = default_lang;
    } else {
        g_CurrentLang = LANG_EN; /* Default = English */
    }
}

AppLanguage i18n_get_language(void) {
    return g_CurrentLang;
}

void i18n_set_language(AppLanguage lang) {
    if (lang < LANG_COUNT) {
        g_CurrentLang = lang;
    }
}

const wchar_t *i18n_get(StringId id) {
    if (id >= STR_COUNT) return L"";

    if (g_CurrentLang == LANG_ID) {
        const wchar_t *str = g_StringsID[id];
        if (str && str[0] != L'\0') return str;
    }

    /* Fallback / default to English */
    const wchar_t *str = g_StringsEN[id];
    return (str) ? str : L"";
}
