# Win10 File Mod Tool Suite (Enkripsi & Advanced Bulk Renamer)

![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20(64--bit)-blue)
![Language](https://img.shields.io/badge/Language-Pure%20C%20(C99)-informational)
![Cryptography](https://img.shields.io/badge/Crypto-Windows%20CNG%20(AES--256--GCM)-success)
![Renamer](https://img.shields.io/badge/Renamer-Advanced%20Renamer%20Style-orange)
![Binary](https://img.shields.io/badge/Size-%3C%20150%20KB%20(Zero%20Dependencies)-brightgreen)

![Win10 File Mod Logo](assets/app_logo.jpg)

> **Aplikasi utilitas berkas native Windows 10/11 berkinerja tinggi, berkeamanan militer (AEAD), dengan sistem multi-halaman/menu, fitur enkripsi berkas + pemusnahan aman (*Secure Shredding*), dan fitur *Bulk File Renamer* interaktif ala Advanced Renamer lengkap dengan Live Preview dan Collision Detection.**

---

## 🌟 Fitur Utama

### 1. Sistem Menu & Navigasi Multi-Halaman
- **Menu Bar Windows Native**: `Berkas` (Buka Folder, Keluar), `Moda Alat` (Enkripsi `Ctrl+1`, Bulk Renamer `Ctrl+2`), dan `Bantuan`.
- **Tab Control Header**: Berpindah antar alat dengan 1-klik (`🔒 Enkripsi & Shredder` dan `🏷️ Bulk File Renamer`).
- **Antarmuka Native Ringan**: Berbasis Win32 API murni (< 150 KB), tampilan modern Windows 10/11 menggunakan visual manifest Common Controls v6.

---

### 2. Moda 🔒: Enkripsi & Secure Shredder
- **Kriptografi Militer**: Menggunakan **Windows CNG (`bcrypt.dll`)** dengan mode **AES-256-GCM** (Hardware AES-NI, kecepatan > 1 GB/s).
- **Anti-Manipulasi (AEAD)**: Dilengkapi 128-bit Authentication Tag yang mendeteksi dan menolak modifikasi/kerusakan data.
- **PBKDF2-HMAC-SHA256**: 100.000 iterasi dengan 16-byte random salt (`BCryptGenRandom`) unik per file, ditambah opsi *Custom Salt Pepper*.
- **Pemusnahan Berkas Fisik (*Atomic Secure Shred*)**: Berkas asli ditimpa byte `0x00` & acak, dipanggil `FlushFileBuffers` ke controller disk, lalu dihapus permanen.
- **Fitur Dekripsi**: Memulihkan berkas `.enc` menjadi berkas asli dengan memvalidasi password dan tag integritas.

---

### 3. Moda 🏷️: Bulk File Renamer (Terinspirasi Advanced Renamer)
Berdasarkan alur kerja [Advanced Renamer](https://www.advancedrenamer.com/user_guide/gettingstarted):
1. **Pemindaian Berkas & Filter**: Menampilkan seluruh berkas ke dalam tabel Explorer-style (ListView Report mode).
2. **5 Metode Penamaan Komprehensif**:
   - **Ganti Teks (Find & Replace)**: Mencari kata dan menggantinya dengan kata baru (misal: `breath-of-fire-` -> `bof1_`).
   - **Tambah Teks (Prefix / Suffix)**: Menambahkan teks di awal (awalan) atau di akhir (akhiran) sebelum ekstensi.
   - **Pola Penomoran Berurutan (Pattern Tags)**: Format kustom dengan tag `<name>` dan `<inc:01>` / `<inc:001>` dengan padding angka nol dinamis.
   - **Ubah Huruf (Case Conversion)**: Huruf kecil (*lowercase*), Huruf BESAR (*UPPERCASE*), dan Huruf Judul (*Title Case / Capitalize*).
   - **Pangkas Karakter (Trim)**: Memangkas N karakter dari depan/belakang dan merapikan spasi.
3. **Pratinjau Langsung (*Real-Time Live Preview*)**:
   - Kolom *"Nama File Baru"* langsung terupdate secara instan saat Anda mengetik atau mengubah konfigurasi metode.
4. **Deteksi Konflik & Duplikasi (*Collision Detection*)**:
   - Otomatis mendeteksi jika dua berkas akan menghasilkan nama yang sama atau jika berkas dengan nama tersebut sudah ada di disk. Ditandai status peringatan agar tidak terjadi penimpaan file yang tidak disengaja.
5. **Eksekusi di Background Thread & Fitur Undo**:
   - Rename dijalankan secara asinkron tanpa membekukan jendela UI.
   - Tombol **BATALKAN (UNDO)** untuk membalikkan batch rename yang baru saja dilakukan ke nama aslinya dalam satu klik!

---

## 🏗️ Struktur Proyek

```
win10-file-mod/
├── assets/                         # Aset visual & manifest aplikasi
│   ├── app.ico                     # Ikon native Windows
│   └── app.manifest                # Manifest Common Controls v6 & DPI Awareness
│
├── include/                        # Header C modular
│   ├── common.h                    # Konstanta umum & konfigurasi
│   ├── crypto.h                    # Wrapper Windows CNG AES-256-GCM & PBKDF2
│   ├── scanner.h                   # Directory scanner rekursif Unicode
│   ├── shredder.h                  # Secure multi-pass shredder fisik
│   ├── renamer.h                   # Bulk renamer methods, live preview, collision, undo
│   └── ui.h                        # Antarmuka Win32, Tab Control, ListView, menu
│
├── src/                            # Kode sumber C
│   ├── main.c                      # WinMain entry point & message pump
│   ├── crypto.c                    # Implementasi AES-256-GCM chunked AEAD & PBKDF2
│   ├── scanner.c                   # Enumerasi direktori & filter ekstensi
│   ├── shredder.c                  # Timpa data fisik, flush buffer hardware, delete
│   ├── renamer.c                   # Engine rename (replace, prefix, pattern tag, case, trim, undo)
│   ├── ui.c                        # UI multi-tab, layout responsif, live preview handler
│   └── resources.rc                # Resource compiler script untuk ikon & manifest
│
├── tests/                          # Automated Unit & Integration Tests
│   ├── test_crypto.c               # 9 test suites verifikasi enkripsi, shredding, & dekripsi
│   └── test_renamer.c              # Unit test engine bulk renamer (replace, tag, case, collision)
│
├── build.bat                       # Script kompilasi 1-klik (MinGW-w64 GCC)
├── Makefile                        # Build target Make
├── plan.md                         # Dokumen arsitektur awal
└── README.md                       # Dokumentasi proyek (dokumen ini)
```

---

## 🚀 Panduan Kompilasi & Menjalankan

### Cara Kompilasi (1-Klik)
Cukup jalankan script:
```cmd
build.bat
```
Atau menggunakan Make:
```cmd
make
```

Hasil kompilasi executable:
`win10_file_enc.exe` (~145 KB)

### Menjalankan Automated Test Suites
1. **Pengujian Kriptografi**:
   ```cmd
   gcc -O2 -municode -mconsole -Iinclude tests\test_crypto.c src\crypto.c src\scanner.c src\shredder.c -lbcrypt -lcomctl32 -lshell32 -lole32 -lshlwapi -o tests\test_crypto.exe
   tests\test_crypto.exe
   ```
2. **Pengujian Bulk Renamer**:
   ```cmd
   gcc -O2 -municode -mconsole -Iinclude tests\test_renamer.c src\renamer.c -o tests\test_renamer.exe
   tests\test_renamer.exe
   ```

---

## 🖥️ Cara Penggunaan Bulk Renamer

1. Buka [**`win10_file_enc.exe`**](win10_file_enc.exe).
2. Klik tab **"🏷️ 2. Bulk File Renamer (Advanced)"** atau tekan `Ctrl+2`.
3. Klik **"Browse..."** untuk memilih folder target (misalnya `D:\Downloads\Breath of Fire 1 Maps`), lalu klik **"PINDAI BERKAS"**.
4. Seluruh berkas akan muncul di tabel daftar dengan kolom: `#`, `Nama File Asli`, `Nama File Baru (Live Preview)`, `Ukuran`, dan `Status`.
5. Pilih metode yang diinginkan dari menu dropdown:
   - **Ganti Teks**: Isi kata yang ingin dicari dan kata pengganti.
   - **Pola Tag**: Gunakan pola seperti `Map_<inc:01>` untuk membuat `Map_01.webp`, `Map_02.webp`, dst.
   - **Tambah Teks**: Pilih Awalan atau Akhiran.
   - **Ubah Huruf**: Pilih huruf kecil, HURUF BESAR, atau Huruf Judul.
6. Kolom *"Nama File Baru"* langsung berubah secara otomatis (*live preview*) tanpa perlu menekan tombol apa pun.
7. Klik **"JALANKAN BATCH RENAME"**.
8. Jika ada kesalahan, cukup klik **"BATALKAN (UNDO)"** untuk mengembalikan nama file ke semula.
