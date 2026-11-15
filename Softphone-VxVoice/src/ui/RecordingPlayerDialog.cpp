#include "RecordingPlayerDialog.h"
#include "ThemeManager.h"
#include "SettingsManager.h"
#include "SoundManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QFile>
#include <QIcon>

RecordingPlayerDialog::RecordingPlayerDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("VxVOICE — Reprodutor de Gravação");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setModal(false);
    setMinimumWidth(430);
    resize(460, 210);
    setupUi();
    applyThemeStyling();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &RecordingPlayerDialog::applyThemeStyling);
}

RecordingPlayerDialog::~RecordingPlayerDialog() {
    stopPlayback();
}

void RecordingPlayerDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Card do player
    auto playerCard = new QFrame(this);
    playerCard->setObjectName("callInfoFrame");
    auto cardLayout = new QVBoxLayout(playerCard);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(8);

    m_lblTitle = new QLabel("🎙 Nenhuma gravação carregada", playerCard);
    m_lblTitle->setStyleSheet("font-size: 14px; font-weight: bold; background: transparent;");
    m_lblTitle->setWordWrap(true);

    m_lblFileName = new QLabel("", playerCard);
    m_lblFileName->setWordWrap(false);

    auto sliderRow = new QHBoxLayout();
    sliderRow->setSpacing(10);

    m_seekSlider = new QSlider(Qt::Horizontal, playerCard);
    m_seekSlider->setRange(0, 0);
    m_seekSlider->setEnabled(false);
    connect(m_seekSlider, &QSlider::sliderPressed, this, [this]() {
        m_isSliderPressed = true;
    });
    connect(m_seekSlider, &QSlider::sliderReleased, this, [this]() {
        m_isSliderPressed = false;
        onSliderMoved(m_seekSlider->value());
    });
    connect(m_seekSlider, &QSlider::sliderMoved, this, &RecordingPlayerDialog::onSliderMoved);

    m_lblTime = new QLabel("00:00 / 00:00", playerCard);
    m_lblTime->setStyleSheet("font-size: 12px; font-weight: bold; background: transparent;");
    m_lblTime->setMinimumWidth(90);
    m_lblTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    sliderRow->addWidget(m_seekSlider, 1);
    sliderRow->addWidget(m_lblTime);

    cardLayout->addWidget(m_lblTitle);
    cardLayout->addWidget(m_lblFileName);
    cardLayout->addLayout(sliderRow);
    mainLayout->addWidget(playerCard);

    // Barra de botões
    auto btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    const int btnHeight = 34;

    m_btnPlayPause = new QPushButton("▶ Reproduzir", this);
    m_btnPlayPause->setFixedHeight(btnHeight);
    m_btnPlayPause->setMinimumWidth(120);
    m_btnPlayPause->setEnabled(false);
    m_btnPlayPause->setCursor(Qt::PointingHandCursor);
    connect(m_btnPlayPause, &QPushButton::clicked, this, &RecordingPlayerDialog::onPlayPauseClicked);

    m_btnStop = new QPushButton("⏹ Parar", this);
    m_btnStop->setFixedHeight(btnHeight);
    m_btnStop->setMinimumWidth(88);
    m_btnStop->setEnabled(false);
    m_btnStop->setCursor(Qt::PointingHandCursor);
    connect(m_btnStop, &QPushButton::clicked, this, &RecordingPlayerDialog::onStopClicked);

    m_btnOpenFolder = new QPushButton("📂 Abrir Pasta da Gravação", this);
    m_btnOpenFolder->setFixedHeight(btnHeight);
    m_btnOpenFolder->setCursor(Qt::PointingHandCursor);
    connect(m_btnOpenFolder, &QPushButton::clicked, this, &RecordingPlayerDialog::onOpenFolderClicked);

    btnLayout->addWidget(m_btnPlayPause);
    btnLayout->addWidget(m_btnStop);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOpenFolder);

    mainLayout->addLayout(btnLayout);

#ifdef HAVE_QT_MULTIMEDIA
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(1.0f);
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        m_seekSlider->setRange(0, static_cast<int>(duration));
        m_seekSlider->setEnabled(duration > 0);
        updateTimeLabel(m_player->position(), duration);
    });

    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
        if (!m_isSliderPressed) {
            m_seekSlider->setValue(static_cast<int>(pos));
        }
        updateTimeLabel(pos, m_player->duration());
    });

    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::PlayingState) {
            m_btnPlayPause->setText("⏸ Pausar");
            m_btnPlayPause->setEnabled(true);
            m_btnStop->setEnabled(true);
        } else if (state == QMediaPlayer::PausedState) {
            m_btnPlayPause->setText("▶ Continuar");
            m_btnPlayPause->setEnabled(true);
            m_btnStop->setEnabled(true);
        } else {
            m_btnPlayPause->setText("▶ Reproduzir");
            m_btnPlayPause->setEnabled(!m_currentFilePath.isEmpty() && QFile::exists(m_currentFilePath));
            m_btnStop->setEnabled(false);
            m_seekSlider->setValue(0);
            updateTimeLabel(0, m_player->duration());
        }
    });
#endif
}

void RecordingPlayerDialog::applyThemeStyling() {
    const bool isLight = ThemeManager::isLightTheme(ThemeManager::instance().currentTheme());
    const QString subColor = isLight ? "#475569" : "#9CA3AF";
    m_lblFileName->setStyleSheet(QString("font-size: 11px; color: %1; background: transparent;").arg(subColor));
}

QString RecordingPlayerDialog::formatMs(qint64 ms) {
    if (ms < 0) ms = 0;
    qint64 totalSec = ms / 1000;
    qint64 mins = totalSec / 60;
    qint64 secs = totalSec % 60;
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

void RecordingPlayerDialog::updateTimeLabel(qint64 positionMs, qint64 durationMs) {
    m_lblTime->setText(QString("%1 / %2").arg(formatMs(positionMs), formatMs(durationMs)));
}

void RecordingPlayerDialog::playFile(const QString &filePath, const QString &displayTitle) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        QMessageBox::warning(this, "Gravação não Encontrada",
                             "O arquivo de áudio desta chamada não pôde ser localizado.");
        return;
    }

    QFileInfo fi(filePath);
    QString title = !displayTitle.isEmpty() ? displayTitle : fi.completeBaseName();

    m_currentFilePath = filePath;
    m_lblTitle->setText(QString("🎙 %1").arg(title));
    m_lblFileName->setText(fi.fileName());
    m_btnPlayPause->setEnabled(true);

#ifdef HAVE_QT_MULTIMEDIA
    if (m_player) {
        m_player->stop();
        m_player->setSource(QUrl::fromLocalFile(filePath));
        m_player->play();
        return;
    }
#endif
    m_btnPlayPause->setText("⏸ Pausar");
    m_btnStop->setEnabled(true);
    SoundManager::instance().playAudioFile(filePath, false);
}

void RecordingPlayerDialog::stopPlayback() {
#ifdef HAVE_QT_MULTIMEDIA
    if (m_player) {
        m_player->stop();
        m_player->setSource(QUrl());
    }
#endif
    SoundManager::instance().stopRinging();
    m_seekSlider->setRange(0, 0);
    m_seekSlider->setEnabled(false);
    updateTimeLabel(0, 0);
    m_btnStop->setEnabled(false);
    m_btnPlayPause->setText("▶ Reproduzir");
    m_btnPlayPause->setEnabled(!m_currentFilePath.isEmpty() && QFile::exists(m_currentFilePath));
}

void RecordingPlayerDialog::stopIfPlaying(const QString &filePath) {
    if (!m_currentFilePath.isEmpty() && m_currentFilePath == filePath) {
        stopPlayback();
        m_currentFilePath.clear();
        m_lblTitle->setText("🎙 Nenhuma gravação carregada");
        m_lblFileName->setText("");
        m_btnPlayPause->setEnabled(false);
        hide();
    }
}

void RecordingPlayerDialog::onPlayPauseClicked() {
    if (m_currentFilePath.isEmpty() || !QFile::exists(m_currentFilePath)) {
        return;
    }

#ifdef HAVE_QT_MULTIMEDIA
    if (m_player) {
        if (m_player->playbackState() == QMediaPlayer::PlayingState) {
            m_player->pause();
            return;
        }
        if (m_player->playbackState() == QMediaPlayer::PausedState) {
            m_player->play();
            return;
        }
        m_player->setSource(QUrl::fromLocalFile(m_currentFilePath));
        m_player->play();
        return;
    }
#endif
    SoundManager::instance().playAudioFile(m_currentFilePath, false);
}

void RecordingPlayerDialog::onStopClicked() {
#ifdef HAVE_QT_MULTIMEDIA
    if (m_player) {
        m_player->stop();
    }
#endif
    SoundManager::instance().stopRinging();
}

void RecordingPlayerDialog::onOpenFolderClicked() {
    QString folderPath;
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fi(m_currentFilePath);
        if (fi.dir().exists()) {
            folderPath = fi.dir().absolutePath();
        }
    }
    if (folderPath.isEmpty()) {
        folderPath = SettingsManager::instance().recordingPath();
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

void RecordingPlayerDialog::onSliderMoved(int positionMs) {
#ifdef HAVE_QT_MULTIMEDIA
    if (m_player && m_player->isSeekable()) {
        m_player->setPosition(static_cast<qint64>(positionMs));
    }
#else
    Q_UNUSED(positionMs);
#endif
}

void RecordingPlayerDialog::closeEvent(QCloseEvent *event) {
    stopPlayback();
    QDialog::closeEvent(event);
}

void RecordingPlayerDialog::reject() {
    stopPlayback();
    QDialog::reject();
}
