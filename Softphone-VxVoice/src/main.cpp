#include <QApplication>
#include "MainWindow.h"
#include "ThemeManager.h"
#include "SettingsManager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Application metadata
    QApplication::setApplicationName("VxVoice");
    QApplication::setApplicationDisplayName("VxVoice Softphone");
    QApplication::setOrganizationName("VxVoiceCorp");
    QApplication::setOrganizationDomain("vxvoice.corp");
    QApplication::setApplicationVersion("1.0.0");

    // Configure purple VX application icon
    app.setWindowIcon(QIcon(":/icons/app_icon.png"));

    // Quit behavior handled via closeEvent and settings
    QApplication::setQuitOnLastWindowClosed(false);

    // Initialize Default VxVoice Corporate Purple Theme
    ThemeManager::instance().setTheme(ThemeManager::Theme::VxVoice);

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

