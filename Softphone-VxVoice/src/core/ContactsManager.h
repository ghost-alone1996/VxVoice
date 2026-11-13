#ifndef CONTACTSMANAGER_H
#define CONTACTSMANAGER_H

#include <QObject>
#include <QString>
#include <QList>

struct ContactEntry {
    QString name;
    QString number;
    QString extension;
    QString email;
    QString company;

    bool operator==(const ContactEntry &o) const {
        return name == o.name && number == o.number && extension == o.extension;
    }
};

class ContactsManager : public QObject {
    Q_OBJECT

public:
    static ContactsManager& instance();

    QList<ContactEntry> contacts() const;
    void addContact(const ContactEntry &contact);
    void updateContact(int index, const ContactEntry &contact);
    void removeContact(int index);

    // Busca o nome do contato pelo número de telefone ou ramal
    QString findContactNameByNumber(const QString &numberOrExt) const;

signals:
    void contactsChanged();

private:
    explicit ContactsManager(QObject *parent = nullptr);
    void loadContacts();
    void saveContacts();

    QList<ContactEntry> m_contacts;
};

#endif // CONTACTSMANAGER_H
