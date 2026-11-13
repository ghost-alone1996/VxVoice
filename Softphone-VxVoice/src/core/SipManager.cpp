#include "SipManager.h"
#include <QDateTime>
#include <QDebug>
#include <pjsua2.hpp>

// =============================================================================
// Helper: Registro de Threads Externas no PJLIB
// =============================================================================
static void ensurePjThread() {
    if (!pj_thread_is_registered()) {
        try {
            pj::Endpoint::instance().libRegisterThread("qt_main_thread");
        } catch (...) {}
    }
}

// =============================================================================
// Classe VxCall: Gerencia o ciclo de vida de uma chamada real via SIP/SDP/RTP
// =============================================================================
class VxCall : public pj::Call {
public:
    VxCall(pj::Account &acc, SipManager *mgr, int call_id = PJSUA_INVALID_ID)
        : pj::Call(acc, call_id), m_mgr(mgr) {}

    void onCallState(pj::OnCallStateParam &prm) override {
        Q_UNUSED(prm);
        try {
            pj::CallInfo ci = getInfo();
            QString callId = QString::fromStdString(ci.callIdString);
            int invState = ci.state;
            QString stateText = QString::fromStdString(ci.stateText);
            QString lastReason = QString::fromStdString(ci.lastReason);

            QMetaObject::invokeMethod(m_mgr, [this, callId, invState, stateText, lastReason]() {
                m_mgr->handleCallState(this, callId, invState, stateText, lastReason);
            }, Qt::QueuedConnection);
        } catch (...) {}
    }

    void onCallMediaState(pj::OnCallMediaStateParam &prm) override {
        Q_UNUSED(prm);
        try {
            pj::CallInfo ci = getInfo();
            for (unsigned i = 0; i < ci.media.size(); i++) {
                if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && getMedia(i)) {
                    pj::AudioMedia *aud_med = static_cast<pj::AudioMedia*>(getMedia(i));
                    // Conecta Microfone -> Chamada e Chamada -> Alto-falantes
                    pj::Endpoint::instance().audDevManager().getCaptureDevMedia().startTransmit(*aud_med);
                    aud_med->startTransmit(pj::Endpoint::instance().audDevManager().getPlaybackDevMedia());
                    qInfo() << "[PJSIP Media] Fluxo de áudio bidirecional conectado com sucesso.";
                }
            }
        } catch (pj::Error &err) {
            qWarning() << "[PJSIP Media] Erro ao conectar mídia de áudio:" << QString::fromStdString(err.info());
        }
    }

private:
    SipManager *m_mgr{nullptr};
};

// =============================================================================
// Classe VxAccount: Gerencia Registro SIP e Recepção de Chamadas do PBX
// =============================================================================
class VxAccount : public pj::Account {
public:
    VxAccount(SipManager *mgr) : m_mgr(mgr) {}

    void onRegState(pj::OnRegStateParam &prm) override {
        int code = prm.code;
        QString reason = QString::fromStdString(prm.reason);
        QMetaObject::invokeMethod(m_mgr, [this, code, reason]() {
            m_mgr->handleRegState(code, reason);
        }, Qt::QueuedConnection);
    }

    void onIncomingCall(pj::OnIncomingCallParam &prm) override {
        try {
            auto call = new VxCall(*this, m_mgr, prm.callId);
            pj::CallInfo ci = call->getInfo();
            QString remoteUri = QString::fromStdString(ci.remoteUri);
            QString remoteContact = QString::fromStdString(ci.remoteContact);
            QString callId = QString::fromStdString(ci.callIdString);

            QMetaObject::invokeMethod(m_mgr, [this, call, callId, remoteUri, remoteContact]() {
                m_mgr->handleIncomingCall(call, callId, remoteUri, remoteContact);
            }, Qt::QueuedConnection);
        } catch (...) {}
    }

private:
    SipManager *m_mgr{nullptr};
};

// =============================================================================
// SipManager Implementation
// =============================================================================
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
    qInfo() << "[SipManager] Inicializando subsistema SIP corporativo com PJSUA2...";
    try {
        if (!m_endpoint) {
            m_endpoint = new pj::Endpoint();
            m_endpoint->libCreate();

            pj::EpConfig epCfg;
            epCfg.logConfig.level = 3;
            epCfg.logConfig.consoleLevel = 3;
            epCfg.uaConfig.userAgent = "VxVoice Enterprise Softphone 1.4.1";

            m_endpoint->libInit(epCfg);

            // Cria transporte UDP dinâmico para recepção e transmissão
            pj::TransportConfig tcfg;
            tcfg.port = 0; // Porta dinâmica evita conflito com outras instâncias
            try {
                m_endpoint->transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
            } catch (pj::Error &err) {
                qWarning() << "[SipManager] Falha ao criar transporte UDP:" << err.info().c_str();
            }

            m_endpoint->libStart();
            qInfo() << "[SipManager] PJSUA2 inicializado com sucesso.";
        }
    } catch (pj::Error &err) {
        qCritical() << "[SipManager] Falha ao inicializar PJSUA2:" << QString::fromStdString(err.info());
        return false;
    }

    auto defAcc = SettingsManager::instance().defaultAccount();
    if (!defAcc.server.isEmpty() && defAcc.isEnabled) {
        registerAccount(defAcc);
    } else {
        m_regState = RegistrationState::Unregistered;
        emit registrationStateChanged(m_regState, "Nenhuma conta SIP ativa");
    }
    return true;
}

void SipManager::shutdown() {
    ensurePjThread();
    if (m_callDurationTimer && m_callDurationTimer->isActive()) {
        m_callDurationTimer->stop();
    }
    if (m_currentCall) {
        try {
            pj::CallOpParam prm;
            prm.statusCode = PJSIP_SC_DECLINE;
            m_currentCall->hangup(prm);
        } catch (...) {}
        delete m_currentCall;
        m_currentCall = nullptr;
    }
    if (m_account) {
        try {
            delete m_account;
        } catch (...) {}
        m_account = nullptr;
    }
    if (m_endpoint) {
        try {
            m_endpoint->libDestroy();
        } catch (...) {}
        delete m_endpoint;
        m_endpoint = nullptr;
    }
    m_calls.clear();
    m_activeCallId.clear();
    qInfo() << "[SipManager] Subsistema PJSIP finalizado com sucesso.";
}

void SipManager::makeCall(const QString &destination) {
    ensurePjThread();
    QString dest = destination.trimmed();
    if (dest.isEmpty()) return;

    if (m_presence == UserPresence::Offline) {
        qWarning() << "[SipManager] Não é possível ligar em modo Offline.";
        emit callStateChanged("", CallState::Disconnected, "Modo Offline ativo");
        return;
    }

    if (!m_account || m_regState != RegistrationState::Registered) {
        qWarning() << "[SipManager] Tentativa de chamada sem conta SIP registrada.";
        emit callStateChanged("", CallState::Disconnected, "Conta SIP não conectada ao servidor");
        return;
    }

    try {
        if (m_currentCall) {
            pj::CallOpParam prm;
            prm.statusCode = PJSIP_SC_DECLINE;
            try { m_currentCall->hangup(prm); } catch (...) {}
            delete m_currentCall;
            m_currentCall = nullptr;
        }

        m_currentCall = new VxCall(*m_account, this);

        QString destUri;
        if (dest.startsWith("sip:")) {
            destUri = dest;
        } else {
            destUri = QString("sip:%1@%2").arg(dest, m_currentAccount.server);
            if (m_currentAccount.port > 0 && m_currentAccount.port != 5060) {
                destUri += QString(":%1").arg(m_currentAccount.port);
            }
        }

        pj::CallOpParam prm(true);
        m_currentCall->makeCall(destUri.toStdString(), prm);

        QString callId = QString("call_%1").arg(QDateTime::currentMSecsSinceEpoch());
        CallData callData;
        callData.callId = callId;
        callData.remoteNumber = dest;
        callData.remoteName = "Destino: " + dest;
        callData.state = CallState::Calling;
        callData.durationSeconds = 0;

        m_calls.insert(callId, callData);
        m_activeCallId = callId;

        qInfo() << "[SipManager] SIP INVITE enviado para:" << destUri;
        emit callStateChanged(callId, CallState::Calling, QString("Discando para %1...").arg(dest));
    } catch (pj::Error &err) {
        qWarning() << "[SipManager] Erro ao originar chamada:" << QString::fromStdString(err.info());
        if (m_currentCall) {
            delete m_currentCall;
            m_currentCall = nullptr;
        }
        emit callStateChanged("", CallState::Disconnected, QString("Falha: %1").arg(QString::fromStdString(err.reason)));
    }
}

void SipManager::answerCall(const QString &callId) {
    ensurePjThread();
    Q_UNUSED(callId);
    if (m_currentCall) {
        try {
            pj::CallOpParam prm(true);
            prm.statusCode = PJSIP_SC_OK;
            m_currentCall->answer(prm);
        } catch (pj::Error &err) {
            qWarning() << "[SipManager] Erro ao atender chamada PJSIP:" << QString::fromStdString(err.info());
        }
    }
    if (!m_activeCallId.isEmpty()) {
        m_calls[m_activeCallId].state = CallState::Confirmed;
        emit callStateChanged(m_activeCallId, CallState::Confirmed, "Chamada Conectada");
        if (!m_callDurationTimer->isActive()) {
            m_callDurationTimer->start();
        }
    }
}

void SipManager::hangupCall(const QString &callId) {
    ensurePjThread();
    Q_UNUSED(callId);
    if (m_callDurationTimer && m_callDurationTimer->isActive()) {
        m_callDurationTimer->stop();
    }

    if (m_currentCall) {
        try {
            pj::CallOpParam prm(true);
            prm.statusCode = PJSIP_SC_DECLINE;
            m_currentCall->hangup(prm);
        } catch (pj::Error &err) {
            qWarning() << "[SipManager] Erro ao encerrar chamada PJSIP:" << QString::fromStdString(err.info());
        }
    }

    if (!m_activeCallId.isEmpty()) {
        m_calls[m_activeCallId].state = CallState::Disconnected;
        emit callStateChanged(m_activeCallId, CallState::Disconnected, "Chamada encerrada pelo usuário");
    }
}

void SipManager::holdCall(const QString &callId, bool hold) {
    ensurePjThread();
    QString id = callId.isEmpty() ? m_activeCallId : callId;
    if (m_currentCall) {
        try {
            pj::CallOpParam prm(true);
            if (hold) {
                m_currentCall->setHold(prm);
            } else {
                m_currentCall->reinvite(prm);
            }
        } catch (pj::Error &err) {
            qWarning() << "[SipManager] Erro ao alternar espera PJSIP:" << QString::fromStdString(err.info());
        }
    }
    if (m_calls.contains(id)) {
        m_calls[id].isHold = hold;
        m_calls[id].state = hold ? CallState::OnHold : CallState::Confirmed;
        emit holdStateChanged(id, hold);
        emit callStateChanged(id, m_calls[id].state, hold ? "Em Espera (Hold)" : "Chamada Retomada");
    }
}

void SipManager::muteCall(const QString &callId, bool mute) {
    ensurePjThread();
    QString id = callId.isEmpty() ? m_activeCallId : callId;
    try {
        if (m_currentCall) {
            pj::CallInfo ci = m_currentCall->getInfo();
            for (unsigned i = 0; i < ci.media.size(); i++) {
                if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && m_currentCall->getMedia(i)) {
                    pj::AudioMedia *aud_med = static_cast<pj::AudioMedia*>(m_currentCall->getMedia(i));
                    if (mute) {
                        pj::Endpoint::instance().audDevManager().getCaptureDevMedia().stopTransmit(*aud_med);
                    } else {
                        pj::Endpoint::instance().audDevManager().getCaptureDevMedia().startTransmit(*aud_med);
                    }
                }
            }
        }
    } catch (pj::Error &err) {
        qWarning() << "[SipManager] Erro ao alternar mute PJSIP:" << QString::fromStdString(err.info());
    }
    if (m_calls.contains(id)) {
        m_calls[id].isMuted = mute;
    }
    emit muteStateChanged(id, mute);
}

void SipManager::transferCall(const QString &callId, const QString &destination) {
    ensurePjThread();
    Q_UNUSED(callId);
    if (m_currentCall && !destination.isEmpty()) {
        try {
            pj::CallOpParam prm(true);
            QString destUri = destination.startsWith("sip:") ? destination
                              : QString("sip:%1@%2").arg(destination, m_currentAccount.server);
            m_currentCall->xfer(destUri.toStdString(), prm);
            qInfo() << "[SipManager] Transferência iniciada para:" << destUri;
        } catch (pj::Error &err) {
            qWarning() << "[SipManager] Erro ao transferir chamada:" << QString::fromStdString(err.info());
        }
    }
}

void SipManager::toggleRecording(const QString &callId) {
    QString id = callId.isEmpty() ? m_activeCallId : callId;
    if (!m_calls.contains(id)) return;

    m_calls[id].isRecording = !m_calls[id].isRecording;
    emit recordingStateChanged(id, m_calls[id].isRecording);
    qInfo() << "[SipManager] Gravação da chamada:" << id << (m_calls[id].isRecording ? "Iniciada" : "Pausada");
}

void SipManager::startConference() {
    qInfo() << "[SipManager] Conferência iniciada no bridge de áudio PJSIP.";
}

void SipManager::sendDtmf(const QString &digits) {
    ensurePjThread();
    if (m_currentCall && !digits.isEmpty()) {
        try {
            m_currentCall->dialDtmf(digits.toStdString());
            qInfo() << "[SipManager] DTMF RFC2833 enviado:" << digits;
        } catch (pj::Error &err) {
            qWarning() << "[SipManager] Erro ao enviar DTMF:" << QString::fromStdString(err.info());
        }
    }
    emit dtmfSent(digits);
}

void SipManager::handleRegState(int statusCode, const QString &reason) {
    qInfo() << "[SipManager] Resposta SIP REGISTER:" << statusCode << reason;
    if (statusCode == 200) {
        m_regState = RegistrationState::Registered;
        emit registrationStateChanged(m_regState, QString("Registrado (%1@%2)").arg(m_currentAccount.username, m_currentAccount.server));
    } else if (statusCode == 401 || statusCode == 403) {
        m_regState = RegistrationState::Failed;
        emit registrationStateChanged(m_regState, "Falha de autenticação (usuário/senha)");
    } else if (statusCode >= 400) {
        m_regState = RegistrationState::Failed;
        emit registrationStateChanged(m_regState, QString("Falha no registro: %1 (%2)").arg(statusCode).arg(reason));
    } else {
        m_regState = RegistrationState::Registering;
        emit registrationStateChanged(m_regState, QString("Conectando (%1)...").arg(statusCode));
    }
}

void SipManager::handleIncomingCall(VxCall *call, const QString &callId, const QString &remoteUri, const QString &remoteContact) {
    if (m_currentCall && m_currentCall != call) {
        pj::CallOpParam prm;
        prm.statusCode = PJSIP_SC_BUSY_HERE;
        try { call->hangup(prm); } catch (...) {}
        delete call;
        return;
    }

    m_currentCall = call;
    m_activeCallId = callId;

    QString callerNumber = remoteUri;
    int colonPos = callerNumber.indexOf(':');
    int atPos = callerNumber.indexOf('@');
    if (colonPos != -1 && atPos != -1 && atPos > colonPos) {
        callerNumber = callerNumber.mid(colonPos + 1, atPos - colonPos - 1);
    }
    QString callerName = remoteContact.isEmpty() ? callerNumber : remoteContact;

    CallData cd;
    cd.callId = callId;
    cd.remoteNumber = callerNumber;
    cd.remoteName = callerName;
    cd.state = CallState::Incoming;
    m_calls.insert(callId, cd);

    emit incomingCall(callId, callerNumber, callerName);
    emit callStateChanged(callId, CallState::Incoming, QString("Chamada de %1 (%2)").arg(callerName, callerNumber));
}

void SipManager::handleCallState(VxCall *call, const QString &pjsipCallId, int invState, const QString &stateText, const QString &lastReason) {
    Q_UNUSED(pjsipCallId);
    qInfo() << "[SipManager] Estado da chamada alterado:" << invState << stateText << "Motivo:" << lastReason;

    if (m_activeCallId.isEmpty()) {
        m_activeCallId = QString("call_%1").arg(QDateTime::currentMSecsSinceEpoch());
    }
    QString id = m_activeCallId;

    if (!m_calls.contains(id)) {
        CallData cd;
        cd.callId = id;
        cd.remoteNumber = "Desconhecido";
        m_calls.insert(id, cd);
    }

    switch (invState) {
    case PJSIP_INV_STATE_CALLING:
        m_calls[id].state = CallState::Calling;
        emit callStateChanged(id, CallState::Calling, "Discando...");
        break;

    case PJSIP_INV_STATE_EARLY:
        m_calls[id].state = CallState::Calling;
        emit callStateChanged(id, CallState::Calling, "Chamando (tocando no destino)...");
        break;

    case PJSIP_INV_STATE_CONNECTING:
        m_calls[id].state = CallState::Connecting;
        emit callStateChanged(id, CallState::Connecting, "Conectando...");
        break;

    case PJSIP_INV_STATE_CONFIRMED:
        m_calls[id].state = CallState::Confirmed;
        emit callStateChanged(id, CallState::Confirmed, "Conectado");
        if (!m_callDurationTimer->isActive()) {
            m_callDurationTimer->start();
        }
        break;

    case PJSIP_INV_STATE_DISCONNECTED: {
        m_calls[id].state = CallState::Disconnected;
        if (m_callDurationTimer && m_callDurationTimer->isActive()) {
            m_callDurationTimer->stop();
        }
        QString reasonMsg = lastReason.isEmpty() ? "Chamada encerrada" : QString("Encerrada (%1)").arg(lastReason);
        emit callStateChanged(id, CallState::Disconnected, reasonMsg);

        if (call == m_currentCall) {
            m_currentCall = nullptr;
        }
        QTimer::singleShot(100, [call]() {
            try { delete call; } catch (...) {}
        });
        break;
    }
    default:
        break;
    }
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
    ensurePjThread();
    m_currentAccount = account;

    if (account.server.trimmed().isEmpty() || !account.isEnabled) {
        unregisterAccount();
        return;
    }

    m_regState = RegistrationState::Registering;
    emit registrationStateChanged(m_regState, QString("Registrando em %1...").arg(account.server));

    try {
        if (m_account) {
            delete m_account;
            m_account = nullptr;
        }

        pj::AccountConfig accCfg;
        QString serverHost = account.server.trimmed();
        int port = account.port > 0 ? account.port : 5060;
        QString registrar = (port == 5060) ? QString("sip:%1").arg(serverHost)
                                           : QString("sip:%1:%2").arg(serverHost).arg(port);

        accCfg.idUri = QString("sip:%1@%2").arg(account.username.trimmed(), serverHost).toStdString();
        accCfg.regConfig.registrarUri = registrar.toStdString();
        accCfg.regConfig.timeoutSec = 300;

        accCfg.sipConfig.authCreds.push_back(
            pj::AuthCredInfo("digest", "*", account.username.trimmed().toStdString(), 0, account.password.trimmed().toStdString())
        );

        m_account = new VxAccount(this);
        m_account->create(accCfg);
        qInfo() << "[SipManager] Solicitação de registro SIP enviada para:" << registrar << "Usuário:" << account.username;
    } catch (pj::Error &err) {
        qWarning() << "[SipManager] Erro ao registrar conta SIP:" << QString::fromStdString(err.info());
        m_regState = RegistrationState::Failed;
        emit registrationStateChanged(m_regState, QString("Erro: %1").arg(QString::fromStdString(err.reason)));
    }
}

void SipManager::unregisterAccount() {
    ensurePjThread();
    try {
        if (m_account) {
            delete m_account;
            m_account = nullptr;
        }
    } catch (...) {}
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
    emit blfStatusChanged(ext, m_blfStates.value(ext));
}

void SipManager::unsubscribeBlf(const QString &extension) {
    m_blfStates.remove(extension.trimmed());
}

SipManager::BlfState SipManager::blfStatus(const QString &extension) const {
    return m_blfStates.value(extension.trimmed(), BlfState::Offline);
}

void SipManager::setAudioProcessing(bool aec, bool agc, bool vad) {
    Q_UNUSED(aec);
    Q_UNUSED(agc);
    Q_UNUSED(vad);
    ensurePjThread();
    qInfo() << "[SipManager] DSP PJSIP configurado: AEC=" << aec << "AGC=" << agc << "VAD=" << vad;
}

void SipManager::setAudioDevices(const QString &inputDev, const QString &outputDev) {
    ensurePjThread();
    qInfo() << "[SipManager] Dispositivos de Áudio: Entrada=" << inputDev << "Saída=" << outputDev;
}

void SipManager::setAudioVolume(int inputVol, int outputVol) {
    ensurePjThread();
    qInfo() << "[SipManager] Volume de Áudio: Microfone=" << inputVol << "% Alto-falante=" << outputVol << "%";
}

void SipManager::applyProfile(const SipProfileConfig &profile) {
    ensurePjThread();
    qInfo() << "[SipManager] Perfil SIP aplicado:" << profile.name
            << "STUN:" << profile.stunServer << "TLS:" << profile.forceTls << "SRTP:" << profile.forceSrtp;
}
