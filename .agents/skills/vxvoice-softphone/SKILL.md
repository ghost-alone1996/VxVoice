---
name: vxvoice-softphone
description: >-
  Guia de desenvolvimento, arquitetura, compilação e integração do softphone SIP
  corporativo VxVoice em C++ e Qt 6 com backend PJSIP.
---

# VxVoice Softphone - Skill de Desenvolvimento e Manutenção

Esta skill documenta os fluxos operacionais, padrões de arquitetura, convenções de código e diretrizes de manutenção do softphone corporativo **VxVoice**, desenvolvido em **C++17** e **Qt 6 (Qt Widgets)**.

---

## 🏗️ 1. Visão Geral da Arquitetura

O sistema é dividido em três camadas desacopladas:

1. **Camada de Telefonia & Serviços (`src/core/`):**
   - `SipManager`: Interface de controle SIP baseada no padrão Observer (Signals & Slots) pronta para conectar à API C++ do PJSIP (`pjsua2.hpp`).
   - `SettingsManager`: Gerenciamento e persistência de preferências com `QSettings`, incluindo autostart no SO (Linux `.desktop` e Windows Registry) e preferência de minimização no System Tray.
   - `ThemeManager`: Gerenciamento dinâmico de folhas de estilo QSS (`VxVoice Roxo`, `Dark Clássico`, `Light`) e paletas do sistema.
   - `SoundManager`: Motor de áudio para ringtones em loop e pré-escuta de toques de chamada recebida.

2. **Camada de Apresentação & Telas (`src/ui/`):**
   - `MainWindow`: Janela principal compacta com cabeçalho de acesso rápido, status com bolinhas coloridas (Pixel-Perfect), visor digital e `QTabWidget` central.
   - `CallWidget`: Aba de discagem com display, teclado numérico 4x3 e botões de chamada e encerramento.
   - `HistoryWidget`: Histórico categorizado de chamadas (Recebida ↙, Efetuada ↗, Perdida ✕) com menu de contexto (clique direito) para ligar, salvar contato, copiar número, excluir e ouvir gravação.
   - `ContactsWidget`: Agenda corporativa com filtro duplo (texto e combobox de empresa), janela completa de cadastro e botões de ligar e enviar e-mail na mesma escala.
   - `RecordingsWidget`: Gestão de gravações de áudio vinculadas aos contatos e ao histórico de chamadas, com reprodução e abertura de diretório.
   - `ActiveCallDialog`: Janela dedicada para ligações ativas com cronômetro, mute, hold, transferência SIP, gravação e DTMF.
   - `CallNotificationPopup`: Toast flutuante sem bordas (Frameless) com botões redondos de atender (verde com fone) e recusar (vermelho com X).
   - `IconHelper`: Gerador vetorial em tempo de execução via `QPainter` para garantir nitidez dos ícones em qualquer sistema operacional.

3. **Recursos & Identidade Visual (`resources/`):**
   - `styles/vxvoice.qss`: Tema oficial corporativo roxo (`#171226`, `#1E1632`, `#8E44AD`).
   - `styles/dark.qss`: Tema escuro neutro corporativo (`#1A2530`).
   - `styles/light.qss`: Tema claro corporativo (`#F5F7FA`).
   - `icons/app_icon.png`: Ícone oficial com as iniciais "VX" em degradê roxo.
   - `sounds/`: Arquivos de áudio para toques de chamada (`ringtone_1` a `ringtone_4`).

---

## 🛠️ 2. Procedimentos de Build e Execução

### Pré-requisitos
- Compilador C++17 (GCC 9+, Clang 10+ ou MSVC 2019+)
- CMake 3.16+
- Qt 6 (Componentes: `Core`, `Gui`, `Widgets`, `Network`)

### Compilação Padrão
```bash
cd Softphone-VxVoice
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Execução
```bash
# Execução normal
./VxVoice

# Iniciar minimizado na bandeja do sistema
./VxVoice --autostart
```

---

## 📐 3. Padrões de Layout e Dimensionamento

- **Proporção da MainWindow:** `380x640 px` (Fixado via `setMinimumSize(380, 640); resize(380, 640);`).
- **Proporção da ActiveCallDialog:** `350x540 px` (Fixado via `setMinimumSize(350, 540); resize(350, 540);`), abrindo adjacente à janela principal.
- **Tabelas com Ajuste ao Maior Conteúdo:**
  Todas as tabelas de listagem (`QTableView`) devem utilizar:
  ```cpp
  m_tableView->verticalHeader()->setVisible(false);
  m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  m_tableView->horizontalHeader()->setStretchLastSection(false);
  m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  ```
  Isso assegura que nenhuma coluna sofra truncamento com reticências e que a rolagem horizontal apareça apenas quando o conteúdo exceder a largura da tela.

---

## 🔌 4. Integração com Backend PJSIP (PJSUA2)

Para plugar o motor de telefonia físico:
1. No `CMakeLists.txt`, habilitar `ENABLE_PJSIP_REAL=ON`.
2. Em `SipManager.cpp`, inicializar o `pj::Endpoint` e registrar contas com `pj::Account`.
3. Direcionar os eventos recebidos nos callbacks do PJSIP para a GUI Thread utilizando `QMetaObject::invokeMethod(this, ...)` ou `emit callStateChanged(...)`.

