# VxVoice - Softphone SIP Corporativo (Qt 6 + C++)

Softphone VoIP corporativo de alto desempenho desenvolvido em **C++17** com **Qt 6 (Qt Widgets)** e arquitetura preparada para o backend **PJSIP (PJSUA2)**, inspirado no design limpo e eficiente do MicroSIP.

---

## 🚀 Novas Funcionalidades e Personalizações

1. **🟣 Tema Oficial VxVoice (Roxo Corporativo):**
   - Novo tema personalizado com a identidade visual da VxVoice: fundo roxo escuro `#171226`, painéis `#1E1632`, acentos `#8E44AD` / `#9B59B6` e tipografia clara `#F5EEF8`.
   - Suporte a 3 temas: **VxVoice (Roxo Corporativo - Padrão)**, **Dark (Escuro Clássico)** e **Light (Claro)**.

2. **📐 Proporção Padrão das Telas:**
   - **Janela Principal (MainWindow):** Proporção padrão fixada em `380x640 px`.
   - **Janela de Chamada Ativa (ActiveCallDialog):** Proporção padrão fixada em `350x540 px`, abrindo posicionada lado a lado com a janela principal na tela.

3. **🟢 Seletor de Status SIP no Cabeçalho:**
   - O combobox de presença (**🟢 Disponível**, **⛔ Não Perturbe**, **⚪ Offline**) agora fica posicionado **diretamente ao lado do ícone VX** no topo.
   - O visor digital abaixo exibe a conta SIP ativa e o relógio em tempo real.

4. **🚪 Botão Vermelho "Sair":**
   - Botão vermelho destacado posicionado ao lado do botão de teste de chamada (`🔔`).
   - Encerra imediatamente a aplicação e todos os processos e threads em segundo plano (`qApp->quit()`).

5. **🛑 Controle do Botão "X" (Minimizar para o Tray vs Fechar):**
   - Por padrão, o botão "X" da janela minimiza o app para o **System Tray**.
   - Em **Configurações > Gerais**, foi adicionada a opção para habilitar ou desabilitar essa função:
     - *Marcado:* O "X" minimiza para a bandeja.
     - *Desmarcado:* O "X" fecha completamente o app.

6. **🎵 Toques de Chamada Recebida:**
   - Os 4 áudios enviados foram integrados ao aplicativo (`ringtone_1` a `ringtone_4`).
   - Em **Configurações > Gerais**, é possível selecionar o toque desejado e testá-lo com o botão **▶ Ouvir Toque / ⏹ Parar**.
   - Ao receber uma chamada, o toque selecionado é executado em loop até que a ligação seja atendida ou recusada.

---

## 🔨 Compilação e Execução

```bash
cd Softphone-VxVoice/build
cmake ..
make -j$(nproc)
./VxVoice
```
