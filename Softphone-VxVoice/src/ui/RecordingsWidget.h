#ifndef RECORDINGSWIDGET_H
#define RECORDINGSWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

struct RecordingItem {
    QString filePath;
    QString contactName;
    QString contactNumber;
    QString callType; // "Recebida" ou "Efetuada"
    QString dateTimeStr;
    QString durationStr;
    QString fileSizeStr;
};

class RecordingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit RecordingsWidget(QWidget *parent = nullptr);
    ~RecordingsWidget() override;

    void refreshList();
    void adjustTableColumns();
    void addRecordingRecord(const QString &filePath, const QString &number,
                            const QString &name, const QString &callType, const QString &duration);
    void playFile(const QString &filePath, const QString &displayTitle = QString());

signals:
    void callRequested(const QString &number);
    void playRecordingRequested(const QString &filePath, const QString &displayTitle);
    void recordingDeleted(const QString &filePath);

private slots:
    void onSearchTextChanged(const QString &text);
    void onPlayRecording();
    void onOpenFolder();
    void onDeleteRecording();
    void onSelectionChanged();

private:
    void setupUi();

    QLineEdit *m_searchEdit{nullptr};
    QTableView *m_tableView{nullptr};
    QStandardItemModel *m_sourceModel{nullptr};
    QSortFilterProxyModel *m_proxyModel{nullptr};

    QPushButton *m_btnPlay{nullptr};
    QPushButton *m_btnOpenFolder{nullptr};
    QPushButton *m_btnDelete{nullptr};
    QPushButton *m_btnRefresh{nullptr};

    QList<RecordingItem> m_items;
};

#endif // RECORDINGSWIDGET_H
