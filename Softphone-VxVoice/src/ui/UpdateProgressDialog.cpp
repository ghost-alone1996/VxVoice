#include "UpdateProgressDialog.h"
#include "ThemeManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QFrame>

UpdateProgressDialog::UpdateProgressDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Atualização de Software — VxVOICE");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setModal(true);
    setMinimumSize(480, 230);
    resize(480, 230);
    setupUi();
}

void UpdateProgressDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    auto card = new QFrame(this);
    card->setObjectName("callInfoFrame");
    auto cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 16, 18, 16);
    cardLayout->setSpacing(12);

    const bool isLight = ThemeManager::isLightTheme(ThemeManager::instance().currentTheme());
    m_lblTitle = new QLabel("🔄 Atualização de Software em Andamento...", card);
    m_lblTitle->setStyleSheet("font-size: 15px; font-weight: bold; background: transparent; color: #2980B9;");

    m_lblStatus = new QLabel("Iniciando processo de atualização do VxVOICE...", card);
    m_lblStatus->setWordWrap(true);
    m_lblStatus->setStyleSheet(
        QString("font-size: 13px; color: %1; background: transparent;")
            .arg(isLight ? "#1E293B" : "#E2E8F0")
    );

    m_progressBar = new QProgressBar(card);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(10);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedHeight(24);
    m_progressBar->setStyleSheet(
        QString("QProgressBar {"
                "  border: 1px solid %1;"
                "  border-radius: 6px;"
                "  text-align: center;"
                "  font-weight: bold;"
                "  font-size: 12px;"
                "  background-color: %2;"
                "  color: %3;"
                "}"
                "QProgressBar::chunk {"
                "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2980B9, stop:1 #27AE60);"
                "  border-radius: 5px;"
                "}")
            .arg(isLight ? "#94A3B8" : "rgba(255, 255, 255, 0.15)",
                 isLight ? "#E2E8F0" : "rgba(0, 0, 0, 0.25)",
                 isLight ? "#0F172A" : "#FFFFFF")
    );

    cardLayout->addWidget(m_lblTitle);
    cardLayout->addWidget(m_lblStatus);
    cardLayout->addWidget(m_progressBar);
    mainLayout->addWidget(card);

    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_btnClose = new QPushButton("Fechar", this);
    m_btnClose->setFixedHeight(36);
    m_btnClose->setMinimumWidth(120);
    m_btnClose->setCursor(Qt::PointingHandCursor);
    m_btnClose->setVisible(false);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(m_btnClose);
    mainLayout->addLayout(btnLayout);
}

void UpdateProgressDialog::setProgress(int percent, const QString &statusText) {
    if (m_progressBar) {
        m_progressBar->setValue(percent);
    }
    if (m_lblStatus) {
        m_lblStatus->setText(statusText);
    }
}

void UpdateProgressDialog::onUpdateCompleted(bool success, const QString &message) {
    m_isUpdating = false;
    if (success) {
        if (m_progressBar) m_progressBar->setValue(100);
        if (m_lblTitle) {
            m_lblTitle->setText("✓ Atualização Concluída!");
            m_lblTitle->setStyleSheet("font-size: 15px; font-weight: bold; background: transparent; color: #2ECC71;");
        }
        if (m_lblStatus) {
            m_lblStatus->setText(message);
        }
    } else {
        if (m_lblTitle) {
            m_lblTitle->setText("⚠ Falha na Atualização");
            m_lblTitle->setStyleSheet("font-size: 15px; font-weight: bold; background: transparent; color: #E74C3C;");
        }
        if (m_lblStatus) {
            m_lblStatus->setText(message);
        }
        if (m_progressBar) {
            m_progressBar->setStyleSheet(
                "QProgressBar {"
                "  border: 1px solid rgba(255, 255, 255, 0.15);"
                "  border-radius: 6px;"
                "  text-align: center;"
                "  font-weight: bold;"
                "  font-size: 12px;"
                "  background-color: rgba(0, 0, 0, 0.25);"
                "  color: #FFFFFF;"
                "}"
                "QProgressBar::chunk {"
                "  background-color: #E74C3C;"
                "  border-radius: 5px;"
                "}"
            );
        }
        if (m_btnClose) {
            m_btnClose->setVisible(true);
        }
    }
}

void UpdateProgressDialog::closeEvent(QCloseEvent *event) {
    if (m_isUpdating) {
        event->ignore(); // Evita interromper a instalação do pacote
    } else {
        event->accept();
    }
}
