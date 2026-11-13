#include "ContactsManager.h"
#include <QSettings>

ContactsManager& ContactsManager::instance() {
    static ContactsManager s_instance;
    return s_instance;
}

ContactsManager::ContactsManager(QObject *parent)
    : QObject(parent) {
    loadContacts();
}

QList<ContactEntry> ContactsManager::contacts() const {
    return m_contacts;
}

void ContactsManager::addContact(const ContactEntry &contact) {
    m_contacts.append(contact);
    saveContacts();
    emit contactsChanged();
}

void ContactsManager::updateContact(int index, const ContactEntry &contact) {
    if (index >= 0 && index < m_contacts.size()) {
        m_contacts[index] = contact;
        saveContacts();
        emit contactsChanged();
    }
}

void ContactsManager::removeContact(int index) {
    if (index >= 0 && index < m_contacts.size()) {
        m_contacts.removeAt(index);
        saveContacts();
        emit contactsChanged();
    }
}

static QString normalizeDigits(const QString &input) {
    QString out;
    for (const QChar &ch : input) {
        if (ch.isDigit()) {
            out.append(ch);
        }
    }
    return out;
}

QString ContactsManager::findContactNameByNumber(const QString &numberOrExt) const {
    QString q = numberOrExt.trimmed();
    if (q.isEmpty()) return QString();

    QString qDigits = normalizeDigits(q);

    // 1. Busca exata por ramal ou número
    for (const auto &c : m_contacts) {
        if (c.extension.compare(q, Qt::CaseInsensitive) == 0) {
            return c.name;
        }
        if (c.number.compare(q, Qt::CaseInsensitive) == 0) {
            return c.name;
        }
    }

    // 2. Busca por dígitos normalizados (ex: ramal ou telefone formatado)
    if (!qDigits.isEmpty()) {
        for (const auto &c : m_contacts) {
            QString extDigits = normalizeDigits(c.extension);
            if (!extDigits.isEmpty() && extDigits == qDigits) {
                return c.name;
            }

            QString numDigits = normalizeDigits(c.number);
            if (!numDigits.isEmpty()) {
                if (numDigits == qDigits || numDigits.endsWith(qDigits) || qDigits.endsWith(numDigits)) {
                    return c.name;
                }
            }
        }
    }

    return QString();
}

void ContactsManager::loadContacts() {
    QSettings settings("VxVoiceCorp", "VxVoice");
    int size = settings.beginReadArray("Contacts");
    m_contacts.clear();

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        ContactEntry c;
        c.name = settings.value("name").toString();
        c.number = settings.value("number").toString();
        c.extension = settings.value("extension").toString();
        c.email = settings.value("email").toString();
        c.company = settings.value("company").toString();

        if (c.name.isEmpty()) continue;

        // Descarta contatos fictícios legados de teste
        if ((c.extension == "1042" && c.name.contains("Suporte TI")) ||
            (c.extension == "1001" && c.name == "Carlos Silva" && c.company == "Matriz SP")) {
            continue;
        }

        m_contacts.append(c);
    }
    settings.endArray();
}

void ContactsManager::saveContacts() {
    QSettings settings("VxVoiceCorp", "VxVoice");
    settings.remove("Contacts");
    settings.beginWriteArray("Contacts", m_contacts.size());
    for (int i = 0; i < m_contacts.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", m_contacts[i].name);
        settings.setValue("number", m_contacts[i].number);
        settings.setValue("extension", m_contacts[i].extension);
        settings.setValue("email", m_contacts[i].email);
        settings.setValue("company", m_contacts[i].company);
    }
    settings.endArray();
    settings.sync();
}

