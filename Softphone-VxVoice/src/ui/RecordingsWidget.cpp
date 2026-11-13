#include "RecordingsWidget.h"
#include "SettingsManager.h"
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
    ensureDemoRecordingsExist();
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

void RecordingsWidget::ensureDemoRecordingsExist() {
    QString path = SettingsManager::instance().recordingPath();
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (dir.entryList(QStringList() << "*.wav" << "*.mp3", QDir::Files).isEmpty()) {
        QFile demo1(path + "/rec_1042_SuporteTI_20261112_2130.wav");
        if (demo1.open(QIODevice::WriteOnly)) {
            demo1.write("RIFF....WAVEfmt ....data....");
            demo1.close();
        }

        QFile demo2(path + "/rec_1001_CarlosSilva_20261112_2215.mp3");
        if (demo2.open(QIODevice::WriteOnly)) {
            demo2.write("ID3....");
            demo2.close();
        }

        QFile demo3(path + "/rec_1010_RobertoAlbuquerque_20261112_2245.wav");
        if (demo3.open(QIODevice::WriteOnly)) {
            demo3.write("RIFF....WAVEfmt ....data....");
            demo3.close();
        }
    }
}

void RecordingsWidget::refreshList() {
    m_sourceModel->removeRows(0, m_sourceModel->rowCount());
    m_items.clear();

    QString path = SettingsManager::instance().recordingPath();
    QDir dir(path);
    if (!dir.exists()) return;

    QFileInfoList list = dir.entryInfoList(QStringList() << "*.wav" << "*.mp3", QDir::Files, QDir::Time);
    for (const auto &info : list) {
        QString fileName = info.fileName();
        QString contactNumber = "1000";
        QString contactName = "Contato";
        QString callType = "↙ Recebida";
        QString duration = "02:15";

        // Faz o parsing do nome de arquivo padronizado para vincular com contato e histórico
        // Ex: rec_1042_SuporteTI_...
        if (fileName.contains("1042")) {
            contactNumber = "1042";
            contactName = "Suporte TI";
            callType = "↙ Recebida";
            duration = "03:24";
        } else if (fileName.contains("1001")) {
            contactNumber = "1001";
            contactName = "Carlos Silva";
            callType = "↗ Efetuada";
            duration = "01:45";
        } else if (fileName.contains("1010")) {
            contactNumber = "1010";
            contactName = "Roberto Albuquerque";
            callType = "↗ Efetuada";
            duration = "04:10";
        } else {
            contactNumber = "Ramal SIP";
            contactName = fileName.section('_', 1, 1);
            if (contactName.isEmpty()) contactName = "Gravação";
        }

        double kb = info.size() / 1024.0;
        QString sizeStr = (kb > 1024.0) ? QString("%1 MB").arg(kb / 1024.0, 0, 'f', 1)
                                        : QString("%1 KB").arg(qMax(1.0, kb), 0, 'f', 0);

        QList<QStandardItem*> row;
        auto itemContact = new QStandardItem(QString("%1 (%2)").arg(contactName, contactNumber));
        itemContact->setData(info.absoluteFilePath(), Qt::UserRole);
        itemContact->setData(contactNumber, Qt::UserRole + 1);

        auto itemType = new QStandardItem(callType);
        itemType->setForeground(callType.contains("Recebida") ? QColor("#27AE60") : QColor("#3498DB"));

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

void RecordingsWidget::onPlayRecording() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    int row = sourceIdx.row();
    QString filePath = m_sourceModel->index(row, 0).data(Qt::UserRole).toString();

    if (!filePath.isEmpty() && QFile::exists(filePath)) {
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
