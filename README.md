# VxVOICE — Softphone SIP Corporativo

Softphone VoIP corporativo desenvolvido em **C++17** com **Qt 6 (Qt Widgets)** e backend **PJSIP (PJSUA2)**, focado em leveza, estabilidade, portabilidade e produtividade para ambientes **Linux** e **Windows**.

**Versão atual:** Linux `v3.0.4` · Windows `v3.0.1` · **Autor:** Leonardo Machado da Silva · **Licença:** [GNU GPL v3](Softphone-VxVoice/LICENSE)

---

## Principais Funcionalidades

### Chamadas SIP e Presença
- **Motor PJSIP (PJSUA2):** Registro de contas SIP com suporte a múltiplos transportes (UDP/TCP/TLS), DTMF, espera, mudo, transferência e janela dedicada de chamada ativa (`ActiveCallDialog`).
- **Janela Principal e Cabeçalho (`360×560 px`):** Proporção compacta e balanceada com seletor de status (**Disponível**, **Não Perturbe** e **Offline**) e botões proporcionais de **Contas SIP**, **Configurações** e **Sair**.
- **Pop-up de Chamada Recebida:** Notificação visual (`CallNotificationPopup`) com identificação do chamador e bloqueio de discagem para o próprio ramal logado (`SipManager::isSelfExtension()`).
- **Toques Personalizáveis:** Quatro toques integrados com pré-visualização em **Configurações > Geral**.

### Perfis e Contas SIP
- **Perfis SIP Reutilizáveis:** Cadastre, edite, duplique e remova perfis com `Servidor SIP/Host`, `Porta SIP`, `Domínio PBX` e `Transporte`.
- **Integração Direta na Conta:** Ao vincular um Perfil SIP em `Contas SIP`, os campos já definidos pelo perfil são ocultados automaticamente, exigindo apenas usuário/ramal e senha.

### Contatos com Múltiplos Números e Histórico
- **Aba Contatos Organizada:** Campo de busca em largura total na parte superior, com o filtro por empresa (`Todas as Empresas`) e o botão **+ Adicionar** logo abaixo.
- **Múltiplos Números por Contato:** Adicione quantos números/ramais forem necessários em `ContactEditDialog`, com busca/autocompletar e tela de seleção (`SelectContactNumberDialog`) ao discar.
- **Histórico de Chamadas:** Registro de chamadas efetuadas, recebidas e perdidas com deduplicação por `callId`, menu de contexto para rediscagem e acesso direto à gravação associada.

### Gravações e Player Interno
- **Aba Condicional (`🎙 Gravação`):** Exibida automaticamente na janela principal quando a opção **Gravar todas as chamadas automaticamente** estiver habilitada em **Configurações > Gravação**.
- **Janela Dedicada do Reprodutor (`RecordingPlayerDialog`):** Ao reproduzir qualquer gravação (pelo Histórico ou pela aba Gravação), abre uma janela própria com barra de progresso, tempo (`MM:SS`), **Reproduzir/Pausar**, **Parar** e **Abrir Pasta da Gravação**.

### Temas Visuais e Bandeja do Sistema
- **9 Temas Integrados:**
  - **VxVOICE (Purple)** — Roxo Corporativo (padrão)
  - **Dark** — Escuro clássico
  - **Light** — Claro
  - **Cinnamon Light (Mint-Y)** — Claro inspirado na interface Cinnamon / Linux Mint
  - **Cinnamon Dark (Mint-Y)** — Escuro inspirado na interface Cinnamon / Linux Mint
  - **Red** — Tons de vermelho
  - **Windows 2000**, **Windows XP (Luna)** e **Windows 7 (Aero)** — Temas clássicos do Windows
- **System Tray & Instância Única:** Minimização opcional ao clicar no botão fechar (`X`), submenu rápido de troca de tema, restauração automática de janela via IPC (`QLocalServer`) e encerramento seguro do PJSIP ao clicar em **Sair**.

### Atualização Automática (`UpdateManager`)
- **Integração Direta com o GitHub:** Consulta `Releases`, `Tags` e `CMakeLists.txt` no repositório oficial.
- **Detecção Inteligente no Linux:** Identifica automaticamente se o **VxVOICE** foi instalado via pacote Debian (`.deb`) ou pelo instalador universal (`.sh`), ignorando releases exclusivas de outras plataformas (`.msi`) e baixando/aplicando o artefato correspondente à instalação Linux atual.

---

## Instalação

### Linux — Pacote `.deb` (Debian / Ubuntu / Linux Mint)
Os pacotes `.deb` já incluem as bibliotecas compartilhadas do **PJSIP** embarcadas em `/usr/lib/vxvoice` (`RPATH`):

```bash
sudo dpkg -i vxvoice-3.0.4-Linux.deb
sudo apt-get install -f
```

### Linux — Instalador Universal `.sh` (Qualquer Distribuição)
O instalador *self-extracting* detecta automaticamente `apt`, `dnf`, `pacman` ou `zypper` para resolver dependências de runtime (Qt6/ALSA), instala o binário, bibliotecas PJSIP, ícones e atalho de menu:

```bash
chmod +x vxvoice-3.0.4-Linux-Installer.sh
./vxvoice-3.0.4-Linux-Installer.sh
```

*(Também disponível em formato compactado `vxvoice-3.0.4-Linux-Installer.zip` contendo `install_vxvoice.sh` e `uninstall_vxvoice.sh`.)*

### Linux — Desinstalação Completa
Para remover completamente o **VxVOICE** (processos, pacote `.deb`, binários, bibliotecas, ícones, atalhos, autostart e configurações):

```bash
./uninstall_vxvoice.sh

# Ou preservando a pasta de gravações:
./uninstall_vxvoice.sh --keep-recordings
```

### Windows (`.msi`)
1. Baixe o instalador da versão mais recente na seção **Releases** (ou na raiz do repositório):
   - `vxvoice-3.0.1-Windows.msi` / `vxvoice-3.0.1-x64.msi`
2. Execute o instalador `.msi` e siga o assistente de instalação.

---

## Compilação e Empacotamento a partir do Código-Fonte

### Requisitos
- **Compilador C++17** (GCC/Clang no Linux ou MinGW 13.10 no Windows)
- **CMake** >= 3.16
- **Qt 6** (`Core`, `Gui`, `Widgets`, `Network`; opcionalmente `Multimedia` e `Test`)
- **PJSIP (PJSUA2)** (ex.: PJSIP 2.17-dev / 2.14.1) e headers ALSA/PulseAudio

### Linux (Build, Testes, `.deb` e Instalador Universal `.sh`/`.zip`)
```bash
cd Softphone-VxVoice
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest --output-on-failure

# Gerar pacote .deb
cpack -G DEB

# Gerar instalador universal .sh e pacote .zip
cd ../..
bash Softphone-VxVoice/build_linux_zip.sh
```

### Windows (Qt 6.7.3 + MinGW + PJSIP + WiX Toolset)
```powershell
cd Softphone-VxVoice
.\build_windows_msi.ps1
```

---

## Estrutura do Repositório

```text
VxVoice/
├── CHANGELOG.md                          # Histórico detalhado de versões
├── README.md                             # Documentação principal
├── uninstall_vxvoice.sh                  # Script de desinstalação completa (Linux)
├── vxvoice-*-Linux.deb                   # Pacote Debian/Ubuntu/Mint (Linux)
├── vxvoice-*-Linux-Installer.sh          # Instalador universal self-extracting (Linux)
├── vxvoice-*-Linux-Installer.zip         # Instalador universal compactado (Linux)
├── vxvoice-*-Windows.msi                 # Instaladores MSI (Windows)
└── Softphone-VxVoice/
    ├── CHANGELOG.md                      # Histórico interno do módulo
    ├── CMakeLists.txt                    # Configuração de build CMake e CPack
    ├── LICENSE                           # Licença GNU GPL v3
    ├── build_linux_zip.sh                # Script de empacotamento Linux (.sh + .zip)
    ├── build_windows_msi.ps1             # Script de build e geração de MSI (Windows)
    ├── resources/                        # Ícones, temas (.qss), sons (.wav), .desktop e .qrc
    ├── src/
    │   ├── core/                         # SipManager, SettingsManager, ContactsManager, ThemeManager, SoundManager, UpdateManager
    │   └── ui/                           # MainWindow, CallWidget, HistoryWidget, ContactsWidget, RecordingsWidget e diálogos
    └── tests/                            # Testes automatizados (VxVoiceUnitTests / CTest)
```

---

## Histórico de Versões

Consulte o arquivo [CHANGELOG.md](CHANGELOG.md) para acompanhar todas as novidades, melhorias e correções de cada versão.
