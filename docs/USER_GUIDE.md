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
5. [Inactivity Auto-Lock & Automatic Re-Encryption](#5-inactivity-auto-lock--automatic-re-encryption)
   - [30-Minute Inactivity Auto-Lock](#30-minute-inactivity-auto-lock)
   - [Close-to-Tray (Hide to Taskbar)](#close-to-tray-hide-to-taskbar)
   - [Exit Security (Full Re-Encryption on Close)](#exit-security-full-re-encryption-on-close)
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
8. [Multi-Language Switching](#8-multi-language-switching)
9. [Frequently Asked Questions (FAQ)](#9-frequently-asked-questions-faq)

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

## 5. Inactivity Auto-Lock & Automatic Re-Encryption

To prevent sensitive plaintext data from lingering on disk, the application enforces automated multi-layered re-encryption:

### 30-Minute Inactivity Auto-Lock
- Any file opened or decrypted enters the **Decrypted File Tracker**.
- While the program is running (whether visible or minimized to the taskbar tray), a background monitor evaluates file access every 15 seconds.
- If an open file has not been accessed or edited for **30 consecutive minutes**:
  - The software automatically re-encrypts the file to `.enc` using AES-256-GCM.
  - The temporary plaintext copy is completely shredded from disk.
  - A Windows system tray balloon notification alerts you that the file has been re-locked.
- **Active Editing Reset**: If you modify or save the file in your editor (e.g., Word, VS Code, Notepad), the file timestamp update is automatically detected, resetting the 30-minute timer.

### Close-to-Tray (Hide to Taskbar)
- Clicking the window close button (**X**) or pressing `Alt+F4` does **not** terminate the application; instead, it **hides the program to the system tray (taskbar notification area)**.
- While hidden, background auto-encryption and Explorer folder watchers remain fully operational.
- To restore the window:
  - **Left-click** or **double-click** the system tray shield icon.
  - Or right-click the tray icon and select **🖥️ Open Win10 File Encryptor**.
  - Or launch `win10_file_enc.exe` again from your desktop/Start menu (the existing instance will bring itself to the front).

### Exit Security (Full Re-Encryption on Close)
When you choose to exit completely via **File ➔ Exit** (`Berkas ➔ Keluar`) or by right-clicking the tray icon and selecting **❌ Exit Completely** (`Keluar Sepenuhnya`):
- All files currently tracked in the decrypted state are **immediately re-encrypted to `.enc`**.
- All plaintext versions are shredded.
- No unprotected data is left on your hard drive after the program terminates.

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

## 8. Multi-Language Switching

The software features complete live runtime localization:
- Open the **🌐 Language** (`🌐 Bahasa`) menu in the top menu bar.
- Choose between:
  - **English (Default)**
  - **Bahasa Indonesia**
- All labels, dialogs, buttons, list columns, and system tray notifications switch instantly without requiring an application restart.

---

## 9. Frequently Asked Questions (FAQ)

#### Q: Where are encrypted files saved?
**A**: Encrypted files are created in the exact same directory as the original file, with the `.enc` extension appended (e.g., `report.docx` becomes `report.docx.enc`).

#### Q: Can I decrypt files on another PC?
**A**: **Yes!** As long as the other computer has this application installed and you enter the exact same recovery email, your `.enc` files can be decrypted anywhere.

#### Q: Can files be recovered after secure shredding?
**A**: **No.** Original files are physically overwritten with multiple patterns and flushed directly to hardware disk controller buffers before removal. File recovery software (such as Recuva) will only find wiped zero-bytes.

#### Q: How do I completely close the application if 'X' only hides it?
**A**: Use **File ➔ Exit** (`Berkas ➔ Keluar`) from the top menu, or right-click the system tray icon and click **Exit Completely** (`Keluar Sepenuhnya`).
