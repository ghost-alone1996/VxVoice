#ifndef ACCOUNTSDIALOG_H
#define ACCOUNTSDIALOG_H

#include <QDialog>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include "SettingsManager.h"

class AccountEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit AccountEditDialog(const SipAccountConfig &account = SipAccountConfig(), QWidget *parent = nullptr);
    SipAccountConfig getAccountConfig() const;

private:
    void setupUi();

    QLineEdit *m_editName{nullptr};
    QLineEdit *m_editServer{nullptr};
    QSpinBox *m_spinPort{nullptr};
    QLineEdit *m_editUsername{nullptr};
    QLineEdit *m_editPassword{nullptr};
    QLineEdit *m_editDomain{nullptr};
    QComboBox *m_comboTransport{nullptr};
    QCheckBox *m_chkEnabled{nullptr};

    QString m_accountId;
    bool m_isDefault{false};
};

class AccountsDialog : public QDialog {
    Q_OBJECT

public:
    explicit AccountsDialog(QWidget *parent = nullptr);

private slots:
    void onAddAccount();
    void onEditAccount();
    void onDeleteAccount();
    void onToggleEnableAccount();
    void onSetDefaultAccount();
    void onSelectionChanged();

private:
    void setupUi();
    void loadAccounts();
    void saveAccounts();

    QTableView *m_tableView{nullptr};
    QStandardItemModel *m_model{nullptr};
    QPushButton *m_btnAdd{nullptr};
    QPushButton *m_btnEdit{nullptr};
    QPushButton *m_btnToggleEnable{nullptr};
    QPushButton *m_btnDelete{nullptr};
    QPushButton *m_btnSetDefault{nullptr};
    QPushButton *m_btnClose{nullptr};

    QList<SipAccountConfig> m_accounts;
};

#endif // ACCOUNTSDIALOG_H
