#ifndef CONTACTSMANAGER_H
#define CONTACTSMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

struct ContactEntry {
    QString name;
    QString number;
    QString extension;
    QStringList additionalNumbers;
    QString email;
    QString company;

    QStringList allNumbers() const {
        QStringList list;
        auto appendUnique = [&list](const QString &val) {
            const QString trimmed = val.trimmed();
            if (!trimmed.isEmpty() && !list.contains(trimmed)) {
                list.append(trimmed);
            }
        };
        appendUnique(extension);
        appendUnique(number);
        for (const QString &extra : additionalNumbers) {
            appendUnique(extra);
        }
        return list;
    }

    bool operator==(const ContactEntry &o) const {
        return name == o.name && number == o.number && extension == o.extension &&
               additionalNumbers == o.additionalNumbers;
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
    QString resolveContactName(const QString &numberOrExt) const { return findContactNameByNumber(numberOrExt); }
    QList<ContactEntry> searchContacts(const QString &query) const;
    QString resolveDestination(const QString &input) const;

signals:
    void contactsChanged();

private:
    explicit ContactsManager(QObject *parent = nullptr);
    void loadContacts();
    void saveContacts();

    QList<ContactEntry> m_contacts;
};

#endif // CONTACTSMANAGER_H
