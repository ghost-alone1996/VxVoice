#include "CallNotificationPopup.h"
#include "IconHelper.h"
#include "ThemeManager.h"
#include <QGuiApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>

CallNotificationPopup::CallNotificationPopup(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::SubWindow) {
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(340, 205);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    m_cardFrame = new QFrame(this);
    m_cardFrame->setObjectName("notificationCard");
    auto cardLayout = new QVBoxLayout(m_cardFrame);
    cardLayout->setContentsMargins(14, 12, 14, 16);
    cardLayout->setSpacing(6);

    // Header layout
    auto headerLayout = new QHBoxLayout();
    m_headerLabel = new QLabel("🔔 Chamada SIP Recebida", m_cardFrame);
    m_headerLabel->setObjectName("notificationHeader");

    m_btnClose = new QPushButton("✕", m_cardFrame);
    m_btnClose->setFixedSize(22, 22);
    m_btnClose->setCursor(Qt::PointingHandCursor);
    connect(m_btnClose, &QPushButton::clicked, this, &CallNotificationPopup::dismiss);

    headerLayout->addWidget(m_headerLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_btnClose);
    cardLayout->addLayout(headerLayout);

    // Caller Details
    m_callerNameLabel = new QLabel(m_cardFrame);
    m_callerNameLabel->setObjectName("notificationCallerName");

    m_callerNumberLabel = new QLabel(m_cardFrame);
    m_callerNumberLabel->setObjectName("notificationCallerNumber");

    m_statusMessageLabel = new QLabel(m_cardFrame);
    m_statusMessageLabel->setObjectName("notificationStatus");
    m_statusMessageLabel->setVisible(false);

    cardLayout->addWidget(m_callerNameLabel);
    cardLayout->addWidget(m_callerNumberLabel);
    cardLayout->addWidget(m_statusMessageLabel);

    // Action Buttons Container (Atender Verde e Rejeitar Vermelho com X)
    m_actionsContainer = new QWidget(m_cardFrame);
    auto actionsLayout = new QHBoxLayout(m_actionsContainer);
    actionsLayout->setContentsMargins(0, 6, 0, 4);
    actionsLayout->setSpacing(40);
    actionsLayout->setAlignment(Qt::AlignCenter);

    // Botão Verde Redondo com Fone de Telefone
    m_btnAnswer = new QPushButton(m_actionsContainer);
    m_btnAnswer->setIcon(IconHelper::createPhoneIcon(Qt::white, 24));
    m_btnAnswer->setIconSize(QSize(24, 24));
    m_btnAnswer->setFixedSize(48, 48);
    m_btnAnswer->setCursor(Qt::PointingHandCursor);
    m_btnAnswer->setToolTip("Atender Chamada");
    m_btnAnswer->setStyleSheet(
        "QPushButton { background-color: #27AE60; border: none; border-radius: 24px; } "
        "QPushButton:hover { background-color: #2ECC71; } "
        "QPushButton:pressed { background-color: #1E8449; }"
    );

    // Botão Vermelho Redondo com "X"
    m_btnReject = new QPushButton(m_actionsContainer);
    m_btnReject->setIcon(IconHelper::createCloseIcon(Qt::white, 22));
    m_btnReject->setIconSize(QSize(22, 22));
    m_btnReject->setFixedSize(48, 48);
    m_btnReject->setCursor(Qt::PointingHandCursor);
    m_btnReject->setToolTip("Encerrar / Recusar Chamada");
    m_btnReject->setStyleSheet(
        "QPushButton { background-color: #C0392B; border: none; border-radius: 24px; } "
        "QPushButton:hover { background-color: #E74C3C; } "
        "QPushButton:pressed { background-color: #922B21; }"
    );

    connect(m_btnAnswer, &QPushButton::clicked, this, [this]() {
        emit answerClicked(m_callId);
        dismiss();
    });

    connect(m_btnReject, &QPushButton::clicked, this, [this]() {
        emit rejectClicked(m_callId);
        dismiss();
    });

    actionsLayout->addWidget(m_btnAnswer);
    actionsLayout->addWidget(m_btnReject);
    cardLayout->addWidget(m_actionsContainer);

    mainLayout->addWidget(m_cardFrame);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &CallNotificationPopup::updateThemeVisuals);

    updateThemeVisuals();
}

void CallNotificationPopup::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    updateThemeVisuals();
}

void CallNotificationPopup::updateThemeVisuals() {
    auto theme = ThemeManager::instance().currentTheme();

    if (theme == ThemeManager::Theme::Red) {
        m_cardFrame->setStyleSheet(
            "QFrame#notificationCard { background-color: #241518; border: 2px solid #E74C3C; border-radius: 10px; }"
        );
        m_headerLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #E74C3C; background: transparent;");
        m_callerNameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFFFFF; background: transparent;");
        m_callerNumberLabel->setStyleSheet("font-size: 13px; color: #FDEDEC; font-weight: 500; background: transparent;");
        m_btnClose->setStyleSheet(
            "QPushButton { border: none; background: transparent; color: #FDEDEC; font-weight: bold; font-size: 14px; } "
            "QPushButton:hover { color: #E74C3C; }"
        );
    } else if (theme == ThemeManager::Theme::Dark) {
        m_cardFrame->setStyleSheet(
            "QFrame#notificationCard { background-color: #101924; border: 2px solid #0284C7; border-radius: 10px; }"
        );
        m_headerLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #38BDF8; background: transparent;");
        m_callerNameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ECF0F1; background: transparent;");
        m_callerNumberLabel->setStyleSheet("font-size: 13px; color: #BDC3C7; font-weight: 500; background: transparent;");
        m_btnClose->setStyleSheet(
            "QPushButton { border: none; background: transparent; color: #BDC3C7; font-weight: bold; font-size: 14px; } "
            "QPushButton:hover { color: #38BDF8; }"
        );
    } else if (theme == ThemeManager::Theme::Light) {
        m_cardFrame->setStyleSheet(
            "QFrame#notificationCard { background-color: #FFFFFF; border: 2px solid #0284C7; border-radius: 10px; }"
        );
        m_headerLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #0284C7; background: transparent;");
        m_callerNameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1E293B; background: transparent;");
        m_callerNumberLabel->setStyleSheet("font-size: 13px; color: #475569; font-weight: 500; background: transparent;");
        m_btnClose->setStyleSheet(
            "QPushButton { border: none; background: transparent; color: #94A3B8; font-weight: bold; font-size: 14px; } "
            "QPushButton:hover { color: #0284C7; }"
        );
    } else { // Purple (VxVoice)
        m_cardFrame->setStyleSheet(
            "QFrame#notificationCard { background-color: #1B132C; border: 2px solid #9B59B6; border-radius: 10px; }"
        );
        m_headerLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #9B59B6; background: transparent;");
        m_callerNameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #F5EEF8; background: transparent;");
        m_callerNumberLabel->setStyleSheet("font-size: 13px; color: #D2B4DE; font-weight: 500; background: transparent;");
        m_btnClose->setStyleSheet(
            "QPushButton { border: none; background: transparent; color: #BDC3C7; font-weight: bold; font-size: 14px; } "
            "QPushButton:hover { color: #9B59B6; }"
        );
    }
}

void CallNotificationPopup::showIncomingCall(const QString &callId, const QString &number, const QString &name) {
    m_callId = callId;
    m_callerNameLabel->setText(name.isEmpty() ? "Chamada SIP Recebida" : name);
    m_callerNumberLabel->setText(QString("Ramal / Número: %1").arg(number));
    m_statusMessageLabel->setVisible(false);

    repositionBottomRight();
    show();
    raise();
}

void CallNotificationPopup::showCallStatus(const QString &title, const QString &message) {
    m_callerNameLabel->setText(title);
    m_callerNumberLabel->setText(message);
    m_statusMessageLabel->setVisible(false);

    repositionBottomRight();
    show();
    raise();
}

void CallNotificationPopup::dismiss() {
    hide();
}

void CallNotificationPopup::repositionBottomRight() {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QRect geom = screen->availableGeometry();
    int x = geom.right() - width() - 16;
    int y = geom.bottom() - height() - 16;
    move(x, y);
}
