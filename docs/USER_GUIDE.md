# Win10 File Encryptor & Bulk Renamer - User Guide

![Banner](../assets/banner.png)

> **Languages:** [English](USER_GUIDE.md) | [Bahasa Indonesia](USER_GUIDE.id.md)

Welcome to the **Win10 File Encryptor & Bulk Renamer** User Guide. This comprehensive handbook provides step-by-step instructions on configuring security credentials, managing protected folders, using the Explorer auto-decrypt service, setting up auto-lock rules, and utilizing the advanced bulk file renamer.

---

## Table of Contents
1. [Overview & System Requirements](#1-overview--system-requirements)
2. [First-Time Security Setup](#2-first-time-security-setup)
3. [Master Password Authentication & Lockout Protection](#3-master-password-authentication--lockout-protection)
4. [Encrypt Explorer Interface](#4-encrypt-explorer-interface)
   - [Directory Tree & File Browsing](#directory-tree--file-browsing)
   - [Column Sorting (Name, Type, Size, Date, Status)](#column-sorting)
   - [Encrypting Selected Files & Folders](#encrypting-selected-files--folders)
   - [Decrypting & Opening Protected Files](#decrypting--opening-protected-files)
5. [Inactivity Auto-Lock, Single Instance & Clean Exit](#5-inactivity-auto-lock-single-instance--clean-exit)
   - [30-Minute Inactivity Auto-Lock](#30-minute-inactivity-auto-lock)
   - [Strict Single Instance Protection](#strict-single-instance-protection)
   - [Clean Window Exit (No Duplicate Tray Icons)](#clean-window-exit-no-duplicate-tray-icons)
   - [Startup Crash & Reboot Recovery](#startup-crash--reboot-recovery)
6. [Windows Explorer Background Service Integration](#6-windows-explorer-background-service-integration)
   - [Enabling the Auto-Decrypt Service](#enabling-the-auto-decrypt-service)
   - [Shell Context Menu & Double-Click Decryption](#shell-context-menu--double-click-decryption)
7. [Bulk File Renamer (Advanced Renamer Style)](#7-bulk-file-renamer-advanced-renamer-style)
   - [Method 1: Find & Replace](#method-1-find--replace)
   - [Method 2: Add Prefix / Suffix](#method-2-add-prefix--suffix)
   - [Method 3: Pattern Tags (<name>, <inc:01>)](#method-3-pattern-tags-name-inc01)
   - [Method 4: Case Conversion](#method-4-case-conversion)
   - [Method 5: Trim Characters](#method-5-trim-characters)
   - [Collision Detection & Undo](#collision-detection--undo)
8. [Duplicate File Finder](#8-duplicate-file-finder)
   - [Selecting Folders & Recursive Subfolders](#selecting-folders--recursive-subfolders)
   - [Fast 2-Tier Engine (File Size & SHA-256 Hash)](#fast-2-tier-engine-file-size--sha-256-hash)
   - [Interactive Checkbox Results View](#interactive-checkbox-results-view)
   - [Auto-Select Duplicate Copies](#auto-select-duplicate-copies)
   - [Safe Recycling via Windows Recycle Bin](#safe-recycling-via-windows-recycle-bin)
   - [Right-Click Context Menu](#right-click-context-menu)
9. [Multi-Language Switching](#9-multi-language-switching)
10. [Frequently Asked Questions (FAQ)](#10-frequently-asked-questions-faq)

---

## 1. Overview & System Requirements

**Win10 File Encryptor & Bulk Renamer** is an ultra-lightweight, high-performance native desktop application built specifically for Windows 10 and 11. It contains no external dependencies, requires no runtime installers (such as .NET or Electron), and operates with sub-millisecond responsiveness.

### System Requirements:
- **Operating System**: Windows 10 (Build 1809 or higher) or Windows 11 (64-bit recommended).
- **Architecture**: x86_64 (64-bit).
- **RAM**: 32 MB minimum.
- **Disk Space**: Less than 2 MB.
- **Hardware Acceleration**: Automatic AES-NI hardware instruction set acceleration via Windows CNG (`bcrypt.dll`).

---

## 2. First-Time Security Setup

When launching the application for the very first time, you are presented with the **Master Security Setup** screen:

1. **Master Password**: Enter a password with a minimum of 4 characters. This password protects GUI access to the application.
2. **Confirm Password**: Re-enter the exact password to confirm.
3. **Recovery Email Address (Mandatory)**:
   - Provide a valid email address (e.g., `user@domain.com`).
   - **Crucial Security Design**: Your encryption key is derived strictly from this registered email address combined with a 128-bit cryptographic salt unique to each file. This ensures that:
     - You can decrypt your files on any other computer running this software by using the same recovery email.
     - You never lose your files if you forget your local master password.
4. Click **Register & Open Application** (`Daftarkan & Buka Aplikasi`).
5. Your credentials are saved securely into a Windows DPAPI-protected vault (`win10_file_enc_vault.dat`).

---

## 3. Master Password Authentication & Lockout Protection

On subsequent launches, the application opens to the **Master Authentication** screen:

- Enter your Master Password and press **Enter** or click **Log In** (`Masuk`).
- **Show Password Checkbox**: Toggle to view masked characters.
- **Lockout Mechanism**:
  - The screen displays your failed attempt count: `Failed attempts: 0 / 5`.
  - If 5 incorrect passwords are submitted, the account locks automatically.
  - To unlock, click **Forgot Password** (`Lupa Password`). A 6-digit one-time verification code (OTP) is generated and dispatched to your registered recovery email. Enter the code to reset your password.

---

## 4. Encrypt Explorer Interface

Once authenticated, Tab 1 displays the dual-pane **Encrypt Explorer**:

### Directory Tree & File Browsing
- **Left Pane (TreeView)**: Displays all available system drive letters (`C:\`, `D:\`, etc.) and directories with native Windows folder icons. Expand nodes to explore folders.
- **Right Pane (ListView)**: Displays the contents of the currently selected directory in detailed report view.
- **Address Bar**: Shows the current path. You can manually enter a path or click **Browse...** to pick a folder.
- **Sub-folder Scanning Checkbox**: When checked, batch operations recursively traverse subdirectories.
- **Remember Folder Checkbox**: Preserves the last viewed folder across application sessions.

### Column Sorting
Click any column header to sort items immediately:
- **Name (`Nama Berkas / Folder`)**: Alphabetical sorting (A-Z or Z-A).
- **Type (`Tipe`)**: Sorts by file extension / file type.
- **Size (`Ukuran`)**: **True numeric sort** by 64-bit byte size (e.g., `2.1 MB` sorts correctly before `10.4 MB`).
- **Date Modified (`Tanggal Dimodifikasi`)**: Chronological sort from newest to oldest or vice-versa.
- **Encryption Status (`Status Enkripsi`)**: Groups encrypted, decrypted, and original files.
- **Visual Sort Arrow**: A native triangle indicator (▲ for Ascending, ▼ for Descending) appears on the active sort column.
- **Folders at Top**: Folders always stay grouped at the top of the list, identical to standard Windows Explorer behavior.

### Encrypting Selected Files & Folders
1. In the file list pane, select one or multiple files:
   - Click to select an individual item.
   - Hold `Ctrl` or `Shift` to multi-select specific files.
   - Press `Ctrl+A` or right-click and choose **Select All** (`Pilih Semua`).
2. Right-click the selection to open the context menu:
   - **🔒 Encrypt Selected File(s)** (`Enkripsi berkas yang dipilih`): Encrypts the selected files with AES-256-GCM, securely shreds the original plaintext files, and saves them as `.enc`.
   - **📁 Encrypt Entire Folder** (`Enkripsi seluruh folder ini`): Batch encrypts all eligible files in the folder.
3. Progress is displayed in real-time on the smooth progress bar and activity log box.

### Decrypting & Opening Protected Files
- **Right-Click Context Menu**: Choose **🔓 Decrypt Selected File(s)** (`Dekripsi berkas yang dipilih`).
- **Double-Click**: Double-clicking any `.enc` file in the list automatically decrypts it, registers it into the 30-minute auto-lock tracker, opens it in its default program, and refreshes its status to `🔓 Decrypted (Auto-lock 30m)`.

---

## 5. Inactivity Auto-Lock, Single Instance & Hide to Taskbar (Background Processing)

To safeguard sensitive data both in memory and on disk, while ensuring an uninterrupted multitasking experience, the application enforces automated multi-layered protections:

### 30-Minute Inactivity Auto-Lock
- Any file opened or decrypted enters the **Decrypted File Tracker**.
- While the program is running, a background monitor evaluates file access every 15 seconds.
- If an open file has not been accessed or edited for **30 consecutive minutes**:
  - The software automatically re-encrypts the file to `.enc` using AES-256-GCM.
  - The temporary plaintext copy is completely shredded from disk (*secure shredding*).
  - A Windows system tray balloon notification alerts you that the file has been re-locked.
- **Active Editing Reset**: If you modify or save the file in your editor (e.g., Word, VS Code, Notepad), the file timestamp update is automatically detected, resetting the 30-minute timer.

### Strict Single Instance Protection
- The application is protected by a system-wide **Windows Named Mutex** (`Win10FileMod_App_SingleInstance_Mutex`).
- Exactly **one application window** is allowed to run at any time.
- If you launch `win10_file_enc.exe` while the program is already active or hidden in the notification area:
  - The running instance automatically restores itself and is brought to the foreground immediately.
  - The second instance exits cleanly and immediately, preventing duplicate memory usage and tray icon clutter.

### Hide to Taskbar & Background Continuation
- Clicking the window close button (**[X]**) or pressing `Alt+F4` **DOES NOT cancel or abort active tasks**.
- The main window is automatically hidden to the Windows taskbar notification area (System Tray) using `SW_HIDE`.
- **All processes across all tabs continue executing seamlessly in the background**:
  - **Batch Encryption & Decryption** in Encrypt Explorer runs to completion.
  - **Duplicate Scanning & SHA-256 Hashing** in Duplicate File Finder continues uninterrupted.
  - **Bulk Renaming** operations in Bulk File Renamer execute until finished.
  - **30-Minute Inactivity Tracker (Auto-Lock)** remains active safeguarding open files.
- **Balloon Notifications**:
  - When the window is hidden while tasks are running, a balloon notification confirms that operations are proceeding in the background.
  - When any task completes (encryption, decryption, duplicate scan, or rename), a native balloon notification pops up over the tray without disruptive modal message dialogs.
- **Restoring the Window**:
  - Double-click the shield icon in the taskbar notification area, or re-launch `win10_file_enc.exe`. The window immediately reappears with up-to-date progress.
- **Complete Application Exit**:
  - To truly close and shut down the application:
    1. Select **File ➔ Exit** (`Berkas ➔ Keluar`) from the menu bar.
    2. Or right-click the notification tray icon and choose **Exit Completely** (`Keluar Sepenuhnya`).
  - Upon full exit, all tracked plaintext files are securely re-encrypted back to `.enc` and their plaintext remnants shredded.

### Startup Crash & Reboot Recovery
If your computer experiences a sudden reboot, power outage, or system crash:
- The persistent tracker file (`win10_file_enc_tracker.dat`) retains the state of any open files.
- The next time the program or background service starts up, it immediately scans the tracker and **re-encrypts any surviving plaintext files to `.enc`**, wiping the plaintext before any user intervention.

---

## 6. Windows Explorer Background Service Integration

The application features seamless Windows Explorer shell integration and a dedicated background daemon:

### Enabling the Auto-Decrypt Service
1. In the menu bar, open the **Service** (`Layanan`) menu.
2. Select **Install Explorer Shell Integration** (`Pasang Integrasi Windows Explorer`).
3. Select **Enable Background Service** (`Aktifkan Layanan Background`).
4. The service will run in the background (or launch with `--service` at Windows startup).

### Shell Context Menu & Double-Click Decryption
- **In Windows Explorer**: Opening any folder containing `.enc` files automatically detects them and restores the plaintext files for seamless workflow.
- **Direct Launch**: Double-clicking an `.enc` file in Windows Explorer invokes `--auto-decrypt`, restoring the original file and launching its associated application.

---

## 7. Bulk File Renamer (Advanced Renamer Style)

Click the **🏷️ Bulk File Renamer** tab in the main header to access the batch renaming tool:

### Method 1: Find & Replace
- **Find**: Text to search for (case-insensitive).
- **Replace With**: New replacement text.
- *Example*: Searching for `photo_` and replacing with `trip_2026_` updates `photo_01.jpg` to `trip_2026_01.jpg`.

### Method 2: Add Prefix / Suffix
- **Add Prefix**: Inserts text at the beginning of the filename.
- **Add Suffix**: Appends text at the end of the filename (before the file extension).

### Method 3: Pattern Tags (`<name>`, `<inc:01>`)
- Use tags to format names dynamically:
  - `<name>`: Preserves the original file name.
  - `<inc:1>`: 1, 2, 3...
  - `<inc:01>`: 01, 02, 03... (zero-padded 2 digits).
  - `<inc:001>`: 001, 002, 003... (zero-padded 3 digits).
  - Custom step and start number controls available.
- *Example*: Pattern `Vacation_<inc:01>` renames files to `Vacation_01.jpg`, `Vacation_02.jpg`.

### Method 4: Case Conversion
- Options:
  - **lowercase**: `document_name.txt`
  - **UPPERCASE**: `DOCUMENT_NAME.TXT`
  - **Title Case**: `Document Name.Txt`

### Method 5: Trim Characters
- Removes `N` characters from the beginning or end of the filename.

### Collision Detection & Undo
- **Real-Time Live Preview**: As you type method parameters, the *New Name (Live Preview)* column updates instantaneously.
- **Collision Flagging**: If any two files would result in identical names, the row is flagged with a warning to prevent accidental file overwriting.
- **Undo Operation**: Click **↩️ Undo Rename** (`Batalkan Rename`) to instantly revert the last batch rename operation to the original filenames.

---

## 8. Duplicate File Finder

Click the **🔍 Duplicate File Finder** tab in the top navigation bar to scan, identify, and safely clean up duplicate files within any chosen directory to reclaim hard disk storage space.

### Selecting Folders & Recursive Subfolders
1. Click the **Browse...** (`Pilih Folder...`) button to select the target directory you wish to inspect.
2. Check **Scan Subfolders** (`Pindai Sub-folder`) to include all child directories recursively, or uncheck it to scan only the top-level directory.
3. Click **Scan Duplicates** (`Mulai Pindai`) to initiate the scanning process. You can abort the scan at any point by clicking **Stop** (`Hentikan`).

### Fast 2-Tier Engine (File Size & SHA-256 Hash)
The scanner executes a high-performance two-tier identification pipeline:
- **Tier 1 (Rapid Size Grouping)**: Queries file size attributes directly from the Windows file system. Any file with a unique byte size is instantly bypassed without performing disk reads, saving massive I/O overhead.
- **Tier 2 (SHA-256 Content Verification)**: Only files that share exact byte-size matches are read from disk and hashed using cryptographic **SHA-256** (via Windows CNG `bcrypt.dll`). Files are only classified as duplicates when their binary content hashes match 100%.

### Interactive Checkbox Results View
Identified duplicate files are grouped and listed in an interactive report view table with the following columns:
- **Checkbox**: Mark or unmark files designated for deletion.
- **File Name**: Name of the duplicate file.
- **Status**:
  - `[Original]` (`[Asli]`): The primary original copy retained in the duplicate cluster.
  - `[Copy]` (`[Salinan]`): Redundant duplicate copies identified with identical content.
- **Size**: Formatted file size (`KB`, `MB`, `GB`).
- **Date Modified**: Last modification timestamp.
- **Full Path**: Absolute storage path on the system.

### Auto-Select Duplicate Copies
To cleanly mark redundant copies while keeping original files intact:
1. Click **Auto-Select Copies** (`Pilih Otomatis Salinan`).
2. The software automatically checks all files marked `[Copy]` while **safely keeping exactly 1 `[Original]` file unchecked** in each duplicate cluster.
3. Convenient **Select All** (`Pilih Semua`) and **Deselect All** (`Batal Pilih Semua`) buttons are also available.

### Safe Recycling via Windows Recycle Bin
1. After reviewing checked items, click **Delete to Recycle Bin** (`Hapus ke Recycle Bin`).
2. A confirmation dialog presents the total number of files selected and the **exact disk space to be freed** (e.g., *Are you sure you want to move 12 duplicate files (345.2 MB) to the Recycle Bin?*).
3. Selected files are sent to the Windows Recycle Bin using the native Shell API (`SHFileOperationW` with `FOF_ALLOWUNDO`), ensuring files **can be restored at any time** if needed.

### Right-Click Context Menu
Right-click any row in the duplicate list to access immediate actions:
- **Open File**: Opens the file with its associated Windows application.
- **Show in Explorer**: Opens Windows Explorer with the specific file selected and highlighted.
- **Toggle Check**: Checks or unchecks the selected item.
- **Delete This File**: Immediately sends only the selected file to the Recycle Bin.

---

## 9. Multi-Language Switching

The software features complete live runtime localization:
- Open the **🌐 Language** (`🌐 Bahasa`) menu in the top menu bar.
- Choose between:
  - **English (Default)**
  - **Bahasa Indonesia**
- All labels, dialogs, buttons, list columns, page tabs, and system tray notifications switch instantly without requiring an application restart.

---

## 10. Frequently Asked Questions (FAQ)

#### Q: Why doesn't the application allow opening a second window when already running?
**A**: To protect encryption integrity and prevent tracker database corruption (`win10_file_enc_tracker.dat`), the app enforces **Strict Single Instance** via a Windows Named Mutex (`Win10FileMod_App_SingleInstance_Mutex`). Launching the executable again safely restores the existing window to the foreground.

#### Q: Why don't duplicate tray icons appear when closing the application?
**A**: The window close procedure has been optimized. Clicking the window Close button ([X]) cleanly terminates the GUI process and removes its tray icon. The background service (`--service`) can run independently without duplicate icons cluttering the notification area.

#### Q: Where are encrypted files saved?
**A**: Encrypted files are created in the exact same directory as the original file, with the `.enc` extension appended (e.g., `report.docx` becomes `report.docx.enc`).

#### Q: Can I decrypt files on another PC?
**A**: **Yes!** As long as the other computer has this application installed and you enter the exact same recovery email, your `.enc` files can be decrypted anywhere.

#### Q: Can files be recovered after secure shredding?
**A**: **No.** Original files are physically overwritten with multiple patterns and flushed directly to hardware disk controller buffers before removal. File recovery software (such as Recuva) will only find wiped zero-bytes.

#### Q: Are duplicate files deleted permanently?
**A**: **No.** Deletion in the Duplicate File Finder moves files to the Windows Recycle Bin (`FOF_ALLOWUNDO`). You can open the Recycle Bin on your desktop and restore any file at any time.
