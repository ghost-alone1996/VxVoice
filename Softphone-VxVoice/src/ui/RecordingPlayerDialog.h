#ifndef RECORDINGPLAYERDIALOG_H
#define RECORDINGPLAYERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QFrame>
#include <QCloseEvent>

#ifdef HAVE_QT_MULTIMEDIA
#include <QMediaPlayer>
#include <QAudioOutput>
#endif

class RecordingPlayerDialog : public QDialog {
    Q_OBJECT

public:
    explicit RecordingPlayerDialog(QWidget *parent = nullptr);
    ~RecordingPlayerDialog() override;

    void playFile(const QString &filePath, const QString &displayTitle = QString());
    void stopPlayback();
    void stopIfPlaying(const QString &filePath);

protected:
    void closeEvent(QCloseEvent *event) override;
    void reject() override;

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onOpenFolderClicked();
    void onSliderMoved(int positionMs);
    void applyThemeStyling();

private:
    void setupUi();
    void updateTimeLabel(qint64 positionMs, qint64 durationMs);
    static QString formatMs(qint64 ms);

    QLabel *m_lblTitle{nullptr};
    QLabel *m_lblFileName{nullptr};
    QSlider *m_seekSlider{nullptr};
    QLabel *m_lblTime{nullptr};

    QPushButton *m_btnPlayPause{nullptr};
    QPushButton *m_btnStop{nullptr};
    QPushButton *m_btnOpenFolder{nullptr};

#ifdef HAVE_QT_MULTIMEDIA
    QMediaPlayer *m_player{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
#endif

    QString m_currentFilePath;
    bool m_isSliderPressed{false};
};

#endif // RECORDINGPLAYERDIALOG_H
