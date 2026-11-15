#include "MainWindow.h"
#include "AccountsDialog.h"
#include "SettingsDialog.h"
#include "ChangelogDialog.h"
#include "ThemeManager.h"
#include "SoundManager.h"
#include "ContactsManager.h"
#include "IconHelper.h"
#include "UpdateManager.h"
#include "UpdateProgressDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QMessageBox>
#include <QStatusBar>
#include <QDateTime>
#include <QScreen>
#include <QActionGroup>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_notificationPopup(new CallNotificationPopup(this)),
      m_activeCallDialog(new ActiveCallDialog(this)) {

    // Configura o ícone oficial da janela e do processo
    setWindowIcon(QIcon(":/icons/app_icon.png"));

    setupUi();
    setupTrayIcon();
    setupConnections();

    // Sincroniza estado inicial do SIP e aplica Status de Registro somente quando iniciado junto com o SO (--autostart)
    SipManager::instance().initialize();
    bool startedWithOs = qApp->arguments().contains("--autostart") && SettingsManager::instance().startWithSystem();
    int savedPresence = startedWithOs
        ? qBound(0, SettingsManager::instance().registrationStatus(), 2)
        : 0;
    m_comboPresence->blockSignals(true);
    m_comboPresence->setCurrentIndex(savedPresence);
    m_comboPresence->blockSignals(false);
    onPresenceIndexChanged(savedPresence);
    updateVisorAccount();

    // Notificação e janela de atualização automática ao abrir o aplicativo
    connect(&UpdateManager::instance(), &UpdateManager::updateAvailable, this, [this](const QString &ver, const QString &log) {
        if (m_trayIcon && m_trayIcon->isVisible()) {
            m_trayIcon->showMessage("Atualização VxVOICE",
                QString("Nova versão v%1 disponível no GitHub!").arg(ver),
                QSystemTrayIcon::Information, 6000);
        }

        if (UpdateManager::instance().isSilentCheck()) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Atualização Disponível — VxVOICE");
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText(QString("<b>Uma nova versão do VxVOICE está disponível no GitHub!</b>"));
            msgBox.setInformativeText(
                QString("Versão atual instalada: <b>v%1</b><br>"
                        "Nova versão disponível: <b>v%2</b><br><br>"
                        "%3<br><br>"
                        "Deseja baixar e aplicar a atualização agora?")
                    .arg(SettingsManager::APP_VERSION, ver, QString(log).replace("\n", "<br>"))
            );
            auto btnUpdate = msgBox.addButton("Atualizar Agora", QMessageBox::AcceptRole);
            msgBox.addButton("Agora Não", QMessageBox::RejectRole);
            msgBox.setDefaultButton(btnUpdate);
            msgBox.exec();

            if (msgBox.clickedButton() == btnUpdate) {
                auto progressDlg = new UpdateProgressDialog(this);
                progressDlg->setAttribute(Qt::WA_DeleteOnClose);

                auto &um = UpdateManager::instance();
                disconnect(&um, &UpdateManager::updateProgress, progressDlg, nullptr);
                disconnect(&um, &UpdateManager::updateCompleted, progressDlg, nullptr);
                connect(&um, &UpdateManager::updateProgress, progressDlg, &UpdateProgressDialog::setProgress);
                connect(&um, &UpdateManager::updateCompleted, progressDlg, &UpdateProgressDialog::onUpdateCompleted);

                progressDlg->show();
                um.applyUpdate();
            }
        }
    });

    // Se acabou de atualizar ou é a primeira execução desta versão, exibe diálogo com melhorias e correções
    bool justUpdated = qApp->arguments().contains("--just-updated");
    QString lastVer = SettingsManager::instance().lastRunVersion();
    if (justUpdated || lastVer != SettingsManager::APP_VERSION) {
        SettingsManager::instance().setLastRunVersion(SettingsManager::APP_VERSION);
        QTimer::singleShot(600, this, [this]() {
            showChangelogDialog();
        });
    }

    // Se o aplicativo iniciou minimizado no System Tray, exibe notificação silenciosa de ramal disponível
    if (SettingsManager::instance().startMinimized()) {
        QTimer::singleShot(1500, this, [this]() {
            if (m_trayIcon && m_trayIcon->isVisible() && !isVisible() && !m_startupTrayNotified) {
                m_startupTrayNotified = true;
                auto acc = SettingsManager::instance().defaultAccount();
                QString ramal = !acc.username.isEmpty() ? acc.username : "";
                QString msg = !ramal.isEmpty()
                    ? QString("O VxVOICE está em execução em segundo plano com o ramal %1 disponível.").arg(ramal)
                    : "O VxVOICE está em execução em segundo plano com o serviço disponível.";
                m_trayIcon->showMessage("VxVOICE Softphone", msg, QSystemTrayIcon::Information, 5000);
            }
        });
    }

    QTimer::singleShot(3000, []() {
        UpdateManager::instance().checkForUpdates(true);
    });
}

MainWindow::~MainWindow() {
    SoundManager::instance().stopRinging();
    SipManager::instance().shutdown();
}

void MainWindow::setupUi() {
    setWindowTitle("VxVOICE");

    // Proporção padrão ajustada horizontalmente para acomodar cabeçalho, abas e discador sem cortes
    int targetW = 360;
    int targetH = 560;
    if (QScreen *scr = QApplication::primaryScreen()) {
        QRect avail = scr->availableGeometry();
        targetW = qMin(targetW, avail.width() - 40);
        targetH = qMin(targetH, avail.height() - 40);
    }
    setMinimumSize(345, qMin(540, targetH));
    resize(targetW, targetH);

    auto centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    setCentralWidget(centralWidget);

    auto rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // =================================================================
    // 1. Header Frame: Status SIP com Bolinhas Nítidas + Botões + Sair
    // =================================================================
    m_headerFrame = new QFrame(this);
    m_headerFrame->setObjectName("headerFrame");
    auto headerLayout = new QHBoxLayout(m_headerFrame);
    headerLayout->setContentsMargins(10, 8, 10, 8);
    headerLayout->setSpacing(6);

    // Status SIP com bolinhas coloridas nítidas (Verde, Vermelho, Cinza)
    m_comboPresence = new QComboBox(m_headerFrame);
    m_comboPresence->addItem(IconHelper::createStatusDot(QColor("#2ECC71"), 13), "Disponível");
    m_comboPresence->addItem(IconHelper::createStatusDot(QColor("#E74C3C"), 13), "Não Perturbe");
    m_comboPresence->addItem(IconHelper::createStatusDot(QColor("#95A5A6"), 13), "Offline");
    m_comboPresence->setIconSize(QSize(13, 13));
    m_comboPresence->setMinimumWidth(138);
    m_comboPresence->setFixedHeight(30);
    m_comboPresence->setToolTip("Alterar Status de Presença SIP");
    connect(m_comboPresence, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPresenceIndexChanged);

    // Botão Contas com ícone de bonequinho / usuário
    m_btnAccounts = new QPushButton(m_headerFrame);
    m_btnAccounts->setIcon(IconHelper::createUserIcon(QColor("#F5EEF8"), 18));
    m_btnAccounts->setIconSize(QSize(18, 18));
    m_btnAccounts->setFixedSize(34, 30);
    m_btnAccounts->setToolTip("Gerenciador de Contas SIP");
    m_btnAccounts->setCursor(Qt::PointingHandCursor);
    connect(m_btnAccounts, &QPushButton::clicked, this, &MainWindow::onOpenAccountsDialog);

    // Botão Configurações com engrenagem nítida
    m_btnSettings = new QPushButton(m_headerFrame);
    m_btnSettings->setIcon(IconHelper::createSettingsIcon(QColor("#F5EEF8"), 17));
    m_btnSettings->setIconSize(QSize(17, 17));
    m_btnSettings->setFixedSize(34, 30);
    m_btnSettings->setToolTip("Preferências e Configurações");
    m_btnSettings->setCursor(Qt::PointingHandCursor);
    connect(m_btnSettings, &QPushButton::clicked, this, &MainWindow::onOpenSettingsDialog);

    // Botão vermelho de Sair definitivo
    m_btnExit = new QPushButton("Sair", m_headerFrame);
    m_btnExit->setObjectName("exitButton");
    m_btnExit->setFixedHeight(30);
    m_btnExit->setMinimumWidth(56);
    m_btnExit->setToolTip("Encerrar VxVOICE e finalizar todos os processos");
    m_btnExit->setCursor(Qt::PointingHandCursor);
    connect(m_btnExit, &QPushButton::clicked, this, &MainWindow::onQuitApplication);

    // Layout do Header
    headerLayout->addWidget(m_comboPresence);
    headerLayout->addStretch();
    headerLayout->addWidget(m_btnAccounts);
    headerLayout->addWidget(m_btnSettings);
    headerLayout->addWidget(m_btnExit);

    rootLayout->addWidget(m_headerFrame);

    // =================================================================
    // 2. Tab Widget: Discador, Histórico, Contatos, Gravação
    // =================================================================
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->tabBar()->setExpanding(true);
    m_tabWidget->tabBar()->setDocumentMode(true);
    m_callWidget = new CallWidget(this);
    m_historyWidget = new HistoryWidget(this);
    m_contactsWidget = new ContactsWidget(this);
    m_recordingsWidget = new RecordingsWidget(this);

    m_tabWidget->addTab(m_callWidget, "📞 Ligação");
    m_tabWidget->addTab(m_historyWidget, "🕒 Histórico");
    m_tabWidget->addTab(m_contactsWidget, "👥 Contatos");
    m_tabWidget->addTab(m_recordingsWidget, "🎙 Gravação");

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    rootLayout->addWidget(m_tabWidget, 1);

    updateHeaderIcons();
    updateRecordingsTabVisibility();
}

void MainWindow::updateRecordingsTabVisibility() {
    if (!m_tabWidget || !m_recordingsWidget) return;
    int idx = m_tabWidget->indexOf(m_recordingsWidget);
    if (idx < 0) return;

    const bool autoRecEnabled = SettingsManager::instance().autoRecord();
    if (!autoRecEnabled && m_tabWidget->currentWidget() == m_recordingsWidget) {
        m_tabWidget->setCurrentWidget(m_callWidget);
    }
    m_tabWidget->setTabVisible(idx, autoRecEnabled);
}

void MainWindow::updateHeaderIcons() {
    auto theme = ThemeManager::instance().currentTheme();
    bool useDarkIcon = ThemeManager::isLightTheme(theme);
    QColor iconColor = useDarkIcon ? QColor("#1E293B") : QColor("#F5EEF8");

    if (m_btnAccounts) {
        m_btnAccounts->setIcon(IconHelper::createUserIcon(iconColor, 20));
    }
    if (m_btnSettings) {
        m_btnSettings->setIcon(IconHelper::createSettingsIcon(iconColor, 18));
    }
}

void MainWindow::updateVisorAccount() {
    if (m_callWidget) {
        m_callWidget->updateAccountDisplay();
    }
}

void MainWindow::setupTrayIcon() {
    QIcon icon(":/icons/app_icon.png");
    m_trayIcon = new QSystemTrayIcon(icon, this);
    m_trayIcon->setToolTip("VxVOICE Softphone");

    m_trayMenu = new QMenu(this);
    auto actionOpen = m_trayMenu->addAction("Abrir VxVOICE");
    connect(actionOpen, &QAction::triggered, this, [this]() {
        showNormal();
        activateWindow();
        raise();
    });

    auto themeMenu = m_trayMenu->addMenu("Tema");
    auto themeGroup = new QActionGroup(themeMenu);
    themeGroup->setExclusive(true);

    struct ThemeEntry {
        QString label;
        ThemeManager::Theme theme;
        QString settingKey;
    };
    const ThemeEntry themes[] = {
        {"Roxo Corporativo (VxVOICE)", ThemeManager::Theme::Purple, "Purple"},
        {"Escuro (Dark)", ThemeManager::Theme::Dark, "Dark"},
        {"Claro (Light)", ThemeManager::Theme::Light, "Light"},
        {"Cinnamon Light (Mint-Y)", ThemeManager::Theme::CinnamonLight, "CinnamonLight"},
        {"Cinnamon Dark (Mint-Y)", ThemeManager::Theme::CinnamonDark, "CinnamonDark"},
        {"Vermelho (Red)", ThemeManager::Theme::Red, "Red"},
        {"Windows 2000 (Clássico)", ThemeManager::Theme::Win2000, "Win2000"},
        {"Windows XP (Luna)", ThemeManager::Theme::WinXP, "WinXP"},
        {"Windows 7 (Aero)", ThemeManager::Theme::Win7, "Win7"}
    };

    QMap<ThemeManager::Theme, QAction*> themeActions;
    for (const auto &entry : themes) {
        QAction *act = themeMenu->addAction(entry.label);
        act->setCheckable(true);
        act->setChecked(ThemeManager::instance().currentTheme() == entry.theme);
        themeGroup->addAction(act);
        themeActions.insert(entry.theme, act);

        connect(act, &QAction::triggered, this, [this, entry]() {
            SettingsManager::instance().setTheme(entry.settingKey);
            ThemeManager::instance().setTheme(entry.theme);
            updateHeaderIcons();
        });
    }

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [themeActions](ThemeManager::Theme activeTheme) {
        if (themeActions.contains(activeTheme)) {
            themeActions.value(activeTheme)->setChecked(true);
        }
    });

    m_trayMenu->addSeparator();

    auto actionQuit = m_trayMenu->addAction("Sair");
    connect(actionQuit, &QAction::triggered, this, &MainWindow::onQuitApplication);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    m_trayIcon->show();
}

void MainWindow::setupConnections() {
    auto &sip = SipManager::instance();

    // CallWidget <-> SipManager
    connect(m_callWidget, &CallWidget::callRequested, &sip, &SipManager::makeCall);
    connect(m_callWidget, &CallWidget::hangupRequested, &sip, &SipManager::hangupCall);
    connect(m_callWidget, &CallWidget::holdRequested, &sip, &SipManager::holdCall);
    connect(m_callWidget, &CallWidget::muteRequested, &sip, &SipManager::muteCall);
    connect(m_callWidget, &CallWidget::transferRequested, &sip, &SipManager::transferCall);
    connect(m_callWidget, &CallWidget::recordToggleRequested, &sip, &SipManager::toggleRecording);
    connect(m_callWidget, &CallWidget::conferenceRequested, &sip, &SipManager::startConference);
    connect(m_callWidget, &CallWidget::dtmfRequested, &sip, &SipManager::sendDtmf);

    // SipManager -> CallWidget
    connect(&sip, &SipManager::callStateChanged, m_callWidget, &CallWidget::onCallStateChanged);
    connect(&sip, &SipManager::callDurationChanged, m_callWidget, &CallWidget::onCallDurationChanged);
    connect(&sip, &SipManager::recordingStateChanged, m_callWidget, &CallWidget::onRecordingStateChanged);
    connect(&sip, &SipManager::holdStateChanged, m_callWidget, &CallWidget::onHoldStateChanged);
    connect(&sip, &SipManager::muteStateChanged, m_callWidget, &CallWidget::onMuteStateChanged);

    // Contacts / History / Recordings -> Call
    connect(m_contactsWidget, &ContactsWidget::callRequested, this, [this](const QString &target) {
        m_tabWidget->setCurrentWidget(m_callWidget);
        m_callWidget->setDialNumber(target);
        SipManager::instance().makeCall(target);
    });

    connect(m_historyWidget, &HistoryWidget::callNumberRequested, this, [this](const QString &number) {
        m_tabWidget->setCurrentWidget(m_callWidget);
        m_callWidget->setDialNumber(number);
        SipManager::instance().makeCall(number);
    });

    connect(m_historyWidget, &HistoryWidget::saveContactRequested, this, &MainWindow::onSaveContactFromHistory);

    auto openRecordingPlayer = [this](const QString &filePath, const QString &displayTitle = QString()) {
        if (!m_recordingPlayerDialog) {
            m_recordingPlayerDialog = new RecordingPlayerDialog(this);
        }
        m_recordingPlayerDialog->playFile(filePath, displayTitle);
        m_recordingPlayerDialog->show();
        m_recordingPlayerDialog->raise();
        m_recordingPlayerDialog->activateWindow();
    };

    connect(m_historyWidget, &HistoryWidget::playRecordingRequested, this, [openRecordingPlayer](const QString &filePath) {
        openRecordingPlayer(filePath);
    });

    connect(m_recordingsWidget, &RecordingsWidget::playRecordingRequested, this, [openRecordingPlayer](const QString &filePath, const QString &displayTitle) {
        openRecordingPlayer(filePath, displayTitle);
    });

    connect(m_recordingsWidget, &RecordingsWidget::recordingDeleted, this, [this](const QString &filePath) {
        if (m_recordingPlayerDialog) {
            m_recordingPlayerDialog->stopIfPlaying(filePath);
        }
    });

    connect(m_recordingsWidget, &RecordingsWidget::callRequested, this, [this](const QString &number) {
        m_tabWidget->setCurrentWidget(m_callWidget);
        m_callWidget->setDialNumber(number);
        SipManager::instance().makeCall(number);
    });

    // Incoming Call & Call State
    connect(&sip, &SipManager::incomingCall, m_callWidget, &CallWidget::onIncomingCall);
    connect(&sip, &SipManager::incomingCall, this, &MainWindow::onIncomingCall);
    connect(&sip, &SipManager::callStateChanged, this, &MainWindow::onCallStateChanged);
    connect(&sip, &SipManager::presenceChanged, this, &MainWindow::onSipPresenceChanged);
    connect(&sip, &SipManager::registrationStateChanged, this, &MainWindow::onRegistrationStateChanged);
    connect(&sip, &SipManager::blfStatusChanged, m_contactsWidget, &ContactsWidget::onBlfStatusChanged);

    // Notification Popup Actions
    connect(m_notificationPopup, &CallNotificationPopup::answerClicked, this, [this](const QString &callId) {
        SoundManager::instance().stopRinging();
        SipManager::instance().answerCall(callId);
    });
    connect(m_notificationPopup, &CallNotificationPopup::rejectClicked, this, [this](const QString &callId) {
        SoundManager::instance().stopRinging();
        SipManager::instance().hangupCall(callId);
    });

    // Theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        updateHeaderIcons();
        if (m_activeCallDialog) {
            m_activeCallDialog->updateThemeVisuals();
        }
    });

    // ActiveCallDialog <-> SipManager
    connect(m_activeCallDialog, &ActiveCallDialog::hangupClicked, this, [this](const QString &callId) {
        SoundManager::instance().stopRinging();
        SipManager::instance().hangupCall(callId);
    });
    connect(m_activeCallDialog, &ActiveCallDialog::transferClicked, &sip, &SipManager::transferCall);
    connect(m_activeCallDialog, &ActiveCallDialog::muteToggled, &sip, &SipManager::muteCall);
    connect(m_activeCallDialog, &ActiveCallDialog::holdToggled, &sip, &SipManager::holdCall);
    connect(m_activeCallDialog, &ActiveCallDialog::recordToggled, &sip, &SipManager::toggleRecording);
    connect(m_activeCallDialog, &ActiveCallDialog::conferenceClicked, &sip, &SipManager::startConference);
    connect(m_activeCallDialog, &ActiveCallDialog::dtmfClicked, &sip, &SipManager::sendDtmf);

    connect(&sip, &SipManager::callStateChanged, m_activeCallDialog, &ActiveCallDialog::onCallStateChanged);
    connect(&sip, &SipManager::callDurationChanged, m_activeCallDialog, &ActiveCallDialog::onCallDurationChanged);
    connect(&sip, &SipManager::muteStateChanged, m_activeCallDialog, &ActiveCallDialog::onMuteStateChanged);
    connect(&sip, &SipManager::holdStateChanged, m_activeCallDialog, &ActiveCallDialog::onHoldStateChanged);
    connect(&sip, &SipManager::recordingStateChanged, m_activeCallDialog, &ActiveCallDialog::onRecordingStateChanged);
}

void MainWindow::onSaveContactFromHistory(const QString &name, const QString &number) {
    ContactInfo info;
    info.name = name.isEmpty() ? "Novo Contato" : name;
    info.extension = number;
    info.company = "Geral";

    ContactEditDialog dlg(info, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_contactsWidget->addContact(dlg.getContactInfo());
        m_tabWidget->setCurrentWidget(m_contactsWidget);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_reallyQuit) {
        SoundManager::instance().stopRinging();
        SipManager::instance().shutdown();
        event->accept();
        qApp->quit();
        return;
    }

    bool minimizeToTray = SettingsManager::instance().minimizeToTrayOnClose();
    if (minimizeToTray) {
        event->ignore();
        hide();
        if (m_trayIcon && m_trayIcon->isVisible()) {
            m_trayIcon->showMessage("VxVOICE em segundo plano",
                                    "O aplicativo continua em execução na bandeja do sistema.",
                                    QSystemTrayIcon::Information, 1500);
        }
    } else {
        SoundManager::instance().stopRinging();
        SipManager::instance().shutdown();
        event->accept();
        qApp->quit();
    }
}

void MainWindow::onPresenceIndexChanged(int index) {
    switch (index) {
    case 0:
        SipManager::instance().setPresence(SipManager::UserPresence::Available);
        break;
    case 1:
        SipManager::instance().setPresence(SipManager::UserPresence::DoNotDisturb);
        break;
    case 2:
        SipManager::instance().setPresence(SipManager::UserPresence::Offline);
        break;
    }
}

void MainWindow::onSipPresenceChanged(SipManager::UserPresence presence) {
    int idx = static_cast<int>(presence);
    if (m_comboPresence->currentIndex() != idx) {
        m_comboPresence->blockSignals(true);
        m_comboPresence->setCurrentIndex(idx);
        m_comboPresence->blockSignals(false);
    }
}

void MainWindow::onRegistrationStateChanged(SipManager::RegistrationState state, const QString &message) {
    updateVisorAccount();
    statusBar()->showMessage(message, 4000);

    // Se iniciou minimizado e registrou com sucesso, notifica uma vez com o ramal ativo
    if (state == SipManager::RegistrationState::Registered && !m_startupTrayNotified && !isVisible()) {
        m_startupTrayNotified = true;
        auto acc = SettingsManager::instance().defaultAccount();
        QString ramal = !acc.username.isEmpty() ? acc.username : "";
        QString msg = !ramal.isEmpty()
            ? QString("O VxVOICE está em execução em segundo plano com o ramal %1 disponível.").arg(ramal)
            : "O VxVOICE está em execução em segundo plano com o serviço disponível.";
        if (m_trayIcon && m_trayIcon->isVisible()) {
            m_trayIcon->showMessage("VxVOICE Softphone", msg, QSystemTrayIcon::Information, 5000);
        }
    }
}

void MainWindow::onIncomingCall(const QString &callId, const QString &number, const QString &name) {
    SoundManager::instance().startRinging();

    QString contactName = ContactsManager::instance().findContactNameByNumber(number);
    QString effectiveName = !contactName.isEmpty() ? contactName : name;

    m_callWidget->onIncomingCall(callId, number, effectiveName);
    m_notificationPopup->showIncomingCall(callId, number, effectiveName);
    if (!callId.isEmpty() && callId != m_recordedHistoryCallId) {
        m_recordedHistoryCallId = callId;
        m_historyWidget->addRecord(HistoryCallType::Incoming, number, effectiveName, "00:00");
    }

    m_activeCallDialog->setupCall(callId, number, effectiveName);
}

void MainWindow::onCallStateChanged(const QString &callId, SipManager::CallState state, const QString &stateText) {
    if (state == SipManager::CallState::Calling) {
        auto active = SipManager::instance().activeCall();
        QString contactName = ContactsManager::instance().findContactNameByNumber(active.remoteNumber);
        QString effectiveName = !contactName.isEmpty() ? contactName : active.remoteName;

        if (!callId.isEmpty() && callId != m_recordedHistoryCallId) {
            m_recordedHistoryCallId = callId;
            m_historyWidget->addRecord(HistoryCallType::Outgoing, active.remoteNumber, effectiveName, "00:00");
        }
        m_activeCallDialog->setupCall(callId, active.remoteNumber, effectiveName);

        QPoint winPos = pos();
        int targetX = winPos.x() + width() + 10;
        int targetY = winPos.y();

        QScreen *scr = QGuiApplication::primaryScreen();
        if (scr && targetX + m_activeCallDialog->width() > scr->geometry().right()) {
            targetX = qMax(scr->geometry().left(), winPos.x() - m_activeCallDialog->width() - 10);
        }

        m_activeCallDialog->move(targetX, targetY);
        m_activeCallDialog->show();
        m_activeCallDialog->raise();
        m_activeCallDialog->activateWindow();

    } else if (state == SipManager::CallState::Confirmed) {
        SoundManager::instance().stopRinging();
        m_notificationPopup->dismiss();

        if (!m_activeCallDialog->isVisible()) {
            auto active = SipManager::instance().activeCall();
            QString contactName = ContactsManager::instance().findContactNameByNumber(active.remoteNumber);
            QString effectiveName = !contactName.isEmpty() ? contactName : active.remoteName;
            m_activeCallDialog->setupCall(callId, active.remoteNumber, effectiveName);

            QPoint winPos = pos();
            int targetX = winPos.x() + width() + 10;
            int targetY = winPos.y();
            m_activeCallDialog->move(targetX, targetY);

            m_activeCallDialog->show();
            m_activeCallDialog->raise();
        }
    } else if (state == SipManager::CallState::Disconnected) {
        SoundManager::instance().stopRinging();
        m_notificationPopup->dismiss();
        if (m_activeCallDialog->isVisible()) {
            m_activeCallDialog->close();
        }

        // Atualiza a linha do histórico com duração real e caminho do áudio gravado
        auto active = SipManager::instance().activeCall();
        int secs = active.durationSeconds;
        QString durationStr = QString("%1:%2")
            .arg(secs / 60, 2, 10, QChar('0'))
            .arg(secs % 60, 2, 10, QChar('0'));
        m_historyWidget->updateLatestRecord(durationStr, active.recordingPath);
        m_recordingsWidget->refreshList();
        m_recordedHistoryCallId.clear();
    }
}

void MainWindow::onTabChanged(int index) {
    // Mantém a proporção única da tela principal tendo a aba Ligação como referência
    switch (index) {
    case 1: // Histórico
        m_historyWidget->adjustTableColumns();
        break;
    case 2: // Contatos
        m_contactsWidget->adjustTableColumns();
        break;
    case 3: // Gravação
        m_recordingsWidget->adjustTableColumns();
        break;
    default:
        break;
    }
}

void MainWindow::onOpenAccountsDialog() {
    AccountsDialog dlg(this);
    dlg.exec();
    updateVisorAccount();
}

void MainWindow::onOpenSettingsDialog() {
    SettingsDialog dlg(this);
    dlg.exec();
    updateRecordingsTabVisibility();
    m_recordingsWidget->refreshList();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        if (isVisible()) {
            hide();
        } else {
            showNormal();
            activateWindow();
            raise();
        }
    }
}

void MainWindow::onQuitApplication() {
    m_reallyQuit = true;
    SoundManager::instance().stopRinging();
    SipManager::instance().shutdown();
    qApp->quit();
}

void MainWindow::showChangelogDialog() {
    ChangelogDialog dlg(this);
    dlg.exec();
}
