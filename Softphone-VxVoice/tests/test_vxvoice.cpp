#include <QtTest/QtTest>
#include <QApplication>
#include "ThemeManager.h"
#include "SettingsManager.h"
#include "ContactsManager.h"
#include "UpdateManager.h"

class VxVoiceTestSuite : public QObject {
    Q_OBJECT

private slots:
    void testThemeMappings() {
        const QList<ThemeManager::Theme> themes = {
            ThemeManager::Theme::Dark,
            ThemeManager::Theme::Light,
            ThemeManager::Theme::Purple,
            ThemeManager::Theme::Red,
            ThemeManager::Theme::Win2000,
            ThemeManager::Theme::WinXP,
            ThemeManager::Theme::Win7
        };

        for (auto t : themes) {
            QString str = ThemeManager::themeToString(t);
            QVERIFY(!str.isEmpty());
            QCOMPARE(ThemeManager::themeFromString(str), t);
            QVERIFY(!ThemeManager::instance().themeName(t).isEmpty());
        }
    }

    void testThemePersistence() {
        auto &sm = SettingsManager::instance();
        const QString original = sm.theme();

        sm.setTheme("Dark");
        QCOMPARE(sm.theme(), QString("Dark"));

        sm.setTheme("Red");
        QCOMPARE(sm.theme(), QString("Red"));

        sm.setTheme(original);
    }

    void testRegistrationStatusPersistence() {
        auto &sm = SettingsManager::instance();
        const int original = sm.registrationStatus();

        sm.setRegistrationStatus(1);
        QCOMPARE(sm.registrationStatus(), 1);

        sm.setRegistrationStatus(2);
        QCOMPARE(sm.registrationStatus(), 2);

        sm.setRegistrationStatus(original);
    }

    void testUpdateManagerConfig() {
        auto &um = UpdateManager::instance();
        QCOMPARE(um.currentVersion(), QString(SettingsManager::APP_VERSION));
        QCOMPARE(um.githubRepoUrl(), QString("https://github.com/ghost-alone1996/VxVoice"));
    }

    void testContactsSearch() {
        auto &cm = ContactsManager::instance();
        int prevCount = cm.contacts().size();

        ContactEntry c;
        c.name = "Teste Automatizado";
        c.extension = "9999";
        c.number = "11999998888";
        c.additionalNumbers = QStringList{"21988887777", "3133334444"};
        c.company = "VxNET";
        QCOMPARE(c.allNumbers().size(), 4);
        cm.addContact(c);

        QCOMPARE(cm.findContactNameByNumber("9999"), QString("Teste Automatizado"));
        QCOMPARE(cm.findContactNameByNumber("21988887777"), QString("Teste Automatizado"));

        auto suggestions = cm.searchContacts("9999");
        QVERIFY(!suggestions.isEmpty());
        auto extraSuggestions = cm.searchContacts("3133334444");
        QVERIFY(!extraSuggestions.isEmpty());

        // Remove o contato de teste adicionado
        if (cm.contacts().size() > prevCount) {
            cm.removeContact(cm.contacts().size() - 1);
        }
        QCOMPARE(cm.contacts().size(), prevCount);
    }

    void testSipProfilesPersistence() {
        auto &sm = SettingsManager::instance();
        QList<SipProfileConfig> backup = sm.profiles();

        SipProfileConfig testProf;
        testProf.id = "prof_test_206";
        testProf.name = "PBX Teste 2.0.6";
        testProf.server = "pbx.teste.local";
        testProf.port = 5080;
        testProf.domain = "teste.local";
        testProf.transport = "TLS";

        QList<SipProfileConfig> updated = backup;
        updated.append(testProf);
        sm.saveProfiles(updated);

        SipProfileConfig loaded = sm.findProfileById("prof_test_206");
        QCOMPARE(loaded.id, QString("prof_test_206"));
        QCOMPARE(loaded.name, QString("PBX Teste 2.0.6"));
        QCOMPARE(loaded.server, QString("pbx.teste.local"));
        QCOMPARE(loaded.port, 5080);
        QCOMPARE(loaded.domain, QString("teste.local"));
        QCOMPARE(loaded.transport, QString("TLS"));

        sm.saveProfiles(backup);
        QCOMPARE(sm.profiles().size(), backup.size());
    }
};

QTEST_MAIN(VxVoiceTestSuite)
#include "test_vxvoice.moc"
