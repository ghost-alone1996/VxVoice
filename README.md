# VxVoice — Softphone SIP Corporativo

Softphone VoIP corporativo desenvolvido em **C++17** com **Qt 6 (Qt Widgets)** e backend **PJSIP (PJSUA2)**, focado em leveza, estabilidade e produtividade para ambientes Windows e Linux.

**Versão atual:** `v2.0.9` · **Licença:** [GNU GPL v3](Softphone-VxVoice/LICENSE)

---

## Principais Funcionalidades

### Chamadas SIP e Presença
- **Motor PJSIP (PJSUA2):** Registro de contas SIP com suporte a múltiplos transportes, DTMF, espera, mudo, transferência e janela dedicada de chamada ativa (`ActiveCallDialog`).
- **Seletor de Status no Cabeçalho:** Alterne rapidamente entre **Disponível**, **Não Perturbe (DND)** e **Offline**, com visor digital exibindo a conta ativa e relógio em tempo real.
- **Pop-up de Chamada Recebida:** Notificação visual (`CallNotificationPopup`) com identificação limpa do chamador e bloqueio de discagem para o próprio ramal logado.
- **Toques Personalizáveis:** Quatro toques integrados com pré-visualização em **Configurações > Gerais** e reprodução contínua nativa no Windows (`PlaySoundW`).

### Perfis e Contas SIP
- **Perfis SIP Reutilizáveis:** Cadastre, edite, duplique e remova perfis com `Servidor SIP/Host`, `Porta SIP`, `Domínio PBX` e `Transporte`.
- **Integração Direta na Conta:** Ao vincular um Perfil SIP em `Contas SIP`, os campos já definidos pelo perfil são ocultados automaticamente, exigindo apenas usuário/ramal e senha.

### Contatos com Múltiplos Números e Histórico
- **Múltiplos Números por Contato:** Adicione quantos números/ramais forem necessários em `ContactEditDialog`, com busca/autocompletar e tela de seleção (`SelectContactNumberDialog`) ao discar.
- **Histórico de Chamadas:** Registro de chamadas efetuadas, recebidas e perdidas com menu de contexto para rediscagem e acesso direto à gravação associada (desabilitado visualmente quando não há áudio salvo).

### Gravações e Player Interno
- **Aba Condicional (`🎙 Gravação`):** Exibida automaticamente na janela principal quando a opção **Gravar todas as chamadas automaticamente** estiver habilitada em **Configurações > Gravação**.
- **Player Integrado:** Reprodutor embutido (`QMediaPlayer` + `QAudioOutput`) com barra de progresso, tempo (`MM:SS`), **Reproduzir/Pausar**, **Parar**, **Abrir Pasta** e **Excluir**.

### Temas Visuais e Bandeja do Sistema
- **7 Temas Integrados:**
  - **VxVoice (Purple)** — Roxo Corporativo (padrão)
  - **Dark** — Escuro clássico
  - **Light** — Claro
  - **Red** — Tons de vermelho
  - **Windows 2000**, **Windows XP (Luna)** e **Windows 7 (Aero)** — Temas clássicos do Windows
- **System Tray:** Minimização opcional ao clicar no botão fechar (`X`), submenu rápido de troca de tema e encerramento seguro do PJSIP ao clicar em **Sair**.

---

## Instalação

### Windows (`.msi`)
1. Baixe o instalador da versão mais recente na seção **Releases** (ou diretamente na raiz do repositório):
   - `vxvoice-2.0.9-Windows.msi` / `vxvoice-2.0.9-x64.msi`
2. Execute o instalador `.msi` e siga o assistente de instalação.

### Linux
Você pode instalar a partir do pacote/instalador gerado para Linux ou compilar diretamente pelo código-fonte:

```bash
# Exemplo com pacote .deb (Debian/Ubuntu)
sudo dpkg -i vxvoice_*.deb
sudo apt-get install -f
```

---

## Compilação a partir do Código-Fonte

### Requisitos
- **Compilador C++17** (GCC/Clang no Linux ou MinGW 13.10 no Windows)
- **CMake** >= 3.16
- **Qt 6** (`Core`, `Gui`, `Widgets`, `Network`; opcionalmente `Multimedia` e `Test`)
- **PJSIP (PJSUA2)** (ex.: PJSIP 2.14.1)

### Linux
```bash
cd Softphone-VxVoice
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./VxVoice
```

### Windows (Qt 6.7.3 + MinGW + PJSIP + WiX Toolset)
Utilize o script PowerShell incluso para compilar e empacotar o instalador `.msi`:

```powershell
cd Softphone-VxVoice
.\build_windows_msi.ps1
```

---

## Estrutura do Repositório

```text
VxVoice/
├── CHANGELOG.md                  # Histórico detalhado de versões
├── README.md                     # Documentação principal
├── vxvoice-*-Windows.msi         # Instaladores MSI gerados para Windows
└── Softphone-VxVoice/
    ├── CMakeLists.txt            # Configuração de build CMake
    ├── LICENSE                   # Licença GNU GPL v3
    ├── build_windows_msi.ps1     # Script de build e geração de MSI (Windows)
    ├── build_linux_zip.sh        # Script de empacotamento (Linux)
    ├── resources/                # Ícones, temas (.qss), sons (.wav) e .qrc
    ├── src/
    │   ├── core/                 # SipManager, SettingsManager, ContactsManager, ThemeManager, SoundManager, UpdateManager
    │   └── ui/                   # MainWindow, CallWidget, HistoryWidget, ContactsWidget, RecordingsWidget e diálogos
    └── tests/                    # Testes automatizados
```

---

## Histórico de Versões

Consulte o arquivo [CHANGELOG.md](CHANGELOG.md) para acompanhar todas as novidades, melhorias e correções de cada versão.
