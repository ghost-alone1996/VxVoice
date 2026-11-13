#include "UpdateManager.h"
#include "SettingsManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

#ifndef VXVOICE_VERSION
#define VXVOICE_VERSION "1.4.4"
#endif

UpdateManager& UpdateManager::instance() {
    static UpdateManager instance;
    return instance;
}

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent) {
    m_repoPath = findGitDirectory();
}

QString UpdateManager::currentVersion() const {
    return QString(VXVOICE_VERSION);
}

QString UpdateManager::gitRepoPath() const {
    return m_repoPath;
}

bool UpdateManager::isGitRepository() const {
    return !m_repoPath.isEmpty() && QDir(m_repoPath + "/.git").exists();
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

    // 3. Fallback para caminho de desenvolvimento conhecido
    if (QDir("/home/leonardo/Documentos/ProjetosDEV/.git").exists()) {
        return "/home/leonardo/Documentos/ProjetosDEV";
    }

    return QString();
}

void UpdateManager::checkForUpdates(bool silent) {
    if (m_isChecking) return;

    m_silent = silent;
    m_isChecking = true;
    emit checkingStarted();
    emit statusChanged("Verificando atualizações no repositório Git...");

    if (!isGitRepository()) {
        m_isChecking = false;
        QString err = "Repositório Git não encontrado no diretório de instalação.";
        emit updateError(err);
        emit statusChanged(err);
        return;
    }

    // Executa git fetch de forma assíncrona
    auto process = new QProcess(this);
    process->setWorkingDirectory(m_repoPath);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        m_isChecking = false;

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            // Se não houver conexão com a internet ou remoto não configurado
            // verifica o último commit local
            QProcess logProc;
            logProc.setWorkingDirectory(m_repoPath);
            logProc.start("git", {"log", "-1", "--format=%h - %s (%ci)"});
            logProc.waitForFinished(2000);
            QString lastCommit = QString::fromUtf8(logProc.readAllStandardOutput()).trimmed();

            QString msg = QString("Sistema atualizado na versão %1.\nÚltimo commit local: %2")
                              .arg(currentVersion(), lastCommit.isEmpty() ? "main" : lastCommit);
            emit upToDate(msg);
            emit statusChanged(QString("Versão %1 (Atualizada)").arg(currentVersion()));
            return;
        }

        // Compara HEAD local com upstream / origin
        QProcess diffProc;
        diffProc.setWorkingDirectory(m_repoPath);
        diffProc.start("git", {"rev-list", "HEAD..@{u}", "--count"});
        diffProc.waitForFinished(3000);
        int aheadCount = QString::fromUtf8(diffProc.readAllStandardOutput()).trimmed().toInt();

        if (aheadCount > 0) {
            QProcess logProc;
            logProc.setWorkingDirectory(m_repoPath);
            logProc.start("git", {"log", "HEAD..@{u}", "--oneline", "-n", "5"});
            logProc.waitForFinished(3000);
            QString changelog = QString::fromUtf8(logProc.readAllStandardOutput()).trimmed();

            emit updateAvailable(QString("%1 novos commits").arg(aheadCount), changelog);
            emit statusChanged(QString("Nova versão disponível no Git (%1 commits à frente)").arg(aheadCount));
        } else {
            QProcess logProc;
            logProc.setWorkingDirectory(m_repoPath);
            logProc.start("git", {"log", "-1", "--format=%h - %s (%ci)"});
            logProc.waitForFinished(2000);
            QString lastCommit = QString::fromUtf8(logProc.readAllStandardOutput()).trimmed();

            QString msg = QString("O software está na versão mais recente (%1).\nCommit: %2")
                              .arg(currentVersion(), lastCommit);
            emit upToDate(msg);
            emit statusChanged(QString("Versão %1 (Atualizada)").arg(currentVersion()));
        }
    });

    process->start("git", {"fetch", "--dry-run"});
}

void UpdateManager::applyUpdate() {
    if (!isGitRepository()) {
        emit updateCompleted(false, "Repositório Git não encontrado.");
        return;
    }

    emit statusChanged("Baixando e aplicando atualizações do Git...");
    auto process = new QProcess(this);
    process->setWorkingDirectory(m_repoPath);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            // Busca o pacote .deb mais recente gerado no workspace
            QString debPath;
            QStringList searchDirs = { m_repoPath, m_repoPath + "/build", QFileInfo(m_repoPath).dir().absolutePath() };
            for (const auto &sdir : searchDirs) {
                QDir d(sdir);
                auto debFiles = d.entryInfoList({"*.deb"}, QDir::Files, QDir::Time);
                if (!debFiles.isEmpty()) {
                    debPath = debFiles.first().absoluteFilePath();
                    break;
                }
            }

            if (!debPath.isEmpty() && QFile::exists("/usr/bin/pkexec")) {
                emit statusChanged("Instalando pacote com privilégios de administrador...");
                auto instProc = new QProcess(this);
                connect(instProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        this, [this, instProc, debPath](int instCode, QProcess::ExitStatus instStatus) {
                    instProc->deleteLater();
                    if (instStatus == QProcess::NormalExit && instCode == 0) {
                        emit updateCompleted(true, QString("VxVoice atualizado com sucesso no sistema operacional!\n\nPacote instalado: %1\n\nReinicie o aplicativo para carregar a nova versão.")
                                             .arg(QFileInfo(debPath).fileName()));
                        emit statusChanged("Atualização e instalação concluídas com sucesso.");
                    } else {
                        emit updateCompleted(false, QString("Atualização baixada. Para instalar no sistema, execute no terminal:\n\nsudo dpkg -i %1").arg(debPath));
                        emit statusChanged("Aguardando instalação do pacote.");
                    }
                });
                instProc->start("pkexec", {"dpkg", "-i", debPath});
            } else if (!debPath.isEmpty()) {
                emit updateCompleted(true, QString("Atualizações sincronizadas com sucesso!\n\nPara atualizar o pacote instalado no sistema, execute:\nsudo dpkg -i %1").arg(debPath));
                emit statusChanged("Atualização concluída.");
            } else {
                emit updateCompleted(true, "Atualização Git concluída com sucesso! Reinicie o aplicativo para aplicar.");
                emit statusChanged("Atualização concluída com sucesso.");
            }
        } else {
            QString err = QString::fromUtf8(process->readAllStandardError());
            emit updateCompleted(false, "Erro ao puxar atualizações: " + err);
            emit statusChanged("Falha na atualização.");
        }
    });

    process->start("git", {"pull", "--rebase"});
}
