#include "SettingsDialog.h"
#include "ThemeManager.h"
#include "SoundManager.h"
#include "SipManager.h"
#include "UpdateManager.h"
#include "ChangelogDialog.h"
#include "UpdateProgressDialog.h"
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
#include <QScrollBar>
#include <QClipboard>
#include <QGuiApplication>

#ifdef HAVE_QT_MULTIMEDIA
#include <QMediaDevices>
#include <QAudioDevice>
#endif

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Configurações");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    resize(520, 580);
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
    m_tabs->addTab(createSipLogTab(), "📋 Log SIP");
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

    // Container de sub-opções indentadas vinculadas ao checkbox "Iniciar junto com o Sistema Operacional"
    m_autostartSubOptions = new QWidget(widget);
    m_autostartSubOptions->setStyleSheet(
        "QCheckBox:disabled, QLabel:disabled { color: #6E7681; }"
        "QCheckBox::indicator:disabled { background-color: #3A3F47; border: 1px solid #555B64; }"
        "QComboBox:disabled { background-color: #2E3239; color: #6E7681; border: 1px solid #495057; }"
    );
    auto subOptionsLayout = new QVBoxLayout(m_autostartSubOptions);
    subOptionsLayout->setContentsMargins(24, 2, 0, 4);
    subOptionsLayout->setSpacing(8);

    m_chkStartMinimized = new QCheckBox("Iniciar minimizado no System Tray", m_autostartSubOptions);
    m_chkStartMinimized->setToolTip("Quando ativado, inicia minimizado apenas ao ligar o computador junto com o Sistema Operacional.");

    auto regStatusRow = new QHBoxLayout();
    regStatusRow->setSpacing(10);
    auto lblRegStatus = new QLabel("Status de Registro:", m_autostartSubOptions);
    m_comboRegistrationStatus = new QComboBox(m_autostartSubOptions);
    m_comboRegistrationStatus->addItem("🟢 Disponível");
    m_comboRegistrationStatus->addItem("🔴 Não Perturbe");
    m_comboRegistrationStatus->addItem("⚫ Offline");
    regStatusRow->addWidget(lblRegStatus);
    regStatusRow->addWidget(m_comboRegistrationStatus, 1);

    subOptionsLayout->addWidget(m_chkStartMinimized);
    subOptionsLayout->addLayout(regStatusRow);

    m_autostartSubOptions->setEnabled(false);
    connect(m_chkStartWithSystem, &QCheckBox::toggled, this, [this](bool checked) {
        m_autostartSubOptions->setEnabled(checked);
    });

    m_chkMinimizeToTrayOnClose = new QCheckBox("Ao clicar em fechar (X), manter na bandeja do sistema", widget);

    layout->addWidget(m_chkStartWithSystem);
    layout->addWidget(m_autostartSubOptions);
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

    // Theme selector com os 8 temas VxVoice
    m_comboTheme = new QComboBox(widget);
    m_comboTheme->addItem("🌑 Dark (Tons de azul escuro)");
    m_comboTheme->addItem("☀️ Light (Tons de azul claro)");
    m_comboTheme->addItem("🟣 Purple (Tons de roxo corporativo)");
    m_comboTheme->addItem("🔴 Red (Tons de vermelho corporativo)");
    m_comboTheme->addItem("🩵 Blue Baby");
    m_comboTheme->addItem("🌊 Blue Capri");
    m_comboTheme->addItem("🌸 Rosa Amaranth");
    m_comboTheme->addItem("🌺 Rosa Hot Magenta");

    formLayout->addRow("Ringtone de Chamada:", ringtoneLayout);
    formLayout->addRow("Temas VxVoice:", m_comboTheme);

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

    m_lblAboutTitle = new QLabel("<b>VxVoice</b>", widget);
    auto updateAboutTitleColor = [this](ThemeManager::Theme t) {
        if (!m_lblAboutTitle) return;
        const bool isLightBg = (t == ThemeManager::Theme::Light || t == ThemeManager::Theme::BlueBaby);
        const QString color = isLightBg ? "#000000" : "#FFFFFF";
        m_lblAboutTitle->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(color));
    };
    updateAboutTitleColor(ThemeManager::instance().currentTheme());
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, updateAboutTitleColor);

    auto lblEmpresa = new QLabel("<b>Empresa:</b> VxNET Comunicações e Telecomunicações", widget);
    auto lblAno = new QLabel("<b>Ano:</b> 2026", widget);
    auto lblVersion = new QLabel(QString("<b>Versão:</b> %1").arg(SettingsManager::APP_VERSION), widget);
    auto lblLicense = new QLabel("<b>Licença:</b> GNU General Public License v3.0 (GPLv3) / Open Source", widget);
    auto lblGithubLink = new QLabel(
        "<a href=\"https://github.com/ghost-alone1996/VxVoice\" style=\"color: #3498DB; text-decoration: none; font-weight: bold;\">🔗 Acessar Repositório Github</a>",
        widget
    );
    lblGithubLink->setOpenExternalLinks(true);
    lblGithubLink->setCursor(Qt::PointingHandCursor);

    // Card de Atualizações Automáticas via Git
    auto updateCard = new QFrame(widget);
    updateCard->setObjectName("callInfoFrame");
    auto updateLayout = new QVBoxLayout(updateCard);
    updateLayout->setContentsMargins(14, 12, 14, 12);
    updateLayout->setSpacing(8);

    auto lblUpdateHeader = new QLabel("<b>🔄 Atualização de Software (GitHub)</b>", updateCard);
    lblUpdateHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: #3498DB;");

    auto formGrid = new QGridLayout();
    formGrid->setHorizontalSpacing(14);
    formGrid->setVerticalSpacing(6);

    auto lblCurVerTitle = new QLabel("<b>Versão Atual Instalada:</b>", updateCard);
    m_lblCurrentVersion = new QLabel(QString("v%1").arg(SettingsManager::APP_VERSION), updateCard);
    m_lblCurrentVersion->setStyleSheet("font-weight: bold; font-size: 13px;");

    auto lblGitVerTitle = new QLabel("<b>Versão Disponível (GitHub):</b>", updateCard);
    m_lblGitVersion = new QLabel("Clique em verificar para consultar...", updateCard);
    m_lblGitVersion->setStyleSheet("font-weight: bold; color: #3498DB; font-size: 13px;");

    auto lblStatusTitle = new QLabel("<b>Status:</b>", updateCard);
    m_lblUpdateStatus = new QLabel("Pronto para verificar atualizações no GitHub.", updateCard);
    m_lblUpdateStatus->setStyleSheet("color: #7F8C8D; font-size: 12px;");

    formGrid->addWidget(lblCurVerTitle, 0, 0);
    formGrid->addWidget(m_lblCurrentVersion, 0, 1);
    formGrid->addWidget(lblGitVerTitle, 1, 0);
    formGrid->addWidget(m_lblGitVersion, 1, 1);
    formGrid->addWidget(lblStatusTitle, 2, 0);
    formGrid->addWidget(m_lblUpdateStatus, 2, 1);
    formGrid->setColumnStretch(1, 1);

    auto btnUpdateRow = new QHBoxLayout();
    btnUpdateRow->setSpacing(8);

    m_btnCheckUpdates = new QPushButton("🔍 Buscar Atualizações", updateCard);
    m_btnCheckUpdates->setCursor(Qt::PointingHandCursor);
    connect(m_btnCheckUpdates, &QPushButton::clicked, this, &SettingsDialog::onCheckGitUpdates);

    m_btnApplyUpdate = new QPushButton("⬇ Aplicar Atualização", updateCard);
    m_btnApplyUpdate->setCursor(Qt::PointingHandCursor);
    m_btnApplyUpdate->setEnabled(false);
    m_btnApplyUpdate->setStyleSheet(
        "QPushButton {"
        "  background-color: #27AE60;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 6px 14px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #2ECC71; }"
        "QPushButton:disabled { background-color: #6C757D; color: #E5E7EB; }"
    );
    connect(m_btnApplyUpdate, &QPushButton::clicked, this, &SettingsDialog::onApplyGitUpdate);

    m_btnShowChangelog = new QPushButton("📋 Ver Novidades e Melhorias", updateCard);
    m_btnShowChangelog->setCursor(Qt::PointingHandCursor);
    connect(m_btnShowChangelog, &QPushButton::clicked, this, &SettingsDialog::onShowChangelog);

    btnUpdateRow->addWidget(m_btnCheckUpdates);
    btnUpdateRow->addWidget(m_btnApplyUpdate);
    btnUpdateRow->addWidget(m_btnShowChangelog);
    btnUpdateRow->addStretch();

    updateLayout->addWidget(lblUpdateHeader);
    updateLayout->addLayout(formGrid);
    updateLayout->addLayout(btnUpdateRow);

    auto txtChangelog = new QTextEdit(widget);
    txtChangelog->setReadOnly(true);
    txtChangelog->setHtml(
        "<b>Novidades e Correções (v2.0.2):</b><br>"
        "<ul>"
        "<li><b>Proporção Única na Tela Principal:</b> Janela principal unificada usando a proporção da aba Ligação, com adaptação automática das colunas e barras de rolagem em Histórico, Contatos e Gravação.</li>"
        "<li><b>Temas VxVoice:</b> Seletor renomeado para 'Temas VxVoice' com 4 novos temas: Blue Baby, Blue Capri, Rosa Amaranth e Rosa Hot Magenta.</li>"
        "<li><b>Proporção da Janela Contas SIP:</b> Ajuste automático das colunas ao conteúdo com redimensionamento proporcional da janela.</li>"
        "<li><b>Testes Automatizados &amp; Histórico:</b> Suíte Qt Test integrada ao CMake/CTest e histórico completo de versões em Markdown.</li>"
        "</ul>"
    );

    layout->addWidget(m_lblAboutTitle);
    layout->addWidget(lblEmpresa);
    layout->addWidget(lblAno);
    layout->addWidget(lblVersion);
    layout->addWidget(lblLicense);
    layout->addWidget(lblGithubLink);
    layout->addWidget(updateCard);
    layout->addWidget(txtChangelog, 1);

    // Consulta inicial rápida de versão Git
    QTimer::singleShot(200, this, [this]() {
        onCheckGitUpdates();
    });

    return widget;
}

QWidget* SettingsDialog::createSipLogTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto topBar = new QHBoxLayout();
    topBar->setSpacing(8);

    m_txtLogFilter = new QLineEdit(widget);
    m_txtLogFilter->setPlaceholderText("Filtrar logs SIP (ex: INVITE, REGISTER, 200 OK, erro)...");
    m_txtLogFilter->setClearButtonEnabled(true);
    connect(m_txtLogFilter, &QLineEdit::textChanged, this, &SettingsDialog::onFilterSipLogs);

    m_chkAutoScrollSipLogs = new QCheckBox("Auto-rolagem", widget);
    m_chkAutoScrollSipLogs->setChecked(true);

    topBar->addWidget(m_txtLogFilter, 1);
    topBar->addWidget(m_chkAutoScrollSipLogs);

    m_txtSipLogs = new QPlainTextEdit(widget);
    m_txtSipLogs->setReadOnly(true);
    m_txtSipLogs->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont monoFont("Monospace");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(9);
    m_txtSipLogs->setFont(monoFont);
    m_txtSipLogs->setStyleSheet(
        "QPlainTextEdit { "
        "  background-color: #121820; "
        "  color: #E2E8F0; "
        "  border: 1px solid #2D3748; "
        "  border-radius: 4px; "
        "  font-family: monospace; "
        "}"
    );

    const auto existingLogs = SipManager::instance().sipLogs();
    for (const auto &line : existingLogs) {
        m_txtSipLogs->appendPlainText(line);
    }
    if (m_chkAutoScrollSipLogs->isChecked()) {
        m_txtSipLogs->verticalScrollBar()->setValue(m_txtSipLogs->verticalScrollBar()->maximum());
    }

    auto bottomBar = new QHBoxLayout();
    auto btnClear = new QPushButton("🗑 Limpar Logs", widget);
    connect(btnClear, &QPushButton::clicked, this, &SettingsDialog::onClearSipLogs);

    auto btnCopy = new QPushButton("📋 Copiar Tudo", widget);
    connect(btnCopy, &QPushButton::clicked, this, &SettingsDialog::onCopySipLogs);

    auto btnSave = new QPushButton("💾 Salvar em Arquivo...", widget);
    connect(btnSave, &QPushButton::clicked, this, &SettingsDialog::onSaveSipLogs);

    bottomBar->addWidget(btnClear);
    bottomBar->addWidget(btnCopy);
    bottomBar->addWidget(btnSave);
    bottomBar->addStretch();

    layout->addLayout(topBar);
    layout->addWidget(m_txtSipLogs, 1);
    layout->addLayout(bottomBar);

    connect(&SipManager::instance(), &SipManager::sipLogReceived, this, &SettingsDialog::onSipLogReceived);

    return widget;
}

void SettingsDialog::onSipLogReceived(const QString &line, int level) {
    Q_UNUSED(level);
    if (!m_txtSipLogs) return;

    QString filter = m_txtLogFilter ? m_txtLogFilter->text().trimmed() : QString();
    if (!filter.isEmpty() && !line.contains(filter, Qt::CaseInsensitive)) {
        return;
    }

    m_txtSipLogs->appendPlainText(line);

    if (m_chkAutoScrollSipLogs && m_chkAutoScrollSipLogs->isChecked()) {
        m_txtSipLogs->verticalScrollBar()->setValue(m_txtSipLogs->verticalScrollBar()->maximum());
    }
}

void SettingsDialog::onFilterSipLogs(const QString &filterText) {
    if (!m_txtSipLogs) return;
    m_txtSipLogs->clear();
    const auto allLogs = SipManager::instance().sipLogs();
    QString f = filterText.trimmed();
    for (const auto &l : allLogs) {
        if (f.isEmpty() || l.contains(f, Qt::CaseInsensitive)) {
            m_txtSipLogs->appendPlainText(l);
        }
    }
    if (m_chkAutoScrollSipLogs && m_chkAutoScrollSipLogs->isChecked()) {
        m_txtSipLogs->verticalScrollBar()->setValue(m_txtSipLogs->verticalScrollBar()->maximum());
    }
}

void SettingsDialog::onClearSipLogs() {
    SipManager::instance().clearSipLogs();
    if (m_txtSipLogs) {
        m_txtSipLogs->clear();
    }
}

void SettingsDialog::onCopySipLogs() {
    if (!m_txtSipLogs) return;
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(m_txtSipLogs->toPlainText());
        QMessageBox::information(this, "Logs Copiados", "Os logs SIP foram copiados para a área de transferência.");
    }
}

void SettingsDialog::onSaveSipLogs() {
    if (!m_txtSipLogs) return;
    QString defaultPath = QDir::homePath() + QString("/vxvoice_sip_log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = QFileDialog::getSaveFileName(this, "Salvar Logs SIP", defaultPath, "Arquivos de Log (*.txt *.log)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << m_txtSipLogs->toPlainText();
        file.close();
        QMessageBox::information(this, "Logs Salvos", QString("Logs SIP exportados com sucesso para:\n%1").arg(filePath));
    } else {
        QMessageBox::warning(this, "Erro", "Não foi possível gravar o arquivo de logs selecionado.");
    }
}

void SettingsDialog::onCheckGitUpdates() {
    m_btnCheckUpdates->setEnabled(false);
    m_btnCheckUpdates->setText("Verificando...");
    m_lblUpdateStatus->setText("Consultando repositório no GitHub...");
    m_lblCurrentVersion->setText(QString("v%1").arg(SettingsManager::APP_VERSION));

    auto &um = UpdateManager::instance();

    // Desconecta listeners anteriores se houver
    disconnect(&um, &UpdateManager::updateChecked, this, nullptr);
    disconnect(&um, &UpdateManager::updateError, this, nullptr);

    connect(&um, &UpdateManager::updateChecked, this, [this](const QString &curVer, const QString &gitVer, bool hasUpdate, const QString &/*changelog*/) {
        m_btnCheckUpdates->setEnabled(true);
        m_btnCheckUpdates->setText("🔍 Buscar Atualizações");
        m_lblCurrentVersion->setText(QString("v%1").arg(curVer));

        if (hasUpdate) {
            m_lblGitVersion->setText(QString("v%1 (Nova versão disponível!)").arg(gitVer));
            m_lblGitVersion->setStyleSheet("font-weight: bold; color: #2ECC71; font-size: 13px;");
            m_lblUpdateStatus->setText("Uma versão mais recente foi localizada no GitHub.");
            m_lblUpdateStatus->setStyleSheet("color: #2ECC71; font-weight: bold; font-size: 12px;");
            m_btnApplyUpdate->setEnabled(true);
            m_btnApplyUpdate->setText(QString("⬇ Aplicar Atualização (v%1)").arg(gitVer));
        } else {
            m_lblGitVersion->setText(QString("v%1 (Versão mais recente)").arg(gitVer));
            m_lblGitVersion->setStyleSheet("font-weight: bold; color: #3498DB; font-size: 13px;");
            m_lblUpdateStatus->setText("O software já está na versão mais recente disponível no GitHub.");
            m_lblUpdateStatus->setStyleSheet("color: #2ECC71; font-size: 12px;");
            m_btnApplyUpdate->setEnabled(false);
            m_btnApplyUpdate->setText("⬇ Aplicar Atualização");
        }
    });

    connect(&um, &UpdateManager::updateError, this, [this](const QString &err) {
        m_btnCheckUpdates->setEnabled(true);
        m_btnCheckUpdates->setText("🔍 Buscar Atualizações");
        m_lblGitVersion->setText("Indisponível");
        m_lblGitVersion->setStyleSheet("color: #E74C3C; font-size: 13px;");
        m_lblUpdateStatus->setText("⚠ " + err);
        m_lblUpdateStatus->setStyleSheet("color: #E74C3C; font-size: 12px;");
        m_btnApplyUpdate->setEnabled(false);
        m_btnApplyUpdate->setText("⬇ Aplicar Atualização");
    });

    um.checkForUpdates(false);
}

void SettingsDialog::onApplyGitUpdate() {
    auto confirm = QMessageBox::question(this, "Atualizar VxVoice",
        "Deseja iniciar a atualização do software agora?\n\nO aplicativo irá instalar o novo pacote e reiniciar automaticamente na nova versão, exibindo as novidades e melhorias.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (confirm != QMessageBox::Yes) {
        return;
    }

    m_btnApplyUpdate->setEnabled(false);
    m_btnApplyUpdate->setText("Atualizando...");
    m_lblUpdateStatus->setText("Baixando pacote e instalando com privilégios...");

    auto progressDlg = new UpdateProgressDialog(this);
    progressDlg->setAttribute(Qt::WA_DeleteOnClose);

    auto &um = UpdateManager::instance();
    disconnect(&um, &UpdateManager::updateCompleted, this, nullptr);
    disconnect(&um, &UpdateManager::updateProgress, progressDlg, nullptr);
    disconnect(&um, &UpdateManager::updateCompleted, progressDlg, nullptr);

    connect(&um, &UpdateManager::updateProgress, progressDlg, &UpdateProgressDialog::setProgress);
    connect(&um, &UpdateManager::updateCompleted, progressDlg, &UpdateProgressDialog::onUpdateCompleted);

    connect(&um, &UpdateManager::updateCompleted, this, [this](bool ok, const QString &msg) {
        m_btnApplyUpdate->setEnabled(UpdateManager::instance().isUpdateAvailable());
        m_btnApplyUpdate->setText("⬇ Aplicar Atualização");
        if (ok) {
            m_lblUpdateStatus->setText("✓ " + msg);
            m_lblUpdateStatus->setStyleSheet("color: #2ECC71; font-weight: bold; font-size: 12px;");
        } else {
            m_lblUpdateStatus->setText("⚠ " + msg);
            m_lblUpdateStatus->setStyleSheet("color: #E74C3C; font-size: 12px;");
        }
    });

    progressDlg->show();
    um.applyUpdate();
}

void SettingsDialog::onShowChangelog() {
    ChangelogDialog dlg(this);
    dlg.exec();
}

void SettingsDialog::loadSettings() {
    auto &sm = SettingsManager::instance();

    m_chkStartWithSystem->setChecked(sm.startWithSystem());
    m_chkStartMinimized->setChecked(sm.startMinimized());
    m_autostartSubOptions->setEnabled(sm.startWithSystem());
    m_chkMinimizeToTrayOnClose->setChecked(sm.minimizeToTrayOnClose());
    m_comboRegistrationStatus->setCurrentIndex(qBound(0, sm.registrationStatus(), 2));

    int ringIdx = m_comboRingtone->findText(sm.ringtone());
    if (ringIdx >= 0) m_comboRingtone->setCurrentIndex(ringIdx);

    // Theme selector
    QString savedTheme = sm.theme();
    auto currentTheme = ThemeManager::themeFromString(savedTheme);
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

    int regStatus = qBound(0, m_comboRegistrationStatus->currentIndex(), 2);
    sm.setRegistrationStatus(regStatus);
    if (m_chkStartWithSystem->isChecked()) {
        SipManager::instance().setPresence(static_cast<SipManager::UserPresence>(regStatus));
    }

    sm.setRingtone(m_comboRingtone->currentText());

    // Theme
    auto selectedTheme = static_cast<ThemeManager::Theme>(m_comboTheme->currentIndex());
    ThemeManager::instance().setTheme(selectedTheme);
    sm.setTheme(ThemeManager::themeToString(selectedTheme));

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
