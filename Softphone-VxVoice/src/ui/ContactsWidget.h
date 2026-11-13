#ifndef CONTACTSWIDGET_H
#define CONTACTSWIDGET_H

#include <QWidget>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QTableView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "ContactsManager.h"
#include "SipManager.h"

using ContactInfo = ContactEntry;

class ContactEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit ContactEditDialog(const ContactInfo &contact = ContactInfo(), QWidget *parent = nullptr);
    ContactInfo getContactInfo() const;

private:
    void setupUi();

    QLineEdit *m_editName{nullptr};
    QLineEdit *m_editNumber{nullptr};
    QLineEdit *m_editExtension{nullptr};
    QLineEdit *m_editEmail{nullptr};
    QLineEdit *m_editCompany{nullptr};
};

class ContactsProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit ContactsProxyModel(QObject *parent = nullptr);

    void setCompanyFilter(const QString &company);
    void setSearchText(const QString &text);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    QString m_companyFilter{"Todas as Empresas"};
    QString m_searchText;
};

class ContactsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ContactsWidget(QWidget *parent = nullptr);

    void addContact(const ContactInfo &contact);

public slots:
    void onBlfStatusChanged(const QString &extension, SipManager::BlfState state);

signals:
    void callRequested(const QString &target);

private slots:
    void onSearchTextChanged(const QString &text);
    void onCompanyFilterChanged(const QString &company);
    void onAddContactClicked();
    void onEditContactClicked();
    void onDeleteContactClicked();
    void onCallClicked();
    void onEmailClicked();
    void onItemDoubleClicked(const QModelIndex &index);
    void onSelectionChanged();

private:
    void setupUi();
    void populateInitialContacts();
    void updateCompanyFilterList();
    void initiateContactCall(const ContactInfo &c);

    QLineEdit *m_searchEdit{nullptr};
    QComboBox *m_comboCompanyFilter{nullptr};
    QPushButton *m_btnAddContact{nullptr};

    QTableView *m_tableView{nullptr};
    QStandardItemModel *m_sourceModel{nullptr};
    ContactsProxyModel *m_proxyModel{nullptr};

    // Action Buttons
    QPushButton *m_btnCall{nullptr};
    QPushButton *m_btnEmail{nullptr};
    QPushButton *m_btnEdit{nullptr};
    QPushButton *m_btnDelete{nullptr};

    QList<ContactInfo> m_contacts;
};

#endif // CONTACTSWIDGET_H
