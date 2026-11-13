#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>

enum class HistoryCallType {
    Incoming,
    Outgoing,
    Missed
};

struct HistoryRecord {
    HistoryCallType type;
    QString remoteNumber;
    QString remoteName;
    QString timestamp;
    QString duration;
    QString recordingPath;
};

class HistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(QWidget *parent = nullptr);

    void addRecord(HistoryCallType type, const QString &number, const QString &name,
                   const QString &duration, const QString &recordingPath = QString());

signals:
    void callNumberRequested(const QString &number);
    void saveContactRequested(const QString &name, const QString &number);
    void playRecordingRequested(const QString &filePath);

private slots:
    void onItemDoubleClicked(const QModelIndex &index);
    void onContextMenu(const QPoint &pos);
    void onClearHistory();

private:
    void setupUi();
    void loadHistory();
    void saveHistory();

    QTableView *m_tableView{nullptr};
    QStandardItemModel *m_model{nullptr};
    QPushButton *m_btnClear{nullptr};
};

#endif // HISTORYWIDGET_H
