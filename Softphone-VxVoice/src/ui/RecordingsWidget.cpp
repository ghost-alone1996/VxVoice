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
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setStretchLastSection(false);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RecordingsWidget::onSelectionChanged);
    connect(m_tableView, &QTableView::doubleClicked, this, &RecordingsWidget::onPlayRecording);

    mainLayout->addWidget(m_tableView);

    // Action Toolbar
    auto btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);

    const int actionBtnHeight = 32;

    m_btnPlay = new QPushButton("▶ Reproduzir", this);
    m_btnPlay->setFixedHeight(actionBtnHeight);
    m_btnPlay->setEnabled(false);
    m_btnPlay->setCursor(Qt::PointingHandCursor);
    connect(m_btnPlay, &QPushButton::clicked, this, &RecordingsWidget::onPlayRecording);

    m_btnCall = new QPushButton("📞 Ligar", this);
    m_btnCall->setObjectName("contactCallButton");
    m_btnCall->setFixedHeight(actionBtnHeight);
    m_btnCall->setEnabled(false);
    m_btnCall->setCursor(Qt::PointingHandCursor);
    connect(m_btnCall, &QPushButton::clicked, this, &RecordingsWidget::onCallContact);

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
    btnLayout->addWidget(m_btnCall);
    btnLayout->addWidget(m_btnOpenFolder);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnDelete);

    mainLayout->addLayout(btnLayout);
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
        QString fileName = info.fileName();
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
    m_btnPlay->setEnabled(has);
    m_btnCall->setEnabled(has);
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

void RecordingsWidget::onPlayRecording() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    int row = sourceIdx.row();
    QString filePath = m_sourceModel->index(row, 0).data(Qt::UserRole).toString();

    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        SoundManager::instance().playAudioFile(filePath, false);
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    }
}

void RecordingsWidget::onCallContact() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    int row = sourceIdx.row();
    QString number = m_sourceModel->index(row, 0).data(Qt::UserRole + 1).toString();

    if (!number.isEmpty()) {
        emit callRequested(number);
    }
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
        QFile::remove(filePath);
        refreshList();
    }
}
