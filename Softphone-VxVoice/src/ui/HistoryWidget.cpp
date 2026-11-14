#include "HistoryWidget.h"
#include "SoundManager.h"
#include "SettingsManager.h"
#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>
#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent) {
    setupUi();
    loadHistory();
}

void HistoryWidget::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    m_tableView = new QTableView(this);
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"Tipo", "Contato/Número", "Data/Hora", "Duração", "Gravação"});

    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setStretchLastSection(false);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Habilita menu de contexto ao clicar com botão direito
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested, this, &HistoryWidget::onContextMenu);
    connect(m_tableView, &QTableView::doubleClicked, this, &HistoryWidget::onItemDoubleClicked);

    mainLayout->addWidget(m_tableView);

    // Bottom action bar
    auto bottomLayout = new QHBoxLayout();
    m_btnClear = new QPushButton("Limpar Histórico", this);
    m_btnClear->setCursor(Qt::PointingHandCursor);
    connect(m_btnClear, &QPushButton::clicked, this, &HistoryWidget::onClearHistory);

    bottomLayout->addStretch();
    bottomLayout->addWidget(m_btnClear);
    mainLayout->addLayout(bottomLayout);
}

void HistoryWidget::addRecord(HistoryCallType type, const QString &number, const QString &name,
                             const QString &duration, const QString &recordingPath) {
    QList<QStandardItem*> row;

    QString typeStr;
    QColor typeColor;
    if (type == HistoryCallType::Incoming) {
        typeStr = "↙ Recebida";
        typeColor = QColor("#27AE60");
    } else if (type == HistoryCallType::Outgoing) {
        typeStr = "↗ Efetuada";
        typeColor = QColor("#3498DB");
    } else {
        typeStr = "✕ Perdida";
        typeColor = QColor("#E74C3C");
    }

    auto itemType = new QStandardItem(typeStr);
    itemType->setForeground(typeColor);

    QString displayContact = name.isEmpty() ? number : QString("%1 (%2)").arg(name, number);
    auto itemContact = new QStandardItem(displayContact);
    itemContact->setData(number, Qt::UserRole);
    itemContact->setData(name, Qt::UserRole + 1);

    auto itemTime = new QStandardItem(QDateTime::currentDateTime().toString("dd/MM hh:mm"));
    auto itemDuration = new QStandardItem(duration.isEmpty() ? "00:00" : duration);

    bool hasRecording = !recordingPath.isEmpty() && QFile::exists(recordingPath);
    auto itemRec = new QStandardItem(hasRecording ? "🎙 Sim" : "—");
    itemRec->setData(recordingPath, Qt::UserRole);
    if (hasRecording) {
        itemRec->setForeground(QColor("#E67E22"));
    }

    row << itemType << itemContact << itemTime << itemDuration << itemRec;
    m_model->insertRow(0, row);
    adjustTableColumns();
    saveHistory();
}

void HistoryWidget::updateLatestRecord(const QString &duration, const QString &recordingPath) {
    if (m_model->rowCount() == 0) return;
    if (!duration.isEmpty()) {
        m_model->item(0, 3)->setText(duration);
    }
    if (!recordingPath.isEmpty() && QFile::exists(recordingPath)) {
        m_model->item(0, 4)->setText("🎙 Sim");
        m_model->item(0, 4)->setData(recordingPath, Qt::UserRole);
        m_model->item(0, 4)->setForeground(QColor("#E67E22"));
    }
    adjustTableColumns();
    saveHistory();
}

void HistoryWidget::adjustTableColumns() {
    if (!m_tableView || !m_model) return;
    auto header = m_tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setStretchLastSection(false);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->resizeColumnsToContents();
}

void HistoryWidget::loadHistory() {
    QSettings settings("VxVoiceCorp", "VxVoice");
    int size = settings.beginReadArray("CallHistory");
    m_model->removeRows(0, m_model->rowCount());

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        int typeVal = settings.value("type", 0).toInt();
        HistoryCallType type = static_cast<HistoryCallType>(typeVal);
        QString number = settings.value("number").toString();
        QString name = settings.value("name").toString();
        QString time = settings.value("time").toString();
        QString duration = settings.value("duration").toString();
        QString recPath = settings.value("recordingPath").toString();

        QString typeStr;
        QColor typeColor;
        if (type == HistoryCallType::Incoming) {
            typeStr = "↙ Recebida";
            typeColor = QColor("#27AE60");
        } else if (type == HistoryCallType::Outgoing) {
            typeStr = "↗ Efetuada";
            typeColor = QColor("#3498DB");
        } else {
            typeStr = "✕ Perdida";
            typeColor = QColor("#E74C3C");
        }

        auto itemType = new QStandardItem(typeStr);
        itemType->setForeground(typeColor);

        QString displayContact = name.isEmpty() ? number : QString("%1 (%2)").arg(name, number);
        auto itemContact = new QStandardItem(displayContact);
        itemContact->setData(number, Qt::UserRole);
        itemContact->setData(name, Qt::UserRole + 1);

        auto itemTime = new QStandardItem(time.isEmpty() ? QDateTime::currentDateTime().toString("dd/MM hh:mm") : time);
        auto itemDuration = new QStandardItem(duration.isEmpty() ? "00:00" : duration);

        bool hasRecording = !recPath.isEmpty() && QFile::exists(recPath);
        auto itemRec = new QStandardItem(hasRecording ? "🎙 Sim" : "—");
        itemRec->setData(recPath, Qt::UserRole);
        if (hasRecording) {
            itemRec->setForeground(QColor("#E67E22"));
        }

        QList<QStandardItem*> row;
        row << itemType << itemContact << itemTime << itemDuration << itemRec;
        m_model->appendRow(row);
    }
    settings.endArray();
}

void HistoryWidget::saveHistory() {
    QSettings settings("VxVoiceCorp", "VxVoice");
    settings.remove("CallHistory");
    int count = qMin(m_model->rowCount(), 200); // Salva até 200 registros mais recentes
    settings.beginWriteArray("CallHistory", count);

    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        QString typeText = m_model->item(i, 0)->text();
        int typeVal = 0;
        if (typeText.contains("Efetuada")) typeVal = 1;
        else if (typeText.contains("Perdida")) typeVal = 2;

        settings.setValue("type", typeVal);
        settings.setValue("number", m_model->item(i, 1)->data(Qt::UserRole).toString());
        settings.setValue("name", m_model->item(i, 1)->data(Qt::UserRole + 1).toString());
        settings.setValue("time", m_model->item(i, 2)->text());
        settings.setValue("duration", m_model->item(i, 3)->text());
        settings.setValue("recordingPath", m_model->item(i, 4)->data(Qt::UserRole).toString());
    }
    settings.endArray();
    settings.sync();
}

void HistoryWidget::onItemDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;
    int row = index.row();

    // Se clicar diretamente na coluna de gravação e ela existir, reproduz
    if (index.column() == 4) {
        QString recPath = m_model->item(row, 4)->data(Qt::UserRole).toString();
        if (!recPath.isEmpty() && QFile::exists(recPath)) {
            SoundManager::instance().playAudioFile(recPath, false);
            QDesktopServices::openUrl(QUrl::fromLocalFile(recPath));
            emit playRecordingRequested(recPath);
            return;
        }
    }

    QModelIndex contactIndex = m_model->index(row, 1);
    QString number = contactIndex.data(Qt::UserRole).toString();

    if (!number.isEmpty()) {
        emit callNumberRequested(number);
    }
}

void HistoryWidget::onContextMenu(const QPoint &pos) {
    QModelIndex index = m_tableView->indexAt(pos);
    if (!index.isValid()) return;

    int row = index.row();
    QModelIndex contactIndex = m_model->index(row, 1);
    QModelIndex recIndex = m_model->index(row, 4);

    QString number = contactIndex.data(Qt::UserRole).toString();
    QString name = contactIndex.data(Qt::UserRole + 1).toString();
    QString recPath = recIndex.data(Qt::UserRole).toString();
    bool hasRecording = !recPath.isEmpty() && QFile::exists(recPath);

    // Se o caminho direto não foi encontrado, busca na pasta de gravações pelo número ou timestamp
    if (!hasRecording && !number.isEmpty()) {
        QString recDir = SettingsManager::instance().recordingPath();
        QDir dir(recDir);
        QStringList filters;
        filters << QString("*%1*.wav").arg(number);
        auto matches = dir.entryInfoList(filters, QDir::Files, QDir::Time);
        if (!matches.isEmpty()) {
            recPath = matches.first().absoluteFilePath();
            hasRecording = true;
            m_model->item(row, 4)->setText("🎙 Sim");
            m_model->item(row, 4)->setData(recPath, Qt::UserRole);
            m_model->item(row, 4)->setForeground(QColor("#E67E22"));
        }
    }

    QMenu menu(this);

    auto actionCall = menu.addAction(QString("📞 Ligar para %1").arg(number));
    connect(actionCall, &QAction::triggered, this, [this, number]() {
        emit callNumberRequested(number);
    });

    auto actionSave = menu.addAction("👤 Salvar nos Contatos");
    connect(actionSave, &QAction::triggered, this, [this, name, number]() {
        emit saveContactRequested(name, number);
    });

    auto actionCopy = menu.addAction("📋 Copiar Número");
    connect(actionCopy, &QAction::triggered, this, [number]() {
        QGuiApplication::clipboard()->setText(number);
    });

    auto actionPlay = menu.addAction("🎙 Ouvir Gravação");
    actionPlay->setEnabled(hasRecording);
    connect(actionPlay, &QAction::triggered, this, [this, recPath]() {
        if (!recPath.isEmpty() && QFile::exists(recPath)) {
            // Reproduz via SoundManager nos alto-falantes e abre player padrão do SO
            SoundManager::instance().playAudioFile(recPath, false);
            QDesktopServices::openUrl(QUrl::fromLocalFile(recPath));
            emit playRecordingRequested(recPath);
        } else {
            QMessageBox::warning(this, "Gravação não Encontrada",
                                 "O arquivo de áudio desta chamada não pôde ser localizado no diretório de gravações.");
        }
    });

    if (hasRecording) {
        auto actionFolder = menu.addAction("📂 Abrir Pasta da Gravação");
        connect(actionFolder, &QAction::triggered, this, [recPath]() {
            QFileInfo fi(recPath);
            QDesktopServices::openUrl(QUrl::fromLocalFile(fi.dir().absolutePath()));
        });
    }

    menu.addSeparator();

    auto actionDelete = menu.addAction("🗑 Excluir do Histórico");
    connect(actionDelete, &QAction::triggered, this, [this, row]() {
        m_model->removeRow(row);
        saveHistory();
    });

    menu.exec(m_tableView->viewport()->mapToGlobal(pos));
}

void HistoryWidget::onClearHistory() {
    if (m_model->rowCount() == 0) return;

    auto reply = QMessageBox::question(this, "Limpar Histórico",
                                       "Deseja realmente apagar todo o histórico de chamadas?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_model->removeRows(0, m_model->rowCount());
        QSettings settings("VxVoiceCorp", "VxVoice");
        settings.remove("CallHistory");
        settings.sync();
    }
}
