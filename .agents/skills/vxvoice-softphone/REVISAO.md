# VxVoice Softphone - Revisão Geral de Arquitetura e Implementação

Este documento apresenta a revisão consolidada de todos os módulos, interfaces gráficas, recursos de áudio, integração com o sistema operacional e subsistema SIP implementados no projeto **VxVoice**.

---

## 📋 1. Stack Tecnológico e Diretrizes de Projeto

- **Linguagem:** C++17 (padrão ISO/IEC 14882:2017)
- **Framework Gráfico:** Qt 6 (módulos `QtCore`, `QtGui`, `QtWidgets`, `QtNetwork`)
- **Sistema de Build:** CMake 3.16+ com suporte a `CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, `CMAKE_AUTOUIC`
- **Backend SIP:** Arquitetura desacoplada via *Signals and Slots*, preparada para o stack PJSIP (PJSUA2 C++ API)
- **Design System:** QSS dinâmico com paleta personalizada **VxVoice Corporate Purple**, **Dark Clássico** e **Light**.

---

## 🎨 2. Identidade Visual e Estilização

### 2.1. Temas Disponíveis
1. **VxVoice (Roxo Corporativo - Padrão):**
   - Fundo da janela: `#171226`
   - Painéis e Visor: `#1E1632`
   - Bordas e divisores: `#3E2A5D`
   - Acentos e Destaques: `#8E44AD` / `#9B59B6`
   - Tipografia: `#F5EEF8`
2. **Dark (Escuro Neutro):**
   - Base `#1A2530`, painéis `#243342`, campos `#141C24` e acentos `#3498DB`.
3. **Light (Claro Corporativo):**
   - Base `#F5F7FA`, painéis `#FFFFFF`, bordas `#BDC3C7` e acentos `#2980B9`.

### 2.2. Botões de Ação Padronizados
- **Iniciar Chamada:** Obrigatoriamente verde `#27AE60` (hover `#2ECC71`, pressed `#1E8449`).
- **Encerrar Chamada:** Obrigatoriamente vermelho `#C0392B` (hover `#E74C3C`, pressed `#922B21`).
- **Botão Sair:** Vermelho `#C0392B` para encerramento do processo em segundo plano.

---

## 📱 3. Telas e Componentes Implementados

### 3.1. Janela Principal (`MainWindow`)
- **Proporção Padrão:** `380x640 px`.
- **Cabeçalho de Acesso Rápido:**
  - Seletor de Presença SIP com bolinhas renderizadas via `QPainter` (Pixel-Perfect):
    - 🟢 **Disponível:** Bolinha verde sólida `#2ECC71`.
    - 🔴 **Não Perturbe:** Bolinha vermelha sólida `#E74C3C`.
    - ⚪ **Offline:** Bolinha cinza sólida `#95A5A6`.
  - Botão de Contas SIP com ícone de silhueta de usuário corporativo.
  - Botão de Configurações com ícone de engrenagem.
  - Botão de Notificação/Teste com ícone de sino dourado.
  - Botão vermelho destacado **Sair**, que finaliza o aplicativo e todos os processos.
- **Visor Digital:**
  - Exibe a conta SIP ativa (`👤 Ramal Principal (1001) (pbx.empresa.local)`) e data/hora em tempo real.
- **Abas Principais (QTabWidget):**
  1. `📞 Ligação`: Discador, teclado numérico 4x3 (com letras associadas) e botões Ligar/Encerrar.
  2. `🕒 Histórico`: Chamadas com ícone colorido por tipo (Recebida ↙, Efetuada ↗, Perdida ✕), menu pop-up de botão direito (ligar, salvar contato, copiar número, ouvir gravação e excluir) e rolagem horizontal quando necessário.
  3. `👥 Contatos`: Filtro duplo (busca por texto e combobox de empresa), botões de Ligar e Enviar E-mail na mesma escala uniforme (32 px) e janela de cadastro completa.
  4. `🎙 Gravação`: Lista de gravações mapeadas com contatos e histórico, reprodução direta, discagem e abertura de diretório.

### 3.2. Janela de Chamada em Curso (`ActiveCallDialog`)
- **Proporção Padrão:** `350x540 px`, abrindo posicionada lado a lado com a janela principal.
- **Controles Integrados:**
  - Avatar, Caller ID, nome, ramal e empresa.
  - Cronômetro de chamada em tempo real (`00:00`).
  - Botão **🎤 Mute** (alterna cor para vermelho quando mutado).
  - Botão **⏸ Espera (Hold / Resume)**.
  - Botão **🔄 Transferência de Chamadas** (abre prompt para informar destino).
  - Botão **⏺ Gravação** (Iniciar/Parar gravação da chamada atual).
  - Botão **👥 Conferência**.
  - **Teclado DTMF retrátil** para navegação em URAs de atendimento.
  - Botão grande e acessível de **📴 Encerrar Chamada**.

### 3.3. Pop-up Flutuante de Chamada Recebida (`CallNotificationPopup`)
- Janela flutuante sem bordas (`Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool`).
- Posicionamento automático no canto inferior direito da tela (`QScreen::availableGeometry()`).
- Exibe o Caller ID e identificação do chamador.
- **Botões Redondos Modernos:**
  - Botão verde redondo (48x48 px, raio 24 px) com ícone de fone de telefone para **Atender**.
  - Botão vermelho redondo (48x48 px, raio 24 px) com ícone de "X" para **Recusar/Encerrar**.

### 3.4. Diálogos Secundários
- **Gerenciador de Contas SIP (`AccountsDialog`):**
  - Tabela com status, nome da conta, servidor, ramal e indicação de conta padrão.
  - Adição, edição, exclusão e definição de conta ativa com suporte a UDP, TCP e TLS.
- **Preferências Corporativas (`SettingsDialog`):**
  - *Gerais:* Iniciar com o SO, iniciar minimizado na bandeja, **minimizar no X vs fechar**, seletor de toques de chamada com botão **▶ Ouvir Toque / ⏹ Parar**, e seletor de temas.
  - *Áudio e Voz:* Dispositivos de entrada e saída, sliders de volume e cancelamento de eco acústico (AEC).
  - *Codecs:* Lista ordenável via **Drag & Drop** com checkboxes para priorização de codecs (Opus, G722, PCMA, PCMU, G729).
  - *Gravação:* Seletor de diretório local e formato de áudio (WAV/MP3).
  - *Sobre:* Logo VxVoice, versão 1.0.0 e créditos corporativos.

---

## 🎵 4. Sistema de Áudio e Toques (`SoundManager`)

- **Novos Toques Corporativos Nativos (WAV 16-bit PCM):**
  - `Cisco Ringtone 1` (`ringtone-cisco-1.wav`)
  - `Cisco Ringtone 2` (`ringtone-cisco-2.wav`)
  - `Cisco Ringtone 3` (`ringtone-cisco-3.wav`)
  - `iPhone Ringtone` (`iphone_ringtone.wav`)
- **Cadeia de Prioridade de Players do SO:**
  1. `canberra-gtk-play -f <arquivo.wav>` (Player nativo de eventos de desktop GNOME/Ubuntu)
  2. `pw-play <arquivo.wav>` (PipeWire nativo)
  3. `paplay <arquivo.wav>` (PulseAudio nativo)
  4. `aplay -q <arquivo.wav>` (ALSA direto no hardware)
  5. `gst-play-1.0 --no-interactive --audiosink=autoaudiosink <arquivo.wav>`
- **Loop Infalível e Herança de Sessão:**
  - O processo herda explicitamente `QProcessEnvironment::systemEnvironment()` (`PULSE_SERVER`, `PIPEWIRE_RUNTIME_DIR`, `XDG_RUNTIME_DIR`).
  - Loop automático conectado ao sinal `finished` do `QProcess` com intervalo de 500ms entre toques enquanto `m_isRinging == true`.
  - Interrupção imediata via `stopRinging()` ao atender, recusar ou encerrar a ligação.

---

## 🖥️ 5. Integração com o Sistema Operacional

- **System Tray (`QSystemTrayIcon`):**
  - Opção em *Configurações > Gerais* permite escolher se o botão "X" minimiza para a bandeja ou fecha a aplicação.
  - Menu de contexto na bandeja com opções para Abrir, Simular Chamada e Sair.
- **Autostart:**
  - Linux: Cria/remove arquivo `.desktop` em `~/.config/autostart/vxvoice.desktop`.
  - Windows: Registro `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`.

---

## 🔨 6. Compilação e Execução

```bash
cd Softphone-VxVoice/build
cmake ..
make -j$(nproc)
./VxVoice
```

---

## 🎨 7. Atualizações Visuais e Ergonômicas Recentes (V3)

- **Visor Superior Integrado (`CallWidget`):** 
  - O painel com informações de Ramal (`m_lblAccountInfo`), Data/Hora e Status (`m_lblDateTimeStatus`) foi posicionado no topo, **acima** do campo de discagem de número.
  - Alinhamento uniforme à esquerda (`Qt::AlignLeft`), fonte aumentada e legível (13px bold) sincronizada em tempo real via relógio de 1s e eventos do `SipManager`.
  - Remoção de duplicidades no topo da janela principal, garantindo mais espaço vertical para o teclado e tabelas.
- **Painel In-Call e Teclado Numérico Dinâmicos:**
  - Layout in-call proporcional em grade com altura calibrada (`34px`), impedindo qualquer sobreposição com o teclado ou corte de botões no rodapé.
  - Teclado numérico (`dialBtn`) com `min-height: 40px` flexível com `QSizePolicy::Expanding`.
- **Janela de Chamada em Curso (`ActiveCallDialog`):**
  - Adaptação completa aos temas corporativos `VxVoice` (Roxo), `Dark` (Escuro) e `Light` (Claro).
  - Remoção de cores hardcoded cinzas/azuis no card de perfil (`activeCallCard`).
  - Avatar de usuário vetorial nítido gerado via `IconHelper::createUserAvatarPixmap`.
  - Botão de teclado DTMF redimensionado para "🔢 DTMF" com tooltip sem cortes de texto.
  - Botão "Encerrar Chamada" padronizado com o vermelho corporativo (`hangupButton`).
- **Pop-up de Chamada Recebida (`CallNotificationPopup`):**
  - Altura expandida para 215px com margens inferiores generosas, garantindo botões circulares perfeitos (48x48 px, raio 24px) verde (atender) e vermelho (recusar) sem nenhum corte na base.
- **Tooltips 100% Legíveis (`QToolTip`):**
  - Estilização explícita nos arquivos `.qss` para todos os temas, eliminando inconsistências do desktop Linux onde textos de dicas ficavam ilegíveis.
- **Identificação de Contatos na Chamada (`ContactsManager`):**
  - Módulo centralizado de contatos singleton com persistência (`QSettings`) e busca inteligente por ramal ou número.
  - O visor informa quem está ligando destacando o nome do contato se estiver salvo na agenda (ex: `📞 Ligando: Suporte TI Corporativo (1042)`).
- **Status de Registro Limpo no Visor:**
  - Remoção de ramal e servidor do título da conta, exibindo apenas o nome da conta e o status SIP imediato (`Online` em verde, `Falha de usuário e senha` em vermelho, `Não registrado` em cinza).
- **Correção de Contraste no Tema Light:**
  - Eliminação de cores fixas claras no C++, garantindo que data/hora e status tenham alto contraste grafite (`#2C3E50`) e cartões com fundo branco e bordas nítidas.

