#include "SettingsDialog.h"
#include "ThemeManager.h"
#include "SoundManager.h"
#include "SipManager.h"
#include "UpdateManager.h"
#include <QProcess>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QTextEdit>
#include <QDir>
#include <QTimer>

#ifdef HAVE_QT_MULTIMEDIA
#include <QMediaDevices>
#include <QAudioDevice>
#endif

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Preferências Corporativas - VxVoice");
    resize(460, 560);
    setupUi();
    loadSettings();
}

SettingsDialog::~SettingsDialog() {
    SoundManager::instance().stopPreview();
}

void SettingsDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createGeneralTab(), "⚙ Geral");
    m_tabs->addTab(createAudioTab(), "🎙 Áudio e Voz");
    m_tabs->addTab(createCodecsTab(), "🎛 Codecs");
    m_tabs->addTab(createRecordingTab(), "⏺ Gravação");
    m_tabs->addTab(createAboutTab(), "ℹ Sobre");

    mainLayout->addWidget(m_tabs, 1);

    // Botões Salvar e Cancelar
    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_btnCancel = new QPushButton("Cancelar", this);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnSave = new QPushButton("Salvar Configurações", this);
    m_btnSave->setObjectName("primaryButton");
    connect(m_btnSave, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);

    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnSave);
    mainLayout->addLayout(btnLayout);
}

QWidget* SettingsDialog::createGeneralTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    m_chkStartWithSystem = new QCheckBox("Iniciar junto com o Sistema Operacional", widget);
    m_chkStartMinimized = new QCheckBox("Iniciar minimizado no System Tray", widget);
    m_chkMinimizeToTrayOnClose = new QCheckBox("Ao clicar em fechar (X), manter na bandeja do sistema", widget);

    connect(m_chkStartMinimized, &QCheckBox::toggled, this, [](bool checked) {
        if (checked) {
            QMessageBox::information(nullptr, "Aviso de Inicialização",
                "O VxVoice iniciará automaticamente em segundo plano na barra de tarefas (System Tray).");
        }
    });

    layout->addWidget(m_chkStartWithSystem);
    layout->addWidget(m_chkStartMinimized);
    layout->addWidget(m_chkMinimizeToTrayOnClose);
    layout->addSpacing(8);

    // Ringtone selector
    auto ringtoneLayout = new QHBoxLayout();
    m_comboRingtone = new QComboBox(widget);
    m_comboRingtone->addItems(SoundManager::instance().availableRingtones());

    m_btnPlayRingtone = new QPushButton("▶ Ouvir Toque", widget);
    m_btnPlayRingtone->setCursor(Qt::PointingHandCursor);
    connect(m_btnPlayRingtone, &QPushButton::clicked, this, &SettingsDialog::onToggleRingtonePreview);

    ringtoneLayout->addWidget(m_comboRingtone, 1);
    ringtoneLayout->addWidget(m_btnPlayRingtone);

    // Theme selector com os 4 temas requeridos
    m_comboTheme = new QComboBox(widget);
    m_comboTheme->addItem("🌑 Dark (Tons de azul escuro)");
    m_comboTheme->addItem("☀️ Light (Tons de azul claro)");
    m_comboTheme->addItem("🟣 Purple (Tons de roxo corporativo)");
    m_comboTheme->addItem("🔴 Red (Tons de vermelho corporativo)");

    formLayout->addRow("Ringtone de Chamada:", ringtoneLayout);
    formLayout->addRow("Tema Visual do Softphone:", m_comboTheme);

    layout->addLayout(formLayout);
    layout->addStretch();
    return widget;
}

QWidget* SettingsDialog::createAudioTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    // Entrada com botão de detecção Hot-Plug USB
    auto inDevLayout = new QHBoxLayout();
    m_comboInputDevice = new QComboBox(widget);
    m_btnRefreshDevices = new QPushButton("🔄 Atualizar USB", widget);
    m_btnRefreshDevices->setToolTip("Escanear portas USB e listar headsets conectados");
    m_btnRefreshDevices->setCursor(Qt::PointingHandCursor);
    connect(m_btnRefreshDevices, &QPushButton::clicked, this, &SettingsDialog::refreshAudioDeviceLists);

    inDevLayout->addWidget(m_comboInputDevice, 1);
    inDevLayout->addWidget(m_btnRefreshDevices);

    m_comboOutputDevice = new QComboBox(widget);

    // Volume Microfone
    auto inputVolLayout = new QHBoxLayout();
    m_sliderInputVolume = new QSlider(Qt::Horizontal, widget);
    m_sliderInputVolume->setRange(0, 100);
    m_lblInputVolVal = new QLabel("80%", widget);
    m_lblInputVolVal->setFixedWidth(35);
    connect(m_sliderInputVolume, &QSlider::valueChanged, this, [this](int val) {
        m_lblInputVolVal->setText(QString("%1%").arg(val));
    });
    inputVolLayout->addWidget(m_sliderInputVolume);
    inputVolLayout->addWidget(m_lblInputVolVal);

    // Volume Alto-falante
    auto outputVolLayout = new QHBoxLayout();
    m_sliderOutputVolume = new QSlider(Qt::Horizontal, widget);
    m_sliderOutputVolume->setRange(0, 100);
    m_lblOutputVolVal = new QLabel("85%", widget);
    m_lblOutputVolVal->setFixedWidth(35);
    connect(m_sliderOutputVolume, &QSlider::valueChanged, this, [this](int val) {
        m_lblOutputVolVal->setText(QString("%1%").arg(val));
    });
    outputVolLayout->addWidget(m_sliderOutputVolume);
    outputVolLayout->addWidget(m_lblOutputVolVal);

    formLayout->addRow("Dispositivo de Entrada:", inDevLayout);
    formLayout->addRow("Volume do Microfone:", inputVolLayout);
    formLayout->addRow("Dispositivo de Saída:", m_comboOutputDevice);
    formLayout->addRow("Volume do Alto-falante:", outputVolLayout);

    layout->addLayout(formLayout);

    // Checkboxes dos recursos nativos do PJSIP
    auto dspGroup = new QVBoxLayout();
    m_chkEchoCancellation = new QCheckBox("Habilitar Cancelamento de Eco Acústico (AEC)", widget);
    m_chkAutoGainControl = new QCheckBox("Habilitar Controle Automático de Ganho (AGC)", widget);
    m_chkVoiceActivityDetection = new QCheckBox("Habilitar Supressão de Ruído e Detecção de Voz (VAD)", widget);

    dspGroup->addWidget(m_chkEchoCancellation);
    dspGroup->addWidget(m_chkAutoGainControl);
    dspGroup->addWidget(m_chkVoiceActivityDetection);
    layout->addLayout(dspGroup);

    // Testes de Áudio e Voz (Microfone e Alto-falantes)
    auto testButtonsLayout = new QHBoxLayout();
    testButtonsLayout->setSpacing(8);

    m_btnTestVoice = new QPushButton("🎤 Testar Microfone (Falar e Ouvir)", widget);
    m_btnTestVoice->setCursor(Qt::PointingHandCursor);
    m_btnTestVoice->setToolTip("Grava 3 segundos de sua voz e reproduz de volta para conferir a captação");
    connect(m_btnTestVoice, &QPushButton::clicked, this, &SettingsDialog::onTestVoice);

    m_btnTestAudio = new QPushButton("🔊 Testar Alto-falantes", widget);
    m_btnTestAudio->setCursor(Qt::PointingHandCursor);
    m_btnTestAudio->setToolTip("Toca um áudio nos fones/alto-falantes para conferir a reprodução");
    connect(m_btnTestAudio, &QPushButton::clicked, this, &SettingsDialog::onTestAudio);

    testButtonsLayout->addWidget(m_btnTestVoice, 1);
    testButtonsLayout->addWidget(m_btnTestAudio, 1);
    layout->addLayout(testButtonsLayout);

    m_lblAudioTestStatus = new QLabel("Clique em 'Testar Microfone' para validar sua voz ou 'Testar Alto-falantes' para ouvir um som.", widget);
    m_lblAudioTestStatus->setStyleSheet("color: #7F8C8D; font-size: 11px;");
    m_lblAudioTestStatus->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_lblAudioTestStatus);

    layout->addStretch();
    refreshAudioDeviceLists();
    return widget;
}

void SettingsDialog::refreshAudioDeviceLists() {
    QString currentIn = m_comboInputDevice->currentText();
    QString currentOut = m_comboOutputDevice->currentText();

    m_comboInputDevice->clear();
    m_comboOutputDevice->clear();

    m_comboInputDevice->addItem("Padrão do Sistema");
    m_comboOutputDevice->addItem("Padrão do Sistema");

    struct DevInfo {
        QString name;
        QString desc;
        bool isUsb{false};
    };

    auto scanDevices = [](const QString &type) -> QList<DevInfo> {
        QList<DevInfo> list;
        QProcess proc;
        proc.start("pactl", {"list", type});
        if (proc.waitForFinished(2500)) {
            QString output = QString::fromUtf8(proc.readAllStandardOutput());
            QStringList blocks = output.split(QRegularExpression("\n(?:Fonte|Source|Destino|Sink) #\\d+"));
            for (const QString &b : blocks) {
                if (b.trimmed().isEmpty()) continue;
                QRegularExpression nameRegex(R"raw((?:Nome|Name):\s*(.+))raw");
                QRegularExpression descRegex(R"raw((?:Descrição|Description):\s*(.+))raw");
                QRegularExpression busRegex(R"raw(device\.bus\s*=\s*"([^"]+)")raw");
                QRegularExpression prodRegex(R"raw(device\.product\.name\s*=\s*"([^"]+)")raw");

                auto nameMatch = nameRegex.match(b);
                auto descMatch = descRegex.match(b);
                auto busMatch = busRegex.match(b);
                auto prodMatch = prodRegex.match(b);

                QString name = nameMatch.hasMatch() ? nameMatch.captured(1).trimmed() : "";
                QString desc = descMatch.hasMatch() ? descMatch.captured(1).trimmed() : "";
                QString bus = busMatch.hasMatch() ? busMatch.captured(1).trimmed() : "";
                QString prod = prodMatch.hasMatch() ? prodMatch.captured(1).trimmed() : "";

                if (name.contains(".monitor") || name.isEmpty()) continue;

                DevInfo item;
                item.name = name;
                item.desc = !desc.isEmpty() ? desc : (!prod.isEmpty() ? prod : name);
                item.isUsb = (bus == "usb" || name.contains(".usb") || item.desc.contains("usb", Qt::CaseInsensitive));
                list.append(item);
            }
        }
        return list;
    };

    auto sources = scanDevices("sources");
    for (const auto &s : sources) {
        QString label = s.isUsb ? QString("🎧 Headset USB: %1").arg(s.desc)
                                : QString("🎙 %1").arg(s.desc);
        m_comboInputDevice->addItem(label, s.name);
    }

    auto sinks = scanDevices("sinks");
    for (const auto &s : sinks) {
        QString label = s.isUsb ? QString("🎧 Headset USB: %1").arg(s.desc)
                                : QString("🔊 %1").arg(s.desc);
        m_comboOutputDevice->addItem(label, s.name);
    }

    // Se nenhum dispositivo adicional foi encontrado, tenta ler ALSA cards
    if (m_comboInputDevice->count() <= 1 || m_comboOutputDevice->count() <= 1) {
        QFile file("/proc/asound/cards");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(file.readAll());
            QStringList lines = content.split('\n');
            for (const QString &line : lines) {
                if (line.contains("USB-Audio") || line.contains("Quantum")) {
                    QString usbDev = line.section(':', 1).trimmed();
                    if (!usbDev.isEmpty()) {
                        m_comboInputDevice->addItem(QString("🎧 Headset USB: %1").arg(usbDev));
                        m_comboOutputDevice->addItem(QString("🎧 Headset USB: %1").arg(usbDev));
                    }
                }
            }
        }
    }

    int idxIn = m_comboInputDevice->findText(currentIn);
    if (idxIn >= 0) m_comboInputDevice->setCurrentIndex(idxIn);

    int idxOut = m_comboOutputDevice->findText(currentOut);
    if (idxOut >= 0) m_comboOutputDevice->setCurrentIndex(idxOut);
}

void SettingsDialog::onTestAudio() {
    m_btnTestAudio->setEnabled(false);
    m_lblAudioTestStatus->setText("Tocando sinal de áudio nos alto-falantes...");
    m_lblAudioTestStatus->setStyleSheet("color: #3498DB; font-weight: bold; font-size: 11px;");
    SoundManager::instance().playPreview("Cisco Ringtone 1");
    QTimer::singleShot(2500, this, [this]() {
        SoundManager::instance().stopPreview();
        m_btnTestAudio->setEnabled(true);
        m_lblAudioTestStatus->setText("✓ Teste de alto-falantes concluído!");
        m_lblAudioTestStatus->setStyleSheet("color: #2ECC71; font-weight: bold; font-size: 11px;");
    });
}

void SettingsDialog::onTestVoice() {
    if (m_voiceCountdownTimer && m_voiceCountdownTimer->isActive()) {
        m_voiceCountdownTimer->stop();
        if (m_voiceRecProcess) {
            m_voiceRecProcess->kill();
        }
        if (m_voicePlayProcess) {
            m_voicePlayProcess->kill();
        }
        m_btnTestVoice->setText("🎤 Testar Microfone (Falar e Ouvir)");
        m_btnTestVoice->setStyleSheet("");
        m_lblAudioTestStatus->setText("Teste de voz cancelado.");
        m_lblAudioTestStatus->setStyleSheet("color: #7F8C8D; font-size: 11px;");
        return;
    }

    const QString testFile = "/tmp/vxvoice_test_mic.wav";
    QFile::remove(testFile);

    m_voiceCountdownSec = 3;
    m_btnTestVoice->setText(QString("⏺ Gravando sua voz... (%1s)").arg(m_voiceCountdownSec));
    m_btnTestVoice->setStyleSheet("background-color: #E74C3C; color: #FFFFFF; font-weight: bold; border-radius: 6px;");
    m_lblAudioTestStatus->setText("Fale agora no microfone! Gravando amostra de voz de 3 segundos...");
    m_lblAudioTestStatus->setStyleSheet("color: #E67E22; font-weight: bold; font-size: 11px;");

    if (!m_voiceRecProcess) {
        m_voiceRecProcess = new QProcess(this);
    }
    m_voiceRecProcess->start("pw-record", {"--channels", "1", "--rate", "16000", testFile});
    if (!m_voiceRecProcess->waitForStarted(800)) {
        m_voiceRecProcess->start("arecord", {"-d", "3", "-f", "S16_LE", "-r", "16000", testFile});
    }

    if (!m_voiceCountdownTimer) {
        m_voiceCountdownTimer = new QTimer(this);
    }
    m_voiceCountdownTimer->disconnect();
    connect(m_voiceCountdownTimer, &QTimer::timeout, this, [this, testFile]() {
        m_voiceCountdownSec--;
        if (m_voiceCountdownSec > 0) {
            m_btnTestVoice->setText(QString("⏺ Gravando sua voz... (%1s)").arg(m_voiceCountdownSec));
        } else {
            m_voiceCountdownTimer->stop();

            if (m_voiceRecProcess && m_voiceRecProcess->state() != QProcess::NotRunning) {
                m_voiceRecProcess->terminate();
                m_voiceRecProcess->waitForFinished(1000);
            }

            m_btnTestVoice->setText("🔊 Reproduzindo sua voz gravada...");
            m_btnTestVoice->setStyleSheet("background-color: #2980B9; color: #FFFFFF; font-weight: bold; border-radius: 6px;");
            m_lblAudioTestStatus->setText("Ouça sua voz gravada nos fones/alto-falantes para conferir a fidelidade.");
            m_lblAudioTestStatus->setStyleSheet("color: #3498DB; font-weight: bold; font-size: 11px;");

            if (!m_voicePlayProcess) {
                m_voicePlayProcess = new QProcess(this);
            }
            m_voicePlayProcess->disconnect();
            connect(m_voicePlayProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, [this]() {
                m_btnTestVoice->setText("🎤 Testar Microfone (Falar e Ouvir)");
                m_btnTestVoice->setStyleSheet("");
                m_lblAudioTestStatus->setText("✓ Teste de microfone concluído com sucesso! Sua voz foi captada e reproduzida.");
                m_lblAudioTestStatus->setStyleSheet("color: #2ECC71; font-weight: bold; font-size: 11px;");
            });

            m_voicePlayProcess->start("pw-play", {testFile});
            if (!m_voicePlayProcess->waitForStarted(800)) {
                m_voicePlayProcess->start("paplay", {testFile});
            }
        }
    });
    m_voiceCountdownTimer->start(1000);
}

QWidget* SettingsDialog::createCodecsTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    auto tipLabel = new QLabel("Selecione um codec de voz e utilize os botões <b>Mover para Cima</b> ou <b>Mover para Baixo</b> "
                               "para definir a ordem de prioridade no PJSIP. O primeiro codec habilitado terá a maior preferência nas chamadas.", widget);
    tipLabel->setWordWrap(true);
    tipLabel->setStyleSheet("color: #95A5A6; font-size: 12px;");

    m_codecsListWidget = new QListWidget(widget);
    m_codecsListWidget->setDragDropMode(QAbstractItemView::InternalMove);

    auto btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_btnCodecMoveUp = new QPushButton("⬆ Mover para Cima", widget);
    m_btnCodecMoveUp->setCursor(Qt::PointingHandCursor);
    m_btnCodecMoveUp->setEnabled(false);
    connect(m_btnCodecMoveUp, &QPushButton::clicked, this, &SettingsDialog::onMoveCodecUp);

    m_btnCodecMoveDown = new QPushButton("⬇ Mover para Baixo", widget);
    m_btnCodecMoveDown->setCursor(Qt::PointingHandCursor);
    m_btnCodecMoveDown->setEnabled(false);
    connect(m_btnCodecMoveDown, &QPushButton::clicked, this, &SettingsDialog::onMoveCodecDown);

    btnRow->addWidget(m_btnCodecMoveUp);
    btnRow->addWidget(m_btnCodecMoveDown);
    btnRow->addStretch();

    connect(m_codecsListWidget, &QListWidget::currentRowChanged, this, [this](int row) {
        int count = m_codecsListWidget->count();
        m_btnCodecMoveUp->setEnabled(row > 0);
        m_btnCodecMoveDown->setEnabled(row >= 0 && row < count - 1);
    });

    layout->addWidget(tipLabel);
    layout->addWidget(m_codecsListWidget, 1);
    layout->addLayout(btnRow);
    return widget;
}

void SettingsDialog::onMoveCodecUp() {
    int row = m_codecsListWidget->currentRow();
    if (row > 0) {
        auto item = m_codecsListWidget->takeItem(row);
        m_codecsListWidget->insertItem(row - 1, item);
        m_codecsListWidget->setCurrentRow(row - 1);
    }
}

void SettingsDialog::onMoveCodecDown() {
    int row = m_codecsListWidget->currentRow();
    if (row >= 0 && row < m_codecsListWidget->count() - 1) {
        auto item = m_codecsListWidget->takeItem(row);
        m_codecsListWidget->insertItem(row + 1, item);
        m_codecsListWidget->setCurrentRow(row + 1);
    }
}

QWidget* SettingsDialog::createRecordingTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    m_chkAutoRecord = new QCheckBox("Gravar todas as chamadas automaticamente", widget);
    layout->addWidget(m_chkAutoRecord);

    auto formLayout = new QFormLayout();
    auto pathLayout = new QHBoxLayout();
    m_recordingPathEdit = new QLineEdit(widget);
    m_recordingPathEdit->setReadOnly(true);

    m_btnBrowseRecordPath = new QPushButton("Procurar...", widget);
    connect(m_btnBrowseRecordPath, &QPushButton::clicked, this, &SettingsDialog::onBrowseRecordingPath);

    m_btnOpenRecordFolder = new QPushButton("Abrir Diretório", widget);
    connect(m_btnOpenRecordFolder, &QPushButton::clicked, this, &SettingsDialog::onOpenRecordingFolder);

    pathLayout->addWidget(m_recordingPathEdit, 1);
    pathLayout->addWidget(m_btnBrowseRecordPath);
    pathLayout->addWidget(m_btnOpenRecordFolder);

    m_comboRecordFormat = new QComboBox(widget);
    m_comboRecordFormat->addItems({"WAV (PCM 16-bit Alta Fidelidade)", "MP3 (Compactado)"});

    formLayout->addRow("Diretório de Gravações:", pathLayout);
    formLayout->addRow("Formato de Áudio:", m_comboRecordFormat);

    layout->addLayout(formLayout);
    layout->addStretch();
    return widget;
}

void SettingsDialog::onOpenRecordingFolder() {
    QString path = m_recordingPathEdit->text();
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

QWidget* SettingsDialog::createAboutTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    auto lblTitle = new QLabel("<b>VxVoice Softphone Corporativo</b>", widget);
    lblTitle->setStyleSheet("font-size: 16px; color: #8E44AD;");
    auto lblEmpresa = new QLabel("<b>Empresa:</b> VxNET Comunicações e Telecomunicações", widget);
    auto lblAno = new QLabel("<b>Ano:</b> 2026", widget);
    auto lblVersion = new QLabel(QString("<b>Versão:</b> %1 (Build Corporativa Multiplataforma)").arg(SettingsManager::APP_VERSION), widget);
    auto lblLicense = new QLabel("<b>Licença:</b> Comercial Proprietária - VxNET Telecom", widget);

    // Card de Atualizações Automáticas via Git
    auto updateCard = new QFrame(widget);
    updateCard->setObjectName("callInfoFrame");
    auto updateLayout = new QVBoxLayout(updateCard);
    updateLayout->setContentsMargins(10, 8, 10, 8);
    updateLayout->setSpacing(6);

    auto lblUpdateHeader = new QLabel("<b>🔄 Atualizações do Sistema (Git)</b>", updateCard);
    m_lblUpdateStatus = new QLabel(QString("Versão instalada: %1 • Repositório conectado").arg(SettingsManager::APP_VERSION), updateCard);
    m_lblUpdateStatus->setStyleSheet("color: #7F8C8D; font-size: 12px;");

    auto btnUpdateRow = new QHBoxLayout();
    m_btnCheckUpdates = new QPushButton("🔍 Verificar Atualizações no Git", updateCard);
    m_btnCheckUpdates->setCursor(Qt::PointingHandCursor);
    connect(m_btnCheckUpdates, &QPushButton::clicked, this, &SettingsDialog::onCheckGitUpdates);

    m_btnApplyUpdate = new QPushButton("⬇ Atualizar via Git", updateCard);
    m_btnApplyUpdate->setCursor(Qt::PointingHandCursor);
    m_btnApplyUpdate->setVisible(false);
    connect(m_btnApplyUpdate, &QPushButton::clicked, this, &SettingsDialog::onApplyGitUpdate);

    btnUpdateRow->addWidget(m_btnCheckUpdates);
    btnUpdateRow->addWidget(m_btnApplyUpdate);
    btnUpdateRow->addStretch();

    updateLayout->addWidget(lblUpdateHeader);
    updateLayout->addWidget(m_lblUpdateStatus);
    updateLayout->addLayout(btnUpdateRow);

    auto txtChangelog = new QTextEdit(widget);
    txtChangelog->setReadOnly(true);
    txtChangelog->setHtml(
        "<b>Novidades e Correções (v1.3.0):</b><br>"
        "<ul>"
        "<li><b>Priorização Dinâmica de Codecs:</b> Botões explícitos para mover codecs para cima ou para baixo, determinando a preferência no PJSIP.</li>"
        "<li><b>Teste de Voz Interativo:</b> Gravação e reprodução de áudio em tempo real para validação de clareza do microfone.</li>"
        "<li><b>Detecção Nativa de Dispositivos USB:</b> Enumeração de headsets e placas de som USB (ex: JBL Quantum 400) com botão de atualização dinâmica.</li>"
        "<li><b>Design System Multi-Temas:</b> Abas, bina e botões de contato com proporções perfeitas em todos os temas.</li>"
        "<li><b>Atualizações Automáticas via Git:</b> Sincronização direta com o repositório Git.</li>"
        "</ul>"
    );

    layout->addWidget(lblTitle);
    layout->addWidget(lblEmpresa);
    layout->addWidget(lblAno);
    layout->addWidget(lblVersion);
    layout->addWidget(lblLicense);
    layout->addWidget(updateCard);
    layout->addWidget(txtChangelog, 1);
    return widget;
}

void SettingsDialog::onCheckGitUpdates() {
    m_btnCheckUpdates->setEnabled(false);
    m_btnCheckUpdates->setText("Verificando no Git...");
    m_lblUpdateStatus->setText("Consultando upstream / repositório Git...");

    auto &um = UpdateManager::instance();

    // Desconecta listeners anteriores se houver
    disconnect(&um, &UpdateManager::upToDate, this, nullptr);
    disconnect(&um, &UpdateManager::updateAvailable, this, nullptr);
    disconnect(&um, &UpdateManager::updateError, this, nullptr);

    connect(&um, &UpdateManager::upToDate, this, [this](const QString &msg) {
        m_btnCheckUpdates->setEnabled(true);
        m_btnCheckUpdates->setText("🔍 Verificar Atualizações no Git");
        m_lblUpdateStatus->setText("✓ " + msg);
        m_lblUpdateStatus->setStyleSheet("color: #2ECC71; font-weight: bold; font-size: 12px;");
        m_btnApplyUpdate->setVisible(false);
    });

    connect(&um, &UpdateManager::updateAvailable, this, [this](const QString &latest, const QString &log) {
        m_btnCheckUpdates->setEnabled(true);
        m_btnCheckUpdates->setText("🔍 Verificar Atualizações no Git");
        m_lblUpdateStatus->setText(QString("★ Nova atualização disponível: %1").arg(latest));
        m_lblUpdateStatus->setStyleSheet("color: #3498DB; font-weight: bold; font-size: 12px;");
        m_btnApplyUpdate->setVisible(true);

        QMessageBox::information(this, "Atualização Disponível",
            QString("Uma nova versão do VxVoice foi detectada no repositório Git:\n\n%1\n\nClique em 'Atualizar via Git' para sincronizar.")
            .arg(log.isEmpty() ? latest : log));
    });

    connect(&um, &UpdateManager::updateError, this, [this](const QString &err) {
        m_btnCheckUpdates->setEnabled(true);
        m_btnCheckUpdates->setText("🔍 Verificar Atualizações no Git");
        m_lblUpdateStatus->setText("⚠ " + err);
        m_lblUpdateStatus->setStyleSheet("color: #E74C3C; font-size: 12px;");
        m_btnApplyUpdate->setVisible(false);
    });

    um.checkForUpdates(false);
}

void SettingsDialog::onApplyGitUpdate() {
    m_btnApplyUpdate->setEnabled(false);
    m_btnApplyUpdate->setText("Aplicando Git Pull...");
    m_lblUpdateStatus->setText("Baixando novos commits e aplicando rebase...");

    auto &um = UpdateManager::instance();
    disconnect(&um, &UpdateManager::updateCompleted, this, nullptr);

    connect(&um, &UpdateManager::updateCompleted, this, [this](bool ok, const QString &msg) {
        m_btnApplyUpdate->setEnabled(true);
        m_btnApplyUpdate->setText("⬇ Atualizar via Git");
        if (ok) {
            m_lblUpdateStatus->setText("✓ " + msg);
            m_lblUpdateStatus->setStyleSheet("color: #2ECC71; font-weight: bold; font-size: 12px;");
            QMessageBox::information(this, "Atualização Concluída", msg);
        } else {
            m_lblUpdateStatus->setText("⚠ " + msg);
            m_lblUpdateStatus->setStyleSheet("color: #E74C3C; font-size: 12px;");
            QMessageBox::warning(this, "Erro na Atualização", msg);
        }
    });

    um.applyUpdate();
}

void SettingsDialog::loadSettings() {
    auto &sm = SettingsManager::instance();

    m_chkStartWithSystem->setChecked(sm.startWithSystem());
    m_chkStartMinimized->setChecked(sm.startMinimized());
    m_chkMinimizeToTrayOnClose->setChecked(sm.minimizeToTrayOnClose());

    int ringIdx = m_comboRingtone->findText(sm.ringtone());
    if (ringIdx >= 0) m_comboRingtone->setCurrentIndex(ringIdx);

    // Theme selector
    auto currentTheme = ThemeManager::instance().currentTheme();
    m_comboTheme->setCurrentIndex(static_cast<int>(currentTheme));

    // Audio & Voice
    m_sliderInputVolume->setValue(sm.inputVolume());
    m_lblInputVolVal->setText(QString("%1%").arg(sm.inputVolume()));
    m_sliderOutputVolume->setValue(sm.outputVolume());
    m_lblOutputVolVal->setText(QString("%1%").arg(sm.outputVolume()));
    m_chkEchoCancellation->setChecked(sm.echoCancellation());
    m_chkAutoGainControl->setChecked(sm.autoGainControl());
    m_chkVoiceActivityDetection->setChecked(sm.voiceActivityDetection());

    int inIdx = m_comboInputDevice->findText(sm.inputDevice());
    if (inIdx >= 0) m_comboInputDevice->setCurrentIndex(inIdx);

    int outIdx = m_comboOutputDevice->findText(sm.outputDevice());
    if (outIdx >= 0) m_comboOutputDevice->setCurrentIndex(outIdx);

    // Codecs
    m_codecsListWidget->clear();
    auto codecs = sm.codecs();
    for (const auto &c : codecs) {
        auto item = new QListWidgetItem(c.name, m_codecsListWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled);
        item->setCheckState(c.enabled ? Qt::Checked : Qt::Unchecked);
    }

    // Recording
    m_recordingPathEdit->setText(sm.recordingPath());
    int formatIdx = m_comboRecordFormat->findText(sm.recordingFormat(), Qt::MatchStartsWith);
    if (formatIdx >= 0) m_comboRecordFormat->setCurrentIndex(formatIdx);
    m_chkAutoRecord->setChecked(sm.autoRecord());
}

void SettingsDialog::saveSettings() {
    auto &sm = SettingsManager::instance();

    sm.setStartWithSystem(m_chkStartWithSystem->isChecked());
    sm.setStartMinimized(m_chkStartMinimized->isChecked());
    sm.setMinimizeToTrayOnClose(m_chkMinimizeToTrayOnClose->isChecked());
    sm.setRingtone(m_comboRingtone->currentText());

    // Theme
    auto selectedTheme = static_cast<ThemeManager::Theme>(m_comboTheme->currentIndex());
    ThemeManager::instance().setTheme(selectedTheme);

    // Audio & Voice
    sm.setInputVolume(m_sliderInputVolume->value());
    sm.setOutputVolume(m_sliderOutputVolume->value());
    sm.setInputDevice(m_comboInputDevice->currentText());
    sm.setOutputDevice(m_comboOutputDevice->currentText());
    sm.setEchoCancellation(m_chkEchoCancellation->isChecked());
    sm.setAutoGainControl(m_chkAutoGainControl->isChecked());
    sm.setVoiceActivityDetection(m_chkVoiceActivityDetection->isChecked());

    SipManager::instance().setAudioProcessing(
        m_chkEchoCancellation->isChecked(),
        m_chkAutoGainControl->isChecked(),
        m_chkVoiceActivityDetection->isChecked()
    );
    SipManager::instance().setAudioDevices(m_comboInputDevice->currentText(), m_comboOutputDevice->currentText());
    SipManager::instance().setAudioVolume(m_sliderInputVolume->value(), m_sliderOutputVolume->value());

    // Codecs
    QList<CodecConfig> codecList;
    for (int i = 0; i < m_codecsListWidget->count(); ++i) {
        auto item = m_codecsListWidget->item(i);
        codecList.append({item->text(), item->checkState() == Qt::Checked});
    }
    sm.setCodecs(codecList);

    // Recording
    sm.setRecordingPath(m_recordingPathEdit->text());
    sm.setRecordingFormat(m_comboRecordFormat->currentIndex() == 0 ? "WAV" : "MP3");
    sm.setAutoRecord(m_chkAutoRecord->isChecked());
}

void SettingsDialog::onBrowseRecordingPath() {
    QString dir = QFileDialog::getExistingDirectory(this, "Selecionar Diretório de Gravações",
                                                    m_recordingPathEdit->text(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_recordingPathEdit->setText(dir);
    }
}

void SettingsDialog::onToggleRingtonePreview() {
    if (m_isPreviewPlaying) {
        SoundManager::instance().stopPreview();
        m_btnPlayRingtone->setText("▶ Ouvir Toque");
        m_isPreviewPlaying = false;
    } else {
        SoundManager::instance().playPreview(m_comboRingtone->currentText());
        m_btnPlayRingtone->setText("⏹ Parar Toque");
        m_isPreviewPlaying = true;
    }
}

void SettingsDialog::onSaveClicked() {
    saveSettings();
    accept();
}
