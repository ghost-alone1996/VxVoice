#ifndef ICONHELPER_H
#define ICONHELPER_H

#include <QIcon>
#include <QColor>
#include <QPixmap>

class IconHelper {
public:
    static QIcon createStatusDot(const QColor &color, int size = 16);
    static QIcon createUserIcon(const QColor &color = QColor("#ECF0F1"), int size = 22);
    static QIcon createSettingsIcon(const QColor &color = QColor("#ECF0F1"), int size = 20);
    static QIcon createBellIcon(const QColor &color = QColor("#F1C40F"), int size = 20);
    static QIcon createPhoneIcon(const QColor &color = Qt::white, int size = 24);
    static QIcon createCloseIcon(const QColor &color = Qt::white, int size = 24);
    static QIcon createBackspaceIcon(const QColor &color = Qt::white, int size = 24);
    static QPixmap createUserAvatarPixmap(const QColor &bgColor, const QColor &fgColor, int size = 80);
};

#endif // ICONHELPER_H
