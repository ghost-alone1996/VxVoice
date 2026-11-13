#ifndef CALLWIDGET_H
#define CALLWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>
#include <QCompleter>
#include <QStandardItemModel>
#include "SipManager.h"

class CallWidget : public QWidget {
    Q_OBJECT

public:
    explicit CallWidget(QWidget *parent = nullptr);

    void setDialNumber(const QString &number);
    QString currentNumber() const;
    void updateAccountDisplay();
    void updateThemeVisuals();

public slots:
    void onIncomingCall(const QString &callId, const QString &number, const QString &name);
    void onCallStateChanged(const QString &callId, SipManager::CallState state, const QString &stateText);
    void onCallDurationChanged(const QString &callId, int seconds);
    void onRecordingStateChanged(const QString &callId, bool isRecording);
    void onHoldStateChanged(const QString &callId, bool isHold);
    void onMuteStateChanged(const QString &callId, bool isMuted);

signals:
    void callRequested(const QString &destination);
    void hangupRequested(const QString &callId);
    void holdRequested(const QString &callId, bool hold);
    void muteRequested(const QString &callId, bool mute);
    void transferRequested(const QString &callId, const QString &destination);
    void recordToggleRequested(const QString &callId);
    void conferenceRequested();
    void dtmfRequested(const QString &digit);

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onDigitClicked();
    void onBackspaceClicked();
    void onCallButtonClicked();
    void onHangupButtonClicked();
    void onHoldButtonClicked();
    void onMuteButtonClicked();
    void onTransferButtonClicked();
    void onRecordButtonClicked();
    void onConferenceButtonClicked();
    void onClockTick();

private:
    void setupUi();
    void updateInCallUiState(bool inCall);
    void refreshStatusLine();
    void updateCompleterModel();

    // Top Info Header (Posicionado no topo, ACIMA do campo de discagem)
    QFrame *m_topInfoFrame{nullptr};
    QLabel *m_lblAccountInfo{nullptr};
    QLabel *m_lblDateTime{nullptr};
    QLabel *m_lblCallStatus{nullptr};
    QTimer *m_clockTimer{nullptr};

    // Dial input & autocomplete search
    QLineEdit *m_numberDisplay{nullptr};
    QPushButton *m_btnBackspace{nullptr};
    QCompleter *m_completer{nullptr};
    QStandardItemModel *m_completerModel{nullptr};
    QTimer *m_backspaceLongPressTimer{nullptr};

    // In-Call adaptive controls
    QFrame *m_inCallControlsFrame{nullptr};
    QPushButton *m_btnMute{nullptr};
    QPushButton *m_btnHold{nullptr};
    QPushButton *m_btnTransfer{nullptr};
    QPushButton *m_btnRecord{nullptr};
    QPushButton *m_btnConference{nullptr};

    // Bottom action buttons
    QPushButton *m_btnCall{nullptr};
    QPushButton *m_btnHangup{nullptr};

    QString m_activeCallId;
    QString m_remoteNumber;
    QString m_remoteName;
    SipManager::CallState m_callState{SipManager::CallState::Idle};
    QString m_currentStatusText{"Pronto para discar"};
    int m_currentDurationSeconds{0};
    bool m_isCallActive{false};
    bool m_isMuted{false};
    bool m_isHold{false};
    bool m_isConference{false};
};

#endif // CALLWIDGET_H
