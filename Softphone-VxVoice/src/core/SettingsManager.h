#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QStringList>

struct SipProfileConfig {
    QString id;
    QString name;
    QString stunServer{"stun.l.google.com:19302"};
    QString turnServer;
    QString turnUser;
    QString turnPassword;
    bool forceTls{false};
    bool forceSrtp{false};
};

struct SipAccountConfig {
    QString id;
    QString accountName;
    QString server;
    int port{5060};
    QString username;
    QString password;
    QString domain;
    QString transport{"UDP"}; // UDP, TCP, TLS
    QString profileId;       // Link para SipProfileConfig
    bool isDefault{false};
    bool isEnabled{true};
};

struct CodecConfig {
    QString name;
    bool enabled;
};

class SettingsManager : public QObject {
    Q_OBJECT

public:
    static constexpr const char* APP_VERSION = "1.4.9";

    static SettingsManager& instance();

    // General Preferences
    bool startWithSystem() const;
    void setStartWithSystem(bool enable);

    bool startMinimized() const;
    void setStartMinimized(bool enable);

    bool minimizeToTrayOnClose() const;
    void setMinimizeToTrayOnClose(bool enable);

    QString ringtone() const;
    void setRingtone(const QString &ringtone);

    QString theme() const;
    void setTheme(const QString &theme);

    QString lastRunVersion() const;
    void setLastRunVersion(const QString &version);

    // Audio & Voice Preferences
    QString inputDevice() const;
    void setInputDevice(const QString &device);

    QString outputDevice() const;
    void setOutputDevice(const QString &device);

    int inputVolume() const;
    void setInputVolume(int volume);

    int outputVolume() const;
    void setOutputVolume(int volume);

    bool echoCancellation() const;
    void setEchoCancellation(bool enable);

    bool autoGainControl() const;
    void setAutoGainControl(bool enable);

    bool voiceActivityDetection() const;
    void setVoiceActivityDetection(bool enable);

    // Codecs
    QList<CodecConfig> codecs() const;
    void setCodecs(const QList<CodecConfig> &codecs);

    // SIP Profiles
    QList<SipProfileConfig> profiles() const;
    void saveProfiles(const QList<SipProfileConfig> &profiles);
    SipProfileConfig findProfileById(const QString &id) const;

    // Recording Preferences
    QString recordingPath() const;
    void setRecordingPath(const QString &path);

    QString recordingFormat() const;
    void setRecordingFormat(const QString &format); // WAV or MP3

    bool autoRecord() const;
    void setAutoRecord(bool enable);

    // Accounts
    QList<SipAccountConfig> accounts() const;
    void saveAccounts(const QList<SipAccountConfig> &accounts);
    SipAccountConfig defaultAccount() const;

    // Platform autostart integration
    bool applySystemAutostart(bool enable);

signals:
    void settingsChanged();

private:
    explicit SettingsManager(QObject *parent = nullptr);
    mutable QSettings m_settings;
};

#endif // SETTINGSMANAGER_H
