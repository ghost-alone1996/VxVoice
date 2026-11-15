#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
OUTPUT_DIR="${SCRIPT_DIR}/.."

# Extrai a versão SemVer diretamente do CMakeLists.txt
VERSION=$(grep -E '^[[:space:]]*VERSION[[:space:]]+[0-9]+\.[0-9]+\.[0-9]+' "${SCRIPT_DIR}/CMakeLists.txt" | head -n1 | awk '{print $2}')
if [ -z "${VERSION}" ]; then
    echo "Erro: Não foi possível detectar a versão no CMakeLists.txt."
    exit 1
fi

STAGING_DIR="${BUILD_DIR}/vxvoice-${VERSION}-linux-installer"
ZIP_PATH="${OUTPUT_DIR}/vxvoice-${VERSION}-Linux-Installer.zip"
SH_PATH="${OUTPUT_DIR}/vxvoice-${VERSION}-Linux-Installer.sh"

if [ ! -f "${BUILD_DIR}/VxVoice" ]; then
    echo "Erro: Binário ${BUILD_DIR}/VxVoice não encontrado. Compile o projeto primeiro."
    exit 1
fi

echo "==> Preparando instalador universal Linux e pacote .zip (v${VERSION})..."
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}/bin"
mkdir -p "${STAGING_DIR}/lib/vxvoice"
mkdir -p "${STAGING_DIR}/icons"
mkdir -p "${STAGING_DIR}/sounds"

# Copia o binário com RPATH de instalação aplicado via cmake --install ou patchelf/stage
TMP_INSTALL="${BUILD_DIR}/_stage_install"
rm -rf "${TMP_INSTALL}"
DESTDIR="${TMP_INSTALL}" cmake --install "${BUILD_DIR}" --prefix /usr >/dev/null

if [ -f "${TMP_INSTALL}/usr/bin/VxVoice" ]; then
    cp "${TMP_INSTALL}/usr/bin/VxVoice" "${STAGING_DIR}/bin/VxVoice"
else
    cp "${BUILD_DIR}/VxVoice" "${STAGING_DIR}/bin/VxVoice"
fi
chmod +x "${STAGING_DIR}/bin/VxVoice"

if [ -d "${TMP_INSTALL}/usr/lib/vxvoice" ]; then
    cp -a "${TMP_INSTALL}/usr/lib/vxvoice/"*.so* "${STAGING_DIR}/lib/vxvoice/" 2>/dev/null || true
fi
rm -rf "${TMP_INSTALL}"

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

cat > "${STAGING_DIR}/install_vxvoice.sh" << EOF
#!/bin/bash
set -e

SCRIPT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
SUDO=""
if [ "\$(id -u)" -ne 0 ]; then
    SUDO="sudo"
fi

echo "=========================================="
echo "  Instalador Universal VxVoice v${VERSION} (Linux)"
echo "=========================================="

echo "[1/5] Verificando dependências de runtime (Qt6 / ALSA)..."
if command -v apt-get >/dev/null 2>&1; then
    if ! dpkg -s libqt6widgets6 >/dev/null 2>&1 && ! dpkg -s libqt6widgets6t64 >/dev/null 2>&1; then
        \${SUDO} apt-get update -qq || true
        \${SUDO} apt-get install -y libqt6widgets6t64 libqt6network6t64 libqt6gui6t64 libqt6core6t64 libasound2t64 || \\
        \${SUDO} apt-get install -y libqt6widgets6 libqt6network6 libqt6gui6 libqt6core6 libasound2 || true
    fi
elif command -v dnf >/dev/null 2>&1; then
    rpm -q qt6-qtbase-gui >/dev/null 2>&1 || \${SUDO} dnf install -y qt6-qtbase qt6-qtbase-gui alsa-lib || true
elif command -v pacman >/dev/null 2>&1; then
    pacman -Qi qt6-base >/dev/null 2>&1 || \${SUDO} pacman -Sy --noconfirm qt6-base alsa-lib || true
elif command -v zypper >/dev/null 2>&1; then
    rpm -q libQt6Widgets6 >/dev/null 2>&1 || \${SUDO} zypper --non-interactive install libQt6Widgets6 libQt6Network6 libQt6Gui6 libQt6Core6 libasound2 || true
fi

echo "[2/5] Instalando binário em /usr/bin/VxVoice e bibliotecas PJSIP em /usr/lib/vxvoice..."
\${SUDO} install -Dm755 "\${SCRIPT_DIR}/bin/VxVoice" /usr/bin/VxVoice
\${SUDO} mkdir -p /usr/lib/vxvoice
if ls "\${SCRIPT_DIR}/lib/vxvoice/"*.so* >/dev/null 2>&1; then
    \${SUDO} cp -a "\${SCRIPT_DIR}/lib/vxvoice/"*.so* /usr/lib/vxvoice/
fi
\${SUDO} ldconfig 2>/dev/null || true

echo "[3/5] Instalando ícones do sistema..."
if [ -f "\${SCRIPT_DIR}/icons/app_icon.png" ]; then
    \${SUDO} install -Dm644 "\${SCRIPT_DIR}/icons/app_icon.png" /usr/share/pixmaps/vxvoice.png
fi
for res in 16 24 32 48 64 128 256; do
    if [ -f "\${SCRIPT_DIR}/icons/\${res}x\${res}/vxvoice.png" ]; then
        \${SUDO} install -Dm644 "\${SCRIPT_DIR}/icons/\${res}x\${res}/vxvoice.png" "/usr/share/icons/hicolor/\${res}x\${res}/apps/vxvoice.png"
    fi
done

echo "[4/5] Instalando arquivos de áudio e atalho do menu..."
\${SUDO} mkdir -p /usr/share/vxvoice/sounds
if ls "\${SCRIPT_DIR}/sounds/"*.wav >/dev/null 2>&1; then
    \${SUDO} cp "\${SCRIPT_DIR}/sounds/"*.wav /usr/share/vxvoice/sounds/
fi
if [ -f "\${SCRIPT_DIR}/vxvoice.desktop" ]; then
    \${SUDO} install -Dm644 "\${SCRIPT_DIR}/vxvoice.desktop" /usr/share/applications/vxvoice.desktop
fi

echo "[5/5] Atualizando cache de ícones e aplicativos..."
\${SUDO} update-desktop-database /usr/share/applications 2>/dev/null || true
\${SUDO} gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true

echo ""
echo "VxVoice v${VERSION} instalado com sucesso!"
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
${SUDO} pkill -9 -x VxVoice 2>/dev/null || true
${SUDO} rm -f /usr/bin/VxVoice
${SUDO} rm -rf /usr/lib/vxvoice
${SUDO} rm -f /usr/share/applications/vxvoice.desktop
${SUDO} rm -f /usr/share/pixmaps/vxvoice.png
for res in 16 24 32 48 64 128 256; do
    ${SUDO} rm -f "/usr/share/icons/hicolor/${res}x${res}/apps/vxvoice.png"
done
${SUDO} rm -rf /usr/share/vxvoice
${SUDO} ldconfig 2>/dev/null || true
${SUDO} update-desktop-database /usr/share/applications 2>/dev/null || true
echo "VxVoice removido."
EOF

chmod +x "${STAGING_DIR}/install_vxvoice.sh" "${STAGING_DIR}/uninstall_vxvoice.sh"

# 1. Gera pacote .zip contendo a pasta do instalador
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

# 2. Gera instalador universal self-extracting (.sh)
TAR_PAYLOAD="${BUILD_DIR}/vxvoice-${VERSION}-payload.tar.gz"
rm -f "${TAR_PAYLOAD}" "${SH_PATH}"
tar -czf "${TAR_PAYLOAD}" -C "${BUILD_DIR}" "vxvoice-${VERSION}-linux-installer"

cat > "${SH_PATH}" << EOF
#!/bin/bash
set -e

echo "Extraindo VxVoice v${VERSION} (Instalador Universal Linux)..."
TMP_DIR="\$(mktemp -d)"
trap 'rm -rf "\${TMP_DIR}"' EXIT

ARCHIVE_LINE=\$(awk '/^__ARCHIVE_BELOW__$/ {print NR + 1; exit 0;}' "\$0")
tail -n +"\${ARCHIVE_LINE}" "\$0" | tar -xz -C "\${TMP_DIR}"
bash "\${TMP_DIR}/vxvoice-${VERSION}-linux-installer/install_vxvoice.sh"
exit 0

__ARCHIVE_BELOW__
EOF

cat "${TAR_PAYLOAD}" >> "${SH_PATH}"
chmod +x "${SH_PATH}"
rm -f "${TAR_PAYLOAD}"

echo "==> Artefatos Linux gerados com sucesso:"
ls -lh "${ZIP_PATH}" "${SH_PATH}"
