#include "AccountsDialog.h"
#include "SipManager.h"
#include "IconHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>

// ===================================================================
// AccountEditDialog Implementation
// ===================================================================

AccountEditDialog::AccountEditDialog(const SipAccountConfig &account, QWidget *parent)
    : QDialog(parent),
      m_accountId(account.id.isEmpty() ? QString("acc_%1").arg(QDateTime::currentMSecsSinceEpoch()) : account.id),
      m_isDefault(account.isDefault) {
    setWindowTitle(account.accountName.isEmpty() ? "Criar Conta SIP" : "Editar Conta SIP");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    resize(420, 380);
    setupUi();

    m_editName->setText(account.accountName);
    m_editServer->setText(account.server);
    m_spinPort->setValue(account.port > 0 ? account.port : 5060);
    m_editUsername->setText(account.username);
    m_editPassword->setText(account.password);
    m_editDomain->setText(account.domain);
    int transIdx = m_comboTransport->findText(account.transport);
    if (transIdx >= 0) m_comboTransport->setCurrentIndex(transIdx);
    m_chkEnabled->setChecked(account.isEnabled);
}

void AccountEditDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    auto formLayout = new QFormLayout();

    m_editName = new QLineEdit(this);
    m_editName->setPlaceholderText("Ex: Ramal Comercial");

    m_editServer = new QLineEdit(this);
    m_editServer->setPlaceholderText("Ex: pbx.empresa.com.br ou 192.168.1.100");

    m_spinPort = new QSpinBox(this);
    m_spinPort->setRange(1, 65535);
    m_spinPort->setValue(5060);

    m_editUsername = new QLineEdit(this);
    m_editUsername->setPlaceholderText("Ex: 1001");

    m_editPassword = new QLineEdit(this);
    m_editPassword->setEchoMode(QLineEdit::Password);

    m_editDomain = new QLineEdit(this);
    m_editDomain->setPlaceholderText("Ex: empresa.com.br");

    m_comboTransport = new QComboBox(this);
    m_comboTransport->addItems({"UDP", "TCP", "TLS"});

    m_chkEnabled = new QCheckBox("Habilitar esta conta imediatamente", this);
    m_chkEnabled->setChecked(true);

    formLayout->addRow("Nome de Exibição:", m_editName);
    formLayout->addRow("Servidor SIP / Host:*", m_editServer);
    formLayout->addRow("Porta SIP:", m_spinPort);
    formLayout->addRow("Usuário / Ramal:*", m_editUsername);
    formLayout->addRow("Senha:*", m_editPassword);
    formLayout->addRow("Domínio PBX:", m_editDomain);
    formLayout->addRow("Transporte:", m_comboTransport);
    formLayout->addRow("", m_chkEnabled);

    mainLayout->addLayout(formLayout);

    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto btnCancel = new QPushButton("Cancelar", this);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    auto btnSave = new QPushButton("Salvar Conta", this);
    btnSave->setObjectName("primaryButton");
    btnSave->setDefault(true);
    btnSave->setAutoDefault(true);
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        if (m_editServer->text().trimmed().isEmpty() || m_editUsername->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validação", "Servidor SIP e Usuário/Ramal são obrigatórios.");
            return;
        }
        accept();
    });

    connect(m_editName, &QLineEdit::returnPressed, btnSave, &QPushButton::click);
    connect(m_editServer, &QLineEdit::returnPressed, btnSave, &QPushButton::click);
    connect(m_editUsername, &QLineEdit::returnPressed, btnSave, &QPushButton::click);
    connect(m_editPassword, &QLineEdit::returnPressed, btnSave, &QPushButton::click);
    connect(m_editDomain, &QLineEdit::returnPressed, btnSave, &QPushButton::click);

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    mainLayout->addLayout(btnLayout);
}

SipAccountConfig AccountEditDialog::getAccountConfig() const {
    SipAccountConfig cfg;
    cfg.id = m_accountId;
    cfg.accountName = m_editName->text().trimmed().isEmpty() ? m_editUsername->text() : m_editName->text().trimmed();
    cfg.server = m_editServer->text().trimmed();
    cfg.port = m_spinPort->value();
    cfg.username = m_editUsername->text().trimmed();
    cfg.password = m_editPassword->text();
    cfg.domain = m_editDomain->text().trimmed();
    cfg.transport = m_comboTransport->currentText();
    cfg.isEnabled = m_chkEnabled->isChecked();
    cfg.isDefault = m_isDefault;
    cfg.profileId = "";

    return cfg;
}

// ===================================================================
// AccountsDialog Implementation
// ===================================================================

AccountsDialog::AccountsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Contas SIP");
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    resize(580, 380);
    setupUi();
    loadAccounts();
}

void AccountsDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(10);

    // Tabela de contas
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"Status", "Conta SIP", "Servidor", "Ramal", "Transporte", "Padrão"});

    m_tableView = new QTableView(this);
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setStretchLastSection(false);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AccountsDialog::onSelectionChanged);
    connect(m_tableView, &QTableView::doubleClicked, this, &AccountsDialog::onEditAccount);

    mainLayout->addWidget(m_tableView, 1);

    // Barra de Ações: Criar, Editar, Habilitar/Desabilitar, Excluir, Padrão
    auto btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);

    m_btnAdd = new QPushButton("➕ Criar Conta", this);
    connect(m_btnAdd, &QPushButton::clicked, this, &AccountsDialog::onAddAccount);

    m_btnEdit = new QPushButton("✏ Editar", this);
    m_btnEdit->setEnabled(false);
    connect(m_btnEdit, &QPushButton::clicked, this, &AccountsDialog::onEditAccount);

    m_btnToggleEnable = new QPushButton("⚡ Habilitar / Desabilitar", this);
    m_btnToggleEnable->setEnabled(false);
    connect(m_btnToggleEnable, &QPushButton::clicked, this, &AccountsDialog::onToggleEnableAccount);

    m_btnSetDefault = new QPushButton("⭐ Definir Padrão", this);
    m_btnSetDefault->setEnabled(false);
    connect(m_btnSetDefault, &QPushButton::clicked, this, &AccountsDialog::onSetDefaultAccount);

    m_btnDelete = new QPushButton("🗑 Excluir", this);
    m_btnDelete->setEnabled(false);
    connect(m_btnDelete, &QPushButton::clicked, this, &AccountsDialog::onDeleteAccount);

    m_btnClose = new QPushButton("Fechar", this);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnEdit);
    btnLayout->addWidget(m_btnToggleEnable);
    btnLayout->addWidget(m_btnSetDefault);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnClose);

    mainLayout->addLayout(btnLayout);
}

void AccountsDialog::loadAccounts() {
    m_accounts = SettingsManager::instance().accounts();
    m_model->removeRows(0, m_model->rowCount());

    for (const auto &acc : m_accounts) {
        QList<QStandardItem*> row;

        QColor statusColor = acc.isEnabled ? QColor("#2ECC71") : QColor("#95A5A6");
        auto itemStatus = new QStandardItem(acc.isEnabled ? "Ativa" : "Inativa");
        itemStatus->setIcon(IconHelper::createStatusDot(statusColor, 12));
        itemStatus->setTextAlignment(Qt::AlignCenter);

        auto itemName = new QStandardItem(acc.accountName);
        auto itemServer = new QStandardItem(acc.server);
        auto itemUser = new QStandardItem(acc.username);
        auto itemTransport = new QStandardItem(acc.transport);
        auto itemDefault = new QStandardItem(acc.isDefault ? "Sim ⭐" : "Não");
        itemDefault->setTextAlignment(Qt::AlignCenter);

        row << itemStatus << itemName << itemServer << itemUser << itemTransport << itemDefault;
        m_model->appendRow(row);
    }

    onSelectionChanged();
    adjustDialogToColumns();
}

void AccountsDialog::adjustDialogToColumns() {
    if (!m_tableView || !m_model) return;

    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setStretchLastSection(false);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tableView->resizeColumnsToContents();

    int totalColsWidth = 0;
    for (int col = 0; col < m_model->columnCount(); ++col) {
        totalColsWidth += qMax(m_tableView->columnWidth(col),
                               m_tableView->horizontalHeader()->sectionSizeHint(col));
    }

    int minLayoutWidth = layout() ? layout()->sizeHint().width() : 600;
    int targetWidth = qMax(totalColsWidth + 56, qMax(minLayoutWidth, 620));
    targetWidth = qMin(targetWidth, 1120);

    int rowCount = m_model->rowCount();
    int targetHeight = qBound(380, 190 + rowCount * 36, 620);

    setMinimumWidth(qMin(targetWidth, 620));
    resize(targetWidth, targetHeight);
}

void AccountsDialog::saveAccounts() {
    SettingsManager::instance().saveAccounts(m_accounts);
    auto def = SettingsManager::instance().defaultAccount();
    if (def.isEnabled) {
        SipManager::instance().registerAccount(def);
    } else {
        SipManager::instance().unregisterAccount();
    }
}

void AccountsDialog::onAddAccount() {
    AccountEditDialog dlg(SipAccountConfig(), this);
    if (dlg.exec() == QDialog::Accepted) {
        auto newAcc = dlg.getAccountConfig();
        if (m_accounts.isEmpty()) {
            newAcc.isDefault = true;
        }
        m_accounts.append(newAcc);
        saveAccounts();
        loadAccounts();
    }
}

void AccountsDialog::onEditAccount() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    int row = selected.first().row();
    if (row < 0 || row >= m_accounts.size()) return;

    AccountEditDialog dlg(m_accounts[row], this);
    if (dlg.exec() == QDialog::Accepted) {
        m_accounts[row] = dlg.getAccountConfig();
        saveAccounts();
        loadAccounts();
    }
}

void AccountsDialog::onToggleEnableAccount() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    int row = selected.first().row();
    if (row < 0 || row >= m_accounts.size()) return;

    m_accounts[row].isEnabled = !m_accounts[row].isEnabled;
    saveAccounts();
    loadAccounts();
}

void AccountsDialog::onDeleteAccount() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    int row = selected.first().row();
    if (row < 0 || row >= m_accounts.size()) return;

    auto reply = QMessageBox::question(this, "Excluir Conta",
                                       QString("Deseja realmente remover a conta '%1'?").arg(m_accounts[row].accountName),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_accounts.removeAt(row);
        if (!m_accounts.isEmpty() && !SettingsManager::instance().defaultAccount().isEnabled) {
            m_accounts.first().isDefault = true;
        }
        saveAccounts();
        loadAccounts();
    }
}

void AccountsDialog::onSetDefaultAccount() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    int row = selected.first().row();
    if (row < 0 || row >= m_accounts.size()) return;

    for (int i = 0; i < m_accounts.size(); ++i) {
        m_accounts[i].isDefault = (i == row);
    }
    saveAccounts();
    loadAccounts();
}

void AccountsDialog::onSelectionChanged() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    bool hasSelection = !selected.isEmpty();
    m_btnEdit->setEnabled(hasSelection);
    m_btnToggleEnable->setEnabled(hasSelection);
    m_btnDelete->setEnabled(hasSelection);
    m_btnSetDefault->setEnabled(hasSelection);
}

void AccountsDialog::closeEvent(QCloseEvent *event) {
    saveAccounts();
    QDialog::closeEvent(event);
}
