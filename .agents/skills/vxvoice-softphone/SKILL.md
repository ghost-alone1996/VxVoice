---
name: vxvoice-softphone
description: >-
  Guia de desenvolvimento, arquitetura, compilação e integração do softphone SIP
  corporativo VxVoice em C++ e Qt 6 com backend PJSIP.
---

# VxVoice Softphone - Skill de Engenharia Sênior

Esta skill orienta o desenvolvimento do softphone corporativo **VxVoice** sob o papel de **Engenheiro de Software Sênior**, especialista em **PJSIP**, **C++17 / Qt 6 (Qt Widgets)** e **UX/UI moderno, intuitivo e autoexplicativo**.

---

## 🎯 1. Diretrizes de Engenharia Sênior e Qualidade

1. **Arquitetura & Código Limpo**:
   - Código padronizado, organizado, modular, seguro e eficiente com os recursos do Sistema Operacional.
   - Separação clara entre núcleo (`src/core/`), interface (`src/ui/`), recursos (`resources/`) e testes (`tests/`).
2. **UX/UI Moderno e Consistente**:
   - **Proporção Única na Tela Principal**: A janela principal (`MainWindow`) usa a aba `Ligação` como referência única (`380x670 px`). Nas abas `Histórico`, `Contatos` e `Gravação`, todas as colunas usam `QHeaderView::ResizeToContents` (`setStretchLastSection(false)`) com barras de rolagem (`Qt::ScrollBarAsNeeded`) para exibir conteúdo completo sem distorcer a janela.
   - **Contas SIP (`AccountsDialog`)**: Colunas ajustadas ao conteúdo com redimensionamento proporcional da janela.
   - **Temas VxVoice (`ThemeManager`)**: Suporte a 8 temas persistentes (`Dark`, `Light`, `Purple`, `Red`, `Blue Baby`, `Blue Capri`, `Rosa Amaranth`, `Rosa Hot Magenta`), com contraste adequado em cada tema.
3. **Testes Automatizados**:
   - Sempre compilar e executar a suíte de testes automatizados (`VxVoiceTests` via `ctest --output-on-failure`) para validar as funções existentes antes de fechar uma versão.
4. **Empacotamento Multiplataforma**:
   - **Linux**: gerar pacote `.deb` (`cpack -G DEB`) e instalador `.sh` compactado em `.zip` (`build_linux_zip.sh`).
   - **Windows**: gerar executável `.exe` e pacote corporativo `.msi` (`build_windows_msi.sh`).
5. **Versionamento, Histórico `.md` e Sincronismo GitHub**:
   - Toda entrega deve gerar uma nova versão semântica, atualizar o histórico em `Softphone-VxVoice/CHANGELOG.md` destacando as versões, criar commit + tag local e enviar ao repositório remoto (`git push origin main --tags` para `https://github.com/ghost-alone1996/VxVoice`).
   - Manter o atualizador automático do VxVoice (`UpdateManager`) integrado diretamente ao repositório GitHub.

---

## 🏗️ 2. Arquitetura do Projeto

- **Camada Core (`src/core/`)**:
  - `SipManager`: Motor SIP PJSIP (`pjsua2.hpp`) com sinais/slots thread-safe para GUI, codecs e logs SIP.
  - `SettingsManager`: Persistência em `QSettings`, autostart OS (Linux `.desktop` / Windows Registry), sub-opções e status de registro.
  - `ContactsManager`: Agenda corporativa com busca em tempo real e resolução de ramal/telefone.
  - `ThemeManager`: Gerenciamento de folhas de estilo QSS (`resources/styles/*.qss`) e `QPalette`.
  - `SoundManager`: Reprodução de ringtones e pré-escuta.
  - `UpdateManager`: Consulta direta à API/Raw do GitHub (`ghost-alone1996/VxVoice`), download de releases e instalação automática.
- **Camada UI (`src/ui/`)**:
  - `MainWindow`, `CallWidget`, `HistoryWidget`, `ContactsWidget`, `RecordingsWidget`, `ActiveCallDialog`, `CallNotificationPopup`, `SelectContactNumberDialog`, `AccountsDialog`, `SettingsDialog`, `ChangelogDialog`, `UpdateProgressDialog`, `IconHelper`.

---

## 🛠️ 3. Build, Testes e Empacotamento

```bash
# Compilar e rodar testes automatizados
cd Softphone-VxVoice/build
cmake ..
make -j$(nproc)
ctest --output-on-failure

# Empacotar Linux (.deb e .sh/.zip) e Windows (.msi)
cpack -G DEB
cp vxvoice-*.deb ../../
cd ../..
bash Softphone-VxVoice/build_linux_zip.sh
bash Softphone-VxVoice/build_windows_msi.sh
```
