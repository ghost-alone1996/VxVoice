#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QProcess>
#include <QStringList>

class SoundManager : public QObject {
    Q_OBJECT

public:
    static SoundManager& instance();

    QStringList availableRingtones() const;
    QString ringtoneFilePath(const QString &ringtoneId) const;

    void startRinging(const QString &ringtoneId = QString());
    void stopRinging();

    void playPreview(const QString &ringtoneId);
    void stopPreview();

private:
    explicit SoundManager(QObject *parent = nullptr);
    ~SoundManager();

    void playAudioFile(const QString &filePath, bool loop);
    void stopAudioProcess();

    QProcess *m_playerProcess{nullptr};
    bool m_isRinging{false};
};

#endif // SOUNDMANAGER_H

