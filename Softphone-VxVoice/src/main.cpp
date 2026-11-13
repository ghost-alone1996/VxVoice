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

    // Iniciar minimizado no System Tray somente quando iniciado junto com o Sistema Operacional (--autostart)
    bool isAutostart = false;
    bool hasExplicitMinimized = false;
    bool hasExplicitShow = false;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--autostart") {
            isAutostart = true;
        } else if (arg == "--minimized" || arg == "-m") {
            hasExplicitMinimized = true;
        } else if (arg == "--show" || arg == "--open" || arg == "--no-minimized") {
            hasExplicitShow = true;
        }
    }

    bool startMinimized = false;
    if (!hasExplicitShow) {
        if (isAutostart && SettingsManager::instance().startWithSystem() && SettingsManager::instance().startMinimized()) {
            startMinimized = true;
        } else if (hasExplicitMinimized && isAutostart) {
            startMinimized = true;
        } else if (hasExplicitMinimized && !isAutostart) {
            startMinimized = true;
        }
    }

    if (!startMinimized) {
        window.show();
    }

    return app.exec();
}

