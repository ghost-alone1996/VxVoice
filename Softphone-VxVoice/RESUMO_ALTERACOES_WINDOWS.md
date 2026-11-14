# Resumo de Alterações para Windows — VxVoice (v2.0.4 → v2.0.5)

Este documento registra todas as adaptações, correções de bugs e melhorias implementadas para a versão Windows do **VxVoice Softphone (Qt 6 + PJSIP / PJSUA2)**.

---

## 1. Infraestrutura de Build e Empacotamento `.msi` (v2.0.4 / v2.0.5)

- **`CMakeLists.txt`**:
  - Adicionado suporte a `PJSIP_ROOT` (padrão `C:/Qt/pjsip-mingw64`) no bloco `if(WIN32 AND EXISTS "${PJSIP_ROOT}/include/pjsua2.hpp")`, linkando as bibliotecas estáticas do PJSIP 2.14.1 (`pjsua2`, `pjsua`, `pjsip-ua`, `pjsip-simple`, `pjsip`, `pjmedia*`, `pjnath`, `pjlib-util`, `srtp`, `resample`, codecs e bibliotecas do sistema Windows: `winmm`, `ole32`, `ws2_32`, `wsock32`, `iphlpapi`, `dsound`, `uuid`, `pthread`) sem depender de `pkg-config` no Windows.
  - Mantido o fluxo original via `PkgConfig` (`libpjproject`) para Linux/Unix.
- **`build_windows_msi.ps1`**:
  - Script PowerShell automatizado para configurar o CMake (`MinGW Makefiles` + Qt 6.7.3 + MinGW 13.1), compilar `VxVoice.exe` e `VxVoiceTests.exe`, executar `windeployqt6.exe` (copiando DLLs do Qt 6, runtime MinGW e plugins) e gerar os instaladores `vxvoice-<versao>-x64.msi` e `vxvoice-<versao>-Windows.msi` via **WiX Toolset 3.14** (`heat.exe`, `candle.exe`, `light.exe`).
- **`build_win/vxvoice_qt.wxs`**:
  - Definição WiX v3 (x64, `perMachine`, idioma `1046` pt-BR) com interface `WixUI_InstallDir`, atalhos na Área de Trabalho e Menu Iniciar, registro de protocolo `sip:` e `tel:`, informações corporativas no Registro (`HKLM\Software\VxNET\VxVoice`) e `MajorUpgrade`.

---

## 2. Correções de Bugs e Melhorias (v2.0.4 → v2.0.5)

### Áudio / Toques de Chamada Recebida (`src/core/SoundManager.cpp`)
- **Problema:** `playAudioFile()` verificava apenas players do Linux (`/usr/bin/pw-play`, `paplay`, `aplay`, `gst-play-1.0`), fazendo com que os toques não reproduzissem no Windows.
- **Correção:** Adicionada implementação nativa para Windows (`#ifdef Q_OS_WIN`) utilizando `PlaySoundW` (`<windows.h>`, `<mmsystem.h>`, linkado com `winmm`):
  - Reprodução assíncrona (`SND_FILENAME | SND_ASYNC | SND_NODEFAULT`) e em loop (`SND_LOOP`) durante chamadas recebidas (`m_isRinging`).
  - `stopAudioProcess()` chama `PlaySoundW(nullptr, nullptr, 0)` para interromper o toque imediatamente.

### Encerramento Seguro do PJSIP (`src/core/SipManager.cpp`)
- **Problema:** Ao clicar em **Sair**, `MainWindow::onQuitApplication()` chamava `SipManager::shutdown()` (destruindo `m_endpoint` via `libDestroy()`), e na sequência `MainWindow::~MainWindow()` chamava `shutdown()` novamente. A segunda chamada executava `ensurePjThread()` -> `pj_thread_is_registered()` após o `libDestroy()`, disparando asserção fatal em `os_core_win32.c:657`.
- **Correção:**
  - Adicionada guarda `if (!m_endpoint)` no início de `SipManager::shutdown()` para evitar duplo encerramento.
  - Adicionada flag `s_pjInitialized` em `ensurePjThread()` (ativada após `libCreate()` e desativada antes de `libDestroy()`), impedindo chamadas ao PJLIB quando o subsistema não está ativo.

### Identificação de Chamada Recebida (`src/core/SipManager.cpp`)
- **Problema:** Ao receber chamada de um número não salvo na agenda, `handleIncomingCall()` usava `remoteContact` (`sip:numero@servidor`) como `callerName`, exibindo a URI SIP completa no popup e na janela de chamada ativa.
- **Correção:** Quando o contato não é encontrado no `ContactsManager`, `callerName` recebe apenas `callerNumber` (somente o número limpo).

### System Tray, Temas e UX/UI (`src/ui/MainWindow.cpp` e `resources/styles/*.qss`)
- **System Tray:**
  - Item `"Sair Definitivo"` renomeado para `"Sair"`.
  - Adicionado submenu **`Tema`** (`Roxo Corporativo (VxVoice)`, `Escuro (Dark)`, `Claro (Light)`, `Vermelho (Red)`) sincronizado com `ThemeManager`.
  - Adicionados estilos `QMenu`, `QMenu::item`, `QMenu::item:selected` e `QMenu::separator` nos 4 arquivos `.qss` (`vxvoice.qss`, `dark.qss`, `light.qss`, `red.qss`).
- **Ajuste de Janela:**
  - Tamanho inicial ajustado para `400x720` (com limite adaptativo à área útil da tela via `QScreen::availableGeometry()`) e mínimo `390x700` para exibir todo o discador e botões sem cortes.

---

## 3. Verificação e Publicação

- **Testes automatizados:** `VxVoiceTests.exe` compilado e executado com código de saída `0`.
- **Release v2.0.5:** Tag `v2.0.5` e Release publicadas no GitHub (`ghost-alone1996/VxVoice`) com os pacotes `vxvoice-2.0.5-Windows.msi` e `vxvoice-2.0.5-x64.msi` anexados.
