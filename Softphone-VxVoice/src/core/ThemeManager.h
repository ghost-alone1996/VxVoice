#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QApplication>

class ThemeManager : public QObject {
    Q_OBJECT

public:
    enum class Theme {
        Dark,    // Tons de azul escuro
        Light,   // Tons de azul claro
        Purple,  // Tons de roxo (VxVoice)
        Red,     // Tons de vermelho
        VxVoice = Purple
    };
    Q_ENUM(Theme)

    static ThemeManager& instance();

    void setTheme(Theme theme);
    Theme currentTheme() const;
    void toggleTheme();
    QString themeName(Theme theme) const;

signals:
    void themeChanged(Theme newTheme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    QString loadStyleSheet(const QString &resourcePath);

    Theme m_currentTheme{Theme::Purple};
};

#endif // THEMEMANAGER_H
