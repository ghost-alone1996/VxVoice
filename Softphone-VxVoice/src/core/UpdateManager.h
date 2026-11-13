#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QString>
#include <QProcess>

class UpdateManager : public QObject {
    Q_OBJECT

public:
    static UpdateManager& instance();

    QString currentVersion() const;
    QString gitRepoPath() const;
    bool isGitRepository() const;

    void checkForUpdates(bool silent = false);
    void applyUpdate();

signals:
    void checkingStarted();
    void updateAvailable(const QString &latestCommit, const QString &changelog);
    void upToDate(const QString &message);
    void updateError(const QString &errorMessage);
    void updateCompleted(bool success, const QString &message);
    void statusChanged(const QString &status);

private:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager() override = default;
    UpdateManager(const UpdateManager&) = delete;
    UpdateManager& operator=(const UpdateManager&) = delete;

    QString findGitDirectory() const;

    bool m_isChecking{false};
    bool m_silent{false};
    QString m_repoPath;
};

#endif // UPDATEMANAGER_H
