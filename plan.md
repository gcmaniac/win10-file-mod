# Project Plan: Windows 10 Native File Encryption & Secure Shredder Tool
**Repository:** `e:\c_projects\win10-file-mod`  
**Platform:** Windows 10 / 11 (64-bit Native PE)  
**Language & Toolchain:** Pure C (C99), MinGW-w64 GCC (MSYS2), GNU `windres`  
**Dependencies:** Zero external third-party DLLs (100% Native Windows CNG `bcrypt.dll`, `shell32.dll`, `comctl32.dll`, `user32.dll`)

---

## 1. Project Overview & Objective

Membangun aplikasi desktop Windows native modern dan sangat aman untuk mengenkripsi kumpulan file dalam suatu folder secara otomatis, dengan alur kerja:
1. **Antarmuka Pengguna (Native Win32 GUI)**: Ringan (< 250 KB), tampilan modern dengan manifest Common Controls v6.
2. **Pemilihan Folder**: Input path dengan tombol *Browse Folder* (`SHBrowseForFolderW` / Common Dialog).
3. **Pencarian Berkas (File Scanner)**: Menelusuri seluruh file dalam folder target dengan opsi checklist rekursif subfolder.
4. **Metode Enkripsi Modern**: Standar industri tertinggi **AES-256-GCM** (Authenticated Encryption with Associated Data - AEAD) via Windows CNG Hardware AES-NI, disertai opsi **AES-256-CBC + HMAC-SHA256**.
5. **Manajemen Kunci & Salt**: 
   - Default: Salt kriptografis acak (CSPRNG 16-byte) di-generate otomatis per file dan disimpan pada file header.
   - Derivasi Kunci: **PBKDF2-HMAC-SHA256** (100.000 iterasi).
   - Opsi Salt Tambahan: Input custom salt (pepper) jika user menginginkannya.
6. **Penghapusan File Asli Secara Aman (Atomic Secure Shred)**:
   - File dienkripsi ke file sementara (`.tmp`).
   - Integritas data diverifikasi (Auth Tag AEAD).
   - Rename file sementara menjadi `.enc`.
   - File asli ditimpa secara aman (*overwritten with 0x00 & random bytes*) lalu dihapus permanen (`FlushFileBuffers` + `DeleteFileW`).
7. **Fitur Dekripsi Terintegrasi**: Mengembalikan file `.enc` menjadi file asli dengan memverifikasi password dan tag integritas.

---

## 2. Directory Layout (Struktur Proyek Mengikuti Standar `win10_cc`)

```
win10-file-mod/
├── assets/                         # Ikon aplikasi (.ico), manifest XML
│   ├── app.ico                     # Ikon native Windows
│   └── app.manifest                # Manifest Common Controls v6 (Modern Visual Styles)
│
├── include/                        # Header C modular
│   ├── common.h                    # Konstanta, format header file .enc, struct data
│   ├── crypto.h                    # Windows CNG wrapper (AES-256-GCM, PBKDF2, CSPRNG)
│   ├── scanner.h                   # Directory scanner rekursif (FindFirstFileW/FindNextFileW)
│   ├── shredder.h                  # Secure file wipe & atomic commit logic
│   └── ui.h                        # Win32 GUI window, kontrol, event handlers, worker thread
│
├── src/                            # Implementasi C modular
│   ├── main.c                      # WinMain entry point, message pump, dialog dispatch
│   ├── crypto.c                    # Implementasi AES-256-GCM & PBKDF2 menggunakan bcrypt.dll
│   ├── scanner.c                   # Enumerasi berkas, Unicode Long Path (\\?\), filter .enc
│   ├── shredder.c                  # Overwriting memory buffer, flush hardware cache, delete
│   ├── ui.c                        # Pembuatan jendela Win32, folder dialog, progress bar, worker thread
│   └── resources.rc                # Resource script untuk ikon dan manifest modern UI
│
├── build.bat                       # Script build otomatis (deteksi GCC & windres)
├── Makefile                        # Build target Make
├── plan.md                         # Dokumen rencana arsitektur ini
└── README.md                       # Dokumentasi lengkap, panduan pengguna, & benchmark
```

---

## 3. Spesifikasi Kriptografi & Format File (`.enc`)

### A. Algoritma & Keamanan
1. **AES-256-GCM (Default - Sangat Direkomendasikan)**:
   - **Tingkat Keamanan**: Militer / AEAD (NIST SP 800-38D).
   - **Akselerasi**: Hardware AES-NI bawaan Intel/AMD (kecepatan > 1 GB/s).
   - **Integritas**: Dilengkapi 128-bit Authentication Tag. Jika file rusak atau dimodifikasi oleh malware/penyerang, dekripsi akan menolak membuka file.
2. **Kunci Enkripsi (Key Derivation)**:
   - Menggunakan algoritma **PBKDF2-HMAC-SHA256**.
   - Input: Password pengguna + Salt (16 byte CSPRNG dari `BCryptGenRandom`).
   - Iterasi: 100.000 rounds (mencegah serangan brute-force GPU).

### B. Struktur Header File Terenkripsi
Setiap file `.enc` memiliki header 64-byte:

| Offset | Panjang | Deskripsi |
| :---: | :---: | :--- |
| `0x00` | 4 Byte | Magic Identifier (`"WENC"`) |
| `0x04` | 1 Byte | Versi Format (`0x01`) |
| `0x05` | 1 Byte | Cipher ID (`0x01` = AES-256-GCM, `0x02` = AES-256-CBC-HMAC) |
| `0x06` | 2 Byte | Reserved Flags |
| `0x08` | 16 Byte | Random Cryptographic Salt (untuk PBKDF2) |
| `0x18` | 12 Byte | IV / Nonce (Unik per file) |
| `0x24` | 16 Byte | Original File Extension (UTF-8, null-terminated, misal: `.docx`, `.pdf`) |
| `0x34` | 12 Byte | Reserved Padding (untuk alignment 64-byte) |
| `0x40` ... | N Byte | **Encrypted Payload** (Ciphertext stream) |
| `Akhir` | 16 Byte | **Authentication Tag** (GCM Tag) |

---

## 4. Mekanisme Keamanan "Langsung Hapus" (Atomic Secure Shredding)

Untuk mencegah kehilangan data jika terjadi kegagalan proses di tengah jalan (misal listrik mati saat enkripsi 50%):
1. **Tahap 1: Enkripsi Streaming**  
   File dibaca per *chunk* (64 KB) dan ditulis ke berkas sementara dengan nama `[nama_file].enc.tmp`.
2. **Tahap 2: Verifikasi Integritas**  
   Auth Tag dihitung dan ditulis. Verifikasi bahwa penulisan file selesai tanpa error disk.
3. **Tahap 3: Atomic Rename**  
   Rename `[nama_file].enc.tmp` menjadi `[nama_file].enc`.
4. **Tahap 4: Secure Shredding File Asli**  
   - Buka handle file asli dengan `GENERIC_WRITE`.
   - Timpa seluruh isi file asli dengan byte nol `0x00` dan byte acak `0xFF` / CSPRNG.
   - Panggil `FlushFileBuffers` agar OS dan controller SSD/HDD benar-benar melakukan penulisan fisik ke disk.
   - Hapus file menggunakan `DeleteFileW`.
5. **Tahap 5: Pembersihan RAM**  
   Semua password dan kunci simetris di memori dibersihkan menggunakan `SecureZeroMemory`.

---

## 5. Rancangan Antarmuka Pengguna (Win32 Native GUI)

Jendela utama berukuran 580 x 520 px dengan kontrol:
- **Target Folder**:
  - Edit Box path folder.
  - Tombol `"Browse..."` (membuka pemilih folder Windows native).
  - Checkbox: `[x] Pindai Subfolder (Rekursif)`.
- **Pengaturan Enkripsi**:
  - Dropdown Algoritma: `AES-256-GCM (Direkomendasikan - AEAD)` / `AES-256-CBC + HMAC`.
  - Input Password & Konfirmasi Password (`ES_PASSWORD`) + Checkbox `Tampilkan Password`.
  - Checkbox: `[ ] Gunakan Custom Salt Tambahan (Opsional)`.
  - Input Custom Salt (aktif hanya jika checkbox dicentang).
- **Mode Operasi**:
  - Radio Button: `(•) Enkripsi & Hapus File Asli` vs `( ) Dekripsi File .enc`.
- **Progress & Eksekusi**:
  - Tombol Aksi: `[ MULAI PROSES ]` dan `[ BATAL ]`.
  - Progress Bar Win32 (`msctls_progress32`).
  - Label Status: Status berkas saat ini (`"Memproses: document.pdf (12/85)..."`).
  - List View / Text Log: Menampilkan daftar riwayat berkas yang telah berhasil diproses.

---

## 6. Tahapan Eksekusi (Implementation Steps)

1. **Langkah 1**: Pembuatan infrastruktur proyek & asset (`assets/`, `app.manifest`, `resources.rc`, ikon aplikasi).
2. **Langkah 2**: Implementasi modul kriptografi CNG (`include/crypto.h`, `src/crypto.c`) - PBKDF2, AES-256-GCM chunk streaming, CSPRNG.
3. **Langkah 3**: Implementasi modul file scanner & shredder (`include/scanner.h`, `src/scanner.c`, `include/shredder.h`, `src/shredder.c`).
4. **Langkah 4**: Implementasi GUI Native Win32 (`include/ui.h`, `src/ui.c`, `src/main.c`) dengan worker thread agar UI responsif.
5. **Langkah 5**: Pembuatan `build.bat` dan kompilasi menggunakan GCC MinGW-w64.
6. **Langkah 6**: Pengujian menyeluruh (uji enkripsi berkas, verifikasi integritas, penghapusan aman, dan uji dekripsi).
