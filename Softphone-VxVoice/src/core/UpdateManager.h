#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QNetworkAccessManager>

class UpdateManager : public QObject {
    Q_OBJECT

public:
    static UpdateManager& instance();

    QString currentVersion() const;
    QString gitVersion() const;
    QString gitRepoPath() const;
    QString githubRepoUrl() const;
    bool isGitRepository() const;
    bool isUpdateAvailable() const;
    bool isSilentCheck() const;

    void checkForUpdates(bool silent = false);
    void applyUpdate();
    void restartApplication();

signals:
    void checkingStarted();
    void updateChecked(const QString &currentVer, const QString &gitVer, bool hasUpdate, const QString &changelog);
    void updateAvailable(const QString &latestCommit, const QString &changelog);
    void upToDate(const QString &message);
    void updateError(const QString &errorMessage);
    void updateProgress(int percent, const QString &statusText);
    void updateCompleted(bool success, const QString &message);
    void statusChanged(const QString &status);

private:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager() override = default;
    UpdateManager(const UpdateManager&) = delete;
    UpdateManager& operator=(const UpdateManager&) = delete;

    QString findGitDirectory() const;
    static int compareVersions(const QString &v1, const QString &v2);

    void fetchGithubLatestRelease();
    void fetchGithubTagsAndMain();
    void fetchGithubMainCommit(const QString &detectedVersion, const QString &releaseNotes);
    void finalizeUpdateCheck(const QString &remoteVer, const QString &changelog);
    void installLinuxDebPackage(const QString &debPath);

    QNetworkAccessManager *m_nam{nullptr};
    bool m_isChecking{false};
    bool m_silent{false};
    bool m_isUpdateAvailable{false};
    QString m_currentVersion;
    QString m_gitVersion;
    QString m_repoPath;
    QString m_downloadAssetUrl;
    QString m_downloadAssetName;
};

#endif // UPDATEMANAGER_H
