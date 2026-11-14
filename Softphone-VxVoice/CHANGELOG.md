# Histórico de Versões — VxVoice Softphone

Registro completo de evolução, correções e melhorias do **VxVoice Enterprise SIP Softphone**.

---

## [v2.0.3] — 2026-11-13
### Correções de Bugs
- **Status de Registro (Configurações > Geral)**: Ao selecionar ou salvar o Status de Registro em Configurações, o status atual em execução não é mais alterado imediatamente; a configuração passa a ser aplicada somente quando o VxVoice inicia junto com o Sistema Operacional (`--autostart`).
- **Temas VxVoice**: Removidos os temas experimentais (`Blue Baby`, `Blue Capri`, `Rosa Amaranth`, `Rosa Hot Magenta`), mantendo os 4 temas oficiais (`Dark`, `Light`, `Purple`, `Red`).
- **Contatos — Bloqueio de Auto-chamada**: Impede iniciar chamada pela aba Contatos quando o ramal/número de destino for igual ao ramal atualmente logado na conta SIP, exibindo janela de aviso.
- **Histórico sem Duplicidade**: Corrigido o registro múltiplo para uma única chamada (deduplicação por `callId` no `MainWindow`).
- **Contas SIP**: Ajuste automático das colunas (`Status`, `Ramal`, `Transporte`, `Padrão` com `ResizeToContents` e `Conta SIP`/`Servidor` com `Stretch`) acompanhando a proporção da janela sem espaço vazio.

---

## [v2.0.2] — 2026-11-13
### Melhorias e Novas Funcionalidades
- **Proporção Única na Tela Principal**: A janela principal (`MainWindow`) passa a manter proporção única baseada na aba **Ligação** (`380x670 px`). Nas abas **Histórico**, **Contatos** e **Gravação**, todas as colunas se adaptam automaticamente ao conteúdo (`ResizeToContents`) com barras de rolagem horizontal e vertical (`ScrollBarAsNeeded`).
- **Temas VxVoice**: Seletor renomeado para **Temas VxVoice** em `Configurações > Geral` com 4 novos temas visuais:
  - **Blue Baby**
  - **Blue Capri**
  - **Rosa Amaranth**
  - **Rosa Hot Magenta**
- **Proporção da Janela Contas SIP**: Ajuste automático das colunas da tabela de Contas SIP de acordo com o conteúdo e redimensionamento proporcional da janela acompanhando o ajuste das colunas.
- **Testes Automatizados (`Qt Test` / `CTest`)**: Suíte de testes unitários integrada ao `CMakeLists.txt` validando mapeamento/persistência de temas, status de registro, busca de contatos e configurações do atualizador GitHub.
- **Histórico em Markdown**: Revisão e consolidação completa de todas as versões anteriores no `CHANGELOG.md`.

---

## [v2.0.1] — 2026-11-13
### Correções
- **Instância Única (System Tray)**: Implementado IPC via `QLocalServer`/`QLocalSocket`. Ao clicar no executável com o VxVoice já em execução (inclusive minimizado no System Tray), a janela existente é restaurada e trazida para frente em vez de abrir uma segunda instância.
- **Sub-opções de Inicialização com o SO**: `Status de Registro` movido para sub-opção de `Iniciar junto com o Sistema Operacional` (abaixo de `Iniciar minimizado no System Tray`). Quando o checkbox principal é desmarcado, ambas as sub-opções ficam bloqueadas e em cinza.

---

## [v2.0.0] — 2026-11-13
### Melhorias e Novas Funcionalidades
- **Inicialização Minimizada Exclusiva do SO**: `Iniciar minimizado no System Tray` passou a ser sub-opção de `Iniciar junto com o Sistema Operacional`, executando apenas quando o app inicia junto com o SO (`--autostart`).
- **Status de Registro Padrão**: Adicionada escolha entre `Disponível`, `Não Perturbe` e `Offline` em `Configurações > Gerais`.
- **Aba Sobre**: Título `VxVoice` com contraste adaptativo por tema (preto no Light/Blue Baby, branco nos temas escuros) e link clicável **Acessar Repositório Github** abaixo da licença.
- **Atualização de Software (GitHub)**: Busca automática ao abrir o aplicativo com janela de confirmação (`Atualizar Agora` / `Agora Não`), botão **Buscar Atualizações** e botão **Aplicar Atualização** (verde quando disponível, cinza quando indisponível).
- **Instalador Linux `.sh` em `.zip`**: Adicionado script `build_linux_zip.sh` gerando pacote com `install_vxvoice.sh` e `uninstall_vxvoice.sh` compactado em `.zip`.

---

## [v1.4.9] — 2026-11-13
- **Atualizador Direto via GitHub**: `UpdateManager` reescrito com `QNetworkAccessManager` para consultar diretamente GitHub Releases, Tags, `CMakeLists.txt` e commits em `https://github.com/ghost-alone1996/VxVoice`, com download direto de pacotes `.deb` e `.msi`.

---

## [v1.4.8] — 2026-11-13
- **Pacote Windows Installer (`.msi`)**: Criação oficial do instalador corporativo `.msi` (x64) compatível com GPO, SCCM, Intune e instalação silenciosa (`/qn`), com atalhos e associação de protocolos SIP/SIPS/TEL.

---

## [v1.4.7] — 2026-11-13
- Correção do modo de exibição no autostart (abrir janela normalmente quando `Iniciar minimizado` estiver desmarcado).
- Remoção de popups de aviso nas configurações e exibição de notificação silenciosa no System Tray informando ramal disponível.
- Adicionada janela de progresso em tempo real durante a instalação de atualizações (`UpdateProgressDialog`).

---

## [v1.4.6] — 2026-11-13
- Correção da persistência do tema visual selecionado após fechar e reabrir o aplicativo.
- Melhorias na tela de atualização com exibição da versão atual, versão disponível no Git e reinício automático com changelog (`ChangelogDialog`).

---

## [v1.4.5] — 2026-11-13
- Correção da proporção da tela de seleção de ramal/telefone (`SelectContactNumberDialog`).
- Reprodução de gravação funcional via menu de contexto (botão direito) no Histórico de Chamadas.
- Ícone oficial multi-resolução integrado ao executável e janelas.

---

## [v1.4.4] — 2026-11-13
- Correção no fluxo de atualização de pacotes.
- Diálogo de escolha quando um contato possui ramal e número cadastrados simultaneamente.

---

## [v1.4.3] — 2026-11-13
- Padronização da marca para **VxVoice** e licença Open Source (GPLv3).
- Aba dedicada de **Log SIP** nas Configurações com filtro e exportação.
- Busca em tempo real nos contatos diretamente pelo campo de discagem e melhoria no botão Backspace.

---

## [v1.4.2] — 2026-11-13
- Persistência completa de contas SIP, histórico de chamadas e contatos.
- Identificação automática do nome do contato (BINA) em chamadas efetuadas e recebidas.
- Revisão de codecs (Opus, G.722, PCMA, PCMU) e gravação de áudio bidirecional sem picotes.

---

## [v1.4.0] – [v1.4.1] — 2026-11-13
- Integração real com motor PJSIP (`PJSUA2`) em C++/Qt6, substituição do modo simulado por telefonia SIP real e empacotamento `.deb` inicial.

---

## [v1.2.0] – [v1.3.0] — 2026-11-12
- Estrutura inicial do Softphone VxVoice em C++17 / Qt 6, temas visuais, discador, histórico, contatos, gravações e System Tray.
