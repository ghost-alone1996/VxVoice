#ifndef SIPMANAGER_H
#define SIPMANAGER_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QTimer>
#include <QMap>
#include "SettingsManager.h"

class SipManager : public QObject {
    Q_OBJECT

public:
    enum class CallState {
        Idle,
        Calling,
        Incoming,
        Connecting,
        Confirmed, // In Call
        OnHold,
        Disconnected
    };
    Q_ENUM(CallState)

    enum class UserPresence {
        Available,
        DoNotDisturb,
        Offline
    };
    Q_ENUM(UserPresence)

    enum class RegistrationState {
        Unregistered,
        Registering,
        Registered,
        Failed
    };
    Q_ENUM(RegistrationState)

    enum class BlfState {
        Idle,        // Verde: Ramal livre / disponível
        Busy,        // Vermelho: Ramal ocupado / em chamada
        Ringing,     // Laranja: Ramal tocando
        Offline      // Cinza: Não registrado / offline
    };
    Q_ENUM(BlfState)

    struct CallData {
        QString callId;
        QString remoteNumber;
        QString remoteName;
        CallState state{CallState::Idle};
        int durationSeconds{0};
        bool isHold{false};
        bool isMuted{false};
        bool isRecording{false};
        bool isConference{false};
    };

    static SipManager& instance();

    bool initialize();
    void shutdown();

    // Telephony Actions
    void makeCall(const QString &destination);
    void answerCall(const QString &callId);
    void hangupCall(const QString &callId);
    void holdCall(const QString &callId, bool hold);
    void muteCall(const QString &callId, bool mute);
    void transferCall(const QString &callId, const QString &destination);
    void toggleRecording(const QString &callId);
    void startConference();
    void sendDtmf(const QString &digits);

    // Presence & Accounts
    void setPresence(UserPresence presence);
    UserPresence currentPresence() const;
    void registerAccount(const SipAccountConfig &account);
    void unregisterAccount();
    RegistrationState registrationState() const;
    QString registrationStateString() const;
    QColor registrationStateColor() const;
    QString currentAccountName() const;
    QString currentAccountDisplayName() const;

    // BLF (Busy Lamp Field - RFC 4235 / RFC 3265)
    void subscribeBlf(const QString &extension);
    void unsubscribeBlf(const QString &extension);
    BlfState blfStatus(const QString &extension) const;

    // Advanced Audio & Media Config
    void setAudioProcessing(bool aec, bool agc, bool vad);
    void setAudioDevices(const QString &inputDev, const QString &outputDev);
    void setAudioVolume(int inputVol, int outputVol);

    // Profile & Security Config (STUN / TURN / TLS / SRTP)
    void applyProfile(const SipProfileConfig &profile);

    // Active Call helpers
    bool hasActiveCall() const;
    CallData activeCall() const;

    // Simulation helpers (for rapid interface testing without physical PBX)
    void triggerSimulatedIncomingCall(const QString &number, const QString &name);

signals:
    void incomingCall(const QString &callId, const QString &callerId, const QString &callerName);
    void callStateChanged(const QString &callId, SipManager::CallState state, const QString &stateText);
    void callDurationChanged(const QString &callId, int seconds);
    void registrationStateChanged(SipManager::RegistrationState state, const QString &message);
    void presenceChanged(SipManager::UserPresence presence);
    void recordingStateChanged(const QString &callId, bool isRecording);
    void holdStateChanged(const QString &callId, bool isHold);
    void muteStateChanged(const QString &callId, bool isMuted);
    void dtmfSent(const QString &digits);
    void blfStatusChanged(const QString &extension, SipManager::BlfState state);

private slots:
    void onCallTimerTick();

private:
    explicit SipManager(QObject *parent = nullptr);
    ~SipManager();

    UserPresence m_presence{UserPresence::Available};
    RegistrationState m_regState{RegistrationState::Unregistered};
    QMap<QString, CallData> m_calls;
    QString m_activeCallId;

    QMap<QString, BlfState> m_blfStates;
    QTimer *m_callDurationTimer{nullptr};
};

#endif // SIPMANAGER_H
