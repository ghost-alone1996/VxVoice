#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
OUTPUT_DIR="${SCRIPT_DIR}/.."
VERSION="2.0.3"
STAGING_DIR="${BUILD_DIR}/vxvoice-${VERSION}-linux-installer"
ZIP_PATH="${OUTPUT_DIR}/vxvoice-${VERSION}-Linux-Installer.zip"

if [ ! -f "${BUILD_DIR}/VxVoice" ]; then
    echo "Erro: Binário ${BUILD_DIR}/VxVoice não encontrado. Compile o projeto primeiro."
    exit 1
fi

echo "==> Preparando pacote instalador .sh em .zip (v${VERSION})..."
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}/bin"
mkdir -p "${STAGING_DIR}/icons"
mkdir -p "${STAGING_DIR}/sounds"

cp "${BUILD_DIR}/VxVoice" "${STAGING_DIR}/bin/VxVoice"
chmod +x "${STAGING_DIR}/bin/VxVoice"

if [ -f "${SCRIPT_DIR}/resources/vxvoice.desktop" ]; then
    cp "${SCRIPT_DIR}/resources/vxvoice.desktop" "${STAGING_DIR}/vxvoice.desktop"
fi

if [ -f "${SCRIPT_DIR}/resources/icons/app_icon.png" ]; then
    cp "${SCRIPT_DIR}/resources/icons/app_icon.png" "${STAGING_DIR}/icons/app_icon.png"
fi

for res in 16 24 32 48 64 128 256; do
    if [ -f "${SCRIPT_DIR}/resources/icons/${res}x${res}/vxvoice.png" ]; then
        mkdir -p "${STAGING_DIR}/icons/${res}x${res}"
        cp "${SCRIPT_DIR}/resources/icons/${res}x${res}/vxvoice.png" "${STAGING_DIR}/icons/${res}x${res}/vxvoice.png"
    fi
done

if [ -d "${SCRIPT_DIR}/resources/sounds" ]; then
    cp "${SCRIPT_DIR}/resources/sounds/"*.wav "${STAGING_DIR}/sounds/" 2>/dev/null || true
fi

cat > "${STAGING_DIR}/install_vxvoice.sh" << 'EOF'
#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO="sudo"
fi

echo "=========================================="
echo "  Instalador VxVoice v2.0.3 (Linux)       "
echo "=========================================="

echo "[1/4] Instalando binário em /usr/bin/VxVoice..."
${SUDO} install -Dm755 "${SCRIPT_DIR}/bin/VxVoice" /usr/bin/VxVoice

echo "[2/4] Instalando ícones do sistema..."
if [ -f "${SCRIPT_DIR}/icons/app_icon.png" ]; then
    ${SUDO} install -Dm644 "${SCRIPT_DIR}/icons/app_icon.png" /usr/share/pixmaps/vxvoice.png
fi
for res in 16 24 32 48 64 128 256; do
    if [ -f "${SCRIPT_DIR}/icons/${res}x${res}/vxvoice.png" ]; then
        ${SUDO} install -Dm644 "${SCRIPT_DIR}/icons/${res}x${res}/vxvoice.png" "/usr/share/icons/hicolor/${res}x${res}/apps/vxvoice.png"
    fi
done

echo "[3/4] Instalando arquivos de áudio e atalho do menu..."
${SUDO} mkdir -p /usr/share/vxvoice/sounds
if ls "${SCRIPT_DIR}/sounds/"*.wav >/dev/null 2>&1; then
    ${SUDO} cp "${SCRIPT_DIR}/sounds/"*.wav /usr/share/vxvoice/sounds/
fi
if [ -f "${SCRIPT_DIR}/vxvoice.desktop" ]; then
    ${SUDO} install -Dm644 "${SCRIPT_DIR}/vxvoice.desktop" /usr/share/applications/vxvoice.desktop
fi

echo "[4/4] Atualizando cache de ícones e aplicativos..."
${SUDO} update-desktop-database /usr/share/applications 2>/dev/null || true
${SUDO} gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true

echo ""
echo "VxVoice v2.0.3 instalado com sucesso!"
echo "Execute 'VxVoice' no terminal ou abra pelo menu de aplicativos."
EOF

cat > "${STAGING_DIR}/uninstall_vxvoice.sh" << 'EOF'
#!/bin/bash
set -e

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO="sudo"
fi

echo "Removendo VxVoice..."
${SUDO} rm -f /usr/bin/VxVoice
${SUDO} rm -f /usr/share/applications/vxvoice.desktop
${SUDO} rm -f /usr/share/pixmaps/vxvoice.png
for res in 16 24 32 48 64 128 256; do
    ${SUDO} rm -f "/usr/share/icons/hicolor/${res}x${res}/apps/vxvoice.png"
done
${SUDO} rm -rf /usr/share/vxvoice
${SUDO} update-desktop-database /usr/share/applications 2>/dev/null || true
echo "VxVoice removido."
EOF

chmod +x "${STAGING_DIR}/install_vxvoice.sh" "${STAGING_DIR}/uninstall_vxvoice.sh"

rm -f "${ZIP_PATH}"
if command -v zip >/dev/null 2>&1; then
    (cd "${BUILD_DIR}" && zip -rq "${ZIP_PATH}" "vxvoice-${VERSION}-linux-installer")
else
    python3 -c "
import os, zipfile
base = '${STAGING_DIR}'
parent = os.path.dirname(base)
with zipfile.ZipFile('${ZIP_PATH}', 'w', zipfile.ZIP_DEFLATED) as zf:
    for root, dirs, files in os.walk(base):
        for f in files:
            full = os.path.join(root, f)
            rel = os.path.relpath(full, parent)
            zi = zipfile.ZipInfo.from_file(full, rel)
            zf.writestr(zi, open(full, 'rb').read())
"
fi

echo "==> Instalador .sh compactado em .zip gerado com sucesso:"
ls -lh "${ZIP_PATH}"
