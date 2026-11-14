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
        "<li><b>📐 Proporção Única na Tela Principal:</b> A janela principal mantém agora uma proporção única baseada na aba <i>Ligação</i>. Nas abas <i>Histórico</i>, <i>Contatos</i> e <i>Gravação</i>, as colunas se adaptam automaticamente ao conteúdo com barras de rolagem horizontal e vertical.</li><br>"
        "<li><b>🎨 Novos Temas VxVoice:</b> Seletor renomeado para <i>Temas VxVoice</i> com 4 novos estilos visuais: <b>Blue Baby</b>, <b>Blue Capri</b>, <b>Rosa Amaranth</b> e <b>Rosa Hot Magenta</b>.</li><br>"
        "<li><b>🪟 Janela Contas SIP Proporcional:</b> As colunas da tabela de Contas SIP se ajustam automaticamente ao conteúdo e a proporção da janela acompanha o ajuste.</li><br>"
        "<li><b>✅ Testes Automatizados &amp; Histórico:</b> Suíte de testes automatizados com Qt Test / CTest e histórico completo de versões documentado em <code>CHANGELOG.md</code>.</li>"
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
