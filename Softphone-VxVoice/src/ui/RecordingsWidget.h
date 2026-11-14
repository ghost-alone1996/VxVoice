#ifndef RECORDINGSWIDGET_H
#define RECORDINGSWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>

#ifdef HAVE_QT_MULTIMEDIA
#include <QMediaPlayer>
#include <QAudioOutput>
#endif

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

private slots:
    void onSearchTextChanged(const QString &text);
    void onPlayRecording();
    void onStopPlayback();
    void onOpenFolder();
    void onDeleteRecording();
    void onSelectionChanged();
    void onSliderMoved(int positionMs);

private:
    void setupUi();
    void updateTimeLabel(qint64 positionMs, qint64 durationMs);
    static QString formatMs(qint64 ms);

    QLineEdit *m_searchEdit{nullptr};
    QTableView *m_tableView{nullptr};
    QStandardItemModel *m_sourceModel{nullptr};
    QSortFilterProxyModel *m_proxyModel{nullptr};

    QFrame *m_playerFrame{nullptr};
    QLabel *m_lblNowPlaying{nullptr};
    QSlider *m_seekSlider{nullptr};
    QLabel *m_lblTime{nullptr};

    QPushButton *m_btnPlay{nullptr};
    QPushButton *m_btnStop{nullptr};
    QPushButton *m_btnOpenFolder{nullptr};
    QPushButton *m_btnDelete{nullptr};
    QPushButton *m_btnRefresh{nullptr};

#ifdef HAVE_QT_MULTIMEDIA
    QMediaPlayer *m_player{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
#endif

    QString m_currentPlayingPath;
    bool m_isSliderPressed{false};
    QList<RecordingItem> m_items;
};

#endif // RECORDINGSWIDGET_H
