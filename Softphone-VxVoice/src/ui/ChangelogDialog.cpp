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
        "<li><b>🐧 Empacotamento e Portabilidade Linux (v3.0.2):</b> Build nativo Linux atualizado para a última versão com bibliotecas PJSIP embarcadas via <code>RPATH</code> (<code>/usr/lib/vxvoice</code>), pacote <code>.deb</code> e instalador universal <code>.sh</code> compatível com Debian/Ubuntu, Fedora/RHEL, Arch e openSUSE.</li><br>"
        "<li><b>📐 Janela Principal Mais Compacta (v3.0.1):</b> Proporção padrão e mínima da janela principal reduzidas em definitivo em 15% (<code>306×551 px</code>) com ajuste proporcional do discador.</li><br>"
        "<li><b>ℹ Aba Sobre Atualizada (v3.0.1):</b> Substituído o campo <i>Empresa</i> por <i>Autor: Leonardo Machado da Silva</i>.</li><br>"
        "<li><b>🎧 Tela Dedicada do Reprodutor Interno (v3.0.0):</b> Ao reproduzir uma gravação (pelo Histórico ou pela aba Gravação), uma janela dedicada do reprodutor é aberta com barra de progresso, tempo, botões <i>Reproduzir/Pausar</i>, <i>Parar</i> e <i>Abrir Pasta da Gravação</i>.</li><br>"
        "<li><b>🕒 Menu de Contexto do Histórico (v3.0.0):</b> Removido o item <i>Abrir Pasta da Gravação</i> do menu pop-up de clique direito no Histórico, centralizando essa ação na tela do reprodutor.</li><br>"
        "<li><b>🎙 Exibição Condicional da Aba Gravação (v2.0.9):</b> A aba <i>Gravação</i> passa a ser exibida somente quando a opção <i>Gravar todas as chamadas automaticamente</i> está habilitada em Configurações &gt; Gravação.</li>"
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
