@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

where gcc >nul 2>&1
if %errorlevel% neq 0 (
    if exist "E:\msys64\mingw64\bin\gcc.exe" (
        set "PATH=E:\msys64\mingw64\bin;!PATH!"
    ) else if exist "C:\msys64\mingw64\bin\gcc.exe" (
        set "PATH=C:\msys64\mingw64\bin;!PATH!"
    )
)

echo =====================================================================
echo  [BUILD] Windows 10 Native File Encryptor ^& Secure Shredder
echo =====================================================================

echo [BUILD] Compiling application icon and visual manifest (src\resources.rc)...
windres src\resources.rc -O coff -o src\resources.o
if %errorlevel% neq 0 (
    echo [ERROR] windres failed to compile resources.rc!
    pause
    exit /b 1
)

echo [BUILD] Compiling win10_file_enc.exe with GCC...
gcc -O2 -Wall -Wextra -municode -mwindows -Iinclude ^
    src\main.c src\crypto.c src\vault.c src\scanner.c src\shredder.c src\renamer.c src\duplicate.c src\i18n.c src\service.c src\ui.c src\resources.o ^
    -lbcrypt -lcomctl32 -lshell32 -lole32 -loleaut32 -luuid -lshlwapi ^
    -o win10_file_enc.exe

if %errorlevel% equ 0 (
    echo [SUCCESS] win10_file_enc.exe built successfully!
    echo Binary: %~dp0win10_file_enc.exe
) else (
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)
