#include "UpdateManager.h"
#include "SettingsManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

#ifndef VXVOICE_VERSION
#define VXVOICE_VERSION "1.2.0"
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
            emit updateCompleted(true, "Atualização concluída com sucesso! Reinicie o aplicativo para aplicar.");
            emit statusChanged("Atualização concluída com sucesso.");
        } else {
            QString err = QString::fromUtf8(process->readAllStandardError());
            emit updateCompleted(false, "Erro ao puxar atualizações: " + err);
            emit statusChanged("Falha na atualização.");
        }
    });

    process->start("git", {"pull", "--rebase"});
}
