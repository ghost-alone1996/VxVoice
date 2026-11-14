#include "CallWidget.h"
#include "ContactsManager.h"
#include "ThemeManager.h"
#include "IconHelper.h"
#include "SelectContactNumberDialog.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QShortcut>
#include <QKeySequence>
#include <QKeyEvent>

CallWidget::CallWidget(QWidget *parent)
    : QWidget(parent),
      m_clockTimer(new QTimer(this)) {
    setupUi();
    updateInCallUiState(false);

    connect(m_clockTimer, &QTimer::timeout, this, &CallWidget::onClockTick);
    m_clockTimer->start(1000);

    connect(&ContactsManager::instance(), &ContactsManager::contactsChanged, this, [this]() {
        refreshStatusLine();
    });

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        updateThemeVisuals();
    });

    updateThemeVisuals();
    onClockTick();
}

void CallWidget::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 8, 10, 8);
    mainLayout->setSpacing(8);

    // =================================================================
    // 1. Visor Informativo Superior (Fica EM CIMA do campo de digitar)
    // Mostra Ramal, Data/Hora e Status alinhados à esquerda com letra aumentada uniforme
    // =================================================================
    m_topInfoFrame = new QFrame(this);
    m_topInfoFrame->setObjectName("callInfoFrame");

    auto topInfoLayout = new QVBoxLayout(m_topInfoFrame);
    topInfoLayout->setContentsMargins(8, 6, 8, 6);
    topInfoLayout->setSpacing(4);

    m_lblAccountInfo = new QLabel(m_topInfoFrame);
    m_lblAccountInfo->setStyleSheet("font-size: 13px; background: transparent;");
    m_lblAccountInfo->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_lblDateTime = new QLabel(m_topInfoFrame);
    m_lblDateTime->setStyleSheet("font-size: 13px; font-weight: 600; background: transparent;");
    m_lblDateTime->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_lblCallStatus = new QLabel(m_topInfoFrame);
    m_lblCallStatus->setStyleSheet("font-size: 13px; font-weight: 600; background: transparent;");
    m_lblCallStatus->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    topInfoLayout->addWidget(m_lblAccountInfo);
    topInfoLayout->addWidget(m_lblDateTime);
    topInfoLayout->addWidget(m_lblCallStatus);

    mainLayout->addWidget(m_topInfoFrame);

    // =================================================================
    // 2. Campo de Digitação e Botão Backspace (ABAIXO do visor informativo)
    // =================================================================
    auto displayLayout = new QHBoxLayout();
    displayLayout->setSpacing(6);

    m_numberDisplay = new QLineEdit(this);
    m_numberDisplay->setObjectName("callerIdDisplay");
    m_numberDisplay->setPlaceholderText("Digite o ramal ou busque um contato...");
    m_numberDisplay->setAlignment(Qt::AlignCenter);
    m_numberDisplay->setFixedHeight(42);
    m_numberDisplay->installEventFilter(this);

    // Sistema de busca em tempo real com auto-completar inteligente nos contatos
    m_completer = new QCompleter(this);
    m_completerModel = new QStandardItemModel(this);
    m_completer->setModel(m_completerModel);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setMaxVisibleItems(7);
    m_numberDisplay->setCompleter(m_completer);

    connect(m_completer, QOverload<const QModelIndex &>::of(&QCompleter::activated), this, [this](const QModelIndex &index) {
        QString num = index.data(Qt::UserRole).toString();
        if (!num.isEmpty()) {
            m_numberDisplay->setText(num);
        }
    });

    connect(&ContactsManager::instance(), &ContactsManager::contactsChanged, this, &CallWidget::updateCompleterModel);
    updateCompleterModel();

    // Botão de apagar (Backspace) aprimorado com ícone vetorial e suporte a clique ou clique longo
    m_btnBackspace = new QPushButton(this);
    m_btnBackspace->setObjectName("btnDialBackspace");
    m_btnBackspace->setIcon(IconHelper::createBackspaceIcon(QColor("#ECF0F1"), 20));
    m_btnBackspace->setIconSize(QSize(20, 20));
    m_btnBackspace->setFixedSize(46, 42);
    m_btnBackspace->setCursor(Qt::PointingHandCursor);
    m_btnBackspace->setToolTip("Apagar (Clique: 1 dígito • Segure: Limpar tudo)");

    m_backspaceLongPressTimer = new QTimer(this);
    m_backspaceLongPressTimer->setInterval(450);
    m_backspaceLongPressTimer->setSingleShot(true);
    connect(m_backspaceLongPressTimer, &QTimer::timeout, this, [this]() {
        m_numberDisplay->clear();
    });

    connect(m_btnBackspace, &QPushButton::pressed, this, [this]() {
        m_backspaceLongPressTimer->start();
    });
    connect(m_btnBackspace, &QPushButton::released, this, [this]() {
        if (m_backspaceLongPressTimer->isActive()) {
            m_backspaceLongPressTimer->stop();
            onBackspaceClicked();
        }
    });

    displayLayout->addWidget(m_numberDisplay, 1);
    displayLayout->addWidget(m_btnBackspace);
    mainLayout->addLayout(displayLayout);

    // =================================================================
    // 3. Painel de Controles In-Call (Aparece alinhado durante a chamada)
    // =================================================================
    m_inCallControlsFrame = new QFrame(this);
    auto inCallLayout = new QGridLayout(m_inCallControlsFrame);
    inCallLayout->setContentsMargins(6, 6, 6, 6);
    inCallLayout->setSpacing(6);

    const int inCallBtnH = 34;

    m_btnMute = new QPushButton("🎤 Mute", m_inCallControlsFrame);
    m_btnMute->setProperty("class", "inCallBtn");
    m_btnMute->setFixedHeight(inCallBtnH);
    m_btnMute->setCursor(Qt::PointingHandCursor);
    m_btnMute->setToolTip("Mutar / Desmutar Microfone");
    connect(m_btnMute, &QPushButton::clicked, this, &CallWidget::onMuteButtonClicked);

    m_btnHold = new QPushButton("⏸ Espera", m_inCallControlsFrame);
    m_btnHold->setProperty("class", "inCallBtn");
    m_btnHold->setFixedHeight(inCallBtnH);
    m_btnHold->setCheckable(true);
    m_btnHold->setCursor(Qt::PointingHandCursor);
    m_btnHold->setToolTip("Colocar chamada em Espera (Hold)");
    connect(m_btnHold, &QPushButton::clicked, this, &CallWidget::onHoldButtonClicked);

    m_btnTransfer = new QPushButton("🔄 Transferir", m_inCallControlsFrame);
    m_btnTransfer->setProperty("class", "inCallBtn");
    m_btnTransfer->setFixedHeight(inCallBtnH);
    m_btnTransfer->setCursor(Qt::PointingHandCursor);
    m_btnTransfer->setToolTip("Transferir chamada para outro ramal");
    connect(m_btnTransfer, &QPushButton::clicked, this, &CallWidget::onTransferButtonClicked);

    m_btnRecord = new QPushButton("⏺ Gravar", m_inCallControlsFrame);
    m_btnRecord->setProperty("class", "inCallBtn");
    m_btnRecord->setFixedHeight(inCallBtnH);
    m_btnRecord->setCheckable(true);
    m_btnRecord->setCursor(Qt::PointingHandCursor);
    m_btnRecord->setToolTip("Iniciar / Parar gravação da chamada");
    connect(m_btnRecord, &QPushButton::clicked, this, &CallWidget::onRecordButtonClicked);

    m_btnConference = new QPushButton("👥 Conferência", m_inCallControlsFrame);
    m_btnConference->setProperty("class", "inCallBtn");
    m_btnConference->setFixedHeight(inCallBtnH);
    m_btnConference->setCursor(Qt::PointingHandCursor);
    m_btnConference->setToolTip("Iniciar modo de Conferência");
    connect(m_btnConference, &QPushButton::clicked, this, &CallWidget::onConferenceButtonClicked);

    inCallLayout->addWidget(m_btnMute, 0, 0);
    inCallLayout->addWidget(m_btnHold, 0, 1);
    inCallLayout->addWidget(m_btnTransfer, 0, 2);
    inCallLayout->addWidget(m_btnRecord, 1, 0);
    inCallLayout->addWidget(m_btnConference, 1, 1, 1, 2);

    mainLayout->addWidget(m_inCallControlsFrame);

    // =================================================================
    // 4. Teclado Numérico (Dialpad Grid 1-9, *, 0, #)
    // =================================================================
    auto gridLayout = new QGridLayout();
    gridLayout->setSpacing(6);

    const struct KeyInfo {
        QString main;
        QString sub;
        int row;
        int col;
    } keys[] = {
        {"1", "", 0, 0}, {"2", "ABC", 0, 1}, {"3", "DEF", 0, 2},
        {"4", "GHI", 1, 0}, {"5", "JKL", 1, 1}, {"6", "MNO", 1, 2},
        {"7", "PQRS", 2, 0}, {"8", "TUV", 2, 1}, {"9", "WXYZ", 2, 2},
        {"*", "", 3, 0}, {"0", "+", 3, 1}, {"#", "", 3, 2}
    };

    for (const auto &k : keys) {
        auto btn = new QPushButton(this);
        btn->setProperty("class", "dialBtn");
        btn->setProperty("digit", k.main);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        btn->setMinimumHeight(44);

        if (k.sub.isEmpty()) {
            btn->setText(k.main);
        } else {
            btn->setText(QString("%1\n%2").arg(k.main, k.sub));
        }

        connect(btn, &QPushButton::clicked, this, &CallWidget::onDigitClicked);
        gridLayout->addWidget(btn, k.row, k.col);
    }

    mainLayout->addLayout(gridLayout, 1);

    // =================================================================
    // 5. Botões de Ação Inferiores (Iniciar Chamada / Encerrar)
    // =================================================================
    auto actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(10);

    const int mainActionH = 42;

    m_btnCall = new QPushButton("📞 Iniciar Chamada", this);
    m_btnCall->setObjectName("callButton");
    m_btnCall->setFixedHeight(mainActionH);
    m_btnCall->setCursor(Qt::PointingHandCursor);
    m_btnCall->setToolTip("Discar para o número digitado");
    connect(m_btnCall, &QPushButton::clicked, this, &CallWidget::onCallButtonClicked);

    m_btnHangup = new QPushButton("📴 Encerrar", this);
    m_btnHangup->setObjectName("hangupButton");
    m_btnHangup->setFixedHeight(mainActionH);
    m_btnHangup->setCursor(Qt::PointingHandCursor);
    m_btnHangup->setToolTip("Encerrar chamada em andamento");
    m_btnHangup->setEnabled(false);
    connect(m_btnHangup, &QPushButton::clicked, this, &CallWidget::onHangupButtonClicked);

    actionLayout->addWidget(m_btnCall, 1);
    actionLayout->addWidget(m_btnHangup, 1);
    mainLayout->addLayout(actionLayout);

    connect(m_numberDisplay, &QLineEdit::returnPressed, this, &CallWidget::onCallButtonClicked);
}

void CallWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    updateThemeVisuals();
}

void CallWidget::updateThemeVisuals() {
    ThemeManager::Theme theme = ThemeManager::instance().currentTheme();

    if (theme == ThemeManager::Theme::Light ||
        theme == ThemeManager::Theme::Win2000 ||
        theme == ThemeManager::Theme::WinXP ||
        theme == ThemeManager::Theme::Win7) {
        QString borderCol = (theme == ThemeManager::Theme::Win2000) ? "#808080"
                          : (theme == ThemeManager::Theme::WinXP)   ? "#7F9DB9"
                          : (theme == ThemeManager::Theme::Win7)    ? "#89A4C7"
                                                                    : "#CBD5E0";
        QString radius = (theme == ThemeManager::Theme::Win2000) ? "0px" : "6px";
        m_topInfoFrame->setStyleSheet(
            QString("QFrame#callInfoFrame { background-color: #FFFFFF; border: 1px solid %1; border-radius: %2; padding: 6px 10px; }")
                .arg(borderCol, radius)
        );
        m_inCallControlsFrame->setStyleSheet(
            QString("QFrame { background-color: #F8F9FA; border: 1px solid %1; border-radius: %2; padding: 4px; }")
                .arg(borderCol, radius)
        );
    } else if (theme == ThemeManager::Theme::Dark) {
        m_topInfoFrame->setStyleSheet(
            "QFrame#callInfoFrame { "
            "  background-color: #151E27; "
            "  border: 1px solid #2C3E50; "
            "  border-radius: 6px; "
            "  padding: 6px 10px; "
            "}"
        );
        m_inCallControlsFrame->setStyleSheet(
            "QFrame { background-color: #1A2530; border: 1px solid #2C3E50; border-radius: 6px; padding: 4px; }"
        );
    } else if (theme == ThemeManager::Theme::Red) {
        m_topInfoFrame->setStyleSheet(
            "QFrame#callInfoFrame { "
            "  background-color: #241518; "
            "  border: 1px solid #4A2329; "
            "  border-radius: 6px; "
            "  padding: 6px 10px; "
            "}"
        );
        m_inCallControlsFrame->setStyleSheet(
            "QFrame { background-color: #241518; border: 1px solid #4A2329; border-radius: 6px; padding: 4px; }"
        );
    } else { // VxVoice Purple
        m_topInfoFrame->setStyleSheet(
            "QFrame#callInfoFrame { "
            "  background-color: #1E1632; "
            "  border: 1px solid #3E2A5D; "
            "  border-radius: 6px; "
            "  padding: 6px 10px; "
            "}"
        );
        m_inCallControlsFrame->setStyleSheet(
            "QFrame { background-color: #201835; border: 1px solid #4A326F; border-radius: 6px; padding: 4px; }"
        );
    }

    QColor backspaceColor = ThemeManager::isLightTheme(theme) ? QColor("#2C3E50") : QColor("#ECF0F1");
    if (m_btnBackspace) {
        m_btnBackspace->setIcon(IconHelper::createBackspaceIcon(backspaceColor, 20));
    }

    updateAccountDisplay();
    refreshStatusLine();
}

void CallWidget::updateCompleterModel() {
    if (!m_completerModel) return;
    m_completerModel->clear();

    const auto contacts = ContactsManager::instance().contacts();
    for (const auto &c : contacts) {
        for (const QString &num : c.allNumbers()) {
            QString label = QString("%1 (%2)").arg(c.name, num);
            auto item = new QStandardItem(label);
            item->setData(num, Qt::UserRole);
            item->setData(c.name, Qt::UserRole + 1);
            item->setIcon(IconHelper::createUserIcon(QColor("#8E44AD"), 16));
            m_completerModel->appendRow(item);
        }
    }
}

void CallWidget::updateAccountDisplay() {
    QString accDisplayName = SipManager::instance().currentAccountDisplayName();
    QString regStatus = SipManager::instance().registrationStateString();
    QColor regColor = SipManager::instance().registrationStateColor();

    ThemeManager::Theme theme = ThemeManager::instance().currentTheme();
    QString accColor = "#D7BDE2";
    if (theme == ThemeManager::Theme::Light) {
        accColor = "#1A5276";
    } else if (theme == ThemeManager::Theme::Dark) {
        accColor = "#3498DB";
    } else if (theme == ThemeManager::Theme::Red) {
        accColor = "#FF7675";
    } else {
        accColor = "#D7BDE2";
    }

    // Mostra apenas o nome da conta SIP e o status de registro ao lado
    m_lblAccountInfo->setText(
        QString("<span style='color: %1; font-weight: bold;'>👤 %2</span>   •   "
                "<span style='color: %3; font-weight: bold;'>%4</span>")
        .arg(accColor, accDisplayName, regColor.name(), regStatus)
    );
}

void CallWidget::onClockTick() {
    refreshStatusLine();
}

void CallWidget::refreshStatusLine() {
    QString timeStr = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    ThemeManager::Theme theme = ThemeManager::instance().currentTheme();

    // Determina a cor base para texto em repouso de acordo com o tema
    QString defaultTextColor = "#F5EEF8";
    if (theme == ThemeManager::Theme::Light) {
        defaultTextColor = "#2C3E50";
    } else if (theme == ThemeManager::Theme::Dark) {
        defaultTextColor = "#ECF0F1";
    } else if (theme == ThemeManager::Theme::Red) {
        defaultTextColor = "#FDEDEC";
    } else {
        defaultTextColor = "#F5EEF8";
    }

    QString activeColor = (theme == ThemeManager::Theme::Light) ? "#27AE60" : "#2ECC71";
    QString ringingColor = (theme == ThemeManager::Theme::Light) ? "#D35400" : "#E67E22";

    // Busca o nome nos contatos se disponível
    QString who = ContactsManager::instance().findContactNameByNumber(m_remoteNumber);
    if (who.isEmpty()) who = m_remoteName;
    if (who.isEmpty()) who = m_remoteNumber;

    // Linha 2: Data e Hora
    m_lblDateTime->setText(
        QString("<span style='color: %1;'>📅 %2</span>").arg(defaultTextColor, timeStr)
    );

    // Linha 3: Status abaixo da Data e Hora
    if (m_callState == SipManager::CallState::Incoming) {
        // Chamada Recebida: mostra QUEM ESTÁ LIGANDO com destaque do nome do contato
        QString whoDisplay = who;
        if (!m_remoteNumber.isEmpty() && who != m_remoteNumber) {
            whoDisplay = QString("%1 (%2)").arg(who, m_remoteNumber);
        }

        m_lblCallStatus->setText(
            QString("<span style='color: %1; font-weight: bold;'>📞 Ligando: %2</span>")
            .arg(ringingColor, whoDisplay)
        );
    } else if (m_callState == SipManager::CallState::Confirmed) {
        // Chamada Atendida / Conectada: mostra contato e duração
        int mins = m_currentDurationSeconds / 60;
        int secs = m_currentDurationSeconds % 60;
        QString timerStr = QString("%1:%2")
                               .arg(mins, 2, 10, QChar('0'))
                               .arg(secs, 2, 10, QChar('0'));

        m_lblCallStatus->setText(
            QString("<span style='color: %1; font-weight: bold;'>📞 Em Chamada: %2 (%3)</span>")
            .arg(activeColor, who, timerStr)
        );
    } else if (m_callState == SipManager::CallState::Calling || m_callState == SipManager::CallState::Connecting) {
        // Discando / Chamando
        QString callingColor = "#3498DB";
        if (theme == ThemeManager::Theme::Light) {
            callingColor = "#2980B9";
        } else if (theme == ThemeManager::Theme::Red) {
            callingColor = "#E74C3C";
        }
        m_lblCallStatus->setText(
            QString("<span style='color: %1; font-weight: bold;'>📞 Chamando: %2...</span>")
            .arg(callingColor, who)
        );
    } else if (m_callState == SipManager::CallState::OnHold) {
        QString holdColor = "#E67E22";
        m_lblCallStatus->setText(
            QString("<span style='color: %1; font-weight: bold;'>⏸ Em Espera: %2</span>")
            .arg(holdColor, who)
        );
    } else {
        // Repouso: Pronto para discar logo abaixo da Data e Hora
        m_lblCallStatus->setText(
            QString("<span style='color: %1; font-weight: 500;'>Status: Pronto para discar</span>")
            .arg(defaultTextColor)
        );
    }
}

void CallWidget::updateInCallUiState(bool inCall) {
    m_isCallActive = inCall;
    m_inCallControlsFrame->setVisible(inCall);
    m_btnHangup->setEnabled(inCall);
    m_btnCall->setEnabled(!inCall);

    if (!inCall) {
        m_currentDurationSeconds = 0;
        m_callState = SipManager::CallState::Idle;
        m_remoteNumber.clear();
        m_remoteName.clear();
        m_isHold = false;
        m_isConference = false;
        m_btnHold->setChecked(false);
        m_btnHold->setText("⏸ Espera");
        m_btnHold->setStyleSheet("");
        m_btnConference->setChecked(false);
        m_btnConference->setText("👥 Conferência");
        m_btnConference->setStyleSheet("");
        m_btnMute->setText("🎤 Mute");
        m_btnMute->setStyleSheet("");
        m_isMuted = false;
        m_btnRecord->setChecked(false);
        m_btnRecord->setText("⏺ Gravar");
    }
    refreshStatusLine();
}

void CallWidget::setDialNumber(const QString &number) {
    m_numberDisplay->setText(number);
    m_numberDisplay->setFocus();
}

QString CallWidget::currentNumber() const {
    return ContactsManager::instance().resolveDestination(m_numberDisplay->text());
}

void CallWidget::onDigitClicked() {
    auto btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;
    QString digit = btn->property("digit").toString();
    m_numberDisplay->setText(m_numberDisplay->text() + digit);

    if (m_isCallActive) {
        emit dtmfRequested(digit);
    }
}

void CallWidget::onBackspaceClicked() {
    QString txt = m_numberDisplay->text();
    if (!txt.isEmpty()) {
        txt.chop(1);
        m_numberDisplay->setText(txt);
    }
}

void CallWidget::onCallButtonClicked() {
    QString rawInput = m_numberDisplay->text().trimmed();

    // Se o usuário digitou o nome direto de um contato que possui ramal e número cadastrados
    for (const auto &c : ContactsManager::instance().contacts()) {
        if (c.name.compare(rawInput, Qt::CaseInsensitive) == 0 &&
            !c.extension.trimmed().isEmpty() && !c.number.trimmed().isEmpty() &&
            c.extension.trimmed() != c.number.trimmed()) {
            SelectContactNumberDialog dlg(c, this);
            if (dlg.exec() == QDialog::Accepted) {
                QString chosen = dlg.selectedNumber();
                if (!chosen.isEmpty()) {
                    m_numberDisplay->setText(chosen);
                } else {
                    return;
                }
            } else {
                return;
            }
            break;
        }
    }

    QString num = currentNumber();
    if (num.isEmpty()) return;

    if (SipManager::isSelfExtension(num)) {
        QMessageBox::warning(this, "Ligação Não Permitida",
            QString("Não é possível realizar uma ligação para o ramal '%1', pois ele está atualmente logado na sua conta SIP.").arg(num));
        return;
    }

    m_remoteNumber = num;
    m_remoteName = ContactsManager::instance().findContactNameByNumber(num);
    m_callState = SipManager::CallState::Calling;
    refreshStatusLine();

    emit callRequested(num);
}

void CallWidget::onHangupButtonClicked() {
    emit hangupRequested(m_activeCallId);
}

void CallWidget::onHoldButtonClicked() {
    m_isHold = !m_isHold;
    m_btnHold->setChecked(m_isHold);
    m_btnHold->setText(m_isHold ? "▶ Retomar" : "⏸ Espera");
    if (m_isHold) {
        m_btnHold->setStyleSheet("background-color: #E67E22; color: #FFFFFF; font-weight: bold; border-radius: 6px;");
        m_callState = SipManager::CallState::OnHold;
    } else {
        m_btnHold->setStyleSheet("");
        m_callState = SipManager::CallState::Confirmed;
    }
    refreshStatusLine();
    emit holdRequested(m_activeCallId, m_isHold);
}

void CallWidget::onMuteButtonClicked() {
    m_isMuted = !m_isMuted;
    m_btnMute->setText(m_isMuted ? "🔊 Desmutar" : "🎤 Mute");
    if (m_isMuted) {
        m_btnMute->setStyleSheet("QPushButton { background-color: #E67E22; color: #FFFFFF; }");
    } else {
        m_btnMute->setStyleSheet("");
    }
    emit muteRequested(m_activeCallId, m_isMuted);
}

void CallWidget::onTransferButtonClicked() {
    bool ok = false;
    QString target = QInputDialog::getText(
        this, "Transferência de Chamada",
        "Digite o ramal ou número de destino:",
        QLineEdit::Normal, "", &ok
    );
    if (ok && !target.trimmed().isEmpty()) {
        emit transferRequested(m_activeCallId, target.trimmed());
    }
}

void CallWidget::onRecordButtonClicked() {
    m_btnRecord->setChecked(!m_btnRecord->isChecked());
    bool rec = m_btnRecord->isChecked();
    m_btnRecord->setText(rec ? "⏹ Gravando..." : "⏺ Gravar");
    emit recordToggleRequested(m_activeCallId);
}

void CallWidget::onConferenceButtonClicked() {
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
        m_callState = SipManager::CallState::Confirmed;
        m_lblCallStatus->setText(QString("<span style='color: #3498DB; font-weight: bold;'>● Conferência com %1</span>").arg(target.trimmed()));
        emit conferenceRequested();
    }
}

void CallWidget::onIncomingCall(const QString &callId, const QString &number, const QString &name) {
    m_activeCallId = callId;
    m_remoteNumber = number;

    // Busca imediatamente nos contatos pelo número
    QString contactName = ContactsManager::instance().findContactNameByNumber(number);
    m_remoteName = !contactName.isEmpty() ? contactName : name;

    m_callState = SipManager::CallState::Incoming;
    refreshStatusLine();
}

void CallWidget::onCallStateChanged(const QString &callId, SipManager::CallState state, const QString &stateText) {
    m_activeCallId = callId;
    m_callState = state;
    m_currentStatusText = stateText;

    if (state != SipManager::CallState::Disconnected) {
        auto active = SipManager::instance().activeCall();
        if (!active.remoteNumber.isEmpty()) {
            m_remoteNumber = active.remoteNumber;
        }
        QString contactName = ContactsManager::instance().findContactNameByNumber(m_remoteNumber);
        m_remoteName = !contactName.isEmpty() ? contactName : active.remoteName;
    }

    switch (state) {
    case SipManager::CallState::Calling:
    case SipManager::CallState::Connecting:
    case SipManager::CallState::Incoming:
        updateInCallUiState(true);
        break;
    case SipManager::CallState::Confirmed:
        updateInCallUiState(true);
        break;
    case SipManager::CallState::Disconnected:
        updateInCallUiState(false);
        m_activeCallId.clear();
        break;
    default:
        break;
    }
    refreshStatusLine();
}

void CallWidget::onCallDurationChanged(const QString &callId, int seconds) {
    if (m_activeCallId == callId || m_activeCallId.isEmpty()) {
        m_currentDurationSeconds = seconds;
        refreshStatusLine();
    }
}

void CallWidget::onRecordingStateChanged(const QString &callId, bool isRecording) {
    if (m_activeCallId == callId) {
        m_btnRecord->setChecked(isRecording);
        m_btnRecord->setText(isRecording ? "⏹ Gravando..." : "⏺ Gravar");
    }
}

void CallWidget::onHoldStateChanged(const QString &callId, bool isHold) {
    if (m_activeCallId == callId) {
        m_isHold = isHold;
        m_btnHold->setChecked(isHold);
        m_btnHold->setText(isHold ? "▶ Retomar" : "⏸ Espera");
        if (isHold) {
            m_btnHold->setStyleSheet("background-color: #E67E22; color: #FFFFFF; font-weight: bold; border-radius: 6px;");
            m_callState = SipManager::CallState::OnHold;
        } else {
            m_btnHold->setStyleSheet("");
            m_callState = SipManager::CallState::Confirmed;
        }
        refreshStatusLine();
    }
}

void CallWidget::onMuteStateChanged(const QString &callId, bool isMuted) {
    if (m_activeCallId == callId) {
        m_isMuted = isMuted;
        m_btnMute->setText(isMuted ? "🔊 Desmutar" : "🎤 Mute");
        if (isMuted) {
            m_btnMute->setStyleSheet("QPushButton { background-color: #E67E22; color: #FFFFFF; }");
        } else {
            m_btnMute->setStyleSheet("");
        }
    }
}

bool CallWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_numberDisplay && event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            onCallButtonClicked();
            return true;
        } else if (keyEvent->key() == Qt::Key_Escape) {
            if (m_isCallActive) {
                onHangupButtonClicked();
            } else {
                m_numberDisplay->clear();
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
