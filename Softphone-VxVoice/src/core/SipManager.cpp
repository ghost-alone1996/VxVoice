#include "SipManager.h"
#include <QDateTime>
#include <QDebug>

SipManager& SipManager::instance() {
    static SipManager s_instance;
    return s_instance;
}

SipManager::SipManager(QObject *parent)
    : QObject(parent),
      m_callDurationTimer(new QTimer(this)) {
    connect(m_callDurationTimer, &QTimer::timeout, this, &SipManager::onCallTimerTick);
    m_callDurationTimer->setInterval(1000);
}

SipManager::~SipManager() {
    shutdown();
}

bool SipManager::initialize() {
    qInfo() << "[SipManager] Inicializando subsistema de telefonia PJSIP...";
    // No modo real PJSIP:
    // endpoint.libCreate();
    // endpoint.libInit(epConfig);
    // endpoint.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
    // endpoint.libStart();

    auto defAcc = SettingsManager::instance().defaultAccount();
    if (!defAcc.server.isEmpty() && defAcc.isEnabled) {
        registerAccount(defAcc);
    } else {
        m_regState = RegistrationState::Unregistered;
        emit registrationStateChanged(m_regState, "Nenhuma conta SIP configurada");
    }
    return true;
}

void SipManager::shutdown() {
    if (m_callDurationTimer && m_callDurationTimer->isActive()) {
        m_callDurationTimer->stop();
    }
    m_calls.clear();
    m_activeCallId.clear();
    qInfo() << "[SipManager] Subsistema de telefonia finalizado.";
}

void SipManager::makeCall(const QString &destination) {
    if (destination.trimmed().isEmpty()) return;

    if (m_presence == UserPresence::Offline) {
        qWarning() << "[SipManager] Não é possível ligar em modo Offline.";
        emit callStateChanged("", CallState::Disconnected, "Modo Offline ativo");
        return;
    }

    QString callId = QString("call_%1").arg(QDateTime::currentMSecsSinceEpoch());
    CallData call;
    call.callId = callId;
    call.remoteNumber = destination.trimmed();
    call.remoteName = "Destino: " + call.remoteNumber;
    call.state = CallState::Calling;
    call.durationSeconds = 0;

    m_calls.insert(callId, call);
    m_activeCallId = callId;

    emit callStateChanged(callId, CallState::Calling, QString("Discando para %1...").arg(destination));
}

void SipManager::answerCall(const QString &callId) {
    QString id = callId.isEmpty() ? m_activeCallId : callId;
    if (!m_calls.contains(id)) return;

    m_calls[id].state = CallState::Confirmed;
    m_activeCallId = id;

    emit callStateChanged(id, CallState::Confirmed, "Chamada Atendida");

    if (!m_callDurationTimer->isActive()) {
        m_callDurationTimer->start();
    }
}

void SipManager::hangupCall(const QString &callId) {
    QString id = callId.isEmpty() ? m_activeCallId : callId;
    if (!m_calls.contains(id)) return;

    m_calls[id].state = CallState::Disconnected;
    emit callStateChanged(id, CallState::Disconnected, "Chamada Encerrada");

    m_calls.remove(id);
    if (m_activeCallId == id) {
        m_activeCallId.clear();
    }

    if (m_calls.isEmpty() && m_callDurationTimer->isActive()) {
        m_callDurationTimer->stop();
    }
}

void SipManager::holdCall(const QString &callId, bool hold) {
    QString id = callId.isEmpty() ? m_activeCallId : callId;
    if (!m_calls.contains(id)) return;

    m_calls[id].isHold = hold;
    m_calls[id].state = hold ? CallState::OnHold : CallState::Confirmed;

    emit holdStateChanged(id, hold);
    emit callStateChanged(id, m_calls[id].state, hold ? "Chamada em Espera (Hold)" : "Chamada Retomada");
}

void SipManager::muteCall(const QString &callId, bool mute) {
    QString id = callId.isEmpty() ? m_activeCallId : callId;
    if (!m_calls.contains(id)) return;

    m_calls[id].isMuted = mute;
    qInfo() << "[SipManager] Chamada" << id << (mute ? "Mutada" : "Desmutada");
    emit muteStateChanged(id, mute);
}

void SipManager::transferCall(const QString &callId, const QString &destination) {
    QString id = callId.isEmpty() ? m_activeCallId : callId;
    if (!m_calls.contains(id)) return;

    qInfo() << "[SipManager] Transferindo chamada" << id << "para" << destination;
    emit callStateChanged(id, CallState::Disconnected, QString("Transferido para %1").arg(destination));
    hangupCall(id);
}

void SipManager::toggleRecording(const QString &callId) {
    QString id = callId.isEmpty() ? m_activeCallId : callId;
    if (!m_calls.contains(id)) return;

    m_calls[id].isRecording = !m_calls[id].isRecording;
    emit recordingStateChanged(id, m_calls[id].isRecording);
}

void SipManager::startConference() {
    qInfo() << "[SipManager] Iniciando modo conferência com participantes ativos.";
    if (!m_activeCallId.isEmpty()) {
        m_calls[m_activeCallId].isConference = true;
        emit callStateChanged(m_activeCallId, CallState::Confirmed, "Conferência Ativa");
    }
}

void SipManager::sendDtmf(const QString &digits) {
    qInfo() << "[SipManager] Enviando DTMF via SIP INFO/RFC2833:" << digits;
    emit dtmfSent(digits);
}

void SipManager::setPresence(UserPresence presence) {
    m_presence = presence;
    emit presenceChanged(presence);

    if (presence == UserPresence::Offline) {
        unregisterAccount();
    } else {
        auto defAcc = SettingsManager::instance().defaultAccount();
        registerAccount(defAcc);
    }
}

SipManager::UserPresence SipManager::currentPresence() const {
    return m_presence;
}

void SipManager::registerAccount(const SipAccountConfig &account) {
    if (account.server.isEmpty() || !account.isEnabled) {
        m_regState = RegistrationState::Unregistered;
        emit registrationStateChanged(m_regState, "Desconectado");
        return;
    }

    m_regState = RegistrationState::Registering;
    emit registrationStateChanged(m_regState, QString("Registrando em %1...").arg(account.server));

    // Simula registro PJSIP bem-sucedido
    QTimer::singleShot(800, this, [this, account]() {
        m_regState = RegistrationState::Registered;
        emit registrationStateChanged(m_regState, QString("Registrado (%1@%2)").arg(account.username, account.server));
    });
}

void SipManager::unregisterAccount() {
    m_regState = RegistrationState::Unregistered;
    emit registrationStateChanged(m_regState, "Offline / Desregistrado");
}

SipManager::RegistrationState SipManager::registrationState() const {
    return m_regState;
}

QString SipManager::registrationStateString() const {
    switch (m_regState) {
    case RegistrationState::Registered:
        return "Online";
    case RegistrationState::Failed:
        return "Falha de usuário e senha";
    case RegistrationState::Registering:
        return "Conectando...";
    case RegistrationState::Unregistered:
    default:
        return "Não registrado";
    }
}

QColor SipManager::registrationStateColor() const {
    switch (m_regState) {
    case RegistrationState::Registered:
        return QColor("#2ECC71"); // Verde nítido
    case RegistrationState::Failed:
        return QColor("#E74C3C"); // Vermelho
    case RegistrationState::Registering:
        return QColor("#F39C12"); // Laranja
    case RegistrationState::Unregistered:
    default:
        return QColor("#95A5A6"); // Cinza
    }
}

QString SipManager::currentAccountDisplayName() const {
    auto defAcc = SettingsManager::instance().defaultAccount();
    if (!defAcc.accountName.isEmpty()) {
        return defAcc.accountName;
    }
    if (!defAcc.username.isEmpty()) {
        return defAcc.username;
    }
    return "Nenhuma conta SIP";
}

QString SipManager::currentAccountName() const {
    auto defAcc = SettingsManager::instance().defaultAccount();
    if (!defAcc.server.isEmpty()) {
        return QString("%1 (%2)").arg(defAcc.accountName.isEmpty() ? defAcc.username : defAcc.accountName, defAcc.server);
    }
    return "Nenhuma conta SIP ativa";
}

bool SipManager::hasActiveCall() const {
    return !m_activeCallId.isEmpty() && m_calls.contains(m_activeCallId);
}

SipManager::CallData SipManager::activeCall() const {
    if (hasActiveCall()) {
        return m_calls.value(m_activeCallId);
    }
    return CallData();
}

void SipManager::onCallTimerTick() {
    if (!m_activeCallId.isEmpty() && m_calls.contains(m_activeCallId)) {
        if (m_calls[m_activeCallId].state == CallState::Confirmed) {
            m_calls[m_activeCallId].durationSeconds++;
            emit callDurationChanged(m_activeCallId, m_calls[m_activeCallId].durationSeconds);
        }
    }
}

void SipManager::subscribeBlf(const QString &extension) {
    if (extension.trimmed().isEmpty()) return;
    QString ext = extension.trimmed();
    if (!m_blfStates.contains(ext)) {
        m_blfStates.insert(ext, BlfState::Idle);
    }
    qInfo() << "[SipManager] BLF SUBSCRIBE registrado para ramal:" << ext;
    emit blfStatusChanged(ext, m_blfStates.value(ext));
}

void SipManager::unsubscribeBlf(const QString &extension) {
    m_blfStates.remove(extension.trimmed());
    qInfo() << "[SipManager] BLF UNSUBSCRIBE para ramal:" << extension;
}

SipManager::BlfState SipManager::blfStatus(const QString &extension) const {
    return m_blfStates.value(extension.trimmed(), BlfState::Offline);
}

void SipManager::setAudioProcessing(bool aec, bool agc, bool vad) {
    qInfo() << "[SipManager] DSP PJSIP configurado: AEC=" << aec << "AGC=" << agc << "VAD=" << vad;
}

void SipManager::setAudioDevices(const QString &inputDev, const QString &outputDev) {
    qInfo() << "[SipManager] Dispositivos de Áudio: Entrada=" << inputDev << "Saída=" << outputDev;
}

void SipManager::setAudioVolume(int inputVol, int outputVol) {
    qInfo() << "[SipManager] Volume de Áudio: Microfone=" << inputVol << "% Alto-falante=" << outputVol << "%";
}

void SipManager::applyProfile(const SipProfileConfig &profile) {
    qInfo() << "[SipManager] Perfil SIP aplicado:" << profile.name
            << "STUN:" << profile.stunServer << "TLS:" << profile.forceTls << "SRTP:" << profile.forceSrtp;
}


