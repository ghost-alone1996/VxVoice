#include "ContactsWidget.h"
#include "IconHelper.h"
#include "SipManager.h"
#include <QHeaderView>
#include <QFormLayout>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QSet>

// ===================================================================
// ContactEditDialog
// ===================================================================

ContactEditDialog::ContactEditDialog(const ContactInfo &contact, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(contact.name.isEmpty() ? "Adicionar Contato" : "Editar Contato");
    resize(360, 260);
    setupUi();

    m_editName->setText(contact.name);
    m_editNumber->setText(contact.number);
    m_editExtension->setText(contact.extension);
    m_editEmail->setText(contact.email);
    m_editCompany->setText(contact.company);
}

void ContactEditDialog::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(10);

    auto formLayout = new QFormLayout();
    m_editName = new QLineEdit(this);
    m_editName->setPlaceholderText("Ex: Roberto Albuquerque");

    m_editNumber = new QLineEdit(this);
    m_editNumber->setPlaceholderText("Ex: +55 (11) 98765-4321");

    m_editExtension = new QLineEdit(this);
    m_editExtension->setPlaceholderText("Ex: 1001");

    m_editEmail = new QLineEdit(this);
    m_editEmail->setPlaceholderText("Ex: roberto@empresa.com.br");

    m_editCompany = new QLineEdit(this);
    m_editCompany->setPlaceholderText("Ex: Vx Telecom / TI");

    formLayout->addRow("Nome Completo:*", m_editName);
    formLayout->addRow("Número Telefônico:", m_editNumber);
    formLayout->addRow("Ramal SIP:", m_editExtension);
    formLayout->addRow("E-mail:", m_editEmail);
    formLayout->addRow("Empresa / Setor:", m_editCompany);

    mainLayout->addLayout(formLayout);

    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto btnCancel = new QPushButton("Cancelar", this);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    auto btnSave = new QPushButton("Salvar Contato", this);
    btnSave->setObjectName("primaryButton");
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        if (m_editName->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validação", "O nome do contato é obrigatório.");
            return;
        }
        if (m_editExtension->text().trimmed().isEmpty() && m_editNumber->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validação", "Informe ao menos um Ramal ou Número de telefone.");
            return;
        }
        accept();
    });

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    mainLayout->addLayout(btnLayout);
}

ContactInfo ContactEditDialog::getContactInfo() const {
    ContactInfo c;
    c.name = m_editName->text().trimmed();
    c.number = m_editNumber->text().trimmed();
    c.extension = m_editExtension->text().trimmed();
    c.email = m_editEmail->text().trimmed();
    c.company = m_editCompany->text().trimmed().isEmpty() ? "Geral" : m_editCompany->text().trimmed();
    return c;
}

// ===================================================================
// ContactsProxyModel
// ===================================================================

ContactsProxyModel::ContactsProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
}

void ContactsProxyModel::setCompanyFilter(const QString &company) {
    m_companyFilter = company;
    invalidateFilter();
}

void ContactsProxyModel::setSearchText(const QString &text) {
    m_searchText = text.trimmed();
    invalidateFilter();
}

bool ContactsProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    auto model = sourceModel();
    if (!model) return true;

    // Colunas: 0: Nome, 1: Ramal, 2: Número, 3: Empresa, 4: E-mail
    QString name = model->index(source_row, 0, source_parent).data().toString();
    QString ext = model->index(source_row, 1, source_parent).data().toString();
    QString num = model->index(source_row, 2, source_parent).data().toString();
    QString company = model->index(source_row, 3, source_parent).data().toString();
    QString email = model->index(source_row, 4, source_parent).data().toString();

    // Filtro por Empresa
    if (!m_companyFilter.isEmpty() && m_companyFilter != "Todas as Empresas") {
        if (company.compare(m_companyFilter, Qt::CaseInsensitive) != 0) {
            return false;
        }
    }

    // Filtro por texto de busca
    if (!m_searchText.isEmpty()) {
        bool match = name.contains(m_searchText, Qt::CaseInsensitive) ||
                     ext.contains(m_searchText, Qt::CaseInsensitive) ||
                     num.contains(m_searchText, Qt::CaseInsensitive) ||
                     company.contains(m_searchText, Qt::CaseInsensitive) ||
                     email.contains(m_searchText, Qt::CaseInsensitive);
        if (!match) return false;
    }

    return true;
}

// ===================================================================
// ContactsWidget
// ===================================================================

ContactsWidget::ContactsWidget(QWidget *parent)
    : QWidget(parent) {
    setupUi();
    populateInitialContacts();
}

void ContactsWidget::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Filter Bar: Search Box + Company Filter + Add Button
    auto filterLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔍 Buscar contato...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ContactsWidget::onSearchTextChanged);

    m_comboCompanyFilter = new QComboBox(this);
    m_comboCompanyFilter->addItem("Todas as Empresas");
    m_comboCompanyFilter->setToolTip("Filtrar por Empresa");
    connect(m_comboCompanyFilter, &QComboBox::currentTextChanged, this, &ContactsWidget::onCompanyFilterChanged);

    m_btnAddContact = new QPushButton("+ Adicionar", this);
    m_btnAddContact->setCursor(Qt::PointingHandCursor);
    m_btnAddContact->setToolTip("Cadastrar novo contato corporativo");
    connect(m_btnAddContact, &QPushButton::clicked, this, &ContactsWidget::onAddContactClicked);

    filterLayout->addWidget(m_searchEdit, 2);
    filterLayout->addWidget(m_comboCompanyFilter, 1);
    filterLayout->addWidget(m_btnAddContact);
    mainLayout->addLayout(filterLayout);

    // Table & Model
    m_sourceModel = new QStandardItemModel(this);
    m_sourceModel->setHorizontalHeaderLabels({"Nome", "Ramal", "Número", "Empresa", "E-mail"});

    m_proxyModel = new ContactsProxyModel(this);
    m_proxyModel->setSourceModel(m_sourceModel);

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
            this, &ContactsWidget::onSelectionChanged);
    connect(m_tableView, &QTableView::doubleClicked, this, &ContactsWidget::onItemDoubleClicked);

    mainLayout->addWidget(m_tableView);

    // Bottom Action Bar: Ligar, Enviar Email, Editar, Excluir com proporção unificada
    auto bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(6);
    bottomLayout->setContentsMargins(0, 4, 0, 0);

    const int actionBtnHeight = 30;

    m_btnCall = new QPushButton("📞 Ligar", this);
    m_btnCall->setObjectName("contactCallButton");
    m_btnCall->setFixedHeight(actionBtnHeight);
    m_btnCall->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_btnCall->setEnabled(false);
    m_btnCall->setCursor(Qt::PointingHandCursor);
    m_btnCall->setToolTip("Iniciar chamada para o contato selecionado");
    connect(m_btnCall, &QPushButton::clicked, this, &ContactsWidget::onCallClicked);

    m_btnEmail = new QPushButton("✉ E-mail", this);
    m_btnEmail->setObjectName("contactEmailButton");
    m_btnEmail->setFixedHeight(actionBtnHeight);
    m_btnEmail->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_btnEmail->setEnabled(false);
    m_btnEmail->setCursor(Qt::PointingHandCursor);
    m_btnEmail->setToolTip("Enviar e-mail para o contato");
    connect(m_btnEmail, &QPushButton::clicked, this, &ContactsWidget::onEmailClicked);

    m_btnEdit = new QPushButton("✏ Editar", this);
    m_btnEdit->setObjectName("contactEditButton");
    m_btnEdit->setFixedHeight(actionBtnHeight);
    m_btnEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_btnEdit->setEnabled(false);
    m_btnEdit->setCursor(Qt::PointingHandCursor);
    m_btnEdit->setToolTip("Editar dados do contato");
    connect(m_btnEdit, &QPushButton::clicked, this, &ContactsWidget::onEditContactClicked);

    m_btnDelete = new QPushButton("🗑 Excluir", this);
    m_btnDelete->setObjectName("contactDeleteButton");
    m_btnDelete->setFixedHeight(actionBtnHeight);
    m_btnDelete->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_btnDelete->setEnabled(false);
    m_btnDelete->setCursor(Qt::PointingHandCursor);
    m_btnDelete->setToolTip("Excluir contato selecionado");
    connect(m_btnDelete, &QPushButton::clicked, this, &ContactsWidget::onDeleteContactClicked);

    bottomLayout->addWidget(m_btnCall);
    bottomLayout->addWidget(m_btnEmail);
    bottomLayout->addWidget(m_btnEdit);
    bottomLayout->addWidget(m_btnDelete);
    mainLayout->addLayout(bottomLayout);
}

void ContactsWidget::addContact(const ContactInfo &contact) {
    if (!m_contacts.contains(contact)) {
        ContactsManager::instance().addContact(contact);
        m_contacts.append(contact);

        QList<QStandardItem*> row;
        auto itemNome = new QStandardItem(contact.name);
        auto itemExt = new QStandardItem(contact.extension);
        itemExt->setTextAlignment(Qt::AlignCenter);
        if (!contact.extension.isEmpty()) {
            itemExt->setIcon(IconHelper::createStatusDot(QColor("#2ECC71"), 12));
            itemExt->setToolTip("Status BLF: Ramal Livre");
            SipManager::instance().subscribeBlf(contact.extension);
        }

        auto itemNum = new QStandardItem(contact.number);
        auto itemComp = new QStandardItem(contact.company);
        auto itemEmail = new QStandardItem(contact.email);

        row << itemNome << itemExt << itemNum << itemComp << itemEmail;
        m_sourceModel->appendRow(row);

        updateCompanyFilterList();
    }
}

void ContactsWidget::updateCompanyFilterList() {
    QString current = m_comboCompanyFilter->currentText();
    QSet<QString> companies;
    for (const auto &c : m_contacts) {
        if (!c.company.isEmpty()) {
            companies.insert(c.company);
        }
    }

    m_comboCompanyFilter->blockSignals(true);
    m_comboCompanyFilter->clear();
    m_comboCompanyFilter->addItem("Todas as Empresas");
    for (const auto &comp : companies) {
        m_comboCompanyFilter->addItem(comp);
    }
    int idx = m_comboCompanyFilter->findText(current);
    if (idx >= 0) m_comboCompanyFilter->setCurrentIndex(idx);
    m_comboCompanyFilter->blockSignals(false);
}

void ContactsWidget::populateInitialContacts() {
    m_contacts.clear();
    m_sourceModel->removeRows(0, m_sourceModel->rowCount());
    auto list = ContactsManager::instance().contacts();
    for (const auto &c : list) {
        m_contacts.append(c);

        QList<QStandardItem*> row;
        auto itemNome = new QStandardItem(c.name);
        auto itemExt = new QStandardItem(c.extension);
        itemExt->setTextAlignment(Qt::AlignCenter);
        if (!c.extension.isEmpty()) {
            itemExt->setIcon(IconHelper::createStatusDot(QColor("#2ECC71"), 12));
            itemExt->setToolTip("Status BLF: Ramal Livre");
            SipManager::instance().subscribeBlf(c.extension);
        }

        auto itemNum = new QStandardItem(c.number);
        auto itemComp = new QStandardItem(c.company);
        auto itemEmail = new QStandardItem(c.email);

        row << itemNome << itemExt << itemNum << itemComp << itemEmail;
        m_sourceModel->appendRow(row);
    }
    updateCompanyFilterList();
}

void ContactsWidget::onSearchTextChanged(const QString &text) {
    m_proxyModel->setSearchText(text);
}

void ContactsWidget::onCompanyFilterChanged(const QString &company) {
    m_proxyModel->setCompanyFilter(company);
}

void ContactsWidget::onAddContactClicked() {
    ContactEditDialog dlg(ContactInfo(), this);
    if (dlg.exec() == QDialog::Accepted) {
        addContact(dlg.getContactInfo());
    }
}

void ContactsWidget::onEditContactClicked() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    int row = sourceIdx.row();
    if (row < 0 || row >= m_contacts.size()) return;

    ContactEditDialog dlg(m_contacts[row], this);
    if (dlg.exec() == QDialog::Accepted) {
        auto updated = dlg.getContactInfo();
        m_contacts[row] = updated;
        ContactsManager::instance().updateContact(row, updated);

        m_sourceModel->item(row, 0)->setText(updated.name);
        m_sourceModel->item(row, 1)->setText(updated.extension);
        m_sourceModel->item(row, 2)->setText(updated.number);
        m_sourceModel->item(row, 3)->setText(updated.company);
        m_sourceModel->item(row, 4)->setText(updated.email);

        updateCompanyFilterList();
        onSelectionChanged();
    }
}

void ContactsWidget::onDeleteContactClicked() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    int row = sourceIdx.row();
    if (row < 0 || row >= m_contacts.size()) return;

    auto reply = QMessageBox::question(this, "Excluir Contato",
                                       QString("Deseja remover '%1'?").arg(m_contacts[row].name),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        ContactsManager::instance().removeContact(row);
        m_contacts.removeAt(row);
        m_sourceModel->removeRow(row);
        updateCompanyFilterList();
        onSelectionChanged();
    }
}

void ContactsWidget::onCallClicked() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    int row = sourceIdx.row();
    if (row < 0 || row >= m_contacts.size()) return;

    const auto &c = m_contacts[row];
    QString target = !c.extension.isEmpty() ? c.extension : c.number;
    if (!target.isEmpty()) {
        emit callRequested(target);
    }
}

void ContactsWidget::onEmailClicked() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
    int row = sourceIdx.row();
    if (row < 0 || row >= m_contacts.size()) return;

    QString email = m_contacts[row].email;
    if (!email.isEmpty()) {
        QDesktopServices::openUrl(QUrl("mailto:" + email));
    }
}

void ContactsWidget::onItemDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;
    QModelIndex sourceIdx = m_proxyModel->mapToSource(index);
    int row = sourceIdx.row();
    if (row < 0 || row >= m_contacts.size()) return;

    const auto &c = m_contacts[row];
    QString target = !c.extension.isEmpty() ? c.extension : c.number;
    if (!target.isEmpty()) {
        emit callRequested(target);
    }
}

void ContactsWidget::onSelectionChanged() {
    auto selected = m_tableView->selectionModel()->selectedRows();
    bool hasSelection = !selected.isEmpty();
    m_btnCall->setEnabled(hasSelection);
    m_btnEdit->setEnabled(hasSelection);
    m_btnDelete->setEnabled(hasSelection);

    if (hasSelection) {
        QModelIndex sourceIdx = m_proxyModel->mapToSource(selected.first());
        int row = sourceIdx.row();
        bool hasEmail = (row >= 0 && row < m_contacts.size() && !m_contacts[row].email.isEmpty());
        m_btnEmail->setEnabled(hasEmail);
    } else {
        m_btnEmail->setEnabled(false);
    }
}

void ContactsWidget::onBlfStatusChanged(const QString &extension, SipManager::BlfState state) {
    QColor dotColor = QColor("#95A5A6");
    QString tip = "Status BLF: Offline";
    switch (state) {
    case SipManager::BlfState::Idle:
        dotColor = QColor("#2ECC71");
        tip = "Status BLF: Ramal Livre";
        break;
    case SipManager::BlfState::Busy:
        dotColor = QColor("#E74C3C");
        tip = "Status BLF: Ramal Ocupado / Em Conversação";
        break;
    case SipManager::BlfState::Ringing:
        dotColor = QColor("#F39C12");
        tip = "Status BLF: Ramal Chamando";
        break;
    case SipManager::BlfState::Offline:
        dotColor = QColor("#95A5A6");
        tip = "Status BLF: Desconectado / Offline";
        break;
    }

    for (int r = 0; r < m_sourceModel->rowCount(); ++r) {
        if (m_sourceModel->item(r, 1) && m_sourceModel->item(r, 1)->text() == extension) {
            m_sourceModel->item(r, 1)->setIcon(IconHelper::createStatusDot(dotColor, 12));
            m_sourceModel->item(r, 1)->setToolTip(tip);
        }
    }
}
