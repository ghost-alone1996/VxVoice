#ifndef UPDATEPROGRESSDIALOG_H
#define UPDATEPROGRESSDIALOG_H

#include <QDialog>
#include <QCloseEvent>

class QProgressBar;
class QLabel;
class QPushButton;

class UpdateProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpdateProgressDialog(QWidget *parent = nullptr);
    ~UpdateProgressDialog() override = default;

public slots:
    void setProgress(int percent, const QString &statusText);
    void onUpdateCompleted(bool success, const QString &message);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();

    QLabel *m_lblTitle{nullptr};
    QLabel *m_lblStatus{nullptr};
    QProgressBar *m_progressBar{nullptr};
    QPushButton *m_btnClose{nullptr};
    bool m_isUpdating{true};
};

#endif // UPDATEPROGRESSDIALOG_H
