#!/bin/bash
set -e

# ==============================================================================
# Script de Desinstalação Completa do VxVoice Softphone (Linux)
# Remove processos, pacote .deb, binários, bibliotecas PJSIP embarcadas,
# ícones, atalhos, autostart, configurações e dados locais.
# Uso: ./uninstall_vxvoice.sh [--keep-recordings]
# ==============================================================================

KEEP_RECORDINGS=0
for arg in "$@"; do
    case "$arg" in
        --keep-recordings)
            KEEP_RECORDINGS=1
            ;;
        -h|--help)
            echo "Uso: $0 [--keep-recordings]"
            echo "  --keep-recordings   Preserva a pasta de gravações em Documentos/VxVoice"
            exit 0
            ;;
    esac
done

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO="sudo"
fi

# Resolve o usuário e HOME reais mesmo quando executado via sudo
TARGET_USER="${SUDO_USER:-$USER}"
TARGET_HOME="$(getent passwd "${TARGET_USER}" | cut -d: -f6)"
if [ -z "${TARGET_HOME}" ]; then
    TARGET_HOME="${HOME}"
fi

echo "=========================================="
echo "  Desinstalação Completa — VxVoice (Linux)"
echo "=========================================="

echo "[1/6] Encerrando processos do VxVoice..."
${SUDO} pkill -9 -x VxVoice 2>/dev/null || true
${SUDO} killall -9 VxVoice 2>/dev/null || true
rm -f /tmp/VxVoice_SingleInstance_IPC 2>/dev/null || true
sleep 0.2

echo "[2/6] Verificando e removendo pacote .deb (se instalado)..."
if command -v dpkg >/dev/null 2>&1 && dpkg -s vxvoice >/dev/null 2>&1; then
    if command -v apt-get >/dev/null 2>&1; then
        ${SUDO} apt-get purge -y vxvoice || ${SUDO} dpkg --purge vxvoice || true
    else
        ${SUDO} dpkg --purge vxvoice || true
    fi
fi

echo "[3/6] Removendo binários, bibliotecas PJSIP e recursos do sistema..."
${SUDO} rm -f /usr/bin/VxVoice /usr/local/bin/VxVoice
${SUDO} rm -rf /usr/lib/vxvoice /usr/local/lib/vxvoice
${SUDO} rm -rf /usr/share/vxvoice /usr/local/share/vxvoice
${SUDO} rm -f /usr/share/applications/vxvoice.desktop /usr/local/share/applications/vxvoice.desktop
${SUDO} rm -f /usr/share/pixmaps/vxvoice.png /usr/local/share/pixmaps/vxvoice.png

for res in 16 24 32 48 64 128 256 scalable; do
    ${SUDO} rm -f "/usr/share/icons/hicolor/${res}x${res}/apps/vxvoice.png"
    ${SUDO} rm -f "/usr/local/share/icons/hicolor/${res}x${res}/apps/vxvoice.png"
done

echo "[4/6] Removendo configurações, autostart e cache do usuário (${TARGET_USER})..."
rm -rf "${TARGET_HOME}/.config/VxVoiceCorp"
rm -rf "${TARGET_HOME}/.config/VxVoice"
rm -f  "${TARGET_HOME}/.config/autostart/vxvoice.desktop"
rm -rf "${TARGET_HOME}/.local/share/VxVoiceCorp"
rm -rf "${TARGET_HOME}/.local/share/VxVoice"
rm -rf "${TARGET_HOME}/.cache/VxVoiceCorp"
rm -rf "${TARGET_HOME}/.cache/VxVoice"

echo "[5/6] Tratando pasta de gravações do usuário..."
if [ "${KEEP_RECORDINGS}" -eq 1 ]; then
    echo "      Gravações preservadas (--keep-recordings)."
else
    DOCS_DIR=""
    if command -v xdg-user-dir >/dev/null 2>&1; then
        DOCS_DIR="$(su - "${TARGET_USER}" -c 'xdg-user-dir DOCUMENTS' 2>/dev/null || true)"
    fi
    for dir in "${DOCS_DIR}/VxVoice" "${TARGET_HOME}/Documentos/VxVoice" "${TARGET_HOME}/Documents/VxVoice"; do
        if [ -n "${dir}" ] && [ -d "${dir}" ]; then
            rm -rf "${dir}"
        fi
    done
fi

echo "[6/6] Atualizando caches do sistema..."
${SUDO} ldconfig 2>/dev/null || true
${SUDO} update-desktop-database /usr/share/applications 2>/dev/null || true
${SUDO} gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true

echo ""
echo "VxVoice foi completamente desinstalado do sistema."
