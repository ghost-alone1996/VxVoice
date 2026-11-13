#include "ChangelogDialog.h"
#include "ThemeManager.h"
#include "SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QFrame>

ChangelogDialog::ChangelogDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("VxVoice — Novidades e Correções");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setModal(true);
    setMinimumSize(540, 440);
    resize(560, 460);
    setupUi();
}

void ChangelogDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    // Card de Cabeçalho
    auto headerCard = new QFrame(this);
    headerCard->setObjectName("callInfoFrame");
    auto headerLayout = new QVBoxLayout(headerCard);
    headerLayout->setContentsMargins(16, 12, 16, 12);
    headerLayout->setSpacing(4);

    auto lblTitle = new QLabel(QString("✨ VxVoice v%1").arg(SettingsManager::APP_VERSION), headerCard);
    lblTitle->setStyleSheet("font-size: 18px; font-weight: bold; background: transparent;");

    auto lblSub = new QLabel("Aplicativo atualizado com sucesso! Confira as novidades e melhorias:", headerCard);
    lblSub->setStyleSheet("font-size: 13px; color: #9CA3AF; background: transparent;");

    headerLayout->addWidget(lblTitle);
    headerLayout->addWidget(lblSub);
    mainLayout->addWidget(headerCard);

    // Navegador de Conteúdo com as Novidades e Correções
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setOpenExternalLinks(true);
    m_textBrowser->setStyleSheet(
        "QTextBrowser {"
        "  border: 1px solid rgba(255, 255, 255, 0.1);"
        "  border-radius: 6px;"
        "  padding: 12px;"
        "  font-size: 13px;"
        "  line-height: 1.5;"
        "}"
    );

    QString html = QString(
        "<h3 style='margin-top: 0;'>Principais Melhorias e Correções da Versão %1:</h3>"
        "<ul style='margin-left: -15px;'>"
        "<li><b>🔄 Busca Automática de Atualizações no GitHub:</b> Ao iniciar o aplicativo, o VxVoice verifica automaticamente novas versões no GitHub "
        "(<code>github.com/ghost-alone1996/VxVoice</code>) e exibe uma janela perguntando se deseja atualizar agora.</li><br>"
        "<li><b>🚀 Sub-opções de Inicialização com o Sistema:</b> A opção <i>Iniciar minimizado no System Tray</i> agora é uma sub-opção vinculada e aplicada exclusivamente quando o aplicativo inicia com o Sistema Operacional.</li><br>"
        "<li><b>🟢 Status de Registro Padrão:</b> Escolha entre <i>Disponível</i>, <i>Não Perturbe</i> ou <i>Offline</i> em Configurações &gt; Gerais.</li><br>"
        "<li><b>🎨 Aba Sobre &amp; Repositório GitHub:</b> Novo link direto para o repositório no GitHub, contraste adaptativo por tema (Light/Dark/Purple/Red) e botão <i>Aplicar Atualização</i> dinâmico.</li><br>"
        "<li><b>📦 Instaladores Multiplataforma:</b> Pacote Linux <code>.deb</code>, instalador <code>.sh</code> compactado em <code>.zip</code> e pacote corporativo Windows <code>.msi</code>.</li>"
        "</ul>"
    ).arg(SettingsManager::APP_VERSION);

    m_textBrowser->setHtml(html);
    mainLayout->addWidget(m_textBrowser, 1);

    // Botão de Confirmação
    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_btnOk = new QPushButton("✓ Entendido / Começar a Usar", this);
    m_btnOk->setFixedHeight(38);
    m_btnOk->setMinimumWidth(220);
    m_btnOk->setCursor(Qt::PointingHandCursor);
    m_btnOk->setStyleSheet(
        "QPushButton {"
        "  background-color: #27AE60;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  border-radius: 6px;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover { background-color: #2ECC71; }"
        "QPushButton:pressed { background-color: #1E8449; }"
    );
    connect(m_btnOk, &QPushButton::clicked, this, &ChangelogDialog::onAcknowledgeClicked);

    btnLayout->addWidget(m_btnOk);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

void ChangelogDialog::onAcknowledgeClicked() {
    accept();
}
