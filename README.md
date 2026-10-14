# Win10 File Encryptor & Bulk Renamer

![Win10 File Encryptor & Bulk Renamer Banner](assets/banner.png)

<p align="center">
  <a href="README.md"><strong>English</strong></a> &bull;
  <a href="README.en.md"><strong>Bahasa Indonesia</strong></a> (juga di <a href="README.id.md">README.id.md</a>)
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20(64--bit)-0078D6?logo=windows" alt="Platform" />
  <img src="https://img.shields.io/badge/Language-Pure%20C%20(C99)-A8B9CC?logo=c" alt="Language" />
  <img src="https://img.shields.io/badge/Cryptography-Windows%20CNG%20(AES--256--GCM)-2ea44f" alt="Crypto" />
  <img src="https://img.shields.io/badge/Key%20Storage-DPAPI%20%2B%20PBKDF2-blueviolet" alt="Key Storage" />
  <img src="https://img.shields.io/badge/Renamer-Advanced%20Renamer%20Style-orange" alt="Renamer" />
  <img src="https://img.shields.io/badge/Dependencies-Zero%20External%20DLLs-brightgreen" alt="Dependencies" />
</p>

---

## 📖 Overview

**Win10 File Encryptor & Bulk Renamer** is a high-performance, native Windows utility suite engineered in pure **ISO C99**. It combines military-grade authenticated encryption (**AES-256-GCM** via Windows CNG), background daemon services for automatic decryption and 30-minute inactivity auto-encryption, and a versatile **Bulk File Renamer** modeled after Advanced Renamer with real-time live preview and collision detection.

Everything compiles to a standalone, zero-dependency executable linking directly to Windows core DLLs.

> 📚 **Complete Documentation:**
> - **User Guide:** [English](docs/USER_GUIDE.md) | [Bahasa Indonesia](docs/USER_GUIDE.id.md)
> - **Developer Guide:** [English](docs/DEVELOPER_GUIDE.md) | [Bahasa Indonesia](docs/DEVELOPER_GUIDE.id.md)

---

## 🌟 Key Features

### 1. 🔐 Security Vault & Master Authentication
- **Windows DPAPI Vault**: Master password hashes and recovery email salts are protected on disk using the Windows Data Protection API (`CryptProtectData`).
- **5-Attempt Lockout & 6-Digit OTP**: Automatic account lockout triggered on 5 consecutive invalid login attempts, with time-limited OTP code verification.
- **Instant Keyboard Submit**: Enter key directly submits credentials with auto-focus on the password input field.
- **Email-Salted PBKDF2**: 100,000 rounds of PBKDF2-HMAC-SHA256 utilizing unique per-file salts and the user's recovery email address.

### 2. 🌐 Multi-Language Support (i18n)
- **English Default**: Launches in English by default across all dialogs, menus, and table headers.
- **Runtime Language Switching**: Seamlessly toggle between **English** and **Bahasa Indonesia** from the `Language` menu bar without restarting.

### 3. 📂 Encrypt Explorer
- **Two-Pane Explorer Navigation**: Directory treeview on the left and detailed file listview on the right.
- **Safe Directory Browsing**: Clicking folders only inspects contents without triggering batch actions.
- **Multi-Selection Support**: Select multiple items with `Ctrl+Click`, `Shift+Click`, or `Ctrl+A`.
- **Context Menu Actions**: Right-click selected items to invoke **"Encrypt Selected Files"** or **"Decrypt Selected Files"**.
- **Bidirectional Column Sorting**: Click any column header (*Name*, *Type*, *Size*, *Status*, *Path*) with native visual ascending/descending arrows (`HDF_SORTUP` / `HDF_SORTDOWN`).
- **Real-Time Encryption Status**: Plainly displays whether files are **[Decrypted]** or **[Encrypted]**.

### 4. ⏱️ 30-Minute Inactivity Auto-Lock & Auto-Re-Encryption
- **Activity Monitoring**: Files decrypted during user sessions are recorded in a persistent tracker (`win10_file_enc_tracker.dat`).
- **Auto-Re-Encryption**: Decrypted files left untouched for 30 minutes are automatically re-encrypted into `.enc` containers and the plaintext versions are shredded.
- **Crash & Reboot Reconciliation**: Upon launch or reboot, any leftover decrypted files from unexpected shutdowns are automatically identified and re-encrypted before user interaction.
- **Full Exit Protection**: Closing the application completely immediately re-encrypts all open decrypted files.

### 5. 🛡️ Strict Single Instance & Clean Window Exit
- **Single Instance Enforcement**: Uses a named Windows Mutex (`Win10FileMod_App_SingleInstance_Mutex`) to ensure strictly one application instance runs at a time.
- **Auto Window Restoration**: If a second instance is launched, the existing window is automatically restored and brought to the foreground.
- **Clean Taskbar Exit (No Duplicate Icons)**: Closing the main window ([X] / Close) cleanly terminates the GUI and removes its tray icon, preventing duplicate tray icon clutter in the taskbar notification area while allowing the background daemon service to operate independently if registered.

### 6. ⚙️ Background Service & Shell Integration
- **Explorer Folder Auto-Decryption**: Background monitor (`IShellWindows` COM) detects navigation to protected folders in Windows Explorer and auto-decrypts them seamlessly.
- **Native File Association**: Double-clicking `.enc` files in File Explorer triggers instant single-click decryption.

### 7. 🧹 Atomic Physical File Shredder
- **Multi-Pass Overwrite**: Zero-fill (`0x00`) pass followed by cryptographically random byte pass (`crypto_generate_random`).
- **Hardware Controller Flush**: Executes `FlushFileBuffers` to commit changes directly to SSD/HDD controllers before unlinking.

### 8. 🏷️ Bulk File Renamer (Advanced Renamer Style)
- **5 Modular Renaming Methods**:
  - **Find & Replace**: Substring replacement.
  - **Add (Prefix / Suffix)**: Text insertion before or after filenames.
  - **Pattern & Tags**: Numbering patterns supporting `<name>`, `<inc:1>`, `<inc:01>`, `<inc:001>` with custom steps.
  - **Case Conversion**: Lowercase, UPPERCASE, and Title Case.
  - **Trim**: Character count stripping and whitespace normalization.
- **Real-Time Live Preview**: The *"New File Name"* column updates synchronously as you type.
- **Collision Detection**: Pre-execution collision scanning warns against duplicate or conflicting file names.
- **Multi-Step Undo**: One-click rollback restores the previous file names using the undo stack.

### 9. 🔍 Duplicate File Finder
- **Fast & 100% Accurate 2-Tier Engine**:
  - **Tier 1 (Rapid Size Grouping)**: Traverses directory (with recursive subfolder toggle) and groups files by file size. Files with unique sizes are skipped immediately without disk reads, drastically reducing I/O operations.
  - **Tier 2 (SHA-256 Content Verification)**: Files sharing identical sizes are hashed and verified byte-for-byte using cryptographic **SHA-256** (via Windows CNG `bcrypt.dll`). Only files with 100% identical hashes are clustered into duplicate groups.
- **Interactive Checkbox ListView**: Displays *File Name*, *Status (Original/Copy)*, *Size*, *Date Modified*, and *Full Path*.
- **Auto-Select Copies**: One-click intelligence button that checks all duplicate copies while safely preserving one original file per group.
- **Safe Recycling via Windows Recycle Bin**: Moves checked duplicate copies to the Recycle Bin using `SHFileOperationW` (`FOF_ALLOWUNDO`) so they can be restored if needed, accompanied by confirmation dialog showing exact disk space saved.
- **Full Right-Click Context Menu**: Open file, show & highlight file in Windows Explorer, toggle checks, or delete individual files.

---

## 📁 Repository Structure

```
win10-file-mod/
├── assets/                         # Application icon, visual manifest, and banners
│   ├── app.ico                     # Multi-resolution native icon (16x16 to 256x256)
│   ├── app.manifest                # Common Controls v6 & Per-Monitor DPI Awareness
│   ├── app_logo.jpg                # Logo graphic
│   └── banner.png                  # Widescreen high-resolution banner
│
├── docs/                           # Documentation
│   ├── USER_GUIDE.md               # User guide (English)
│   ├── USER_GUIDE.id.md            # User guide (Bahasa Indonesia)
│   ├── DEVELOPER_GUIDE.md          # Developer guide (English)
│   └── DEVELOPER_GUIDE.id.md       # Developer guide (Bahasa Indonesia)
│
├── include/                        # C header declarations
│   ├── common.h                    # Core constants and shared structures
│   ├── crypto.h                    # CNG AES-256-GCM and PBKDF2 API
│   ├── duplicate.h                 # SHA-256 duplicate finder engine & Recycle Bin API
│   ├── i18n.h                      # Multi-language string tables and API
│   ├── renamer.h                   # Renamer structures, collision detection, and undo
│   ├── scanner.h                   # Directory traversal walker
│   ├── service.h                   # Daemon monitor, inactivity tracker, and shell verbs
│   ├── shredder.h                  # Hardware multi-pass shredder API
│   ├── ui.h                        # Win32 window classes, message pumps, and controls
│   └── vault.h                     # DPAPI credentials, lockout, and OTP engine
│
├── src/                            # C implementation files
│   ├── crypto.c                    # CNG BCrypt encryption, decryption, and CSPRNG
│   ├── duplicate.c                 # 2-tier duplicate finder engine (sizing + SHA-256)
│   ├── i18n.c                      # Localized string lookup tables
│   ├── main.c                      # WinMain entry point, CLI parser, and single instance
│   ├── renamer.c                   # Advanced Renamer methods, tag parser, and undo
│   ├── scanner.c                   # Recursive FindFirstFileW / FindNextFileW
│   ├── service.c                   # Shell integration, COM monitor, and inactivity engine
│   ├── shredder.c                  # Hardware flush and file wipe routines
│   ├── ui.c                        # Win32 UI, Explorer listview, column sorting, tray
│   ├── vault.c                     # DPAPI CryptProtectData vault and OTP generator
│   └── resources.rc                # GNU windres resource script
│
├── tests/                          # Automated test suites
│   ├── test_crypto.c               # 9 suites verifying CNG AES-GCM, salt, and shredding
│   ├── test_duplicate.c            # Duplicate group clustering, SHA-256, and auto-select
│   ├── test_i18n.c                 # 100% key coverage verification across String IDs
│   ├── test_renamer.c              # Renamer methods, tag patterns, and collision detection
│   └── test_service.c              # Auto-decrypt, tracker persistence, and reconciliation
│
├── build.bat                       # MinGW-w64 GCC build script (1-click compile)
├── Makefile                        # GNU Make build file
├── README.md                       # Project overview (English)
├── README.en.md                    # Project overview (Bahasa Indonesia - as requested)
└── README.id.md                    # Project overview (Bahasa Indonesia - standard alias)
```

---

## 🛠️ Build & Installation

### Prerequisites
- **Operating System:** Windows 10 or Windows 11 (64-bit).
- **Compiler:** MinGW-w64 GCC (with `x86_64-w64-mingw32`).

### 1-Click Build Script
Run the batch file in Windows Command Prompt or PowerShell:
```cmd
build.bat
```

### Building with Make
```cmd
make
```

### Manual Compilation Command
```cmd
windres src\resources.rc -O coff -o src\resources.o
gcc -O2 -Wall -Wextra -municode -mwindows -Iinclude ^
    src\main.c src\crypto.c src\vault.c src\scanner.c src\shredder.c src\renamer.c src\duplicate.c src\i18n.c src\service.c src\ui.c src\resources.o ^
    -lbcrypt -lcomctl32 -lshell32 -lole32 -loleaut32 -luuid -lshlwapi ^
    -o win10_file_enc.exe
```

---

## 🧪 Automated Testing

Five automated test suites are included to verify functionality without launching the GUI:

```cmd
# 1. Cryptography & Shredder tests (9 suites)
gcc -O2 -Iinclude tests\test_crypto.c src\crypto.c src\shredder.c -lbcrypt -o tests\test_crypto.exe
tests\test_crypto.exe

# 2. Renamer engine & Collision tests
gcc -O2 -Iinclude tests\test_renamer.c src\renamer.c -o tests\test_renamer.exe
tests\test_renamer.exe

# 3. Background service & Inactivity tracker tests
gcc -O2 -Iinclude tests\test_service.c src\service.c src\crypto.c src\vault.c src\shredder.c -lbcrypt -lshlwapi -lole32 -loleaut32 -luuid -o tests\test_service.exe
tests\test_service.exe

# 4. Multi-language (i18n) coverage tests
gcc -O2 -Iinclude tests\test_i18n.c src\i18n.c -o tests\test_i18n.exe
tests\test_i18n.exe

# 5. Duplicate File Finder tests (SHA-256 & Auto-Select)
gcc -O2 -Iinclude tests\test_duplicate.c src\duplicate.c -lbcrypt -lshell32 -o tests\test_duplicate.exe
tests\test_duplicate.exe
```

---

## 💻 Command-Line Interface (CLI)

`win10_file_enc.exe` supports automated headless workflows:

| Parameter | Description |
|---|---|
| *(none)* | Launches the full Win32 GUI application. |
| `--service` | Runs the headless background daemon (inactivity tracker & Explorer monitor). |
| `--auto-decrypt <path>` | Decrypts the specified `.enc` file and launches its native default application. |
| `--install-service` | Registers Explorer shell verbs and autostart registry entries in Windows. |
| `--uninstall-service` | Cleans up and uninstalls all registry entries. |

---

## 📄 License & Attribution

Developed with pure native C99 for Windows 10/11. Designed for enterprise security, high throughput, and zero runtime overhead.
