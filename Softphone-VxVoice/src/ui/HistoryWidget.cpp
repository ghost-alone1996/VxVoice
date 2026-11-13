#include "HistoryWidget.h"
#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>
#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>

HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent) {
    setupUi();
    populateSampleData();
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
}

void HistoryWidget::populateSampleData() {
    addRecord(HistoryCallType::Incoming, "1042", "Suporte TI", "03:24");
    addRecord(HistoryCallType::Outgoing, "2001", "Diretoria", "01:15");
    addRecord(HistoryCallType::Missed, "08007070", "Cliente Externo", "00:00");
    addRecord(HistoryCallType::Incoming, "1002", "Recepção Central", "00:45");
}

void HistoryWidget::onItemDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;
    int row = index.row();
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
            QDesktopServices::openUrl(QUrl::fromLocalFile(recPath));
            emit playRecordingRequested(recPath);
        }
    });

    menu.addSeparator();

    auto actionDelete = menu.addAction("🗑 Excluir do Histórico");
    connect(actionDelete, &QAction::triggered, this, [this, row]() {
        m_model->removeRow(row);
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
    }
}
