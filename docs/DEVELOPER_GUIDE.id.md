# Win10 File Encryptor & Bulk Renamer - Panduan Pengembang (Developer Guide)

![Banner](../assets/banner.png)

> **Bahasa:** [English](DEVELOPER_GUIDE.md) | [Bahasa Indonesia](DEVELOPER_GUIDE.id.md)

Selamat datang di Panduan Pengembang **Win10 File Encryptor & Bulk Renamer**. Dokumen teknis ini merinci arsitektur, alur pipeline kriptografi, manajemen memori, API subsistem, pola UI Win32 native, konfigurasi build, serta kerangka pengujian otomatis (automated testing) dalam basis kode ini.

---

## Daftar Isi
1. [Arsitektur & Prinsip Desain](#1-arsitektur--prinsip-desain)
2. [Struktur Direktori Kode Sumber](#2-struktur-direktori-kode-sumber)
3. [Subsistem Kriptografi (`crypto.c`, `crypto.h`)](#3-subsistem-kriptografi-cryptoc-cryptoh)
   - [Windows CNG & AES-256-GCM (AEAD)](#windows-cng--aes-256-gcm-aead)
   - [Penurunan Kunci PBKDF2-HMAC-SHA256](#penurunan-kunci-pbkdf2-hmac-sha256)
   - [Format Header & Kontainer Biner](#format-header--kontainer-biner)
4. [Subsistem Security Vault (`vault.c`, `vault.h`)](#4-subsistem-security-vault-vaultc-vaulth)
   - [Proteksi DPAPI Windows](#proteksi-dpapi-windows)
   - [Penguncian Akun & Pengiriman OTP](#penguncian-akun--pengiriman-otp)
5. [Pelacak Inaktivitas & Enkripsi Ulang Otomatis (`service.c`, `service.h`)](#5-pelacak-inaktivitas--enkripsi-ulang-otomatis-servicec-serviceh)
   - [Penyimpanan Status Persisten (`win10_file_enc_tracker.dat`)](#penyimpanan-status-persisten-win10_file_enc_trackerdat)
   - [Mesin Inaktivitas 30 Menit](#mesin-inaktivitas-30-menit)
   - [Rekonsiliasi Saat Startup Setelah Crash & Reboot](#rekonsiliasi-saat-startup-setelah-crash--reboot)
   - [Enkripsi Saat Aplikasi Ditutup Penuh](#enkripsi-saat-aplikasi-ditutup-penuh)
6. [Subsistem Penghancur Berkas Fisik / Shredder (`shredder.c`, `shredder.h`)](#6-subsistem-penghancur-berkas-fisik--shredder-shredderc-shredderh)
7. [Mesin Bulk Renamer (`renamer.c`, `renamer.h`)](#7-mesin-bulk-renamer-renamerc-renamerh)
   - [Pipeline Metode & Pengurai Tag](#pipeline-metode--pengurai-tag)
   - [Deteksi Tabrakan Nama Waktu-Nyata](#deteksi-tabrakan-nama-waktu-nyata)
   - [Tumpukan Undo Multi-Langkah](#tumpukan-undo-multi-langkah)
8. [Subsistem Pencari Berkas Duplikat (`duplicate.c`, `duplicate.h`)](#8-subsistem-pencari-berkas-duplikat-duplicatec-duplicateh)
   - [Arsitektur Deteksi 2-Tier (Ukuran & Hash SHA-256)](#arsitektur-deteksi-2-tier-ukuran--hash-sha-256)
   - [Struktur Data Hasil & Pengelompokan Klaster](#struktur-data-hasil--pengelompokan-klaster)
   - [Logika Auto-Select Salinan Duplikat](#logika-auto-select-salinan-duplikat)
   - [Penghapusan Aman ke Windows Recycle Bin](#penghapusan-aman-ke-windows-recycle-bin)
9. [Arsitektur Win32 UI (`ui.c`, `ui.h`)](#9-arsitektur-win32-ui-uic-uih)
   - [Loop Pesan Jendela & Subclassing](#loop-pesan-jendela--subclassing)
   - [Proteksi Single Instance Mutex](#proteksi-single-instance-mutex)
   - [Siklus Penutupan Bersih Jendela (`WM_CLOSE`)](#siklus-penutupan-bersih-jendela-wm_close)
   - [Pengurutan Kustom ListView & Panah Header](#pengurutan-kustom-listview--panah-header)
10. [Layanan Latar Belakang & Integrasi Shell Explorer (`service.c`)](#10-layanan-latar-belakang--integrasi-shell-explorer-servicec)
    - [Pemantauan COM IShellWindows](#pemantauan-com-ishellwindows)
    - [Asosiasi Berkas Registry Windows](#asosiasi-berkas-registry-windows)
11. [Subsistem Internasionalisasi (`i18n.c`, `i18n.h`)](#11-subsistem-internasionalisasi-i18nc-i18nh)
12. [Sistem Kompilasi (Build) & Pengujian Otomatis](#12-sistem-kompilasi-build--pengujian-otomatis)
    - [Opsi Kompilasi GCC](#opsi-kompilasi-gcc)
    - [Rangkaian Pengujian (Test Suites)](#rangkaian-pengujian-test-suites)

---

## 1. Arsitektur & Prinsip Desain

Aplikasi ini dibangun murni menggunakan standar **ISO C99** yang menargetkan native Win32 API. Prinsip arsitektur utama meliputi:

1. **Nol Dependensi Eksternal**: File biner terkompilasi hanya menautkan (link) DLL bawaan Windows (`bcrypt.dll`, `kernel32.dll`, `user32.dll`, `gdi32.dll`, `shell32.dll`, `comctl32.dll`, `ole32.dll`, `oleaut32.dll`, `shlwapi.dll`, `uuid.dll`, `crypt32.dll`). Tidak memerlukan runtime eksternal (seperti OpenSSL, .NET, atau Electron).
2. **Keamanan Memori Deterministik**: Semua kunci kriptografi, buffer teks asli (plaintext), dan token otentikasi segera dihapus tuntas dari RAM menggunakan `SecureZeroMemory` setelah selesai digunakan untuk mencegah serangan memory scraping atau dump inspection.
3. **Akselerasi Perangkat Keras**: Windows Cryptography Next Generation (CNG) mendeteksi dan memanfaatkan instruksi perangkat keras CPU (`AES-NI`, `CLMUL`) secara otomatis, mencapai kecepatan enkripsi/dekripsi melampaui 1 GB/detik pada prosesor x86_64 modern.
4. **Pemisahan Logika & Tampilan (Decoupling)**: Logika komputasi (kriptografi, shredder, renamer, scanner direktori) terpisah total dari kode antarmuka grafis (UI), memungkinkan otomasi penuh melalui CLI dan pengujian unit tanpa antarmuka grafis.

---

## 2. Struktur Direktori Kode Sumber

```
win10-file-mod/
├── assets/                         # Ikon aplikasi, manifes visual, dan banner
│   ├── app.ico                     # Ikon native multi-resolusi (16x16 hingga 256x256)
│   ├── app.manifest                # Common Controls v6 & Per-Monitor DPI Awareness
│   ├── app_logo.jpg                # Logo grafis aplikasi
│   └── banner.png                  # Banner repositori layar lebar resolusi tinggi
│
├── docs/                           # Dokumentasi
│   ├── USER_GUIDE.md               # Panduan Pengguna (Bahasa Inggris)
│   ├── USER_GUIDE.id.md            # Panduan Pengguna (Bahasa Indonesia)
│   ├── DEVELOPER_GUIDE.md          # Panduan Pengembang Teknis (Bahasa Inggris)
│   └── DEVELOPER_GUIDE.id.md       # Panduan Pengembang Teknis (Bahasa Indonesia)
│
├── include/                        # Header C publik
│   ├── common.h                    # Konstanta inti, definisi tipe, dan batas path
│   ├── crypto.h                    # API Windows CNG AES-256-GCM AEAD & PBKDF2
│   ├── duplicate.h                 # API pencari berkas duplikat SHA-256 & Recycle Bin
│   ├── i18n.h                      # Enumerasi StringId & API kendali multi-bahasa
│   ├── renamer.h                   # Struktur renamer, metode, deteksi tabrakan & undo
│   ├── scanner.h                   # Penelusur direktori Unicode & kontainer FileList
│   ├── service.h                   # Watcher daemon, integrasi shell & API tracker
│   ├── shredder.h                  # API penghancur berkas fisik multi-pass
│   ├── ui.h                        # Antarmuka jendela utama Win32 & ID kontrol
│   └── vault.h                     # Kredensial DPAPI, penguncian & API reset OTP
│
├── src/                            # Berkas implementasi C
│   ├── crypto.c                    # Enkripsi/dekripsi CNG BCrypt, PBKDF2, CSPRNG
│   ├── duplicate.c                 # Mesin pencari duplikat 2-tier (ukuran + SHA-256)
│   ├── i18n.c                      # Tabel lookup string multi-bahasa (EN / ID)
│   ├── main.c                      # Titik masuk WinMain, parser CLI & single instance
│   ├── renamer.c                   # Metode Advanced Renamer, pengurai tag, undo
│   ├── scanner.c                   # Penelusur rekursif FindFirstFileW / FindNextFileW
│   ├── service.c                   # Integrasi shell, daemon IShellWindows, tracker inaktivitas
│   ├── shredder.c                  # Flush buffer perangkat keras & penimpaan multi-pass
│   ├── ui.c                        # Win32 UI, ListView Explorer, sorting kolom, tray
│   ├── vault.c                     # Vault DPAPI CryptProtectData & generator OTP
│   └── resources.rc                # Skrip sumber daya GNU windres
│
├── tests/                          # Rangkaian pengujian unit dan integrasi otomatis
│   ├── test_crypto.c               # 9 rangkaian verifikasi CNG AES-GCM, salt, shred
│   ├── test_duplicate.c            # Pengujian pengelompokan duplikat SHA-256 & auto-select
│   ├── test_i18n.c                 # Cakupan terjemahan 100% pada semua String ID
│   ├── test_renamer.c              # Metode rename, format tag, deteksi tabrakan
│   └── test_service.c              # Auto-dekripsi, tracker inaktivitas, startup recovery
│
├── build.bat                       # Skrip kompilasi MinGW-w64 GCC (1-klik kompilasi)
├── Makefile                        # Skrip kompilasi GNU Make
├── README.md                       # Ringkasan proyek (Bahasa Inggris)
├── README.en.md                    # Ringkasan proyek (Bahasa Indonesia - sesuai instruksi)
└── README.id.md                    # Ringkasan proyek (Bahasa Indonesia - alias standar)
```

---

## 3. Subsistem Kriptografi (`crypto.c`, `crypto.h`)

### Windows CNG & AES-256-GCM (AEAD)
Seluruh proses kriptografi memanfaatkan pustaka Windows Cryptography Next Generation (`bcrypt.dll`).
- **Algoritma**: `BCRYPT_AES_ALGORITHM` dengan chaining mode `BCRYPT_CHAIN_MODE_GCM`.
- **Ukuran Chunk**: Potongan 64 KB (`65,536` byte) untuk mendukung streaming berkas berukuran besar (dari kilobyte hingga terabyte) dengan penggunaan RAM yang sangat minim dan konstan.
- **Nonce/IV**: Nonce acak 12-byte (96-bit) yang dibuat secara kriptografis per chunk menggunakan `BCryptGenRandom`.
- **Tag Otentikasi**: Tag GCM 16-byte (128-bit) dihitung untuk setiap chunk, menjamin integritas ciphertext dan mencegah manipulasi data.

### Penurunan Kunci PBKDF2-HMAC-SHA256
- **Kata Sandi Masukan**: String alamat email pemulihan pengguna (`wchar_t`).
- **Salt**: Salt acak 16-byte (128-bit) yang aman secara kriptografis dibuat khusus per berkas.
- **Iterasi**: 100.000 putaran kalkulasi `BCRYPT_SHA256_ALGORITHM`.
- **Kunci Keluaran**: Kunci simetris master 32-byte (256-bit).

```c
bool crypto_derive_key(
    const wchar_t *email,
    const uint8_t *salt,
    size_t salt_len,
    uint8_t *out_key
);
```

### Format Header & Kontainer Biner
Setiap berkas berformat `.enc` diawali oleh header kontainer biner berukuran 44 byte:

| Offset | Bidang (Field) | Ukuran | Deskripsi |
|---|---|---|---|
| `0x00` | `magic` | 4 byte | Pengenal ajaib (Magic Identifier): ASCII `"W10E"` (`0x45303157`) |
| `0x04` | `version` | 2 byte | Versi format (`0x0002`) |
| `0x06` | `cipher_id` | 2 byte | Pengenal Cipher (`0x0001` = AES-256-GCM) |
| `0x08` | `salt` | 16 byte | Salt acak kriptografis untuk PBKDF2 |
| `0x18` | `auth_tag` | 16 byte | Tag otentikasi integritas global AEAD |
| `0x28` | `original_size` | 8 byte | Ukuran asli berkas sebelum dienkripsi (64-bit unsigned) |
| `0x30` | `payload` | Variabel | Ciphertext data terenkripsi dalam format chunk |

---

## 4. Subsistem Security Vault (`vault.c`, `vault.h`)

### Proteksi DPAPI Windows
Berkas konfigurasi vault (`win10_file_enc_vault.dat`) mengamankan kredensial pengguna di disk menggunakan Windows Data Protection API (`CryptProtectData` dan `CryptUnprotectData` dari `crypt32.dll`):
- Kunci enkripsi terikat secara internal dengan deskriptor keamanan akun Windows lokal pengguna.
- Data yang tersimpan mencakup:
  - Hash PBKDF2 dari Master Password.
  - Alamat email pemulihan yang dienkripsi dengan Windows DPAPI.
  - Penghitung percobaan login yang gagal.
  - Timestamp penguncian akun saat gagal berulang kali.

### Penguncian Akun & Pengiriman OTP
- **Penguncian 5 Percobaan**: Jika `g_FailedAttempts >= 5`, fungsi `vault_verify_password` mengembalikan `false` dan menyetel flag `g_bAccountLocked = true`.
- **Pembuatan OTP**: Menggunakan `BCryptGenRandom` untuk menghasilkan 6 digit angka acak (100000..999999) dengan masa berlaku 10 menit.
- **Audit & Pengiriman Email**: Kode dicatat pada sistem log event lokal dan pipeline dispatch audit aplikasi.

---

## 5. Pelacak Inaktivitas & Enkripsi Ulang Otomatis (`service.c`, `service.h`)

Untuk mengeliminasi risiko keamanan berkas berharga tertinggal dalam kondisi terbuka (plaintext) di penyimpanan disk, modul `service.c` mengelola mesin pelacak berkas:

### Penyimpanan Status Persisten (`win10_file_enc_tracker.dat`)
Setiap kali berkas terproteksi didekripsi (baik melalui File Explorer, klik kanan, atau klik ganda di Encrypt Explorer), path lengkap, tick akses terakhir, waktu wall-clock, dan `FILETIME` dicatat ke berkas `win10_file_enc_tracker.dat`:

```c
typedef struct {
    wchar_t plain_path[MAX_PATH * 2];
    DWORD last_access_tick;
    time_t last_access_time;
    FILETIME last_write_time;
    bool active;
} TrackedDecryptedFile;
```

### Mesin Inaktivitas 30 Menit
1. **Evaluasi Berkala**: Diperiksa setiap 15 detik melalui `WM_TIMER` (`TIMER_AUTO_ENCRYPT_ID = 7001`) pada thread UI utama dan thread pemantau daemon latar belakang.
2. **Pemeriksaan Modifikasi Eksternal**: Menjalankan `GetFileTime` pada `plain_path`. Jika `ftLastWriteTime` lebih baru daripada data sebelumnya, artinya berkas sedang diedit oleh pengguna di aplikasi eksternal (Notepad, Word, Visual Studio), sehingga `last_access_tick` dan `last_access_time` otomatis di-reset.
3. **Pemicu Batas Waktu**: Jika `now - last_access >= 1,800,000 ms` (30 menit):
   - Menjalankan `service_auto_encrypt_single_file(plain_path)`.
   - Menghancurkan (shred) salinan plaintext.
   - Menonaktifkan entri pelacakan dan memperbarui `win10_file_enc_tracker.dat`.
   - Menampilkan notifikasi balon (balloon tip) di taskbar system tray.

### Rekonsiliasi Saat Startup Setelah Crash & Reboot
Ketika aplikasi pertama kali dijalankan (`wWinMain` pada `main.c`, `service_run_daemon` pada `service.c`, dan `WM_CREATE` pada `ui.c`):
- `tracker_init()` membaca daftar dari `win10_file_enc_tracker.dat`.
- `tracker_encrypt_all_decrypted_files()` memeriksa seluruh entri yang tercatat. Jika ada berkas plaintext yang masih tertinggal (akibat pemadaman listrik, crash sistem operasi, atau force kill), berkas tersebut **seketika dienkripsi kembali menjadi `.enc` dan dihancurkan sebelum pengguna dapat berinteraksi**.

### Enkripsi Saat Aplikasi Ditutup Penuh
Ketika aplikasi keluar secara tuntas (`app_exit_completely` atau `WM_DESTROY`):
- `tracker_encrypt_all_decrypted_files()` memastikan seluruh berkas terbuka yang sedang terlacak segera dienkripsi menjadi `.enc` dan di-shred sebelum proses aplikasi diterminasi dari sistem.

---

## 6. Subsistem Penghancur Berkas Fisik / Shredder (`shredder.c`, `shredder.h`)

Menghapus berkas hanya dengan `DeleteFileW` sekadar menghapus pointer entri pada tabel sistem berkas (MFT), menyisakan sektor fisik data utuh pada disk. Modul `shredder.c` menjamin pemusnahan berkas secara permanen:

1. **Pass 1**: Menimpa seluruh sektor berkas dengan nilai byte nol (`0x00`).
2. **Pass 2**: Menimpa seluruh sektor dengan byte acak kriptografis (CSPRNG) yang dihasilkan oleh `crypto_generate_random`.
3. **Hardware Commit**: Memanggil `FlushFileBuffers(hFile)` untuk memaksa cache penulisan fisik pada hard drive / pengontrol SSD menulis data secara permanen ke media magnetik/flash sebelum dihapus.
4. **Pemotongan & Penghapusan**: Mengatur ukuran berkas menjadi 0 byte melalui `SetEndOfFile`, menutup handle, dan menghapusnya menggunakan `DeleteFileW`.

---

## 7. Mesin Bulk Renamer (`renamer.c`, `renamer.h`)

### Pipeline Metode & Pengurai Tag
Modul penamaan massal menerapkan 5 algoritma modular:
1. `METHOD_REPLACE`: Pencarian dan penggantian substring nama berkas.
2. `METHOD_ADD`: Penyisipan teks di awal (awalan) atau akhir (akhiran).
3. `METHOD_PATTERN`: Interpolasi tag canggih yang mendukung `<name>` (nama dasar asli), `<inc:1>` (1, 2, 3), `<inc:01>` (01, 02), `<inc:001>` (001, 002) dengan kenaikan langkah dinamis.
4. `METHOD_CASE`: HURUF BESAR (`_wcsupr`), huruf kecil (`_wcslwr`), dan Title Case (huruf besar pada awal kata).
5. `METHOD_TRIM`: Pemotongan sejumlah karakter dari awal atau akhir beserta pembersihan spasi berlebih.

### Deteksi Tabrakan Nama Waktu-Nyata
Fungsi `renamer_detect_collisions(RenameList *list)` menjalankan pengurutan dan pemindaian $O(N \log N)$ terhadap seluruh nama tujuan yang diusulkan. Jika ditemukan dua nama tujuan yang identik atau jika berkas tujuan sudah ada pada disk, status `is_collision` ditandai `true`, mencegah hilangnya data penting akibat tertimpa secara tidak sengaja.

### Tumpukan Undo Multi-Langkah
Setiap operasi perubahan nama massal dicatat ke dalam linked list `UndoHistory`. Setiap commit batch menyimpan pasangan path lama dan baru. Memanggil `renamer_undo_last` akan melakukan operasi kebalikan secara batch dan menghapus status dari tumpukan undo.

---

## 8. Subsistem Pencari Berkas Duplikat (`duplicate.c`, `duplicate.h`)

Modul `duplicate.c` bertanggung jawab melakukan pemindaian berkas dalam suatu direktori, mengidentifikasi berkas yang memiliki konten identik 100%, dan menyediakan fungsionalitas pembersihan aman ke Windows Recycle Bin.

### Arsitektur Deteksi 2-Tier (Ukuran & Hash SHA-256)
Untuk memproses puluhan ribu berkas secara instan tanpa membebani disk I/O, mesin pencari menerapkan pipeline 2-tahap:
1. **Tier 1 (Penyaringan Ukuran Cepat)**:
   - Modul memanfaatkan `scanner_scan()` untuk menelusuri folder target (opsi rekursif melalui flag `recursive`).
   - Berkas-berkas dikumpulkan dan diurutkan berdasarkan `file_size` ($O(N \log N)$).
   - Berkas yang memiliki ukuran berkas unik (tidak ada berkas lain dengan ukuran persis sama) langsung dilewati tanpa dibaca sama sekali dari disk.
2. **Tier 2 (Verifikasi Konten SHA-256 Kriptografis)**:
   - Untuk berkas-berkas dengan ukuran sama, aplikasi membuka berkas dan menghitung ringkasan kriptografi SHA-256 menggunakan Windows CNG API (`BCryptCreateHash`, `BCryptHashData`, `BCryptFinishHash` dengan algoritma `BCRYPT_SHA256_ALGORITHM`).
   - Berkas dibaca dalam potongan *chunk* 64 KB, sehingga berkas berukuran gigabyte sekalipun dapat di-hash dengan konsumsi RAM yang konstan dan minimal.
   - Hanya berkas dengan ukuran identik DAN hash SHA-256 biner identik yang dikelompokkan ke dalam satu grup duplikat (`DuplicateGroup`).

### Struktur Data Hasil & Pengelompokan Klaster
```c
typedef struct {
    wchar_t path[MAX_PATH_LEN];
    wchar_t filename[256];
    uint64_t file_size;
    FILETIME modified_time;
    uint8_t hash[32];          /* SHA-256 binary hash */
    bool is_duplicate;         /* false = Asli (pertama), true = Salinan */
    bool is_selected;          /* Checked for recycling/deletion */
} DuplicateItem;

typedef struct {
    uint8_t hash[32];
    uint64_t file_size;
    DuplicateItem *items;
    size_t count;
    size_t capacity;
} DuplicateGroup;

typedef struct {
    DuplicateGroup *groups;
    size_t group_count;
    size_t total_duplicates;
    uint64_t reclaimable_bytes;
} DuplicateResults;
```

### Logika Auto-Select Salinan Duplikat
Fungsi `duplicate_auto_select_copies(DuplicateResults *results)` secara otomatis menandai `item->is_selected = true` untuk semua item yang memiliki `is_duplicate == true` (salinan redundan), sementara elemen pertama dalam kelompok (`is_duplicate == false`) selalu dipertahankan tidak dicentang (`is_selected = false`). Ini menjamin pengguna tidak akan secara tidak sengaja menghapus semua salinan dari sebuah berkas.

### Penghapusan Aman ke Windows Recycle Bin
Fungsi `duplicate_delete_item(const wchar_t *path, bool to_recycle_bin)` menggunakan Windows Shell File Operation:
```c
SHFILEOPSTRUCTW sf = {0};
sf.wFunc = FO_DELETE;
sf.pFrom = double_null_path;
sf.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
SHFileOperationW(&sf);
```
Dengan flag `FOF_ALLOWUNDO`, berkas dipindahkan ke Windows Recycle Bin secara native, memungkinkan pemulihan berkas sewaktu-waktu oleh pengguna.

---

## 9. Arsitektur Win32 UI (`ui.c`, `ui.h`)

### Loop Pesan Jendela & Subclassing
- **Kelas Jendela Utama**: `L"Win10FileModMainClass"`.
- **Penanganan Keyboard**:
  - `login_pass_edit_proc`: Melakukan subclassing pada kontrol kotak input kata sandi agar tombol `VK_RETURN` (Enter) memicu login secara langsung.
  - `explorer_list_proc`: Melakukan subclassing pada ListView untuk menangkap pintasan `Ctrl+A` guna memilih seluruh baris secara instan.

### Proteksi Single Instance Mutex
Untuk mencegah terjadinya multiple instances atau konflik konkurensi pada basis data pelacak enkripsi (`win10_file_enc_tracker.dat`), `wWinMain` memanfaatkan Windows Named Mutex:
```c
HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Win10FileMod_App_SingleInstance_Mutex");
if (GetLastError() == ERROR_ALREADY_EXISTS) {
    /* Cari jendela yang sudah berjalan, pulihkan dan fokuskan ke depan */
    HWND hExisting = FindWindowW(L"Win10FileModMainClass", NULL);
    if (hExisting) {
        ShowWindow(hExisting, SW_RESTORE);
        SetForegroundWindow(hExisting);
    }
    CloseHandle(hMutex);
    return 0;
}
```

### Siklus Hide to Taskbar & Pemrosesan Latar Belakang (`WM_CLOSE`)
Ketika pengguna menekan tombol tutup window ([X] / Close) atau shortcut `Alt+F4`:
- Pesan `WM_CLOSE` di-intercept: aplikasi **TIDAK menghentikan thread atau proses** (`g_bCancelRequested` dan `g_bDupCancel` tidak dipicu).
- Pengaturan folder terakhir disimpan, lalu jendela disembunyikan menggunakan `ShowWindow(hWnd, SW_HIDE)`.
- Ikon area notifikasi taskbar diaktifkan melalui `show_main_tray_icon(hWnd, true)`.
- Jika terdapat proses yang sedang berjalan (`g_bIsWorking == true`), notifikasi balon otomatis ditampilkan untuk memberi tahu pengguna bahwa aplikasi tetap aktif dan proses diteruskan di latar belakang.
- **Perutean Notifikasi Selesai**: Saat pesan `WM_APP_FINISHED`, `WM_APP_REN_FINISHED`, atau `WM_APP_DUP_FINISHED` diterima oleh loop pesan, aplikasi memeriksa flag `g_bMainTrayActive`. Jika jendela sedang tersembunyi di tray, aplikasi menampilkan notifikasi balon melalui `Shell_NotifyIconW(NIM_MODIFY, &nid)` tanpa memanggil `MessageBoxW` modal yang dapat memblokir eksekusi.
- **Keluar Sepenuhnya (`app_exit_completely`)**:
  Penghentian aplikasi secara tuntas hanya terjadi bila pengguna memilih *Berkas ➔ Keluar* (`IDM_FILE_EXIT`) atau klik kanan ikon tray *Keluar Sepenuhnya* (`ID_MAINTRAY_EXIT`). Prosedur ini mengamankan seluruh berkas terbuka via `tracker_encrypt_all_decrypted_files()`, menghapus ikon tray, dan memanggil `DestroyWindow(hWnd)`.
- **Deduplikasi Ikon Taskbar**:
  Layanan background daemon (`service.c`) secara rutin memeriksa keberadaan jendela utama menggunakan `FindWindowW(L"Win10FileModMainClass", NULL)`. Jika jendela utama terdeteksi aktif, daemon menekan atau menghapus ikonnya sendiri sehingga tidak pernah muncul dua ikon di taskbar.

### Pengurutan Kustom ListView & Panah Header
- **Notifikasi Klik Kolom**: Ditangkap melalui kode notifikasi `LVN_COLUMNCLICK` dalam `WM_NOTIFY`.
- **Komparator Pengurutan**: `ListView_SortItems(hList, explorer_list_compare_func, lParamSort)`.
- **Panah Pengurutan Visual**: Mengambil header melalui `ListView_GetHeader` dan menerapkan flag native `HDF_SORTUP` / `HDF_SORTDOWN` pada `HDITEMW.fmt`.

---

## 10. Layanan Latar Belakang & Integrasi Shell Explorer (`service.c`)

### Pemantauan COM IShellWindows
Modul `service.c` menjalankan `explorer_monitor_thread` yang secara berkala memeriksa jendela File Explorer yang sedang aktif di Windows:
1. Memanggil `CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_ALL, &IID_IShellWindows, &psw)`.
2. Mengambil objek antarmuka `IWebBrowserApp` dan mengekstrak `LocationURL` menggunakan `PathCreateFromUrlW`.
3. Jika folder yang baru saja dibuka berisi berkas terlindungi `.enc` dan jeda waktu pemanggilan telah terpenuhi, aplikasi secara otomatis memanggil `service_auto_decrypt_folder(folderPath)`.

### Asosiasi Berkas Registry Windows
Fungsi `service_install()` mendaftarkan verb shell ke Registry Windows pengguna:
- `HKCU\Software\Classes\.enc`: Menetapkan nilai default ke `Win10FileEnc.ProtectedFile`.
- `HKCU\Software\Classes\Win10FileEnc.ProtectedFile\shell\open\command`: Mengarahkan ke perintah `"%exe%" --auto-decrypt "%1"`.
- `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`: Mendaftarkan startup otomatis dengan parameter `--service`.

---

## 11. Subsistem Internasionalisasi (`i18n.c`, `i18n.h`)

Seluruh teks antarmuka, item menu, judul kolom tabel, pesan dialog, dan tab dikelola melalui enumerasi `StringId` (total **160 ID string**):
- Bahasa Bawaan (Default): `LANG_EN` (Bahasa Inggris).
- Bahasa yang Didukung: `LANG_EN`, `LANG_ID` (Bahasa Indonesia).
- Penggantian Dinamis: Memanggil fungsi `i18n_set_language` yang dilanjutkan dengan `update_ui_texts(hWnd)` akan memperbarui seluruh teks label, tab, menu, dan kolom secara langsung tanpa perlu me-restart aplikasi.

---

## 12. Sistem Kompilasi (Build) & Pengujian Otomatis

### Opsi Kompilasi GCC
Kompilasi menggunakan MinGW-w64 GCC dengan dukungan Unicode dan subsistem Windows aktif:

```cmd
windres src\resources.rc -O coff -o src\resources.o
gcc -O2 -Wall -Wextra -municode -mwindows -Iinclude ^
    src\main.c src\crypto.c src\vault.c src\scanner.c src\shredder.c src\renamer.c src\duplicate.c src\i18n.c src\service.c src\ui.c src\resources.o ^
    -lbcrypt -lcomctl32 -lshell32 -lole32 -loleaut32 -luuid -lshlwapi ^
    -o win10_file_enc.exe
```

### Rangkaian Pengujian (Test Suites)
Tersedia lima file biner pengujian mandiri tanpa harus memuat antarmuka grafis pengguna:

1. **`tests/test_crypto.exe`**:
   - Menguji keacakan `BCryptGenRandom`, penurunan kunci PBKDF2 dari salt email, enkripsi/dekripsi AES-256-GCM, penghancuran berkas (shredding), serta kegagalan autentikasi tag AEAD jika email pemulihan tidak cocok.
2. **`tests/test_renamer.exe`**:
   - Menguji fitur Cari & Ganti, penambahan awalan/akhiran, pola tag (`<name>`, `<inc:01>`), transformasi kapitalisasi teks, deteksi tabrakan nama, dan riwayat undo.
3. **`tests/test_service.exe`**:
   - Menguji dekripsi otomatis berkas tunggal dan folder, pelacak inaktivitas 30 menit, serialisasi berkas tracker persisten, enkripsi otomatis saat penutupan penuh, serta pendaftaran registry shell Explorer.
4. **`tests/test_duplicate.exe`**:
   - Menguji pemindaian berkas duplikat 2-tier, pengelompokan hash SHA-256 identik, diskriminasi berkas unik, recursive directory traversal, dan penandaan otomatis salinan (*auto-select copies*).
5. **`tests/test_i18n.exe`**:
   - Memvalidasi 100% kelengkapan kunci terjemahan pada seluruh 160 `StringId` untuk Bahasa Inggris dan Bahasa Indonesia.
