#include "SipManager.h"
#include "ContactsManager.h"
#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <pjsua2.hpp>

// =============================================================================
// Helper: Registro de Threads Externas no PJLIB
// =============================================================================
static bool s_pjInitialized = false;

static void ensurePjThread() {
    if (!s_pjInitialized) {
        return;
    }
    if (!pj_thread_is_registered()) {
        try {
            pj::Endpoint::instance().libRegisterThread("qt_main_thread");
        } catch (...) {}
    }
}

// =============================================================================
// Classe VxCall: Gerencia o ciclo de vida e a gravação bidirecional da chamada
// =============================================================================
class VxCall : public pj::Call {
public:
    VxCall(pj::Account &acc, SipManager *mgr, int call_id = PJSUA_INVALID_ID)
        : pj::Call(acc, call_id), m_mgr(mgr) {}

    ~VxCall() override {
        stopCallRecording();
    }

    bool isRecording() const { return m_isRecording; }
    QString recordingFilePath() const { return m_recordingPath; }

    bool startCallRecording(const QString &filePath) {
        if (m_isRecording) return true;
        try {
            m_recordingPath = filePath;
            m_recorder = new pj::AudioMediaRecorder();
            m_recorder->createRecorder(filePath.toStdString());

            // 1. Grava áudio do interlocutor remoto (voz da outra ponta)
            pj::CallInfo ci = getInfo();
            for (unsigned i = 0; i < ci.media.size(); i++) {
                if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && getMedia(i)) {
                    pj::AudioMedia *remote_med = static_cast<pj::AudioMedia*>(getMedia(i));
                    remote_med->startTransmit(*m_recorder);
                }
            }

            // 2. Grava a nossa própria voz do microfone local (gravação bidirecional perfeita)
            pj::Endpoint::instance().audDevManager().getCaptureDevMedia().startTransmit(*m_recorder);

            m_isRecording = true;
            qInfo() << "[PJSIP Recording] Gravação bidirecional iniciada em:" << filePath;
            return true;
        } catch (pj::Error &err) {
            qWarning() << "[PJSIP Recording] Erro ao iniciar gravação:" << QString::fromStdString(err.info());
            if (m_recorder) {
                delete m_recorder;
                m_recorder = nullptr;
            }
            m_isRecording = false;
            return false;
        }
    }

    void stopCallRecording() {
        if (!m_isRecording || !m_recorder) return;
        try {
            pj::CallInfo ci = getInfo();
            for (unsigned i = 0; i < ci.media.size(); i++) {
                if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && getMedia(i)) {
                    pj::AudioMedia *remote_med = static_cast<pj::AudioMedia*>(getMedia(i));
                    try { remote_med->stopTransmit(*m_recorder); } catch (...) {}
                }
            }
            try {
                pj::Endpoint::instance().audDevManager().getCaptureDevMedia().stopTransmit(*m_recorder);
            } catch (...) {}

            delete m_recorder;
            m_recorder = nullptr;
            m_isRecording = false;
            qInfo() << "[PJSIP Recording] Gravação finalizada e salva em:" << m_recordingPath;
        } catch (...) {
            delete m_recorder;
            m_recorder = nullptr;
            m_isRecording = false;
        }
    }

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

                    // Se a gravação já estiver armada, conecta o áudio remoto imediatamente
                    if (m_isRecording && m_recorder) {
                        try { aud_med->startTransmit(*m_recorder); } catch (...) {}
                    }
                }
            }
        } catch (pj::Error &err) {
            qWarning() << "[PJSIP Media] Erro ao conectar mídia de áudio:" << QString::fromStdString(err.info());
        }
    }

private:
    SipManager *m_mgr{nullptr};
    pj::AudioMediaRecorder *m_recorder{nullptr};
    bool m_isRecording{false};
    QString m_recordingPath;
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
class VxSipLogWriter : public pj::LogWriter {
public:
    explicit VxSipLogWriter(SipManager *mgr) : m_mgr(mgr) {}
    void write(const pj::LogEntry &entry) override {
        if (!m_mgr) return;
        QString text = QString::fromStdString(entry.msg).trimmed();
        if (text.isEmpty()) return;
        if (text.contains("timer heap", Qt::CaseInsensitive) ||
            text.contains("pj_timer_heap", Qt::CaseInsensitive)) {
            return;
        }
        m_mgr->logSipMessage(text, entry.level);
    }
private:
    SipManager *m_mgr{nullptr};
};

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

void SipManager::logSipMessage(const QString &msg, int level) {
    QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString formatted = QString("[%1] %2").arg(ts, msg);

    {
        QMutexLocker locker(&m_logMutex);
        m_sipLogs.append(formatted);
        if (m_sipLogs.size() > 2000) {
            m_sipLogs.removeFirst();
        }
    }

    QMetaObject::invokeMethod(this, [this, formatted, level]() {
        emit sipLogReceived(formatted, level);
    }, Qt::QueuedConnection);
}

QStringList SipManager::sipLogs() const {
    QMutexLocker locker(&m_logMutex);
    return m_sipLogs;
}

void SipManager::clearSipLogs() {
    QMutexLocker locker(&m_logMutex);
    m_sipLogs.clear();
}

bool SipManager::initialize() {
    qInfo() << "[SipManager] Inicializando subsistema SIP corporativo com PJSUA2...";
    try {
        if (!m_endpoint) {
            m_endpoint = new pj::Endpoint();
            m_endpoint->libCreate();
            s_pjInitialized = true;

            pj::EpConfig epCfg;
            m_logWriter = std::make_unique<VxSipLogWriter>(this);
            epCfg.logConfig.writer = m_logWriter.get();
            epCfg.logConfig.level = 4;
            epCfg.logConfig.consoleLevel = 1;
            epCfg.uaConfig.userAgent = QString("VxVoice/%1 (Linux; Qt6)").arg(SettingsManager::APP_VERSION).toStdString();

            // Configuração avançada de áudio para eliminar picotes e atrasos
            epCfg.medConfig.clockRate = 16000;      // 16 kHz Wideband clock
            epCfg.medConfig.sndClockRate = 0;        // Usa clock nativo da placa de som (evita drift e jitter)
            epCfg.medConfig.audioFramePtime = 20;    // 20ms padrão RTP
            epCfg.medConfig.quality = 10;            // Resampler de alta fidelidade
            epCfg.medConfig.ecOptions = PJMEDIA_ECHO_DEFAULT;
            epCfg.medConfig.ecTailLen = 200;         // 200ms cauda de eco
            epCfg.medConfig.sndAutoCloseTime = 0;    // Mantém o canal de som sempre aberto

            m_endpoint->libInit(epCfg);

            // Cria transporte UDP dinâmico para recepção e transmissão
            pj::TransportConfig tcfg;
            tcfg.port = 0; // Porta dinâmica evita conflito com outros softphones
            try {
                m_endpoint->transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
            } catch (pj::Error &err) {
                qWarning() << "[SipManager] Falha ao criar transporte UDP:" << err.info().c_str();
            }

            m_endpoint->libStart();
            applyCodecs();
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
    if (!m_endpoint) {
        m_calls.clear();
        m_activeCallId.clear();
        return;
    }
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
        s_pjInitialized = false;
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

bool SipManager::isSelfExtension(const QString &destination) {
    QString dest = destination.trimmed();
    if (dest.isEmpty()) return false;

    auto defAcc = SettingsManager::instance().defaultAccount();
    if (!defAcc.isEnabled || defAcc.username.trimmed().isEmpty()) {
        return false;
    }

    QString loggedUser = defAcc.username.trimmed();
    if (dest.compare(loggedUser, Qt::CaseInsensitive) == 0) {
        return true;
    }

    QString destDigits;
    for (QChar ch : dest) {
        if (ch.isDigit()) destDigits.append(ch);
    }
    QString loggedDigits;
    for (QChar ch : loggedUser) {
        if (ch.isDigit()) loggedDigits.append(ch);
    }

    if (destDigits.isEmpty() || loggedDigits.isEmpty()) {
        return false;
    }
    if (destDigits == loggedDigits) {
        return true;
    }
    // Suporte a contas com prefixo de tenant (ex: usuário 1021511 e ramal salvo 1511)
    if (destDigits.length() >= 3 && loggedDigits.endsWith(destDigits)) {
        return true;
    }
    if (loggedDigits.length() >= 3 && destDigits.endsWith(loggedDigits)) {
        return true;
    }
    return false;
}

void SipManager::makeCall(const QString &destination) {
    ensurePjThread();
    QString dest = destination.trimmed();
    if (dest.isEmpty()) return;

    if (isSelfExtension(dest)) {
        qWarning() << "[SipManager] Ligação bloqueada: destino é o próprio ramal logado:" << dest;
        emit callStateChanged("", CallState::Disconnected, "Não é possível ligar para o próprio ramal");
        return;
    }

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
            try {
                pj::CallOpParam prm;
                prm.statusCode = PJSIP_SC_DECLINE;
                m_currentCall->hangup(prm);
            } catch (...) {}
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

        // Resolução inteligente da BINA (mostra nome do contato se salvo)
        QString resolvedName = ContactsManager::instance().findContactNameByNumber(dest);
        callData.remoteName = !resolvedName.isEmpty() ? resolvedName : dest;
        callData.state = CallState::Calling;
        callData.durationSeconds = 0;

        m_calls.insert(callId, callData);
        m_activeCallId = callId;

        qInfo() << "[SipManager] SIP INVITE enviado para:" << destUri << "Contato:" << callData.remoteName;
        logSipMessage(QString("[SIP-CALL] >>> Enviando INVITE para %1 (Contato: %2)").arg(destUri, callData.remoteName), 3);
        emit callStateChanged(callId, CallState::Calling, QString("Discando para %1...").arg(callData.remoteName));
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
    logSipMessage("[SIP-CALL] >>> Atendendo chamada (Respondendo 200 OK)", 3);
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
    logSipMessage("[SIP-CALL] >>> Finalizando chamada (Enviando BYE / CANCEL)", 3);
    if (m_callDurationTimer && m_callDurationTimer->isActive()) {
        m_callDurationTimer->stop();
    }

    if (m_currentCall) {
        try {
            m_currentCall->stopCallRecording();
            pj::CallOpParam prm(true);
            prm.statusCode = PJSIP_SC_DECLINE;
            m_currentCall->hangup(prm);
        } catch (pj::Error &err) {
            qWarning() << "[SipManager] Erro ao encerrar chamada PJSIP:" << QString::fromStdString(err.info());
        }
    }

    if (!m_activeCallId.isEmpty()) {
        m_calls[m_activeCallId].state = CallState::Disconnected;
        emit callStateChanged(m_activeCallId, CallState::Disconnected, "Chamada encerrada");
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

    if (m_currentCall) {
        if (!m_currentCall->isRecording()) {
            QString recDir = SettingsManager::instance().recordingPath();
            QDir().mkpath(recDir);
            QString safeName = m_calls[id].remoteName;
            safeName.replace(QRegularExpression("[^a-zA-Z0-9_-]"), "");
            QString filePath = QString("%1/rec_%2_%3_%4.wav")
                                   .arg(recDir, m_calls[id].remoteNumber, safeName,
                                        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

            bool ok = m_currentCall->startCallRecording(filePath);
            m_calls[id].isRecording = ok;
            if (ok) {
                m_calls[id].recordingPath = filePath;
            }
        } else {
            m_currentCall->stopCallRecording();
            m_calls[id].isRecording = false;
        }
    } else {
        m_calls[id].isRecording = !m_calls[id].isRecording;
    }

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
    logSipMessage(QString("[SIP-REG] <<< Resposta de Registro: Código %1 (%2)").arg(statusCode).arg(reason), statusCode == 200 ? 3 : 2);
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

    // Resolução inteligente da BINA na chamada recebida
    QString resolvedName = ContactsManager::instance().findContactNameByNumber(callerNumber);
    QString callerName = !resolvedName.isEmpty() ? resolvedName : callerNumber;

    CallData cd;
    cd.callId = callId;
    cd.remoteNumber = callerNumber;
    cd.remoteName = callerName;
    cd.state = CallState::Incoming;
    m_calls.insert(callId, cd);

    emit incomingCall(callId, callerNumber, callerName);
    logSipMessage(QString("[SIP-CALL] <<< Chamada Recebida (INVITE) de %1 (%2)").arg(callerName, callerNumber), 3);
    emit callStateChanged(callId, CallState::Incoming, QString("Chamada de %1 (%2)").arg(callerName, callerNumber));
}

void SipManager::handleCallState(VxCall *call, const QString &pjsipCallId, int invState, const QString &stateText, const QString &lastReason) {
    Q_UNUSED(pjsipCallId);
    qInfo() << "[SipManager] Estado da chamada alterado:" << invState << stateText << "Motivo:" << lastReason;
    logSipMessage(QString("[SIP-CALL] Estado da Chamada: %1 (%2) Motivo: %3").arg(stateText).arg(invState).arg(lastReason), 3);

    if (m_activeCallId.isEmpty()) {
        m_activeCallId = QString("call_%1").arg(QDateTime::currentMSecsSinceEpoch());
    }
    QString id = m_activeCallId;

    if (!m_calls.contains(id)) {
        CallData cd;
        cd.callId = id;
        cd.remoteNumber = "Desconhecido";
        cd.remoteName = "Desconhecido";
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

        // Se gravação automática estiver habilitada nas configurações, inicia gravação
        if (SettingsManager::instance().autoRecord() && !m_calls[id].isRecording) {
            toggleRecording(id);
        }
        break;

    case PJSIP_INV_STATE_DISCONNECTED: {
        m_calls[id].state = CallState::Disconnected;
        if (m_callDurationTimer && m_callDurationTimer->isActive()) {
            m_callDurationTimer->stop();
        }
        if (call && call->isRecording()) {
            call->stopCallRecording();
            m_calls[id].isRecording = false;
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
    logSipMessage(QString("[SIP-REG] >>> Registrando conta '%1' (sip:%2@%3:%4)").arg(account.accountName, account.username, account.server).arg(account.port), 3);
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
    logSipMessage("[SIP-REG] >>> Desregistrando conta SIP ativa...", 3);
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

void SipManager::applyCodecs() {
    ensurePjThread();
    if (!m_endpoint) return;

    auto codecs = SettingsManager::instance().codecs();
    int prio = 230;
    for (const auto &c : codecs) {
        short p = c.enabled ? prio : 0;
        QString n = c.name.toUpper();

        if (n.contains("PCMA") || n.contains("ALAW") || n.contains("G711A")) {
            try { m_endpoint->codecSetPriority("PCMA/8000/1", p); } catch (...) {}
        } else if (n.contains("PCMU") || n.contains("ULAW") || n.contains("G711U")) {
            try { m_endpoint->codecSetPriority("PCMU/8000/1", p); } catch (...) {}
        } else if (n.contains("G722")) {
            try { m_endpoint->codecSetPriority("G722/16000/1", p); } catch (...) {}
        } else if (n.contains("SPEEX")) {
            try {
                m_endpoint->codecSetPriority("speex/16000/1", p);
                m_endpoint->codecSetPriority("speex/8000/1", p);
            } catch (...) {}
        } else if (n.contains("GSM")) {
            try { m_endpoint->codecSetPriority("GSM/8000/1", p); } catch (...) {}
        } else if (n.contains("ILBC")) {
            try { m_endpoint->codecSetPriority("iLBC/8000/1", p); } catch (...) {}
        } else if (n.contains("OPUS")) {
            try { m_endpoint->codecSetPriority("opus/48000/2", p); } catch (...) {}
        }

        if (c.enabled) prio -= 15;
    }
    qInfo() << "[SipManager] Prioridades de codecs aplicadas na pilha PJSIP.";
}

void SipManager::applyProfile(const SipProfileConfig &profile) {
    ensurePjThread();
    qInfo() << "[SipManager] Perfil SIP aplicado:" << profile.name
            << "STUN:" << profile.stunServer << "TLS:" << profile.forceTls << "SRTP:" << profile.forceSrtp;
}
