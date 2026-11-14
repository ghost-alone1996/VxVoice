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

    const bool isLight = ThemeManager::isLightTheme(ThemeManager::instance().currentTheme());
    const QString titleColor = isLight ? "#0F172A" : "#FFFFFF";
    const QString subColor = isLight ? "#475569" : "#9CA3AF";
    const QString browserBorder = isLight ? "rgba(0, 0, 0, 0.18)" : "rgba(255, 255, 255, 0.12)";
    const QString browserBg = isLight ? "#FFFFFF" : "rgba(15, 23, 42, 0.35)";
    const QString browserText = isLight ? "#0F172A" : "#E2E8F0";

    auto lblTitle = new QLabel(QString("✨ VxVoice v%1").arg(SettingsManager::APP_VERSION), headerCard);
    lblTitle->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1; background: transparent;").arg(titleColor));

    auto lblSub = new QLabel("Aplicativo atualizado com sucesso! Confira as novidades e correções:", headerCard);
    lblSub->setStyleSheet(QString("font-size: 13px; color: %1; background: transparent;").arg(subColor));

    headerLayout->addWidget(lblTitle);
    headerLayout->addWidget(lblSub);
    mainLayout->addWidget(headerCard);

    // Navegador de Conteúdo com as Novidades e Correções
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setOpenExternalLinks(true);
    m_textBrowser->setStyleSheet(
        QString("QTextBrowser {"
                "  background-color: %1;"
                "  color: %2;"
                "  border: 1px solid %3;"
                "  border-radius: 6px;"
                "  padding: 12px;"
                "  font-size: 13px;"
                "  line-height: 1.5;"
                "}")
            .arg(browserBg, browserText, browserBorder)
    );

    QString html = QString(
        "<h3 style='margin-top: 0;'>Novidades e Correções — Versão %1:</h3>"
        "<ul style='margin-left: -15px;'>"
        "<li><b>📞 Tela \"Ligar para...\" Adaptada aos Temas Windows (v2.0.8):</b> Os botões de seleção de número/ramal e o subtítulo da tela de escolha de destino agora seguem integralmente os temas Windows 2000, Windows XP e Windows 7.</li><br>"
        "<li><b>🎨 Temas Clássicos Windows (2000, XP e 7) Aprimorados (v2.0.7):</b> Correção de contraste e legibilidade em todas as telas, visores, ícones e diálogos.</li><br>"
        "<li><b>📐 Janela Principal Mais Compacta e Aba Gravação Simplificada (v2.0.7):</b> Proporção reduzida em 10% (360×648), remoção do botão Ligar em Gravação e limpeza da aba Sobre.</li><br>"
        "<li><b>🌐 Perfis SIP e Contatos com Múltiplos Números (v2.0.6):</b> Aba dedicada de Perfis SIP nas Configurações com integração automática em Contas SIP, além de múltiplos números por contato.</li>"
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
