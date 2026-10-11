# Win10 File Encryptor & Bulk Renamer

![Win10 File Encryptor & Bulk Renamer Banner](assets/banner.png)

<p align="center">
  <a href="README.md"><strong>English</strong></a> &bull;
  <a href="README.en.md"><strong>Bahasa Indonesia</strong></a> (juga di <a href="README.id.md">README.id.md</a>)
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20(64--bit)-0078D6?logo=windows" alt="Platform" />
  <img src="https://img.shields.io/badge/Bahasa-Murni%20C%20(C99)-A8B9CC?logo=c" alt="Bahasa" />
  <img src="https://img.shields.io/badge/Kriptografi-Windows%20CNG%20(AES--256--GCM)-2ea44f" alt="Kripto" />
  <img src="https://img.shields.io/badge/Penyimpanan%20Kunci-DPAPI%20%2B%20PBKDF2-blueviolet" alt="Penyimpanan Kunci" />
  <img src="https://img.shields.io/badge/Renamer-Advanced%20Renamer%20Style-orange" alt="Renamer" />
  <img src="https://img.shields.io/badge/Dependensi-Nol%20DLL%20Eksternal-brightgreen" alt="Dependensi" />
</p>

---

## 📖 Ringkasan Proyek

**Win10 File Encryptor & Bulk Renamer** adalah rangkaian utilitas berkas native Windows berkinerja tinggi yang dibangun murni menggunakan standar **ISO C99**. Aplikasi ini menggabungkan enkripsi terotentikasi berstandar militer (**AES-256-GCM** via Windows CNG), layanan daemon latar belakang untuk dekripsi otomatis dan enkripsi ulang otomatis saat inaktivitas 30 menit, serta **Bulk File Renamer** interaktif terinspirasi dari Advanced Renamer dengan pratinjau langsung (*live preview*) dan deteksi konflik nama (*collision detection*).

Semua modul dikompilasi menjadi satu berkas biner mandiri yang sangat ringan tanpa ketergantungan pada runtime eksternal (menautkan langsung ke DLL bawaan Windows).

> 📚 **Dokumentasi Lengkap:**
> - **Panduan Pengguna (User Guide):** [Bahasa Indonesia](docs/USER_GUIDE.id.md) | [English](docs/USER_GUIDE.md)
> - **Panduan Pengembang (Developer Guide):** [Bahasa Indonesia](docs/DEVELOPER_GUIDE.id.md) | [English](docs/DEVELOPER_GUIDE.md)

---

## 🌟 Fitur Utama

### 1. 🔐 Security Vault & Otentikasi Master
- **Windows DPAPI Vault**: Hash kata sandi master dan salt email pemulihan dilindungi pada disk menggunakan Windows Data Protection API (`CryptProtectData`).
- **Penguncian 5 Percobaan & OTP 6 Digit**: Akun otomatis terkunci jika terjadi 5 kali kegagalan login berturut-turut, disertai verifikasi kode OTP dengan batas waktu berlaku 10 menit.
- **Konfirmasi Instan Tombol Enter**: Pengguna dapat langsung menekan tombol Enter pada kolom kata sandi untuk login, dengan fokus otomatis pada kolom input saat aplikasi dibuka.
- **PBKDF2 dengan Salt Email**: 100.000 iterasi algoritma PBKDF2-HMAC-SHA256 menggunakan salt unik per-berkas dan alamat email pemulihan pengguna.

### 2. 🌐 Dukungan Multi-Bahasa (i18n)
- **Bahasa Inggris Bawaan (Default)**: Seluruh kotak dialog, menu, dan judul tabel menggunakan Bahasa Inggris saat pertama kali dijalankan.
- **Penggantian Bahasa Dinamis**: Dapat beralih secara langsung antara **English** dan **Bahasa Indonesia** melalui bilah menu `Language` tanpa perlu me-restart aplikasi.

### 3. 📂 Encrypt Explorer
- **Navigasi Dua Panel**: Treeview direktori di panel kiri dan ListView berkas detail di panel kanan.
- **Penjelajahan Aman**: Mengklik folder hanya menampilkan isi direktori tanpa memicu aksi batch otomatis yang tidak disengaja.
- **Dukungan Pilihan Ganda (Multi-Selection)**: Memilih banyak berkas sekaligus menggunakan `Ctrl+Klik`, `Shift+Klik`, atau tombol pintasan `Ctrl+A`.
- **Menu Konteks Klik Kanan**: Klik kanan berkas yang dipilih untuk memanggil menu **"Encrypt Selected Files"** atau **"Decrypt Selected Files"**.
- **Pengurutan Kolom Dua Arah**: Klik nama kolom mana pun (*Name*, *Type*, *Size*, *Status*, *Path*) dengan panah visual native naik/turun (`HDF_SORTUP` / `HDF_SORTDOWN`).
- **Status Enkripsi Terkini**: Menampilkan status berkas secara jelas: **[Decrypted]** atau **[Encrypted]**.

### 4. ⏱️ Pelacak Inaktivitas 30 Menit & Enkripsi Ulang Otomatis
- **Pemantauan Status Berkas**: Berkas yang didekripsi dicatat dalam basis data pelacak persisten (`win10_file_enc_tracker.dat`).
- **Enkripsi Ulang Otomatis**: Berkas terbuka yang tidak diakses/diedit selama 30 menit akan otomatis dienkripsi kembali menjadi `.enc` dan salinan terbukanya dihancurkan (shred).
- **Rekonsiliasi Startup & Reboot**: Saat aplikasi dinyalakan kembali setelah crash atau restart sistem operasi, seluruh berkas terbuka yang tertinggal akan otomatis diidentifikasi dan langsung dienkripsi sebelum pengguna dapat mengaksesnya.
- **Perlindungan Penutupan Penuh**: Menutup aplikasi secara tuntas akan mengenkripsi seluruh berkas yang masih terbuka di sistem.

### 5. 🔕 Sembunyikan ke Taskbar Tray (Close-to-Tray)
- **Perlindungan Berkelanjutan**: Menekan tombol **X** (tutup) menyembunyikan jendela ke area notifikasi taskbar system tray (`SW_HIDE`), menjaga layanan pemantau tetap aktif di latar belakang.
- **Menu Tray Klik Kanan**: Klik kanan ikon tray untuk membuka jendela kembali (**Open**) atau keluar dari aplikasi secara penuh (**Exit Completely**).

### 6. ⚙️ Layanan Latar Belakang & Integrasi Shell
- **Dekripsi Otomatis Folder Explorer**: Layanan pemantau latar belakang (`IShellWindows` COM) mendeteksi saat pengguna membuka folder terenkripsi di File Explorer Windows dan mendekripsinya secara otomatis.
- **Asosiasi Berkas Native**: Klik ganda pada berkas `.enc` di Windows Explorer langsung memicu proses dekripsi instan.

### 7. 🧹 Penghancur Berkas Fisik (Atomic Secure Shredder)
- **Penimpaan Multi-Pass**: Penimpaan tahap pertama menggunakan byte nol (`0x00`), dilanjutkan tahap kedua menggunakan byte acak kriptografis (`crypto_generate_random`).
- **Flush Buffer Kontroler Perangkat Keras**: Menjalankan fungsi `FlushFileBuffers` untuk memastikan data benar-benar tertulis ke sel chip penyimpanan SSD/HDD sebelum berkas dihapus dari sistem berkas.

### 8. 🏷️ Bulk File Renamer (Model Advanced Renamer)
- **5 Metode Penamaan Komprehensif**:
  - **Find & Replace**: Pencarian dan penggantian teks atau kata.
  - **Add (Prefix / Suffix)**: Penambahan awalan atau akhiran nama berkas.
  - **Pattern & Tags**: Pola penomoran canggih dengan tag `<name>`, `<inc:1>`, `<inc:01>`, `<inc:001>` serta kelipatan angka dinamis.
  - **Ubah Huruf (Case)**: Huruf kecil (*lowercase*), HURUF BESAR (*UPPERCASE*), dan Huruf Judul (*Title Case*).
  - **Trim**: Pemotongan sejumlah karakter dari awal/akhir dan pembersihan spasi ganda.
- **Pratinjau Langsung (Real-Time Live Preview)**: Kolom *"Nama File Baru"* langsung terbarui secara instan saat Anda mengetik konfigurasi metode.
- **Deteksi Tabrakan Nama (Collision Detection)**: Peringatan otomatis sebelum eksekusi jika terdapat nama tujuan yang sama atau berkas yang sudah ada di disk.
- **Riwayat Undo Multi-Langkah**: Batalkan perubahan nama massal dan kembalikan ke nama asli hanya dengan satu klik tombol undo.

---

## 📁 Struktur Direktori Proyek

```
win10-file-mod/
├── assets/                         # Ikon aplikasi, manifes visual, dan banner
│   ├── app.ico                     # Ikon native Windows multi-resolusi
│   ├── app.manifest                # Manifes Common Controls v6 & DPI Awareness
│   ├── app_logo.jpg                # Logo aplikasi
│   └── banner.png                  # Banner beresolusi tinggi untuk dokumentasi
│
├── docs/                           # Dokumentasi teknis & panduan pengguna
│   ├── USER_GUIDE.md               # Panduan Pengguna (Bahasa Inggris)
│   ├── USER_GUIDE.id.md            # Panduan Pengguna (Bahasa Indonesia)
│   ├── DEVELOPER_GUIDE.md          # Panduan Pengembang (Bahasa Inggris)
│   └── DEVELOPER_GUIDE.id.md       # Panduan Pengembang (Bahasa Indonesia)
│
├── include/                        # Header C publik
│   ├── common.h                    # Konstanta umum & struktur terbagi
│   ├── crypto.h                    # API Windows CNG AES-256-GCM & PBKDF2
│   ├── i18n.h                      # Tabel string multi-bahasa & API kendali
│   ├── renamer.h                   # Struktur renamer, deteksi tabrakan & undo
│   ├── scanner.h                   # Penelusur rekursif direktori
│   ├── service.h                   # Daemon monitor, pelacak inaktivitas & registry
│   ├── shredder.h                  # API penghancur berkas perangkat keras
│   ├── ui.h                        # Antarmuka jendela Win32 & kontrol
│   └── vault.h                     # Vault kredensial DPAPI & engine OTP
│
├── src/                            # Berkas sumber kode C
│   ├── crypto.c                    # Enkripsi/dekripsi CNG BCrypt & CSPRNG
│   ├── i18n.c                      # Tabel pencarian string terjemahan
│   ├── main.c                      # Titik masuk WinMain & parser CLI
│   ├── renamer.c                   # Metode renamer, pengurai tag & riwayat undo
│   ├── scanner.c                   # Penelusur FindFirstFileW / FindNextFileW
│   ├── service.c                   # Integrasi shell, monitor COM & pelacak berkas
│   ├── shredder.c                  # Penghapusan aman & pembersihan buffer hardware
│   ├── ui.c                        # UI Win32, ListView Explorer, sorting kolom, tray
│   ├── vault.c                     # Vault DPAPI CryptProtectData & generator OTP
│   └── resources.rc                # Skrip sumber daya GNU windres
│
├── tests/                          # Rangkaian pengujian otomatis
│   ├── test_crypto.c               # 9 rangkaian uji CNG AES-GCM, salt, dan shredder
│   ├── test_i18n.c                 # Verifikasi cakupan 100% kunci String ID
│   ├── test_renamer.c              # Pengujian metode renamer & deteksi tabrakan
│   └── test_service.c              # Pengujian auto-dekripsi, tracker & rekonsiliasi
│
├── build.bat                       # Skrip kompilasi MinGW-w64 GCC (1-klik kompilasi)
├── Makefile                        # Skrip kompilasi GNU Make
├── README.md                       # Ringkasan proyek (Bahasa Inggris)
├── README.en.md                    # Ringkasan proyek (Bahasa Indonesia - sesuai instruksi)
└── README.id.md                    # Ringkasan proyek (Bahasa Indonesia - alias standar)
```

---

## 🛠️ Panduan Kompilasi & Instalasi

### Prasyarat
- **Sistem Operasi:** Windows 10 atau Windows 11 (64-bit).
- **Kompilator:** MinGW-w64 GCC (dengan target `x86_64-w64-mingw32`).

### Kompilasi 1-Klik Menggunakan Skrip Batch
Jalankan file batch melalui Command Prompt atau PowerShell:
```cmd
build.bat
```

### Kompilasi Menggunakan Make
```cmd
make
```

### Perintah Kompilasi Manual
```cmd
windres src\resources.rc -O coff -o src\resources.o
gcc -O2 -Wall -Wextra -municode -mwindows -Iinclude ^
    src\main.c src\crypto.c src\vault.c src\scanner.c src\shredder.c src\renamer.c src\i18n.c src\service.c src\ui.c src\resources.o ^
    -lbcrypt -lcomctl32 -lshell32 -lole32 -loleaut32 -luuid -lshlwapi ^
    -o win10_file_enc.exe
```

---

## 🧪 Pengujian Otomatis (Automated Testing)

Tersedia empat rangkaian pengujian mandiri untuk memvalidasi seluruh fungsionalitas tanpa membuka UI grafis:

```cmd
# 1. Pengujian Kriptografi & Shredder (9 pengujian)
gcc -O2 -Iinclude tests\test_crypto.c src\crypto.c src\shredder.c -lbcrypt -o tests\test_crypto.exe
tests\test_crypto.exe

# 2. Pengujian Mesin Renamer & Deteksi Tabrakan
gcc -O2 -Iinclude tests\test_renamer.c src\renamer.c -o tests\test_renamer.exe
tests\test_renamer.exe

# 3. Pengujian Layanan Latar Belakang & Pelacak Inaktivitas
gcc -O2 -Iinclude tests\test_service.c src\service.c src\crypto.c src\vault.c src\shredder.c -lbcrypt -lshlwapi -lole32 -loleaut32 -luuid -o tests\test_service.exe
tests\test_service.exe

# 4. Pengujian Cakupan Multi-Bahasa (i18n)
gcc -O2 -Iinclude tests\test_i18n.c src\i18n.c -o tests\test_i18n.exe
tests\test_i18n.exe
```

---

## 💻 Antarmuka Baris Perintah (CLI)

`win10_file_enc.exe` mendukung alur kerja otomatis melalui terminal:

| Parameter | Deskripsi |
|---|---|
| *(tanpa parameter)* | Menjalankan aplikasi grafis Win32 secara penuh. |
| `--service` | Menjalankan daemon latar belakang (pelacak inaktivitas & monitor Explorer). |
| `--auto-decrypt <path>` | Mendekripsi berkas `.enc` yang ditentukan dan membukanya di aplikasi default. |
| `--install-service` | Mendaftarkan verb shell Explorer dan startup otomatis di Registry Windows. |
| `--uninstall-service` | Menghapus seluruh pendaftaran registry aplikasi dari sistem. |

---

## 📄 Lisensi & Hak Cipta

Dibangun menggunakan C99 murni untuk Windows 10/11. Didesain untuk keamanan kelas enterprise, kecepatan pemrosesan tinggi, dan tanpa ketergantungan runtime pihak ketiga.
