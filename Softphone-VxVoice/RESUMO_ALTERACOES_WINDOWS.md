# Resumo de Alterações para Windows — VxVoice (v2.0.4 → v2.0.8)

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

## 3. Novidades e Melhorias (v2.0.6)

### Temas Clássicos Windows (`resources/styles/win2000.qss`, `winxp.qss`, `win7.qss`, `ThemeManager.*`, `MainWindow.cpp`, `CallWidget.cpp`, `CallNotificationPopup.cpp`, `SettingsDialog.cpp`)
- Adicionados três novos temas clássicos fiéis ao visual histórico do Windows:
  - **Windows 2000 Clássico (`win2000.qss`)**: cinza `#D4D0C8`, bordas chanfradas 3D (`outset`/`inset`), azul marinho `#0A246A` na seleção/foco e cantos retos.
  - **Windows XP Luna (`winxp.qss`)**: fundo `#ECE9D8`, barra/destaque azul Luna `#0054E3`/`#316AC5`, botões com gradiente suave e bordas `#003C74`.
  - **Windows 7 Aero (`win7.qss`)**: superfícies translúcidas/azul-claro `#F0F4F9` / `#EAF3FC`, gradientes vítreos `#EAF6FD` → `#BEE6FD` e realces `#3C7FB1`.
- Todos os novos temas foram integrados ao `resources.qrc`, `ThemeManager` (`isLightTheme`, `themeName`, `themeToString`, `themeFromString`, `toggleTheme`), menu de bandeja (`System Tray`), `SettingsDialog`, `CallWidget` e `CallNotificationPopup`.

### Contatos com Múltiplos Números (`ContactsManager.*`, `ContactsWidget.*`, `SelectContactNumberDialog.cpp`, `CallWidget.cpp`)
- **`ContactEditDialog`**: adicionado botão **`+ Adicionar Número`** permitindo cadastrar múltiplos números adicionais por contato, com botão de remoção por linha e rolagem automática.
- **Persistência e Busca (`ContactsManager`)**: suporte a `additionalNumbers` (`QStringList`) e `allNumbers()` na serialização JSON, busca por número (`findContactNameByNumber`), autocompletar (`searchContacts`) e exibição `(+N)` na tabela de contatos.
- **`SelectContactNumberDialog`**: janela redimensionada dinamicamente (`setMinimumWidth(460)`, com `QScrollArea` para listas extensas) listando o Ramal SIP, Telefone principal e todos os números adicionais do contato para seleção antes da chamada.

### Configurações — Aba Perfil SIP (`SettingsManager.*`, `SettingsDialog.*`)
- Adicionada a aba **`🌐 Perfil SIP`** nas Configurações com listagem dos perfis cadastrados (`Nome`, `Servidor SIP/Host`, `Porta`, `Domínio PBX`, `Transporte`) e botões **Adicionar**, **Editar**, **Copiar** e **Excluir**.
- Diálogo de criação/edição de Perfil SIP contemplando os campos: `Nome do Perfil`, `Servidor SIP / Host`, `Porta SIP`, `Domínio PBX` e `Transporte` (`UDP`, `TCP`, `TLS`).

### Conta SIP integrada ao Perfil SIP (`AccountsDialog.*`, `SettingsManager.*`)
- Adicionado seletor **`Perfil SIP:`** na janela de criação/edição de Conta SIP (`AccountEditDialog`).
- Quando um Perfil SIP cadastrado é selecionado, os campos já definidos no perfil (`Servidor SIP / Host`, `Porta SIP`, `Domínio PBX` e `Transporte`) são ocultados automaticamente (`setRowVisible(..., false)`), mantendo apenas **`Nome de Exibição`**, **`Usuário / Ramal`**, **`Senha`** e **`Habilitar`**. Ao selecionar `"Configuração Manual (Personalizada)"`, os campos voltam a ser exibidos.

---

## 4. Correções e Melhorias (v2.0.7)

### Correções de Temas Clássicos e Contraste (`CallWidget.cpp`, `MainWindow.cpp`, `SettingsDialog.cpp`, `ChangelogDialog.cpp`, `UpdateProgressDialog.cpp`)
- Substituídas verificações isoladas de `ThemeManager::Theme::Light` por `ThemeManager::isLightTheme(theme)` em `CallWidget` (`updateAccountDisplay`, `refreshStatusLine`), `SettingsDialog` (`m_lblAboutTitle`), `ChangelogDialog` e `UpdateProgressDialog`, corrigindo textos brancos/claros sobre fundos claros nos temas **Windows 2000**, **Windows XP** e **Windows 7**.
- Ajustado `MainWindow::updateHeaderIcons()` para aplicar ícones escuros (`#1E293B`) em todos os temas claros, incluindo **Windows XP**, garantindo visibilidade dos botões de Contas e Configurações no cabeçalho.

### Aba Sobre e Novidades e Correções (`SettingsDialog.cpp`, `ChangelogDialog.cpp`)
- Renomeado o botão da aba Sobre para **`Ver Novidades e Correções`**.
- Removido o bloco de resumo fixo antigo (`v2.0.4`) exibido dentro da aba Sobre.
- Atualizado o conteúdo de `ChangelogDialog` para exibir corretamente as alterações da versão atual (`v2.0.7`).

### Melhorias de Usabilidade (`MainWindow.cpp`, `CallWidget.cpp`, `RecordingsWidget.*`)
- **Proporção da Janela Principal:** Reduzida em 10% (`400x720` → `360x648`, mínimo `350x630`), com ajuste proporcional dos botões do discador para manter todo o conteúdo visível sem cortes.
- **Aba Gravação:** Removido o botão **Ligar** da barra inferior de ações em `RecordingsWidget`.

---

## 5. Correções (v2.0.8)

- **Tela "Ligar para..." (`SelectContactNumberDialog.cpp`)**: Os botões de seleção de número/ramal e o subtítulo agora respeitam os temas clássicos do Windows (`Win2000`, `WinXP`, `Win7`) e `ThemeManager::isLightTheme()`, removendo estilos fixos que não se adaptavam ao tema ativo.

---

## 6. Verificação e Publicação

- **Testes automatizados (`VxVoiceTests.exe`)**: compilado e executado após as alterações com todos os testes aprovados.
- **Build & Empacotamento `.msi`:** `VxVoice.exe`, `vxvoice-2.0.8-Windows.msi` e `vxvoice-2.0.8-x64.msi` gerados via `build_windows_msi.ps1` (Qt 6.7.3 + MinGW 13.1 + PJSIP 2.14.1 + WiX 3.14).
- **Release v2.0.8:** Commit, tag `v2.0.8` e Release publicadas no GitHub (`ghost-alone1996/VxVoice`) com os pacotes `vxvoice-2.0.8-Windows.msi` e `vxvoice-2.0.8-x64.msi` anexados.
