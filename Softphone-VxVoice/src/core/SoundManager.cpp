#include "SoundManager.h"
#include "SettingsManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QTimer>
#include <QDebug>

SoundManager& SoundManager::instance() {
    static SoundManager s_instance;
    return s_instance;
}

SoundManager::SoundManager(QObject *parent)
    : QObject(parent),
      m_playerProcess(new QProcess(this)) {

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    m_playerProcess->setProcessEnvironment(env);

    // Quando o player terminar e a ligação ainda estiver chamando, repete o toque em loop
    connect(m_playerProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(exitCode);
        Q_UNUSED(status);

        if (m_isRinging) {
            QTimer::singleShot(400, this, [this]() {
                if (m_isRinging) {
                    QString chosen = SettingsManager::instance().ringtone();
                    QString path = ringtoneFilePath(chosen);
                    playAudioFile(path, false);
                }
            });
        }
    });
}

SoundManager::~SoundManager() {
    stopAudioProcess();
}

QStringList SoundManager::availableRingtones() const {
    return {
        "Toque 1",
        "Toque 2",
        "Toque 3",
        "Toque 4"
    };
}

QString SoundManager::ringtoneFilePath(const QString &ringtoneId) const {
    QString soundDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sounds";
    QDir dir(soundDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString wavFilename = "ringtone_1.wav";
    if (ringtoneId.contains("2")) {
        wavFilename = "ringtone_2.wav";
    } else if (ringtoneId.contains("3")) {
        wavFilename = "ringtone_3.wav";
    } else if (ringtoneId.contains("4")) {
        wavFilename = "ringtone_4.wav";
    } else {
        wavFilename = "ringtone_1.wav";
    }

    QString destPath = soundDir + "/" + wavFilename;

    // Se o arquivo não existe ou tem tamanho diferente do recurso compilado, extrai
    QFile resFile(QString(":/sounds/%1").arg(wavFilename));
    if (resFile.open(QIODevice::ReadOnly)) {
        qint64 resSize = resFile.size();
        QFileInfo destInfo(destPath);
        if (!destInfo.exists() || destInfo.size() != resSize) {
            QFile out(destPath);
            if (out.open(QIODevice::WriteOnly)) {
                out.write(resFile.readAll());
                out.close();
            }
        }
        resFile.close();
    }

    return destPath;
}

void SoundManager::startRinging(const QString &ringtoneId) {
    if (m_isRinging) return;
    m_isRinging = true;

    QString chosen = ringtoneId.isEmpty() ? SettingsManager::instance().ringtone() : ringtoneId;
    QString path = ringtoneFilePath(chosen);

    playAudioFile(path, false);
}

void SoundManager::stopRinging() {
    m_isRinging = false;
    stopAudioProcess();
}

void SoundManager::playPreview(const QString &ringtoneId) {
    m_isRinging = false;
    stopAudioProcess();

    QString path = ringtoneFilePath(ringtoneId);
    playAudioFile(path, false);
}

void SoundManager::stopPreview() {
    m_isRinging = false;
    stopAudioProcess();
}

void SoundManager::playAudioFile(const QString &filePath, bool loop) {
    Q_UNUSED(loop);
    stopAudioProcess();

    if (!QFile::exists(filePath)) {
        qWarning() << "[SoundManager] Arquivo de toque não encontrado em:" << filePath;
        return;
    }

    QString program;
    QStringList args;

    // Prioridade para os motores reais de som no Linux: PipeWire e PulseAudio nativos
    // Evita canberra-gtk-play que falha quando 'Sound disabled' no desktop
    if (QFile::exists("/usr/bin/pw-play")) {
        program = "/usr/bin/pw-play";
        args << filePath;
    } else if (QFile::exists("/usr/bin/paplay")) {
        program = "/usr/bin/paplay";
        args << filePath;
    } else if (QFile::exists("/usr/bin/aplay")) {
        program = "/usr/bin/aplay";
        args << "-q" << filePath;
    } else if (QFile::exists("/usr/bin/gst-play-1.0")) {
        program = "/usr/bin/gst-play-1.0";
        args << "--no-interactive" << "--audiosink=autoaudiosink" << filePath;
    } else {
        qWarning() << "[SoundManager] Nenhum player de áudio encontrado no sistema.";
        return;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    m_playerProcess->setProcessEnvironment(env);
    m_playerProcess->setStandardInputFile(QProcess::nullDevice());

    m_playerProcess->start(program, args);
}

void SoundManager::stopAudioProcess() {
    if (m_playerProcess && m_playerProcess->state() != QProcess::NotRunning) {
        m_playerProcess->terminate();
        if (!m_playerProcess->waitForFinished(200)) {
            m_playerProcess->kill();
        }
    }
}
