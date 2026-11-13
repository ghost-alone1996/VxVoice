#ifndef SELECTCONTACTNUMBERDIALOG_H
#define SELECTCONTACTNUMBERDIALOG_H

#include <QDialog>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include "ContactsManager.h"

class SelectContactNumberDialog : public QDialog {
    Q_OBJECT

public:
    explicit SelectContactNumberDialog(const ContactEntry &contact, QWidget *parent = nullptr);

    QString selectedNumber() const { return m_selectedNumber; }

private:
    void setupUi(const ContactEntry &contact);

    QString m_selectedNumber;
};

#endif // SELECTCONTACTNUMBERDIALOG_H
