@echo off
REM Build and run the C version of the affine cipher on Windows.
REM Usage: double click this file, or run it in the VS Code terminal.

chcp 65001 > nul
cd /d "%~dp0"

echo === 1. Compile affine_cipher.c with gcc ===
gcc -Wall -Wextra -O2 -g affine_cipher.c -o affine_cipher.exe
if errorlevel 1 (
    echo [FAILED] gcc compile error, please check the toolchain.
    pause
    exit /b 1
)
echo [OK] affine_cipher.exe generated.

echo.
echo === 2. Run demo and selftest ===
affine_cipher.exe

echo.
echo === 3. Run Python unittest ===
python -m unittest -v test_affine_cipher.py

pause
