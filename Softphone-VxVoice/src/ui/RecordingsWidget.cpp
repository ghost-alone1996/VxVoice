#include "RecordingsWidget.h"
#include "SettingsManager.h"
#include "SoundManager.h"
#include <QHeaderView>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>

RecordingsWidget::RecordingsWidget(QWidget *parent)
    : QWidget(parent) {
    setupUi();
    refreshList();
}

RecordingsWidget::~RecordingsWidget() {
#ifdef HAVE_QT_MULTIMEDIA
    if (m_player) {
        m_player->stop();
        m_player->setSource(QUrl());
    }
#endif
}

void RecordingsWidget::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Search bar
    auto searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔍 Filtrar gravações por contato, ramal ou data...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &RecordingsWidget::onSearchTextChanged);

    m_btnRefresh = new QPushButton("🔄", this);
    m_btnRefresh->setFixedWidth(34);
    m_btnRefresh->setToolTip("Atualizar lista de gravações");
    m_btnRefresh->setCursor(Qt::PointingHandCursor);
    connect(m_btnRefresh, &QPushButton::clicked, this, &RecordingsWidget::refreshList);

    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(m_btnRefresh);
    mainLayout->addLayout(searchLayout);

    // Table & Model
    m_sourceModel = new QStandardItemModel(this);
    m_sourceModel->setHorizontalHeaderLabels({"Contato / Ramal", "Tipo", "Data e Hora", "Duração", "Tamanho"});

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_sourceModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);

    m_tableView = new QTableView(this);
    m_tableView->setModel(m_proxyModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setAutoScroll(false);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setStretchLastSection(false);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RecordingsWidget::onSelectionChanged);
    connect(m_tableView, &QTableView::doubleClicked, this, &RecordingsWidget::onPlayRecording);

    mainLayout->addWidget(m_tableView, 1);

    // Embedded Audio Player Card
    m_playerFrame = new QFrame(this);
    m_playerFrame->setObjectName("callInfoFrame");
    auto playerLayout = new QVBoxLayout(m_playerFrame);
    playerLayout->setContentsMargins(8, 6, 8, 6);
    playerLayout->setSpacing(4);

    m_lblNowPlaying = new QLabel("🎙 Selecione uma gravação para ouvir no aplicativo", m_playerFrame);
    m_lblNowPlaying->setStyleSheet("font-size: 12px; font-weight: bold; background: transparent;");
    m_lblNowPlaying->setWordWrap(false);

    auto sliderRow = new QHBoxLayout();
    sliderRow->setSpacing(8);

    m_seekSlider = new QSlider(Qt::Horizontal, m_playerFrame);
    m_seekSlider->setRange(0, 0);
    m_seekSlider->setEnabled(false);
    connect(m_seekSlider, &QSlider::sliderPressed, this, [this]() {
        m_isSliderPressed = true;
    });
    connect(m_seekSlider, &QSlider::sliderReleased, this, [this]() {
        m_isSliderPressed = false;
        onSliderMoved(m_seekSlider->value());
    });
    connect(m_seekSlider, &QSlider::sliderMoved, this, &RecordingsWidget::onSliderMoved);

    m_lblTime = new QLabel("00:00 / 00:00", m_playerFrame);
    m_lblTime->setStyleSheet("font-size: 11px; font-weight: bold; background: transparent;");
    m_lblTime->setMinimumWidth(84);
    m_lblTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    sliderRow->addWidget(m_seekSlider, 1);
    sliderRow->addWidget(m_lblTime);

    playerLayout->addWidget(m_lblNowPlaying);
    playerLayout->addLayout(sliderRow);
    mainLayout->addWidget(m_playerFrame);

    // Action Toolbar
    auto btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);

    const int actionBtnHeight = 32;

    m_btnPlay = new QPushButton("▶ Reproduzir", this);
    m_btnPlay->setFixedHeight(actionBtnHeight);
    m_btnPlay->setEnabled(false);
    m_btnPlay->setCursor(Qt::PointingHandCursor);
    connect(m_btnPlay, &QPushButton::clicked, this, &RecordingsWidget::onPlayRecording);

    m_btnStop = new QPushButton("⏹ Parar", this);
    m_btnStop->setFixedHeight(actionBtnHeight);
    m_btnStop->setEnabled(false);
    m_btnStop->setCursor(Qt::PointingHandCursor);
    connect(m_btnStop, &QPushButton::clicked, this, &RecordingsWidget::onStopPlayback);

    m_btnOpenFolder = new QPushButton("📂 Abrir Pasta", this);
    m_btnOpenFolder->setFixedHeight(actionBtnHeight);
    m_btnOpenFolder->setCursor(Qt::PointingHandCursor);
    connect(m_btnOpenFolder, &QPushButton::clicked, this, &RecordingsWidget::onOpenFolder);

    m_btnDelete = new QPushButton("🗑 Excluir", this);
    m_btnDelete->setFixedHeight(actionBtnHeight);
    m_btnDelete->setEnabled(false);
    m_btnDelete->setCursor(Qt::PointingHandCursor);
    connect(m_btnDelete, &QPushButton::clicked, this, &RecordingsWidget::onDeleteRecording);

    btnLayout->addWidget(m_btnPlay);
    btnLayout->addWidget(m_btnStop);
    btnLayout->addWidget(m_btnOpenFolder);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnDelete);

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
            m_btnPlay->setText("⏸ Pausar");
            m_btnPlay->setEnabled(true);
            m_btnStop->setEnabled(true);
        } else if (state == QMediaPlayer::PausedState) {
            m_btnPlay->setText("▶ Continuar");
            m_btnPlay->setEnabled(true);
            m_btnStop->setEnabled(true);
        } else {
            m_btnPlay->setText("▶ Reproduzir");
            m_btnStop->setEnabled(false);
            m_seekSlider->setValue(0);
            updateTimeLabel(0, m_player->duration());
            onSelectionChanged();
        }
    });
#endif
}

QString RecordingsWidget::formatMs(qint64 ms) {
    if (ms < 0) ms = 0;
    qint64 totalSec = ms / 1000;
    qint64 mins = totalSec / 60;
    qint64 secs = totalSec % 60;
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

void RecordingsWidget::updateTimeLabel(qint64 positionMs, qint64 durationMs) {
    if (m_lblTime) {
        m_lblTime->setText(QString("%1 / %2").arg(formatMs(positionMs), formatMs(durationMs)));
    }
}

void RecordingsWidget::refreshList() {
    m_sourceModel->removeRows(0, m_sourceModel->rowCount());
    m_items.clear();

    QString path = SettingsManager::instance().recordingPath();
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
        return;
    }

    QFileInfoList list = dir.entryInfoList(QStringList() << "*.wav" << "*.mp3", QDir::Files, QDir::Time);
    for (const auto &info : list) {
        QString contactNumber = "";
        QString contactName = info.baseName();
        QString callType = "Chamada Gravada";
        QString duration = "--:--";

        // Ex: rec_<numero>_<nome>_<timestamp>.wav
        QStringList parts = info.baseName().split('_');
        if (parts.size() >= 3 && parts[0] == "rec") {
            contactNumber = parts[1];
            contactName = parts[2];
        } else if (parts.size() >= 2) {
            contactNumber = parts[0];
            contactName = parts[1];
        }

        double kb = info.size() / 1024.0;
        QString sizeStr = (kb > 1024.0) ? QString("%1 MB").arg(kb / 1024.0, 0, 'f', 1)
                                        : QString("%1 KB").arg(qMax(1.0, kb), 0, 'f', 0);

        QList<QStandardItem*> row;
        QString displayTitle = contactNumber.isEmpty() ? contactName : QString("%1 (%2)").arg(contactName, contactNumber);
        auto itemContact = new QStandardItem(displayTitle);
        itemContact->setData(info.absoluteFilePath(), Qt::UserRole);
        itemContact->setData(contactNumber, Qt::UserRole + 1);

        auto itemType = new QStandardItem(callType);
        itemType->setForeground(QColor("#3498DB"));

        auto itemDate = new QStandardItem(info.lastModified().toString("dd/MM/yyyy hh:mm"));
        auto itemDuration = new QStandardItem(duration);
        auto itemSize = new QStandardItem(sizeStr);

        row << itemContact << itemType << itemDate << itemDuration << itemSize;
        m_sourceModel->appendRow(row);
    }

    onSelectionChanged();
}

void RecordingsWidget::addRecordingRecord(const QString &filePath, const QString &number,
                                        const QString &name, const QString &callType, const QString &duration) {
    QFileInfo info(filePath);
    double kb = info.size() / 1024.0;
    QString sizeStr = (kb > 1024.0) ? QString("%1 MB").arg(kb / 1024.0, 0, 'f', 1)
                                    : QString("%1 KB").arg(qMax(1.0, kb), 0, 'f', 0);

    QList<QStandardItem*> row;
    auto itemContact = new QStandardItem(QString("%1 (%2)").arg(name, number));
    itemContact->setData(filePath, Qt::UserRole);
    itemContact->setData(number, Qt::UserRole + 1);

    auto itemType = new QStandardItem(callType);
    itemType->setForeground(callType.contains("Recebida") ? QColor("#27AE60") : QColor("#3498DB"));

    auto itemDate = new QStandardItem(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"));
    auto itemDuration = new QStandardItem(duration);
    auto itemSize = new QStandardItem(sizeStr);

    row << itemContact << itemType << itemDate << itemDuration << itemSize;
    m_sourceModel->insertRow(0, row);
}

void RecordingsWidget::onSearchTextChanged(const QString &text) {
    m_proxyModel->setFilterFixedString(text);
}

void RecordingsWidget::onSelectionChanged() {
    bool has = m_tableView->selectionModel() && m_tableView->selectionModel()->hasSelection();
#ifdef HAVE_QT_MULTIMEDIA
    bool isPlayingOrPaused = m_player && (m_player->playbackState() != QMediaPlayer::StoppedState);
    m_btnPlay->setEnabled(has || isPlayingOrPaused);
#else
    m_btnPlay->setEnabled(has);
#endif
    m_btnDelete->setEnabled(has);
}

void RecordingsWidget::adjustTableColumns() {
    if (!m_tableView || !m_sourceModel) return;
    auto header = m_tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setStretchLastSection(false);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->resizeColumnsToContents();
}

void RecordingsWidget::playFile(const QString &filePath, const QString &displayTitle) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        QMessageBox::warning(this, "Gravação não Encontrada",
                             "O arquivo de áudio desta chamada não pôde ser localizado.");
        return;
    }

    QString label = !displayTitle.isEmpty() ? displayTitle : QFileInfo(filePath).fileName();
    m_currentPlayingPath = filePath;
    m_lblNowPlaying->setText(QString("▶ Reproduzindo: %1").arg(label));

#ifdef HAVE_QT_MULTIMEDIA
    if (m_player) {
        m_player->stop();
        m_player->setSource(QUrl::fromLocalFile(filePath));
        m_player->play();
        return;
    }
#endif
    SoundManager::instance().playAudioFile(filePath, false);
}

void RecordingsWidget::onPlayRecording() {
#ifdef HAVE_QT_MULTIMEDIA
    auto selected = m_tableView->selectionModel()->selectedRows();
    QString selectedPath;
    QString selectedTitle;
    if (!selected.isEmpty()) {
        QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
        int row = sourceIdx.row();
        selectedPath = m_sourceModel->index(row, 0).data(Qt::UserRole).toString();
        selectedTitle = m_sourceModel->index(row, 0).data(Qt::DisplayRole).toString();
    }

    if (m_player) {
        // Se já está tocando o mesmo arquivo (ou nenhum selecionado), alterna Pause/Resume
        if (m_player->playbackState() == QMediaPlayer::PlayingState &&
            (selectedPath.isEmpty() || selectedPath == m_currentPlayingPath)) {
            m_player->pause();
            return;
        }
        if (m_player->playbackState() == QMediaPlayer::PausedState &&
            (selectedPath.isEmpty() || selectedPath == m_currentPlayingPath)) {
            m_player->play();
            return;
        }
    }

    if (!selectedPath.isEmpty()) {
        playFile(selectedPath, selectedTitle);
    }
#else
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    int row = sourceIdx.row();
    QString filePath = m_sourceModel->index(row, 0).data(Qt::UserRole).toString();
    QString title = m_sourceModel->index(row, 0).data(Qt::DisplayRole).toString();
    playFile(filePath, title);
#endif
}

void RecordingsWidget::onStopPlayback() {
#ifdef HAVE_QT_MULTIMEDIA
    if (m_player) {
        m_player->stop();
    }
#endif
    SoundManager::instance().stopRinging();
    m_lblNowPlaying->setText("⏹ Reprodução parada");
}

void RecordingsWidget::onSliderMoved(int positionMs) {
#ifdef HAVE_QT_MULTIMEDIA
    if (m_player && m_player->isSeekable()) {
        m_player->setPosition(static_cast<qint64>(positionMs));
    }
#else
    Q_UNUSED(positionMs);
#endif
}

void RecordingsWidget::onOpenFolder() {
    QString path = SettingsManager::instance().recordingPath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void RecordingsWidget::onDeleteRecording() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    int row = sourceIdx.row();
    QString filePath = m_sourceModel->index(row, 0).data(Qt::UserRole).toString();

    auto reply = QMessageBox::question(this, "Excluir Gravação",
                                       "Deseja realmente remover esta gravação?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
#ifdef HAVE_QT_MULTIMEDIA
        if (m_player && m_currentPlayingPath == filePath) {
            m_player->stop();
            m_player->setSource(QUrl());
            m_currentPlayingPath.clear();
            m_lblNowPlaying->setText("🎙 Selecione uma gravação para ouvir no aplicativo");
            m_seekSlider->setRange(0, 0);
            m_seekSlider->setEnabled(false);
            updateTimeLabel(0, 0);
        }
#endif
        QFile::remove(filePath);
        refreshList();
    }
}
