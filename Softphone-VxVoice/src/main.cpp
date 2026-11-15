#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QIcon>
#include "MainWindow.h"
#include "ThemeManager.h"
#include "SettingsManager.h"

namespace {
constexpr const char* SINGLE_INSTANCE_KEY = "VxVoice_SingleInstance_IPC";
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Application metadata
    QApplication::setApplicationName("VxVOICE");
    QApplication::setApplicationDisplayName("VxVOICE");
    QApplication::setDesktopFileName("vxvoice");
    QApplication::setOrganizationName("VxVoiceCorp");
    QApplication::setOrganizationDomain("vxvoice.corp");
    QApplication::setApplicationVersion(SettingsManager::APP_VERSION);

    // Verifica se já existe uma instância do VxVoice em execução
    {
        QLocalSocket probeSocket;
        probeSocket.connectToServer(SINGLE_INSTANCE_KEY);
        if (probeSocket.waitForConnected(250)) {
            // Envia comando para restaurar/trazer à frente a janela já aberta
            probeSocket.write("SHOW\n");
            probeSocket.flush();
            probeSocket.waitForBytesWritten(250);
            probeSocket.disconnectFromServer();
            return 0;
        }
    }

    // Remove eventual socket órfão de encerramento inesperado
    QLocalServer::removeServer(SINGLE_INSTANCE_KEY);
    QLocalServer instanceServer;
    instanceServer.listen(SINGLE_INSTANCE_KEY);

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

    // Ao receber tentativa de nova abertura, traz a instância existente para o primeiro plano
    QObject::connect(&instanceServer, &QLocalServer::newConnection, &window, [&instanceServer, &window]() {
        while (instanceServer.hasPendingConnections()) {
            QLocalSocket *client = instanceServer.nextPendingConnection();
            if (!client) continue;
            client->waitForReadyRead(150);
            client->readAll();
            client->close();
            client->deleteLater();
        }
        window.setWindowState((window.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        window.show();
        window.raise();
        window.activateWindow();
    });

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
        } else if (hasExplicitMinimized) {
            startMinimized = true;
        }
    }

    if (!startMinimized) {
        window.show();
    }

    return app.exec();
}
