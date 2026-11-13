#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_WIN_DIR="${SCRIPT_DIR}/build_win"
DIST_DIR="${BUILD_WIN_DIR}/dist"
OUTPUT_DIR="${SCRIPT_DIR}/.."

echo "==> Preparando ambiente de compilação Windows (.msi)..."
mkdir -p "${DIST_DIR}/sounds"

# 1. Copia recursos de áudio e ícones
cp "${SCRIPT_DIR}/resources/sounds/"*.wav "${DIST_DIR}/sounds/"
cp "${SCRIPT_DIR}/resources/icons/app_icon.ico" "${DIST_DIR}/vxvoice.ico"

# 2. Compila recurso de ícone e versão do Windows (.res)
export PATH="${HOME}/.local/mingw/usr/bin:${PATH}"
echo "==> Compilando recursos Win32/Win64..."
x86_64-w64-mingw32-windres "${BUILD_WIN_DIR}/version.rc" -O coff -o "${BUILD_WIN_DIR}/version.res"

# 3. Compila binário executável Windows x64 (VxVoice.exe)
echo "==> Compilando executável PE x64 (VxVoice.exe)..."
x86_64-w64-mingw32-gcc -O2 -I "${HOME}/.local/mingw/usr/share/mingw-w64/include" -c "${BUILD_WIN_DIR}/win_main.c" -o "${BUILD_WIN_DIR}/win_main.o"
x86_64-w64-mingw32-gcc -mwindows -L"${HOME}/.local/mingw/usr/x86_64-w64-mingw32/lib" "${BUILD_WIN_DIR}/win_main.o" "${BUILD_WIN_DIR}/version.res" -o "${DIST_DIR}/VxVoice.exe" -lgdi32 -lshell32 -luser32 -lcomctl32

# 4. Compila instalador MSI com WiX Toolset (wixl)
echo "==> Gerando pacote Windows Installer (.msi) com wixl..."
"${HOME}/.local/bin/wixl" -a x64 -o "${OUTPUT_DIR}/vxvoice-1.4.9-x64.msi" "${BUILD_WIN_DIR}/vxvoice.wxs"
cp "${OUTPUT_DIR}/vxvoice-1.4.9-x64.msi" "${OUTPUT_DIR}/vxvoice-1.4.9-Windows.msi"

echo "==> Pacote MSI gerado com sucesso em:"
ls -lh "${OUTPUT_DIR}/vxvoice-1.4.9-x64.msi" "${OUTPUT_DIR}/vxvoice-1.4.9-Windows.msi"
