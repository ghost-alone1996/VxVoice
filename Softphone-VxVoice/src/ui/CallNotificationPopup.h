#ifndef CALLNOTIFICATIONPOPUP_H
#define CALLNOTIFICATIONPOPUP_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

class CallNotificationPopup : public QWidget {
    Q_OBJECT

public:
    explicit CallNotificationPopup(QWidget *parent = nullptr);

    void showIncomingCall(const QString &callId, const QString &number, const QString &name);
    void showCallStatus(const QString &title, const QString &message);
    void dismiss();
    void updateThemeVisuals();

signals:
    void answerClicked(const QString &callId);
    void rejectClicked(const QString &callId);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void repositionBottomRight();

    QString m_callId;
    QLabel *m_headerLabel{nullptr};
    QLabel *m_callerNameLabel{nullptr};
    QLabel *m_callerNumberLabel{nullptr};
    QLabel *m_statusMessageLabel{nullptr};
    QPushButton *m_btnAnswer{nullptr};
    QPushButton *m_btnReject{nullptr};
    QPushButton *m_btnClose{nullptr};
    QWidget *m_actionsContainer{nullptr};
    QFrame *m_cardFrame{nullptr};
};

#endif // CALLNOTIFICATIONPOPUP_H
