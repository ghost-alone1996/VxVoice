#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QTimer>
#include "CallWidget.h"
#include "HistoryWidget.h"
#include "ContactsWidget.h"
#include "RecordingsWidget.h"
#include "CallNotificationPopup.h"
#include "ActiveCallDialog.h"
#include "SipManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPresenceIndexChanged(int index);
    void onSipPresenceChanged(SipManager::UserPresence presence);
    void onRegistrationStateChanged(SipManager::RegistrationState state, const QString &message);
    void onIncomingCall(const QString &callId, const QString &number, const QString &name);
    void onCallStateChanged(const QString &callId, SipManager::CallState state, const QString &stateText);

    void onOpenAccountsDialog();
    void onOpenSettingsDialog();
    void onSimulateIncomingCall();
    void onSaveContactFromHistory(const QString &name, const QString &number);

    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onQuitApplication();

private:
    void setupUi();
    void setupTrayIcon();
    void setupConnections();
    void updateVisorAccount();
    void updateHeaderIcons();

    // UI Widgets Header
    QFrame *m_headerFrame{nullptr};
    QComboBox *m_comboPresence{nullptr}; // Posicionado no topo
    QPushButton *m_btnAccounts{nullptr};  // Ícone de bonequinho / usuário
    QPushButton *m_btnSettings{nullptr};  // Ícone de engrenagem
    QPushButton *m_btnTestIncoming{nullptr}; // Ícone de sino
    QPushButton *m_btnExit{nullptr};      // Botão vermelho de Sair

    // Tabs
    QTabWidget *m_tabWidget{nullptr};
    CallWidget *m_callWidget{nullptr};
    HistoryWidget *m_historyWidget{nullptr};
    ContactsWidget *m_contactsWidget{nullptr};
    RecordingsWidget *m_recordingsWidget{nullptr};

    // System Tray
    QSystemTrayIcon *m_trayIcon{nullptr};
    QMenu *m_trayMenu{nullptr};

    // Popups and Windows
    CallNotificationPopup *m_notificationPopup{nullptr};
    ActiveCallDialog *m_activeCallDialog{nullptr};

    bool m_reallyQuit{false};
};

#endif // MAINWINDOW_H
