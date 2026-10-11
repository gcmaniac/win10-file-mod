# Win10 File Encryptor & Bulk Renamer - Developer Guide

![Banner](../assets/banner.png)

> **Languages:** [English](DEVELOPER_GUIDE.md) | [Bahasa Indonesia](DEVELOPER_GUIDE.id.md)

Welcome to the **Win10 File Encryptor & Bulk Renamer** Developer Guide. This technical manual details the architecture, cryptographic pipelines, memory management, subsystem APIs, Win32 UI patterns, build configurations, and automated testing frameworks of the codebase.

---

## Table of Contents
1. [Architecture & Design Principles](#1-architecture--design-principles)
2. [Source Code Tree](#2-source-code-tree)
3. [Cryptographic Subsystem (`crypto.c`, `crypto.h`)](#3-cryptographic-subsystem)
   - [Windows CNG & AES-256-GCM (AEAD)](#windows-cng--aes-256-gcm-aead)
   - [PBKDF2-HMAC-SHA256 Key Derivation](#pbkdf2-hmac-sha256-key-derivation)
   - [Header & Binary Container Format](#header--binary-container-format)
4. [Security Vault Subsystem (`vault.c`, `vault.h`)](#4-security-vault-subsystem)
   - [DPAPI Protection](#dpapi-protection)
   - [Lockout & OTP Dispatch](#lockout--otp-dispatch)
5. [Inactivity Tracker & Auto-Re-Encryption (`service.c`, `service.h`)](#5-inactivity-tracker--auto-re-encryption)
   - [Persistent State Storage (`win10_file_enc_tracker.dat`)](#persistent-state-storage)
   - [30-Minute Inactivity Engine](#30-minute-inactivity-engine)
   - [Startup Crash & Reboot Reconciliation](#startup-crash--reboot-reconciliation)
   - [Graceful Shutdown Encryption](#graceful-shutdown-encryption)
6. [Physical File Shredder Subsystem (`shredder.c`, `shredder.h`)](#6-physical-file-shredder-subsystem)
7. [Bulk Renamer Engine (`renamer.c`, `renamer.h`)](#7-bulk-renamer-engine)
   - [Method Pipeline & Tag Parser](#method-pipeline--tag-parser)
   - [Real-Time Collision Detection](#real-time-collision-detection)
   - [Multi-Step Undo Stack](#multi-step-undo-stack)
8. [Win32 UI Architecture (`ui.c`, `ui.h`)](#8-win32-ui-architecture)
   - [Window Message Loop & Subclassing](#window-message-loop--subclassing)
   - [ListView Custom Sorting & Visual Headers](#listview-custom-sorting--visual-headers)
   - [Close-to-Tray (`WM_CLOSE` to `SW_HIDE`)](#close-to-tray-wm_close-to-sw_hide)
9. [Windows Explorer Shell & Daemon Service](#9-windows-explorer-shell--daemon-service)
   - [IShellWindows COM Monitoring](#ishellwindows-com-monitoring)
   - [Registry File Associations](#registry-file-associations)
10. [Internationalization Subsystem (`i18n.c`, `i18n.h`)](#10-internationalization-subsystem)
11. [Build System & Automated Testing](#11-build-system--automated-testing)
   - [GCC Compilation Flags](#gcc-compilation-flags)
   - [Test Suites](#test-suites)

---

## 1. Architecture & Design Principles

The application is written strictly in **pure ISO C99** targeting the native Win32 API. Key architectural tenets:

1. **Zero External Dependencies**: The compiled binary links exclusively to standard Windows system DLLs (`bcrypt.dll`, `kernel32.dll`, `user32.dll`, `gdi32.dll`, `shell32.dll`, `comctl32.dll`, `ole32.dll`, `oleaut32.dll`, `shlwapi.dll`, `uuid.dll`, `crypt32.dll`). No external runtimes (such as OpenSSL, .NET, or Electron) are required.
2. **Deterministic Memory Security**: All cryptographic keys, plaintext memory blocks, and authentication tokens are wiped immediately after use via `SecureZeroMemory` to prevent memory scraping attacks.
3. **Hardware Acceleration**: Windows Cryptography Next Generation (CNG) detects and utilizes CPU hardware instructions (`AES-NI`, `CLMUL`) dynamically, achieving encryption throughput exceeding 1 GB/s on modern multi-core x86_64 processors.
4. **Resilient Decoupling**: Business logic (cryptography, shredding, renaming, scanning) is completely decoupled from UI window code, allowing full headless automation and unit testing.

---

## 2. Source Code Tree

```
win10-file-mod/
├── assets/                         # Application icon, visual manifest, and banners
│   ├── app.ico                     # Multi-resolution native icon (16x16 to 256x256)
│   ├── app.manifest                # Common Controls v6 & Per-Monitor DPI Awareness
│   ├── app_logo.jpg                # Application logo graphic
│   └── banner.png                  # Widescreen high-resolution repository banner
│
├── docs/                           # Documentation
│   ├── USER_GUIDE.md               # User guide (English)
│   ├── USER_GUIDE.id.md            # User guide (Bahasa Indonesia)
│   ├── DEVELOPER_GUIDE.md          # Technical developer guide (English)
│   └── DEVELOPER_GUIDE.id.md       # Technical developer guide (Bahasa Indonesia)
│
├── include/                        # Public C headers
│   ├── common.h                    # Core constants, type definitions, and path limits
│   ├── crypto.h                    # Windows CNG AES-256-GCM AEAD & PBKDF2 API
│   ├── i18n.h                      # StringId enumeration & language control API
│   ├── renamer.h                   # Renamer structures, methods, collision & undo API
│   ├── scanner.h                   # Unicode directory traversal & FileList container
│   ├── service.h                   # Daemon watcher, shell integration & tracker API
│   ├── shredder.h                  # Physical multi-pass disk shredder API
│   ├── ui.h                        # Win32 main window interface & control IDs
│   └── vault.h                     # DPAPI credentials, lockout & OTP reset API
│
├── src/                            # Implementation files
│   ├── crypto.c                    # CNG BCrypt encrypt/decrypt, PBKDF2, CSPRNG
│   ├── i18n.c                      # Localized string lookup tables (EN / ID)
│   ├── main.c                      # WinMain entry point, CLI dispatch & single instance
│   ├── renamer.c                   # Advanced Renamer style methods, tag parsing, undo
│   ├── scanner.c                   # Recursive FindFirstFileW / FindNextFileW walker
│   ├── service.c                   # Shell integration, IShellWindows daemon, inactivity tracker
│   ├── shredder.c                  # Hardware buffer flushing & multi-pass file overwrite
│   ├── ui.c                        # Win32 UI, Explorer ListView, column sort, tray
│   ├── vault.c                     # DPAPI CryptProtectData vault & OTP generator
│   └── resources.rc                # GNU windres resource script
│
├── tests/                          # Automated unit and integration test suites
│   ├── test_crypto.c               # 9-suite verification of CNG AES-GCM, salt, shred
│   ├── test_i18n.c                 # 100% translation coverage across all String IDs
│   ├── test_renamer.c              # Rename methods, tag formatting, collision detection
│   └── test_service.c              # Auto-decrypt, inactivity tracker, startup recovery
│
├── build.bat                       # MinGW-w64 GCC build script (1-click compile)
├── Makefile                        # GNU Make build script
├── README.md                       # Project overview (English)
├── README.en.md                    # Project overview (Bahasa Indonesia - as requested)
└── README.id.md                    # Project overview (Bahasa Indonesia)
```

---

## 3. Cryptographic Subsystem (`crypto.c`, `crypto.h`)

### Windows CNG & AES-256-GCM (AEAD)
All encryption utilizes Windows CNG (`bcrypt.dll`).
- **Algorithm**: `BCRYPT_AES_ALGORITHM` with `BCRYPT_CHAIN_MODE_GCM`.
- **Chunk Size**: 64 KB (`65,536` bytes) chunks to support streaming arbitrary-sized files (from kilobytes to multiple terabytes) with minimal RAM overhead.
- **Nonce/IV**: 12-byte (96-bit) cryptographically random nonce generated per chunk via `BCryptGenRandom`.
- **Authentication Tag**: 16-byte (128-bit) GCM tag computed for every chunk, guaranteeing ciphertext integrity and preventing tampering.

### PBKDF2-HMAC-SHA256 Key Derivation
- **Input Password**: Recovery email address string (`wchar_t`).
- **Salt**: 16-byte (128-bit) cryptographically secure pseudorandom salt generated per file.
- **Iterations**: 100,000 rounds of `BCRYPT_SHA256_ALGORITHM`.
- **Output Key**: 32-byte (256-bit) symmetric master key.

```c
bool crypto_derive_key(
    const wchar_t *email,
    const uint8_t *salt,
    size_t salt_len,
    uint8_t *out_key
);
```

### Header & Binary Container Format
Every `.enc` file begins with a serialized 44-byte binary container header:

| Offset | Field | Size | Description |
|---|---|---|---|
| `0x00` | `magic` | 4 bytes | Magic identifier: ASCII `"W10E"` (`0x45303157`) |
| `0x04` | `version` | 2 bytes | Format version (`0x0002`) |
| `0x06` | `cipher_id` | 2 bytes | Cipher identifier (`0x0001` = AES-256-GCM) |
| `0x08` | `salt` | 16 bytes | Cryptographic random salt for PBKDF2 |
| `0x18` | `auth_tag` | 16 bytes | AEAD global integrity authentication tag |
| `0x28` | `original_size` | 8 bytes | 64-bit unsigned original file size in bytes |
| `0x30` | `payload` | Variable | Encrypted chunked ciphertext |

---

## 4. Security Vault Subsystem (`vault.c`, `vault.h`)

### DPAPI Protection
The configuration vault (`win10_file_enc_vault.dat`) secures user credentials on disk using the Windows Data Protection API (`CryptProtectData` and `CryptUnprotectData` from `crypt32.dll`):
- Keys are tied to the local user's Windows security descriptor.
- Stored data includes:
  - PBKDF2 hash of the Master Password.
  - Windows DPAPI encrypted recovery email address.
  - Failed login attempt counter.
  - Account lockout timestamp.

### Lockout & OTP Dispatch
- **5-Attempt Lockout**: If `g_FailedAttempts >= 5`, `vault_verify_password` returns `false` and sets `g_bAccountLocked = true`.
- **OTP Generation**: Uses `BCryptGenRandom` to generate a 6-digit random code (100000..999999) with an expiration window of 10 minutes.
- **Email Dispatch Mock/Audit**: Emits the code to the system event log and audit dispatch pipe.

---

## 5. Inactivity Tracker & Auto-Re-Encryption (`service.c`, `service.h`)

To eliminate the security risk of unprotected plaintext files remaining on disk, `service.c` maintains a unified file tracking engine:

### Persistent State Storage (`win10_file_enc_tracker.dat`)
Every time a protected file is decrypted (via Windows Explorer, context menu, or double-click in Encrypt Explorer), its absolute path, last-access tick, wall-clock time, and `FILETIME` are serialized to `win10_file_enc_tracker.dat`:

```c
typedef struct {
    wchar_t plain_path[MAX_PATH * 2];
    DWORD last_access_tick;
    time_t last_access_time;
    FILETIME last_write_time;
    bool active;
} TrackedDecryptedFile;
```

### 30-Minute Inactivity Engine
1. **Periodic Evaluation**: Checked every 15 seconds via `WM_TIMER` (`TIMER_AUTO_ENCRYPT_ID = 7001`) in the UI thread and by the daemon monitor thread.
2. **External Modification Check**: Queries `GetFileTime` on `plain_path`. If `ftLastWriteTime` is newer than recorded, the file is currently being modified by the user in an external application (Word, Notepad, IDE), so `last_access_tick` and `last_access_time` reset.
3. **Timeout Trigger**: If `now - last_access >= 1,800,000 ms` (30 minutes):
   - Invokes `service_auto_encrypt_single_file(plain_path)`.
   - Shreds the plaintext copy.
   - De-activates the entry and updates `win10_file_enc_tracker.dat`.
   - Fires a system tray balloon notification.

### Startup Crash & Reboot Reconciliation
On application launch (`wWinMain` in `main.c`, `service_run_daemon` in `service.c`, and `WM_CREATE` in `ui.c`):
- `tracker_init()` reads `win10_file_enc_tracker.dat`.
- `tracker_encrypt_all_decrypted_files()` iterates over all recorded entries. If any plaintext file still exists on disk (due to a power failure, system crash, or forced kill), it is **immediately encrypted back to `.enc` and wiped before user interaction**.

### Graceful Shutdown Encryption
When the application exits completely (`app_exit_completely` or `WM_DESTROY`):
- `tracker_encrypt_all_decrypted_files()` ensures that all active decrypted files are securely encrypted to `.enc` and shredded before the process terminates.

---

## 6. Physical File Shredder Subsystem (`shredder.c`, `shredder.h`)

Deleting a file via `DeleteFileW` only removes the file entry from the file system table (MFT), leaving the physical sectors intact. `shredder.c` ensures irreversible destruction:

1. **Pass 1**: Overwrite file sectors with zero bytes (`0x00`).
2. **Pass 2**: Overwrite file sectors with CSPRNG pseudorandom bytes generated by `crypto_generate_random`.
3. **Hardware Commit**: Calls `FlushFileBuffers(hFile)` to force physical write caches on the storage controller / SSD to commit before deletion.
4. **Truncation & Deletion**: Sets file size to 0 bytes via `SetEndOfFile`, closes handle, and unlinks with `DeleteFileW`.

---

## 7. Bulk Renamer Engine (`renamer.c`, `renamer.h`)

### Method Pipeline & Tag Parser
The renamer implements 5 modular algorithms:
1. `METHOD_REPLACE`: Substring search & replacement.
2. `METHOD_ADD`: Prefix and suffix insertion.
3. `METHOD_PATTERN`: Tag interpolation supporting `<name>` (original basename), `<inc:1>` (1, 2, 3), `<inc:01>` (01, 02), `<inc:001>` (001, 002) with dynamic step sizes.
4. `METHOD_CASE`: UPPERCASE (`_wcsupr`), lowercase (`_wcslwr`), and Title Case (capitalizes first letter of words).
5. `METHOD_TRIM`: Character stripping from start or end with whitespace normalization.

### Real-Time Collision Detection
`renamer_detect_collisions(RenameList *list)` performs an $O(N \log N)$ sort and scan over proposed file names. If two prospective names are identical, or if a destination file already exists on disk, `is_collision` is set to `true`, preventing accidental data loss.

### Multi-Step Undo Stack
Renaming operations are recorded in an `UndoHistory` linked list. Each batch commit saves the paired old and new paths. Invoking `renamer_undo_last` performs an inverse batch rename and pops the undo stack.

---

## 8. Win32 UI Architecture (`ui.c`, `ui.h`)

### Window Message Loop & Subclassing
- **Main Window Class**: `L"Win10FileModMainClass"`.
- **Keyboard Handling**:
  - `login_pass_edit_proc`: Subclasses the password edit control to process `VK_RETURN` (Enter key) as a direct login submission.
  - `explorer_list_proc`: Subclasses the ListView to handle `Ctrl+A` for instant select-all.
- **Single Instance Detection**: `wWinMain` checks `FindWindowW(L"Win10FileModMainClass", NULL)`. If found, it un-hides and focuses the existing instance (`SW_RESTORE`) and exits early.

### ListView Custom Sorting & Visual Headers
- **Column Header Notification**: Handled via `LVN_COLUMNCLICK` in `WM_NOTIFY`.
- **Sorting Comparator**: `ListView_SortItems(hList, explorer_list_compare_func, lParamSort)`.
- **Header Sort Arrows**: Dynamically queries `ListView_GetHeader` and applies `HDF_SORTUP` / `HDF_SORTDOWN` to `HDITEMW.fmt`.

### Close-to-Tray (`WM_CLOSE` to `SW_HIDE`)
- Intercepts `WM_CLOSE`:
  - Saves persistent configurations.
  - Calls `ShowWindow(hWnd, SW_HIDE)` and `show_main_tray_icon(hWnd, true)`.
  - Suppresses `DestroyWindow`.
- Intercepts `WM_APP_MAINTRAY`:
  - `WM_LBUTTONUP` / `WM_LBUTTONDBLCLK`: Restores the window via `ShowWindow(hWnd, SW_RESTORE)` and removes the tray icon.
  - `WM_RBUTTONUP`: Displays a native context menu with **Open** and **Exit Completely**.

---

## 9. Windows Explorer Shell & Daemon Service

### IShellWindows COM Monitoring
`service.c` runs `explorer_monitor_thread` which enumerates active File Explorer windows:
1. Calls `CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_ALL, &IID_IShellWindows, &psw)`.
2. Queries `IWebBrowserApp` and extracts `LocationURL` via `PathCreateFromUrlW`.
3. If an opened directory contains `.enc` files and cooldown has elapsed, automatically invokes `service_auto_decrypt_folder(folderPath)`.

### Registry File Associations
`service_install()` writes shell verb associations:
- `HKCU\Software\Classes\.enc`: Sets default to `Win10FileEnc.ProtectedFile`.
- `HKCU\Software\Classes\Win10FileEnc.ProtectedFile\shell\open\command`: Maps to `"%exe%" --auto-decrypt "%1"`.
- `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`: Registers auto-startup with `--service`.

---

## 10. Internationalization Subsystem (`i18n.c`, `i18n.h`)

All UI strings, menu items, table headers, and notification messages are referenced via the `StringId` enum (132 total keys):
- Default Language: `LANG_EN` (English).
- Supported Languages: `LANG_EN`, `LANG_ID` (Bahasa Indonesia).
- Runtime Switching: Calling `i18n_set_language` followed by `update_ui_texts(hWnd)` re-applies all titles, tabs, menus, and column headers dynamically.

---

## 11. Build System & Automated Testing

### GCC Compilation Flags
Compile using MinGW-w64 GCC with Unicode and Windows subsystems enabled:

```cmd
windres src\resources.rc -O coff -o src\resources.o
gcc -O2 -Wall -Wextra -municode -mwindows -Iinclude ^
    src\main.c src\crypto.c src\vault.c src\scanner.c src\shredder.c src\renamer.c src\i18n.c src\service.c src\ui.c src\resources.o ^
    -lbcrypt -lcomctl32 -lshell32 -lole32 -loleaut32 -luuid -lshlwapi ^
    -o win10_file_enc.exe
```

### Test Suites
Four standalone test suites validate functionality without launching the GUI:

1. **`tests/test_crypto.exe`**:
   - Tests `BCryptGenRandom`, PBKDF2 key derivation from email salt, AES-256-GCM encryption/decryption, physical shredding, and AEAD tag authentication failure upon incorrect email.
2. **`tests/test_renamer.exe`**:
   - Tests Find & Replace, prefix/suffix insertion, tag patterns (`<name>`, `<inc:01>`), case transformations, collision detection, and undo history.
3. **`tests/test_service.exe`**:
   - Tests single-file and folder auto-decryption, 30-minute inactivity tracking, persistent tracker serialization, full exit auto-encryption, and Explorer shell registry registration.
4. **`tests/test_i18n.exe`**:
   - Validates 100% translation key coverage across all 132 `StringId` keys for both English and Bahasa Indonesia.
