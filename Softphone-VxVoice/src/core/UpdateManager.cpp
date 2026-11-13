#include "UpdateManager.h"
#include "SettingsManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>

#ifndef VXVOICE_VERSION
#define VXVOICE_VERSION "1.4.6"
#endif

UpdateManager& UpdateManager::instance() {
    static UpdateManager instance;
    return instance;
}

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent),
      m_currentVersion(SettingsManager::APP_VERSION),
      m_gitVersion(SettingsManager::APP_VERSION) {
    m_repoPath = findGitDirectory();
}

QString UpdateManager::currentVersion() const {
    return QString(VXVOICE_VERSION);
}

QString UpdateManager::gitVersion() const {
    return m_gitVersion;
}

QString UpdateManager::gitRepoPath() const {
    return m_repoPath;
}

bool UpdateManager::isGitRepository() const {
    return !m_repoPath.isEmpty() && QDir(m_repoPath + "/.git").exists();
}

bool UpdateManager::isUpdateAvailable() const {
    return m_isUpdateAvailable;
}

int UpdateManager::compareVersions(const QString &v1, const QString &v2) {
    QString clean1 = v1.trimmed();
    if (clean1.startsWith('v', Qt::CaseInsensitive)) clean1 = clean1.mid(1);
    QString clean2 = v2.trimmed();
    if (clean2.startsWith('v', Qt::CaseInsensitive)) clean2 = clean2.mid(1);

    auto parts1 = clean1.split('.');
    auto parts2 = clean2.split('.');
    int maxLen = qMax(parts1.size(), parts2.size());

    for (int i = 0; i < maxLen; ++i) {
        int num1 = (i < parts1.size()) ? parts1[i].toInt() : 0;
        int num2 = (i < parts2.size()) ? parts2[i].toInt() : 0;
        if (num1 > num2) return 1;
        if (num1 < num2) return -1;
    }
    return 0;
}

QString UpdateManager::findGitDirectory() const {
    // 1. Diretório de trabalho atual
    QDir dir(QDir::currentPath());
    for (int i = 0; i < 5; ++i) {
        if (dir.exists(".git")) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) break;
    }

    // 2. Diretório do executável
    QDir appDir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        if (appDir.exists(".git")) {
            return appDir.absolutePath();
        }
        if (!appDir.cdUp()) break;
    }

    // 3. Fallback para caminho do repositório
    if (QDir("/home/leonardo/Documentos/ProjetosDEV/.git").exists()) {
        return "/home/leonardo/Documentos/ProjetosDEV";
    }

    return QString();
}

void UpdateManager::checkForUpdates(bool silent) {
    if (m_isChecking) return;

    m_silent = silent;
    m_isChecking = true;
    m_currentVersion = currentVersion();
    emit checkingStarted();
    emit statusChanged("Verificando atualizações no repositório Git...");

    if (!isGitRepository()) {
        m_isChecking = false;
        QString err = "Repositório Git não encontrado.";
        m_gitVersion = m_currentVersion;
        emit updateError(err);
        emit statusChanged(err);
        return;
    }

    // Consulta assíncrona ao Git
    auto process = new QProcess(this);
    process->setWorkingDirectory(m_repoPath);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/) {
        process->deleteLater();
        m_isChecking = false;

        // 1. Obtém a tag de versão mais recente no Git
        QProcess tagProc;
        tagProc.setWorkingDirectory(m_repoPath);
        tagProc.start("git", {"tag", "-l", "--sort=-v:refname", "v*"});
        tagProc.waitForFinished(2000);
        QString tagsOutput = QString::fromUtf8(tagProc.readAllStandardOutput()).trimmed();
        QString latestTag;
        if (!tagsOutput.isEmpty()) {
            latestTag = tagsOutput.split('\n').first().trimmed();
        }

        if (latestTag.isEmpty()) {
            latestTag = "v" + m_currentVersion;
        }

        QString cleanGitVer = latestTag.startsWith('v') ? latestTag.mid(1) : latestTag;
        m_gitVersion = cleanGitVer;

        // 2. Obtém resumo do último commit
        QProcess logProc;
        logProc.setWorkingDirectory(m_repoPath);
        logProc.start("git", {"log", "-1", "--format=%h - %s (%ci)"});
        logProc.waitForFinished(2000);
        QString lastCommit = QString::fromUtf8(logProc.readAllStandardOutput()).trimmed();

        // 3. Verifica contagem de commits à frente se houver upstream
        QProcess diffProc;
        diffProc.setWorkingDirectory(m_repoPath);
        diffProc.start("git", {"rev-list", "HEAD..@{u}", "--count"});
        diffProc.waitForFinished(2000);
        int aheadCount = QString::fromUtf8(diffProc.readAllStandardOutput()).trimmed().toInt();

        // Compara versões semânticas
        int cmp = compareVersions(m_gitVersion, m_currentVersion);
        if (cmp > 0 || aheadCount > 0) {
            m_isUpdateAvailable = true;
            QString changelog = QString("Nova versão v%1 disponível no Git.\nÚltimo commit: %2")
                                    .arg(m_gitVersion, lastCommit);
            emit updateChecked(m_currentVersion, m_gitVersion, true, changelog);
            emit updateAvailable(m_gitVersion, changelog);
            emit statusChanged(QString("Nova versão v%1 disponível no Git.").arg(m_gitVersion));
        } else {
            m_isUpdateAvailable = false;
            QString msg = QString("O software está na versão mais recente (%1).\nCommit: %2")
                              .arg(m_currentVersion, lastCommit);
            emit updateChecked(m_currentVersion, m_gitVersion, false, msg);
            emit upToDate(msg);
            emit statusChanged(QString("Versão %1 (Atualizada)").arg(m_currentVersion));
        }
    });

    // Se houver remoto configurado, faz fetch rápido
    process->start("git", {"fetch", "--dry-run"});
}

void UpdateManager::applyUpdate() {
    if (!isGitRepository()) {
        emit updateCompleted(false, "Repositório Git não encontrado.");
        return;
    }

    emit statusChanged("Preparando e sincronizando pacote de atualização...");
    auto process = new QProcess(this);
    process->setWorkingDirectory(m_repoPath);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/) {
        process->deleteLater();

        // Procura o pacote .deb mais recente gerado para instalação
        QString debPath;
        QStringList searchDirs = {
            m_repoPath,
            m_repoPath + "/Softphone-VxVoice/build",
            m_repoPath + "/build",
            QFileInfo(m_repoPath).dir().absolutePath()
        };

        for (const auto &sdir : searchDirs) {
            QDir d(sdir);
            if (!d.exists()) continue;
            auto debFiles = d.entryInfoList({"*.deb"}, QDir::Files, QDir::Time);
            if (!debFiles.isEmpty()) {
                debPath = debFiles.first().absoluteFilePath();
                break;
            }
        }

        if (debPath.isEmpty()) {
            emit updateCompleted(false, "Nenhum pacote .deb de atualização encontrado.");
            emit statusChanged("Erro: Pacote .deb não localizado.");
            return;
        }

        if (QFile::exists("/usr/bin/pkexec")) {
            emit statusChanged("Instalando pacote com privilégios de administrador...");
            auto instProc = new QProcess(this);
            connect(instProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, [this, instProc, debPath](int instCode, QProcess::ExitStatus instStatus) {
                instProc->deleteLater();
                if (instStatus == QProcess::NormalExit && instCode == 0) {
                    emit updateCompleted(true, QString("VxVoice atualizado com sucesso!\n\nPacote instalado: %1\n\nReiniciando para carregar a nova versão...")
                                         .arg(QFileInfo(debPath).fileName()));
                    emit statusChanged("Atualização e instalação concluídas. Reiniciando...");
                    // Reinicia o softphone automaticamente na nova versão
                    restartApplication();
                } else {
                    emit updateCompleted(false, QString("Atualização cancelada ou com erro. Para instalar manualmente:\nsudo dpkg -i %1").arg(debPath));
                    emit statusChanged("Aguardando instalação do pacote.");
                }
            });
            instProc->start("pkexec", {"dpkg", "-i", debPath});
        } else {
            emit updateCompleted(true, QString("Atualizações sincronizadas.\nInstale manualmente executando:\nsudo dpkg -i %1").arg(debPath));
            emit statusChanged("Atualização sincronizada.");
        }
    });

    process->start("git", {"pull", "--rebase"});
}

void UpdateManager::restartApplication() {
    QString execPath = "/usr/bin/VxVoice";
    if (!QFile::exists(execPath)) {
        execPath = QCoreApplication::applicationFilePath();
    }

    // Inicia nova instância desacoplada informando a atualização recém-feita
    bool started = QProcess::startDetached(execPath, {"--just-updated"});
    if (started) {
        // Encerra imediatamente o processo atual
        QTimer::singleShot(400, []() {
            QCoreApplication::quit();
        });
    }
}
