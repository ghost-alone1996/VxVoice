#include "SettingsManager.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

SettingsManager& SettingsManager::instance() {
    static SettingsManager s_instance;
    return s_instance;
}

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent),
      m_settings("VxVoiceCorp", "VxVoice") {
}

bool SettingsManager::startWithSystem() const {
    return m_settings.value("General/StartWithSystem", false).toBool();
}

void SettingsManager::setStartWithSystem(bool enable) {
    m_settings.setValue("General/StartWithSystem", enable);
    applySystemAutostart(enable);
    emit settingsChanged();
}

bool SettingsManager::startMinimized() const {
    return m_settings.value("General/StartMinimized", false).toBool();
}

void SettingsManager::setStartMinimized(bool enable) {
    m_settings.setValue("General/StartMinimized", enable);
    if (startWithSystem()) {
        applySystemAutostart(true);
    }
    emit settingsChanged();
}

bool SettingsManager::minimizeToTrayOnClose() const {
    return m_settings.value("General/MinimizeToTrayOnClose", true).toBool();
}

void SettingsManager::setMinimizeToTrayOnClose(bool enable) {
    m_settings.setValue("General/MinimizeToTrayOnClose", enable);
    emit settingsChanged();
}

int SettingsManager::registrationStatus() const {
    return m_settings.value("General/RegistrationStatus", 0).toInt();
}

void SettingsManager::setRegistrationStatus(int status) {
    m_settings.setValue("General/RegistrationStatus", status);
    emit settingsChanged();
}

QString SettingsManager::ringtone() const {
    return m_settings.value("General/Ringtone", "Toque 1").toString();
}

void SettingsManager::setRingtone(const QString &ringtone) {
    m_settings.setValue("General/Ringtone", ringtone);
    emit settingsChanged();
}

QString SettingsManager::theme() const {
    return m_settings.value("General/Theme", "Purple").toString();
}

void SettingsManager::setTheme(const QString &theme) {
    m_settings.setValue("General/Theme", theme);
    emit settingsChanged();
}

QString SettingsManager::lastRunVersion() const {
    return m_settings.value("General/LastRunVersion", "").toString();
}

void SettingsManager::setLastRunVersion(const QString &version) {
    m_settings.setValue("General/LastRunVersion", version);
}

QString SettingsManager::inputDevice() const {
    return m_settings.value("Audio/InputDevice", "Dispositivo Padrão").toString();
}

void SettingsManager::setInputDevice(const QString &device) {
    m_settings.setValue("Audio/InputDevice", device);
    emit settingsChanged();
}

QString SettingsManager::outputDevice() const {
    return m_settings.value("Audio/OutputDevice", "Dispositivo Padrão").toString();
}

void SettingsManager::setOutputDevice(const QString &device) {
    m_settings.setValue("Audio/OutputDevice", device);
    emit settingsChanged();
}

int SettingsManager::inputVolume() const {
    return m_settings.value("Audio/InputVolume", 80).toInt();
}

void SettingsManager::setInputVolume(int volume) {
    m_settings.setValue("Audio/InputVolume", volume);
    emit settingsChanged();
}

int SettingsManager::outputVolume() const {
    return m_settings.value("Audio/OutputVolume", 85).toInt();
}

void SettingsManager::setOutputVolume(int volume) {
    m_settings.setValue("Audio/OutputVolume", volume);
    emit settingsChanged();
}

bool SettingsManager::echoCancellation() const {
    return m_settings.value("Audio/EchoCancellation", true).toBool();
}

void SettingsManager::setEchoCancellation(bool enable) {
    m_settings.setValue("Audio/EchoCancellation", enable);
    emit settingsChanged();
}

bool SettingsManager::autoGainControl() const {
    return m_settings.value("Audio/AutoGainControl", true).toBool();
}

void SettingsManager::setAutoGainControl(bool enable) {
    m_settings.setValue("Audio/AutoGainControl", enable);
    emit settingsChanged();
}

bool SettingsManager::voiceActivityDetection() const {
    return m_settings.value("Audio/VoiceActivityDetection", true).toBool();
}

void SettingsManager::setVoiceActivityDetection(bool enable) {
    m_settings.setValue("Audio/VoiceActivityDetection", enable);
    emit settingsChanged();
}

QList<CodecConfig> SettingsManager::codecs() const {
    QList<CodecConfig> list;
    int size = m_settings.beginReadArray("Codecs");
    if (size == 0) {
        m_settings.endArray();
        return {
            {"Opus (48kHz)", true},
            {"G.722 (16kHz)", true},
            {"PCMA / G.711a (8kHz)", true},
            {"PCMU / G.711u (8kHz)", true},
            {"G.729 (8kHz)", false}
        };
    }
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        CodecConfig cfg;
        cfg.name = m_settings.value("name").toString();
        cfg.enabled = m_settings.value("enabled").toBool();
        list.append(cfg);
    }
    m_settings.endArray();
    return list;
}

void SettingsManager::setCodecs(const QList<CodecConfig> &codecs) {
    m_settings.beginWriteArray("Codecs");
    for (int i = 0; i < codecs.size(); ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("name", codecs[i].name);
        m_settings.setValue("enabled", codecs[i].enabled);
    }
    m_settings.endArray();
    emit settingsChanged();
}

QList<SipProfileConfig> SettingsManager::profiles() const {
    QList<SipProfileConfig> list;
    int size = m_settings.beginReadArray("Profiles");
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        SipProfileConfig prof;
        prof.id = m_settings.value("id").toString();
        prof.name = m_settings.value("name").toString();
        prof.server = m_settings.value("server").toString();
        prof.port = m_settings.value("port", 5060).toInt();
        prof.domain = m_settings.value("domain").toString();
        prof.transport = m_settings.value("transport", "UDP").toString();
        prof.stunServer = m_settings.value("stunServer", "stun.l.google.com:19302").toString();
        prof.turnServer = m_settings.value("turnServer").toString();
        prof.turnUser = m_settings.value("turnUser").toString();
        prof.turnPassword = m_settings.value("turnPassword").toString();
        prof.forceTls = m_settings.value("forceTls", false).toBool();
        prof.forceSrtp = m_settings.value("forceSrtp", false).toBool();
        list.append(prof);
    }
    m_settings.endArray();
    return list;
}

void SettingsManager::saveProfiles(const QList<SipProfileConfig> &profiles) {
    m_settings.remove("Profiles");
    m_settings.beginWriteArray("Profiles", profiles.size());
    for (int i = 0; i < profiles.size(); ++i) {
        m_settings.setArrayIndex(i);
        const auto &p = profiles[i];
        m_settings.setValue("id", p.id.isEmpty() ? QString("prof_%1").arg(i + 1) : p.id);
        m_settings.setValue("name", p.name);
        m_settings.setValue("server", p.server);
        m_settings.setValue("port", p.port > 0 ? p.port : 5060);
        m_settings.setValue("domain", p.domain);
        m_settings.setValue("transport", p.transport.isEmpty() ? "UDP" : p.transport);
        m_settings.setValue("stunServer", p.stunServer);
        m_settings.setValue("turnServer", p.turnServer);
        m_settings.setValue("turnUser", p.turnUser);
        m_settings.setValue("turnPassword", p.turnPassword);
        m_settings.setValue("forceTls", p.forceTls);
        m_settings.setValue("forceSrtp", p.forceSrtp);
    }
    m_settings.endArray();
    m_settings.sync();
    emit settingsChanged();
}

SipProfileConfig SettingsManager::findProfileById(const QString &id) const {
    if (id.isEmpty()) return SipProfileConfig();
    const auto profs = profiles();
    for (const auto &p : profs) {
        if (p.id == id) return p;
    }
    return SipProfileConfig();
}

QString SettingsManager::recordingPath() const {
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VxVoice/Recordings";
    return m_settings.value("Recording/Path", defaultPath).toString();
}

void SettingsManager::setRecordingPath(const QString &path) {
    m_settings.setValue("Recording/Path", path);
    emit settingsChanged();
}

QString SettingsManager::recordingFormat() const {
    return m_settings.value("Recording/Format", "WAV").toString();
}

void SettingsManager::setRecordingFormat(const QString &format) {
    m_settings.setValue("Recording/Format", format);
    emit settingsChanged();
}

bool SettingsManager::autoRecord() const {
    return m_settings.value("Recording/AutoRecord", false).toBool();
}

void SettingsManager::setAutoRecord(bool enable) {
    m_settings.setValue("Recording/AutoRecord", enable);
    emit settingsChanged();
}

QList<SipAccountConfig> SettingsManager::accounts() const {
    QList<SipAccountConfig> list;
    int size = m_settings.beginReadArray("Accounts");
    if (size == 0) {
        m_settings.endArray();
        m_settings.beginGroup("Accounts");
        int count = 1;
        while (m_settings.contains(QString("%1/server").arg(count)) ||
               m_settings.contains(QString("%1/accountName").arg(count))) {
            count++;
        }
        m_settings.endGroup();
        if (count > 1) {
            size = count - 1;
            m_settings.beginReadArray("Accounts");
        } else {
            return list;
        }
    }

    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        SipAccountConfig acc;
        acc.id = m_settings.value("id").toString();
        acc.accountName = m_settings.value("accountName").toString();
        acc.server = m_settings.value("server").toString();
        acc.port = m_settings.value("port", 5060).toInt();
        acc.username = m_settings.value("username").toString();
        acc.password = m_settings.value("password").toString();
        acc.domain = m_settings.value("domain").toString();
        acc.transport = m_settings.value("transport", "UDP").toString();
        acc.profileId = m_settings.value("profileId", "").toString();
        acc.isDefault = m_settings.value("isDefault", false).toBool();
        acc.isEnabled = m_settings.value("isEnabled", true).toBool();

        // Ignora contas fictícias de teste geradas durante desenvolvimento
        if (acc.id == "acc_primary" && acc.username == "1001" && acc.server == "pbx.empresa.local") {
            continue;
        }

        list.append(acc);
    }
    m_settings.endArray();
    return list;
}

void SettingsManager::saveAccounts(const QList<SipAccountConfig> &accounts) {
    m_settings.remove("Accounts");
    m_settings.beginWriteArray("Accounts", accounts.size());
    for (int i = 0; i < accounts.size(); ++i) {
        m_settings.setArrayIndex(i);
        const auto &acc = accounts[i];
        m_settings.setValue("id", acc.id.isEmpty() ? QString::number(i) : acc.id);
        m_settings.setValue("accountName", acc.accountName);
        m_settings.setValue("server", acc.server);
        m_settings.setValue("port", acc.port);
        m_settings.setValue("username", acc.username);
        m_settings.setValue("password", acc.password);
        m_settings.setValue("domain", acc.domain);
        m_settings.setValue("transport", acc.transport);
        m_settings.setValue("profileId", acc.profileId);
        m_settings.setValue("isDefault", acc.isDefault);
        m_settings.setValue("isEnabled", acc.isEnabled);
    }
    m_settings.endArray();
    m_settings.sync();
    emit settingsChanged();
}

SipAccountConfig SettingsManager::defaultAccount() const {
    const auto accs = accounts();
    for (const auto &acc : accs) {
        if (acc.isDefault && acc.isEnabled) return acc;
    }
    if (!accs.isEmpty()) return accs.first();
    return SipAccountConfig();
}

bool SettingsManager::applySystemAutostart(bool enable) {
    QString appPath = QCoreApplication::applicationFilePath();
    if (QFile::exists("/usr/bin/VxVoice") && appPath.contains("/build")) {
        appPath = "/usr/bin/VxVoice";
    }
    QString appName = QCoreApplication::applicationName();

#ifdef Q_OS_WIN
    QSettings bootSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enable) {
        QString execCmd = startMinimized() ? QString("\"%1\" --autostart --minimized").arg(QDir::toNativeSeparators(appPath))
                                           : QString("\"%1\" --autostart --show").arg(QDir::toNativeSeparators(appPath));
        bootSettings.setValue(appName, execCmd);
    } else {
        bootSettings.remove(appName);
    }
    return true;
#elif defined(Q_OS_LINUX)
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir dir(autostartDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString desktopFilePath = autostartDir + "/vxvoice.desktop";
    if (enable) {
        QFile file(desktopFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Version=1.0\n";
            out << "Name=VxVOICE\n";
            out << "Comment=Enterprise SIP Softphone Client\n";
            if (startMinimized()) {
                out << "Exec=" << appPath << " --autostart --minimized\n";
            } else {
                out << "Exec=" << appPath << " --autostart --show\n";
            }
            out << "Terminal=false\n";
            out << "Categories=Network;Telephony;\n";
            out << "StartupNotify=false\n";
            out << "X-GNOME-Autostart-enabled=true\n";
            file.close();
            return true;
        }
        return false;
    } else {
        if (QFile::exists(desktopFilePath)) {
            return QFile::remove(desktopFilePath);
        }
        return true;
    }
#else
    return false;
#endif
}
