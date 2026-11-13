#ifndef CHANGELOGDIALOG_H
#define CHANGELOGDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;
class QTextBrowser;
class QCheckBox;

class ChangelogDialog : public QDialog {
    Q_OBJECT

public:
    explicit ChangelogDialog(QWidget *parent = nullptr);
    ~ChangelogDialog() override = default;

private slots:
    void onAcknowledgeClicked();

private:
    void setupUi();

    QTextBrowser *m_textBrowser{nullptr};
    QPushButton *m_btnOk{nullptr};
};

#endif // CHANGELOGDIALOG_H
