#include "ActiveCallDialog.h"
#include "IconHelper.h"
#include "ThemeManager.h"
#include <QInputDialog>
#include <QApplication>

ActiveCallDialog::ActiveCallDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("VxVoice - Chamada em Curso");
    setMinimumSize(360, 320);
    resize(360, 340);
    setupUi();
}

void ActiveCallDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(10);

    // 1. Caller Profile Card com Destaque Máximo às Informações (Sem foto/avatar)
    m_profileCard = new QFrame(this);
    m_profileCard->setObjectName("activeCallCard");
    m_profileCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto profileLayout = new QVBoxLayout(m_profileCard);
    profileLayout->setContentsMargins(14, 14, 14, 14);
    profileLayout->setAlignment(Qt::AlignCenter);
    profileLayout->setSpacing(4);

    m_lblHeaderBadge = new QLabel("📞 Chamada em Curso", m_profileCard);
    m_lblHeaderBadge->setObjectName("activeCallBadge");
    m_lblHeaderBadge->setAlignment(Qt::AlignCenter);

    m_lblName = new QLabel("Identificando...", m_profileCard);
    m_lblName->setObjectName("activeCallName");
    m_lblName->setAlignment(Qt::AlignCenter);

    m_lblNumber = new QLabel("", m_profileCard);
    m_lblNumber->setObjectName("activeCallNumber");
    m_lblNumber->setAlignment(Qt::AlignCenter);

    m_lblCompany = new QLabel("", m_profileCard);
    m_lblCompany->setObjectName("activeCallCompany");
    m_lblCompany->setAlignment(Qt::AlignCenter);
    m_lblCompany->setVisible(false);

    // Linha de status e gravação
    auto statusRow = new QHBoxLayout();
    statusRow->setAlignment(Qt::AlignCenter);
    statusRow->setSpacing(8);

    m_lblStatus = new QLabel("Conectando...", m_profileCard);
    m_lblStatus->setObjectName("activeCallStatus");

    m_lblRecBadge = new QLabel("● REC", m_profileCard);
    m_lblRecBadge->setObjectName("activeCallRecBadge");
    m_lblRecBadge->setVisible(false);

    statusRow->addWidget(m_lblStatus);
    statusRow->addWidget(m_lblRecBadge);

    m_lblTimer = new QLabel("00:00", m_profileCard);
    m_lblTimer->setObjectName("activeCallTimer");
    m_lblTimer->setAlignment(Qt::AlignCenter);

    profileLayout->addWidget(m_lblHeaderBadge);
    profileLayout->addWidget(m_lblName);
    profileLayout->addWidget(m_lblNumber);
    profileLayout->addWidget(m_lblCompany);
    profileLayout->addLayout(statusRow);
    profileLayout->addWidget(m_lblTimer);

    mainLayout->addWidget(m_profileCard);

    // 2. Action Controls Grid (Mute, Hold, Transfer, Record, Conf, DTMF)
    auto controlsGrid = new QGridLayout();
    controlsGrid->setSpacing(6);

    const int ctrlBtnH = 34;

    m_btnMute = new QPushButton("🎤 Mute", this);
    m_btnMute->setProperty("class", "inCallBtn");
    m_btnMute->setFixedHeight(ctrlBtnH);
    m_btnMute->setCursor(Qt::PointingHandCursor);
    m_btnMute->setToolTip("Mutar / Desmutar Microfone");
    connect(m_btnMute, &QPushButton::clicked, this, &ActiveCallDialog::onMute);

    m_btnHold = new QPushButton("⏸ Espera", this);
    m_btnHold->setProperty("class", "inCallBtn");
    m_btnHold->setFixedHeight(ctrlBtnH);
    m_btnHold->setCheckable(true);
    m_btnHold->setCursor(Qt::PointingHandCursor);
    m_btnHold->setToolTip("Colocar chamada em Espera (Hold)");
    connect(m_btnHold, &QPushButton::clicked, this, &ActiveCallDialog::onHold);

    m_btnTransfer = new QPushButton("🔄 Transferir", this);
    m_btnTransfer->setProperty("class", "inCallBtn");
    m_btnTransfer->setFixedHeight(ctrlBtnH);
    m_btnTransfer->setCursor(Qt::PointingHandCursor);
    m_btnTransfer->setToolTip("Transferência de Chamada");
    connect(m_btnTransfer, &QPushButton::clicked, this, &ActiveCallDialog::onTransfer);

    m_btnRecord = new QPushButton("⏺ Gravar", this);
    m_btnRecord->setProperty("class", "inCallBtn");
    m_btnRecord->setFixedHeight(ctrlBtnH);
    m_btnRecord->setCheckable(true);
    m_btnRecord->setCursor(Qt::PointingHandCursor);
    m_btnRecord->setToolTip("Iniciar / Parar Gravação");
    connect(m_btnRecord, &QPushButton::clicked, this, &ActiveCallDialog::onRecord);

    m_btnConference = new QPushButton("👥 Conferência", this);
    m_btnConference->setProperty("class", "inCallBtn");
    m_btnConference->setFixedHeight(ctrlBtnH);
    m_btnConference->setCursor(Qt::PointingHandCursor);
    m_btnConference->setToolTip("Iniciar Modo Conferência");
    connect(m_btnConference, &QPushButton::clicked, this, &ActiveCallDialog::onConference);

    m_btnDtmfToggle = new QPushButton("🔢 DTMF", this);
    m_btnDtmfToggle->setProperty("class", "inCallBtn");
    m_btnDtmfToggle->setFixedHeight(ctrlBtnH);
    m_btnDtmfToggle->setCheckable(true);
    m_btnDtmfToggle->setCursor(Qt::PointingHandCursor);
    m_btnDtmfToggle->setToolTip("Exibir ou Ocultar Teclado DTMF");
    connect(m_btnDtmfToggle, &QPushButton::clicked, this, &ActiveCallDialog::onToggleDtmfPad);

    controlsGrid->addWidget(m_btnMute, 0, 0);
    controlsGrid->addWidget(m_btnHold, 0, 1);
    controlsGrid->addWidget(m_btnTransfer, 0, 2);
    controlsGrid->addWidget(m_btnRecord, 1, 0);
    controlsGrid->addWidget(m_btnConference, 1, 1);
    controlsGrid->addWidget(m_btnDtmfToggle, 1, 2);

    mainLayout->addLayout(controlsGrid);

    // 3. Teclado DTMF Retrátil (Ajusta a janela dinamicamente ao expandir)
    m_dtmfFrame = new QFrame(this);
    m_dtmfFrame->setObjectName("dtmfFrame");
    m_dtmfFrame->setVisible(false);
    auto dtmfLayout = new QGridLayout(m_dtmfFrame);
    dtmfLayout->setContentsMargins(2, 2, 2, 2);
    dtmfLayout->setSpacing(4);

    const QString dtmfKeys[] = {
        "1", "2", "3",
        "4", "5", "6",
        "7", "8", "9",
        "*", "0", "#"
    };

    for (int i = 0; i < 12; ++i) {
        auto btn = new QPushButton(dtmfKeys[i], m_dtmfFrame);
        btn->setProperty("class", "dialBtn");
        btn->setProperty("digit", dtmfKeys[i]);
        btn->setFixedHeight(34);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, &ActiveCallDialog::onDtmfDigit);
        dtmfLayout->addWidget(btn, i / 3, i % 3);
    }
    mainLayout->addWidget(m_dtmfFrame);

    // 4. Hangup Button (Vermelho Corporativo Padronizado)
    m_btnHangup = new QPushButton("📴 Encerrar Chamada", this);
    m_btnHangup->setObjectName("hangupButton");
    m_btnHangup->setFixedHeight(42);
    m_btnHangup->setCursor(Qt::PointingHandCursor);
    m_btnHangup->setToolTip("Encerrar a chamada atual");
    connect(m_btnHangup, &QPushButton::clicked, this, &ActiveCallDialog::onHangup);
    mainLayout->addWidget(m_btnHangup);

    updateThemeVisuals();
}

void ActiveCallDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    updateThemeVisuals();
}

void ActiveCallDialog::updateThemeVisuals() {
    if (qApp) {
        setStyleSheet(qApp->styleSheet());
    }
}

void ActiveCallDialog::setupCall(const QString &callId, const QString &number, const QString &name, const QString &company) {
    m_callId = callId;
    m_lblName->setText(name.isEmpty() ? "Chamada SIP" : name);
    m_lblNumber->setText(number.isEmpty() ? "" : QString("Ramal: %1").arg(number));

    if (!company.isEmpty()) {
        m_lblCompany->setText("🏢 " + company);
        m_lblCompany->setVisible(true);
    } else {
        m_lblCompany->setVisible(false);
    }

    m_lblStatus->setText("● Chamando...");
    m_lblStatus->setStyleSheet("color: #F39C12; font-weight: bold; background: transparent; font-size: 13px;");
    m_lblTimer->setText("00:00");
    m_lblRecBadge->setVisible(false);

    m_isHold = false;
    m_isConference = false;
    m_btnHold->setChecked(false);
    m_btnHold->setText("⏸ Espera");
    m_btnHold->setStyleSheet("");
    m_btnConference->setChecked(false);
    m_btnConference->setText("👥 Conferência");
    m_btnConference->setStyleSheet("");
    m_btnMute->setText("🎤 Mute");
    m_isMuted = false;
    m_btnRecord->setChecked(false);
    m_btnRecord->setText("⏺ Gravar");

    // Fecha DTMF se estava aberto e recalibra tamanho da janela
    if (m_dtmfFrame->isVisible()) {
        m_dtmfFrame->setVisible(false);
        m_btnDtmfToggle->setChecked(false);
        setMinimumHeight(320);
        resize(width(), 340);
    }

    updateThemeVisuals();
}

void ActiveCallDialog::onCallStateChanged(const QString &callId, SipManager::CallState state, const QString &stateText) {
    if (m_callId.isEmpty() || m_callId == callId) {
        m_callId = callId;

        if (state == SipManager::CallState::Confirmed) {
            m_lblStatus->setText("● Conectado");
            m_lblStatus->setStyleSheet("color: #2ECC71; font-weight: bold; background: transparent; font-size: 13px;");
        } else if (state == SipManager::CallState::Disconnected) {
            m_lblStatus->setText("● Desconectado");
            m_lblStatus->setStyleSheet("color: #E74C3C; font-weight: bold; background: transparent; font-size: 13px;");
            hide();
        } else {
            m_lblStatus->setText("● " + stateText);
            m_lblStatus->setStyleSheet("color: #F39C12; font-weight: bold; background: transparent; font-size: 13px;");
        }
    }
}

void ActiveCallDialog::onCallDurationChanged(const QString &callId, int seconds) {
    if (m_callId.isEmpty() || m_callId == callId) {
        int mins = seconds / 60;
        int secs = seconds % 60;
        m_lblTimer->setText(QString("%1:%2")
                                .arg(mins, 2, 10, QChar('0'))
                                .arg(secs, 2, 10, QChar('0')));
    }
}

void ActiveCallDialog::onMuteStateChanged(const QString &callId, bool isMuted) {
    if (m_callId.isEmpty() || m_callId == callId) {
        m_isMuted = isMuted;
        m_btnMute->setText(isMuted ? "🔊 Desmutar" : "🎤 Mute");
    }
}

void ActiveCallDialog::onHoldStateChanged(const QString &callId, bool isHold) {
    if (m_callId.isEmpty() || m_callId == callId) {
        m_isHold = isHold;
        m_btnHold->setChecked(isHold);
        m_btnHold->setText(isHold ? "▶ Retomar" : "⏸ Espera");
        if (isHold) {
            m_lblStatus->setText("● Em Espera (Hold)");
            m_lblStatus->setStyleSheet("color: #E67E22; font-weight: bold; background: transparent; font-size: 13px;");
            m_btnHold->setStyleSheet("background-color: #E67E22; color: #FFFFFF; font-weight: bold; border-radius: 6px;");
        } else {
            m_lblStatus->setText("● Conectado");
            m_lblStatus->setStyleSheet("color: #2ECC71; font-weight: bold; background: transparent; font-size: 13px;");
            m_btnHold->setStyleSheet("");
        }
    }
}

void ActiveCallDialog::onRecordingStateChanged(const QString &callId, bool isRecording) {
    if (m_callId.isEmpty() || m_callId == callId) {
        m_isRecording = isRecording;
        m_btnRecord->setChecked(isRecording);
        m_btnRecord->setText(isRecording ? "⏹ Gravando..." : "⏺ Gravar");
        m_lblRecBadge->setVisible(isRecording);
        if (isRecording) {
            m_lblRecBadge->setStyleSheet("color: #E74C3C; font-weight: bold; font-size: 12px; background: rgba(231, 76, 60, 0.2); border-radius: 3px; padding: 2px 6px;");
        }
    }
}

void ActiveCallDialog::onHangup() {
    emit hangupClicked(m_callId);
    hide();
}

void ActiveCallDialog::onTransfer() {
    bool ok = false;
    QString target = QInputDialog::getText(
        this, "Transferir Chamada",
        "Digite o ramal ou número de destino:",
        QLineEdit::Normal, "", &ok
    );
    if (ok && !target.trimmed().isEmpty()) {
        emit transferClicked(m_callId, target.trimmed());
    }
}

void ActiveCallDialog::onMute() {
    m_isMuted = !m_isMuted;
    m_btnMute->setText(m_isMuted ? "🔊 Desmutar" : "🎤 Mute");
    emit muteToggled(m_callId, m_isMuted);
}

void ActiveCallDialog::onHold() {
    m_isHold = !m_isHold;
    m_btnHold->setChecked(m_isHold);
    m_btnHold->setText(m_isHold ? "▶ Retomar" : "⏸ Espera");
    if (m_isHold) {
        m_lblStatus->setText("● Em Espera (Hold)");
        m_lblStatus->setStyleSheet("color: #E67E22; font-weight: bold; background: transparent; font-size: 13px;");
        m_btnHold->setStyleSheet("background-color: #E67E22; color: #FFFFFF; font-weight: bold; border-radius: 6px;");
    } else {
        m_lblStatus->setText("● Conectado");
        m_lblStatus->setStyleSheet("color: #2ECC71; font-weight: bold; background: transparent; font-size: 13px;");
        m_btnHold->setStyleSheet("");
    }
    emit holdToggled(m_callId, m_isHold);
}

void ActiveCallDialog::onConference() {
    bool ok = false;
    QString target = QInputDialog::getText(
        this, "Conferência SIP",
        "Digite o ramal ou número para adicionar à conferência:",
        QLineEdit::Normal, "", &ok
    );
    if (ok && !target.trimmed().isEmpty()) {
        m_isConference = true;
        m_btnConference->setChecked(true);
        m_btnConference->setText("👥 Conf. Ativa");
        m_btnConference->setStyleSheet("background-color: #2980B9; color: #FFFFFF; font-weight: bold; border-radius: 6px;");
        m_lblStatus->setText(QString("● Conferência com %1").arg(target.trimmed()));
        m_lblStatus->setStyleSheet("color: #3498DB; font-weight: bold; background: transparent; font-size: 13px;");
        emit conferenceClicked();
    }
}

void ActiveCallDialog::onRecord() {
    m_btnRecord->setChecked(!m_btnRecord->isChecked());
    bool rec = m_btnRecord->isChecked();
    m_btnRecord->setText(rec ? "⏹ Gravando..." : "⏺ Gravar");
    emit recordToggled(m_callId);
}

void ActiveCallDialog::onDtmfDigit() {
    auto btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;
    QString digit = btn->property("digit").toString();
    emit dtmfClicked(digit);
}

void ActiveCallDialog::onToggleDtmfPad() {
    bool visible = !m_dtmfFrame->isVisible();
    m_dtmfFrame->setVisible(visible);
    m_btnDtmfToggle->setChecked(visible);

    // Ajusta o tamanho da janela dinamicamente para não sobrepor botões
    if (visible) {
        setMinimumHeight(490);
        resize(width(), 510);
    } else {
        setMinimumHeight(320);
        resize(width(), 340);
    }
    adjustSize();
}
