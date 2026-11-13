#include <QApplication>
#include "MainWindow.h"
#include "ThemeManager.h"
#include "SettingsManager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Application metadata
    QApplication::setApplicationName("VxVoice");
    QApplication::setApplicationDisplayName("VxVoice");
    QApplication::setDesktopFileName("vxvoice");
    QApplication::setOrganizationName("VxVoiceCorp");
    QApplication::setOrganizationDomain("vxvoice.corp");
    QApplication::setApplicationVersion(SettingsManager::APP_VERSION);

    // Configure application icon (resolves from system theme or internal resource)
    QIcon appIcon = QIcon::fromTheme("vxvoice", QIcon(":/icons/app_icon.png"));
    app.setWindowIcon(appIcon);

    // Quit behavior handled via closeEvent and settings
    QApplication::setQuitOnLastWindowClosed(false);

    // Initialize Theme from Saved Settings (Persistência Garantida)
    QString savedTheme = SettingsManager::instance().theme();
    ThemeManager::Theme theme = ThemeManager::themeFromString(savedTheme);
    ThemeManager::instance().setTheme(theme);

    MainWindow window;

    // Check if application should start minimized in tray
    bool startMinimized = SettingsManager::instance().startMinimized();
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--autostart" || arg == "-m" || arg == "--minimized") {
            startMinimized = true;
            break;
        }
    }

    if (!startMinimized) {
        window.show();
    }

    return app.exec();
}

