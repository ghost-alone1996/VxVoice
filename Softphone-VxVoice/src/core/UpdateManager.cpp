#include "UpdateManager.h"
#include "SettingsManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QDebug>

#ifndef VXVOICE_VERSION
#define VXVOICE_VERSION "2.0.3"
#endif

namespace {
constexpr const char* GITHUB_REPO_URL      = "https://github.com/ghost-alone1996/VxVoice";
constexpr const char* GITHUB_API_RELEASE   = "https://api.github.com/repos/ghost-alone1996/VxVoice/releases/latest";
constexpr const char* GITHUB_API_TAGS      = "https://api.github.com/repos/ghost-alone1996/VxVoice/tags?per_page=15";
constexpr const char* GITHUB_API_COMMIT    = "https://api.github.com/repos/ghost-alone1996/VxVoice/commits/main";
constexpr const char* GITHUB_RAW_CMAKE     = "https://raw.githubusercontent.com/ghost-alone1996/VxVoice/main/Softphone-VxVoice/CMakeLists.txt";

QNetworkRequest makeGithubRequest(const QString &url, bool isApi = true) {
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, "VxVoice-Softphone-Updater/2.0.3");
    if (isApi) {
        req.setRawHeader("Accept", "application/vnd.github+json");
    }
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(15000);
    return req;
}

QString stripVPrefix(const QString &v) {
    QString clean = v.trimmed();
    if (clean.startsWith('v', Qt::CaseInsensitive)) {
        clean = clean.mid(1);
    }
    return clean;
}
} // namespace

UpdateManager& UpdateManager::instance() {
    static UpdateManager instance;
    return instance;
}

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent),
      m_nam(new QNetworkAccessManager(this)),
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

QString UpdateManager::githubRepoUrl() const {
    return QString(GITHUB_REPO_URL);
}

bool UpdateManager::isGitRepository() const {
    return !m_repoPath.isEmpty() && QDir(m_repoPath + "/.git").exists();
}

bool UpdateManager::isUpdateAvailable() const {
    return m_isUpdateAvailable;
}

bool UpdateManager::isSilentCheck() const {
    return m_silent;
}

int UpdateManager::compareVersions(const QString &v1, const QString &v2) {
    QString clean1 = stripVPrefix(v1);
    QString clean2 = stripVPrefix(v2);

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
    QDir dir(QDir::currentPath());
    for (int i = 0; i < 5; ++i) {
        if (dir.exists(".git")) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) break;
    }

    QDir appDir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        if (appDir.exists(".git")) {
            return appDir.absolutePath();
        }
        if (!appDir.cdUp()) break;
    }

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
    m_downloadAssetUrl.clear();
    m_downloadAssetName.clear();

    emit checkingStarted();
    emit statusChanged("Consultando repositório no GitHub...");

    fetchGithubLatestRelease();
}

void UpdateManager::fetchGithubLatestRelease() {
    auto reply = m_nam->get(makeGithubRequest(GITHUB_API_RELEASE, true));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && httpStatus == 200) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();
            QString tagName = obj.value("tag_name").toString().trimmed();
            QString body = obj.value("body").toString().trimmed();

            if (!tagName.isEmpty()) {
                const QJsonArray assets = obj.value("assets").toArray();
#ifdef Q_OS_WIN
                const QString targetExt = ".msi";
#else
                const QString targetExt = ".deb";
#endif
                for (const auto &assetVal : assets) {
                    QJsonObject asset = assetVal.toObject();
                    QString name = asset.value("name").toString();
                    if (name.endsWith(targetExt, Qt::CaseInsensitive)) {
                        m_downloadAssetName = name;
                        m_downloadAssetUrl = asset.value("browser_download_url").toString();
                        break;
                    }
                }

                QString remoteVer = stripVPrefix(tagName);
                fetchGithubMainCommit(remoteVer, body);
                return;
            }
        }

        // Sem release publicada ainda (404) ou erro de API: consulta tags e CMakeLists.txt no branch main
        if (httpStatus == 404 || reply->error() == QNetworkReply::ContentNotFoundError) {
            fetchGithubTagsAndMain();
            return;
        }

        // Caso ocorra erro de rede (sem internet, DNS, etc.), tenta informar claramente
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[UpdateManager] Erro ao consultar GitHub Releases:" << reply->errorString();
            fetchGithubTagsAndMain();
        } else {
            fetchGithubTagsAndMain();
        }
    });
}

void UpdateManager::fetchGithubTagsAndMain() {
    auto rawReply = m_nam->get(makeGithubRequest(GITHUB_RAW_CMAKE, false));
    connect(rawReply, &QNetworkReply::finished, this, [this, rawReply]() {
        rawReply->deleteLater();

        QString cmakeVer;
        if (rawReply->error() == QNetworkReply::NoError) {
            QString content = QString::fromUtf8(rawReply->readAll());
            QRegularExpression re(R"(project\s*\(\s*VxVoice[^)]*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+))",
                                  QRegularExpression::DotMatchesEverythingOption);
            QRegularExpressionMatch m = re.match(content);
            if (m.hasMatch()) {
                cmakeVer = m.captured(1).trimmed();
            }
        } else {
            m_isChecking = false;
            QString err = QString("Não foi possível conectar ao GitHub (%1).").arg(rawReply->errorString());
            m_gitVersion = m_currentVersion;
            emit updateError(err);
            emit statusChanged(err);
            return;
        }

        auto tagsReply = m_nam->get(makeGithubRequest(GITHUB_API_TAGS, true));
        connect(tagsReply, &QNetworkReply::finished, this, [this, tagsReply, cmakeVer]() {
            tagsReply->deleteLater();

            QString bestTagVer;
            if (tagsReply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(tagsReply->readAll());
                const QJsonArray arr = doc.array();
                for (const auto &item : arr) {
                    QString t = stripVPrefix(item.toObject().value("name").toString());
                    if (!t.isEmpty() && (bestTagVer.isEmpty() || compareVersions(t, bestTagVer) > 0)) {
                        bestTagVer = t;
                    }
                }
            }

            QString remoteVer = m_currentVersion;
            if (!cmakeVer.isEmpty() && compareVersions(cmakeVer, remoteVer) > 0) {
                remoteVer = cmakeVer;
            }
            if (!bestTagVer.isEmpty() && compareVersions(bestTagVer, remoteVer) > 0) {
                remoteVer = bestTagVer;
            }

            // Se nenhuma versão maior que a atual foi encontrada, usa a versão detectada no GitHub
            if (!cmakeVer.isEmpty() && bestTagVer.isEmpty()) {
                remoteVer = cmakeVer;
            } else if (!bestTagVer.isEmpty() && cmakeVer.isEmpty()) {
                remoteVer = bestTagVer;
            } else if (!cmakeVer.isEmpty() && !bestTagVer.isEmpty()) {
                remoteVer = (compareVersions(cmakeVer, bestTagVer) >= 0) ? cmakeVer : bestTagVer;
            }

            fetchGithubMainCommit(remoteVer, QString());
        });
    });
}

void UpdateManager::fetchGithubMainCommit(const QString &detectedVersion, const QString &releaseNotes) {
    auto commitReply = m_nam->get(makeGithubRequest(GITHUB_API_COMMIT, true));
    connect(commitReply, &QNetworkReply::finished, this, [this, commitReply, detectedVersion, releaseNotes]() {
        commitReply->deleteLater();

        QString commitSummary;
        if (commitReply->error() == QNetworkReply::NoError) {
            QJsonObject obj = QJsonDocument::fromJson(commitReply->readAll()).object();
            QString sha = obj.value("sha").toString().left(7);
            QJsonObject commitObj = obj.value("commit").toObject();
            QString msg = commitObj.value("message").toString().split('\n').first().trimmed();
            QString date = commitObj.value("author").toObject().value("date").toString().left(10);
            if (!sha.isEmpty()) {
                commitSummary = QString("%1 - %2 (%3)").arg(sha, msg, date);
            }
        }

        QString changelog;
        if (!releaseNotes.isEmpty()) {
            changelog = releaseNotes;
            if (!commitSummary.isEmpty()) {
                changelog += QString("\n\nCommit GitHub: %1").arg(commitSummary);
            }
        } else if (!commitSummary.isEmpty()) {
            changelog = QString("Repositório GitHub: %1\nÚltimo commit (main): %2").arg(GITHUB_REPO_URL, commitSummary);
        } else {
            changelog = QString("Repositório GitHub: %1").arg(GITHUB_REPO_URL);
        }

        finalizeUpdateCheck(detectedVersion, changelog);
    });
}

void UpdateManager::finalizeUpdateCheck(const QString &remoteVer, const QString &changelog) {
    m_isChecking = false;
    m_gitVersion = remoteVer.isEmpty() ? m_currentVersion : remoteVer;

    int cmp = compareVersions(m_gitVersion, m_currentVersion);
    if (cmp > 0) {
        m_isUpdateAvailable = true;
        QString fullLog = QString("Nova versão v%1 disponível no GitHub.\n%2").arg(m_gitVersion, changelog);
        emit updateChecked(m_currentVersion, m_gitVersion, true, fullLog);
        emit updateAvailable(m_gitVersion, fullLog);
        emit statusChanged(QString("Nova versão v%1 disponível no GitHub.").arg(m_gitVersion));
    } else {
        m_isUpdateAvailable = false;
        QString msg = QString("O software está na versão mais recente (%1).\n%2").arg(m_currentVersion, changelog);
        emit updateChecked(m_currentVersion, m_gitVersion, false, msg);
        emit upToDate(msg);
        emit statusChanged(QString("Versão %1 (Atualizada com o GitHub)").arg(m_currentVersion));
    }
}

void UpdateManager::applyUpdate() {
    // 1. Se houver pacote instalador (.deb / .msi) anexado na Release do GitHub, baixa diretamente do GitHub
    if (!m_downloadAssetUrl.isEmpty()) {
        emit statusChanged("Baixando instalador oficial do GitHub...");
        emit updateProgress(10, QString("Baixando %1 do GitHub...").arg(m_downloadAssetName));

        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString targetFile = tempDir + "/" + m_downloadAssetName;

        auto reply = m_nam->get(makeGithubRequest(m_downloadAssetUrl, false));
        connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
            if (total > 0) {
                int pct = 10 + static_cast<int>((received * 55) / total);
                emit updateProgress(pct, QString("Baixando atualização do GitHub (%1%)...").arg((received * 100) / total));
            }
        });

        connect(reply, &QNetworkReply::finished, this, [this, reply, targetFile]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                emit updateProgress(0, "Erro ao baixar pacote do GitHub.");
                emit updateCompleted(false, QString("Falha no download do GitHub: %1").arg(reply->errorString()));
                return;
            }

            QFile outFile(targetFile);
            if (!outFile.open(QIODevice::WriteOnly)) {
                emit updateProgress(0, "Erro ao salvar pacote temporário.");
                emit updateCompleted(false, "Não foi possível salvar o instalador baixado em " + targetFile);
                return;
            }
            outFile.write(reply->readAll());
            outFile.close();

#ifdef Q_OS_WIN
            emit updateProgress(85, "Iniciando instalador Windows (.msi)...");
            if (QProcess::startDetached("msiexec", {"/i", QDir::toNativeSeparators(targetFile), "/passive"})) {
                emit updateProgress(100, "Instalador iniciado! Encerrando VxVoice para concluir...");
                emit updateCompleted(true, "Instalador .msi iniciado com sucesso. O VxVoice será atualizado.");
                QTimer::singleShot(500, []() { QCoreApplication::quit(); });
            } else {
                emit updateProgress(0, "Falha ao executar msiexec.");
                emit updateCompleted(false, "Execute manualmente o instalador em: " + targetFile);
            }
#else
            installLinuxDebPackage(targetFile);
#endif
        });
        return;
    }

    // 2. Caso ainda não haja asset na Release do GitHub, sincroniza via git pull do GitHub e compila/instala
    if (!isGitRepository()) {
        emit updateProgress(0, "Nenhum instalador encontrado na Release do GitHub.");
        emit updateCompleted(false,
            QString("A versão v%1 está no GitHub (%2), porém nenhum pacote instalador (.deb/.msi) foi publicado nas Releases ainda.")
                .arg(m_gitVersion, GITHUB_REPO_URL));
        emit statusChanged("Aguardando publicação do pacote na Release do GitHub.");
        return;
    }

    emit statusChanged("Sincronizando repositório com o GitHub...");
    emit updateProgress(15, "Sincronizando código-fonte com o GitHub (git pull)...");

    auto process = new QProcess(this);
    process->setWorkingDirectory(m_repoPath);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/) {
        process->deleteLater();

        emit updateProgress(45, "Localizando ou gerando pacote Debian de instalação...");

        QString debPath;
        QStringList searchDirs = {
            m_repoPath,
            m_repoPath + "/Softphone-VxVoice/build",
            m_repoPath + "/build",
            QFileInfo(m_repoPath).dir().absolutePath()
        };

        QString expectedName = QString("vxvoice-%1-Linux.deb").arg(m_gitVersion);
        for (const auto &sdir : searchDirs) {
            QDir d(sdir);
            if (!d.exists()) continue;
            if (QFile::exists(d.filePath(expectedName))) {
                debPath = d.filePath(expectedName);
                break;
            }
        }

        if (debPath.isEmpty()) {
            // Tenta compilar e empacotar o código recém-sincronizado do GitHub
            QString buildDir = m_repoPath + "/Softphone-VxVoice/build";
            if (QDir(buildDir).exists()) {
                emit updateProgress(55, "Compilando nova versão sincronizada do GitHub...");
                auto buildProc = new QProcess(this);
                buildProc->setWorkingDirectory(buildDir);
                connect(buildProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        this, [this, buildProc, buildDir](int bCode, QProcess::ExitStatus bStatus) {
                    buildProc->deleteLater();
                    if (bStatus == QProcess::NormalExit && bCode == 0) {
                        QDir bd(buildDir);
                        auto debFiles = bd.entryInfoList({"*.deb"}, QDir::Files, QDir::Time);
                        if (!debFiles.isEmpty()) {
                            installLinuxDebPackage(debFiles.first().absoluteFilePath());
                            return;
                        }
                    }
                    emit updateProgress(0, "Erro ao gerar pacote .deb da nova versão.");
                    emit updateCompleted(false, "Código sincronizado com o GitHub, mas não foi possível gerar o pacote .deb automaticamente.");
                });
                buildProc->start("bash", {"-c", "cmake .. && make -j$(nproc) && cpack -G DEB"});
                return;
            }

            for (const auto &sdir : searchDirs) {
                QDir d(sdir);
                if (!d.exists()) continue;
                auto debFiles = d.entryInfoList({"*.deb"}, QDir::Files, QDir::Time);
                if (!debFiles.isEmpty()) {
                    debPath = debFiles.first().absoluteFilePath();
                    break;
                }
            }
        }

        if (debPath.isEmpty()) {
            emit updateProgress(0, "Erro: Pacote .deb de instalação não encontrado.");
            emit updateCompleted(false, "Nenhum pacote .deb de atualização encontrado.");
            emit statusChanged("Erro: Pacote .deb não localizado.");
            return;
        }

        installLinuxDebPackage(debPath);
    });

    process->start("bash", {"-c", "git fetch origin --tags && git pull --rebase origin main"});
}

void UpdateManager::installLinuxDebPackage(const QString &debPath) {
    if (QFile::exists("/usr/bin/pkexec")) {
        emit statusChanged("Instalando pacote com privilégios de administrador...");
        emit updateProgress(75, QString("Instalando %1 via gerenciador de pacotes...").arg(QFileInfo(debPath).fileName()));
        auto instProc = new QProcess(this);
        connect(instProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, instProc, debPath](int instCode, QProcess::ExitStatus instStatus) {
            instProc->deleteLater();
            if (instStatus == QProcess::NormalExit && instCode == 0) {
                emit updateProgress(100, "Instalação concluída com sucesso! Reiniciando o VxVoice...");
                emit updateCompleted(true, QString("VxVoice atualizado com sucesso!\n\nPacote instalado: %1\n\nReiniciando para carregar a nova versão...")
                                     .arg(QFileInfo(debPath).fileName()));
                emit statusChanged("Atualização e instalação concluídas. Reiniciando...");
                restartApplication();
            } else {
                emit updateProgress(0, "A instalação foi cancelada ou falhou.");
                emit updateCompleted(false, QString("Atualização cancelada ou com erro. Para instalar manualmente:\nsudo dpkg -i %1").arg(debPath));
                emit statusChanged("Aguardando instalação do pacote.");
            }
        });
        instProc->start("pkexec", {"dpkg", "-i", debPath});
    } else {
        emit updateProgress(100, "Atualização sincronizada.");
        emit updateCompleted(true, QString("Pacote pronto.\nInstale manualmente executando:\nsudo dpkg -i %1").arg(debPath));
        emit statusChanged("Atualização pronta.");
    }
}

void UpdateManager::restartApplication() {
    QString execPath = "/usr/bin/VxVoice";
    if (!QFile::exists(execPath)) {
        execPath = QCoreApplication::applicationFilePath();
    }

    bool started = QProcess::startDetached(execPath, {"--just-updated"});
    if (started) {
        QTimer::singleShot(400, []() {
            QCoreApplication::quit();
        });
    }
}
