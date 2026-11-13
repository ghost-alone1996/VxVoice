#ifndef ACTIVECALLDIALOG_H
#define ACTIVECALLDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include "SipManager.h"

class ActiveCallDialog : public QDialog {
    Q_OBJECT

public:
    explicit ActiveCallDialog(QWidget *parent = nullptr);

    void setupCall(const QString &callId, const QString &number, const QString &name, const QString &company = QString());
    void updateThemeVisuals();

public slots:
    void onCallStateChanged(const QString &callId, SipManager::CallState state, const QString &stateText);
    void onCallDurationChanged(const QString &callId, int seconds);
    void onMuteStateChanged(const QString &callId, bool isMuted);
    void onHoldStateChanged(const QString &callId, bool isHold);
    void onRecordingStateChanged(const QString &callId, bool isRecording);

signals:
    void hangupClicked(const QString &callId);
    void transferClicked(const QString &callId, const QString &destination);
    void muteToggled(const QString &callId, bool mute);
    void holdToggled(const QString &callId, bool hold);
    void recordToggled(const QString &callId);
    void conferenceClicked();
    void dtmfClicked(const QString &digit);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onHangup();
    void onTransfer();
    void onMute();
    void onHold();
    void onRecord();
    void onConference();
    void onDtmfDigit();
    void onToggleDtmfPad();

private:
    void setupUi();

    QString m_callId;
    bool m_isMuted{false};
    bool m_isHold{false};
    bool m_isRecording{false};
    bool m_isConference{false};

    QFrame *m_profileCard{nullptr};
    QLabel *m_lblHeaderBadge{nullptr};
    QLabel *m_lblName{nullptr};
    QLabel *m_lblNumber{nullptr};
    QLabel *m_lblCompany{nullptr};
    QLabel *m_lblStatus{nullptr};
    QLabel *m_lblTimer{nullptr};
    QLabel *m_lblRecBadge{nullptr};

    QPushButton *m_btnMute{nullptr};
    QPushButton *m_btnHold{nullptr};
    QPushButton *m_btnTransfer{nullptr};
    QPushButton *m_btnRecord{nullptr};
    QPushButton *m_btnConference{nullptr};
    QPushButton *m_btnDtmfToggle{nullptr};
    QPushButton *m_btnHangup{nullptr};

    QFrame *m_dtmfFrame{nullptr};
};

#endif // ACTIVECALLDIALOG_H
