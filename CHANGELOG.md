# Changelog — VxVoice Softphone

Todas as alterações notáveis deste projeto estão documentadas neste arquivo seguindo o padrão [Semantic Versioning](https://semver.org/).

---

## [v2.0.8] — 2026-11-14

### Correções de Bugs
- **Tela "Ligar para..." (`SelectContactNumberDialog`)**: Botões de seleção de número/ramal e subtítulo adaptados aos temas clássicos do Windows (`Windows 2000`, `Windows XP`, `Windows 7`), removendo cores fixas que destoavam do tema ativo.

---

## [v2.0.7] — 2026-11-14

### Correções de Bugs
- **Temas Clássicos e Contraste (`CallWidget`, `MainWindow`, `SettingsDialog`, `ChangelogDialog`, `UpdateProgressDialog`)**: Substituídas verificações isoladas de `Theme::Light` por `ThemeManager::isLightTheme(theme)`, corrigindo textos brancos sobre fundo claro nos temas Windows 2000, XP e 7 e aplicando ícones escuros no cabeçalho do tema Windows XP.
- **Novidades e Correções (`ChangelogDialog`)**: Conteúdo atualizado dinamicamente para exibir as alterações reais da versão atual (`v2.0.7`).
- **Aba Sobre (`SettingsDialog`)**: Removido o bloco antigo de resumo fixo (`v2.0.4`) e renomeado o botão para **Ver Novidades e Correções**.

### Melhorias
- **Proporção da Janela Principal**: Reduzida em 10% (`400×720` → `360×648`, mínimo `350×630`) com ajuste proporcional dos botões do discador.
- **Aba Gravação (`RecordingsWidget`)**: Removido o botão **Ligar** da barra inferior de ações.

---

## [v2.0.6] — 2026-11-14

### Novas Funcionalidades
- **Temas Clássicos Windows**: Adicionados os temas **Windows 2000**, **Windows XP** e **Windows 7** (`win2000.qss`, `winxp.qss`, `win7.qss`), integrados ao `ThemeManager`, System Tray e Configurações.
- **Contatos com Múltiplos Números**: Botão **`+ Adicionar Número`** em `ContactEditDialog`, persistência em `additionalNumbers`, busca/autocompletar e tela `SelectContactNumberDialog` com rolagem dinâmica.
- **Aba Perfil SIP (`SettingsDialog`)**: Cadastro, edição, cópia e exclusão de perfis SIP (`Servidor SIP/Host`, `Porta SIP`, `Domínio PBX`, `Transporte`).
- **Conta SIP Integrada ao Perfil SIP (`AccountsDialog`)**: Ao selecionar um Perfil SIP cadastrado, os campos já preenchidos no perfil são ocultados automaticamente, mantendo apenas usuário/ramal e senha.

---

## [v2.0.5] — 2026-11-14

### Correções de Bugs e Melhorias
- **Toques de Chamada no Windows (`SoundManager`)**: Implementação nativa com `PlaySoundW` (`winmm`) para reprodução contínua do toque em chamadas recebidas.
- **Encerramento Seguro do PJSIP (`SipManager`)**: Guarda contra duplo `shutdown()` e verificação de inicialização antes de `pj_thread_is_registered()`, eliminando a asserção em `os_core_win32.c:657` ao clicar em **Sair**.
- **Identificação de Chamada Recebida**: Exibição apenas do número limpo quando o chamador não está salvo nos contatos.
- **System Tray e Janela**: Adicionado submenu **Tema** na bandeja, renomeado **Sair Definitivo** para **Sair** e ajustada a janela principal.

---

## [v2.0.4] — 2026-11-13

### Novas Funcionalidades e Correções
- Suporte inicial completo para compilação no Windows (Qt 6.7.3 + MinGW + PJSIP 2.14.1) e geração de instalador `.msi` via WiX Toolset 3.14.
- Bloqueio de ligação para o próprio ramal logado, ajuste da janela de seleção de número e remoção de duplicidade no Histórico.

---

## Instalação Rápida

### Windows (`.msi`)
1. Baixe `vxvoice-2.0.8-Windows.msi` (ou `vxvoice-2.0.8-x64.msi`) na seção **Releases** do GitHub.
2. Execute o instalador `.msi` e siga o assistente de instalação.

### Linux (`.deb` / `.sh`)
```bash
# Via pacote .deb (Debian/Ubuntu)
sudo dpkg -i vxvoice_*.deb
sudo apt-get install -f

# Ou via script instalador
chmod +x install.sh
sudo ./install.sh
```
