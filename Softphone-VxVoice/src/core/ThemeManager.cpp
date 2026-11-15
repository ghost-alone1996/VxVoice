#include "ThemeManager.h"
#include <QFile>
#include <QTextStream>
#include <QPalette>
#include <QColor>
#include <QDebug>

ThemeManager& ThemeManager::instance() {
    static ThemeManager s_instance;
    return s_instance;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent), m_currentTheme(Theme::Purple) {
}

void ThemeManager::setTheme(Theme theme) {
    m_currentTheme = theme;
    QString path;
    if (theme == Theme::Purple) {
        path = ":/styles/vxvoice.qss";
    } else if (theme == Theme::Dark) {
        path = ":/styles/dark.qss";
    } else if (theme == Theme::Light) {
        path = ":/styles/light.qss";
    } else if (theme == Theme::Red) {
        path = ":/styles/red.qss";
    } else if (theme == Theme::Win2000) {
        path = ":/styles/win2000.qss";
    } else if (theme == Theme::WinXP) {
        path = ":/styles/winxp.qss";
    } else if (theme == Theme::Win7) {
        path = ":/styles/win7.qss";
    } else if (theme == Theme::CinnamonLight) {
        path = ":/styles/cinnamon_light.qss";
    } else if (theme == Theme::CinnamonDark) {
        path = ":/styles/cinnamon_dark.qss";
    }

    QString qss = loadStyleSheet(path);

    if (qApp) {
        if (theme == Theme::Purple) {
            // Paleta Oficial VxVoice (Roxo Corporativo)
            QPalette purplePalette;
            purplePalette.setColor(QPalette::Window, QColor("#171226"));
            purplePalette.setColor(QPalette::WindowText, QColor("#F5EEF8"));
            purplePalette.setColor(QPalette::Base, QColor("#120D1F"));
            purplePalette.setColor(QPalette::AlternateBase, QColor("#1E1632"));
            purplePalette.setColor(QPalette::ToolTipBase, QColor("#F5EEF8"));
            purplePalette.setColor(QPalette::ToolTipText, QColor("#171226"));
            purplePalette.setColor(QPalette::Text, QColor("#F5EEF8"));
            purplePalette.setColor(QPalette::Button, QColor("#261B40"));
            purplePalette.setColor(QPalette::ButtonText, QColor("#F5EEF8"));
            purplePalette.setColor(QPalette::BrightText, Qt::white);
            purplePalette.setColor(QPalette::Link, QColor("#9B59B6"));
            purplePalette.setColor(QPalette::Highlight, QColor("#8E44AD"));
            purplePalette.setColor(QPalette::HighlightedText, Qt::white);

            purplePalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#7D6B90"));
            purplePalette.setColor(QPalette::Disabled, QPalette::Text, QColor("#7D6B90"));
            purplePalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#5A4470"));

            qApp->setPalette(purplePalette);

        } else if (theme == Theme::Dark) {
            // Paleta Dark (Tons de azul escuro)
            QPalette darkPalette;
            darkPalette.setColor(QPalette::Window, QColor("#101924"));
            darkPalette.setColor(QPalette::WindowText, QColor("#ECF0F1"));
            darkPalette.setColor(QPalette::Base, QColor("#0B1118"));
            darkPalette.setColor(QPalette::AlternateBase, QColor("#162332"));
            darkPalette.setColor(QPalette::ToolTipBase, QColor("#ECF0F1"));
            darkPalette.setColor(QPalette::ToolTipText, QColor("#101924"));
            darkPalette.setColor(QPalette::Text, QColor("#ECF0F1"));
            darkPalette.setColor(QPalette::Button, QColor("#1A2B3D"));
            darkPalette.setColor(QPalette::ButtonText, QColor("#ECF0F1"));
            darkPalette.setColor(QPalette::BrightText, Qt::white);
            darkPalette.setColor(QPalette::Link, QColor("#38BDF8"));
            darkPalette.setColor(QPalette::Highlight, QColor("#0284C7"));
            darkPalette.setColor(QPalette::HighlightedText, Qt::white);

            darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#7F8C8D"));
            darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor("#7F8C8D"));
            darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#566573"));

            qApp->setPalette(darkPalette);

        } else if (theme == Theme::Light) {
            // Paleta Light (Tons de azul claro)
            QPalette lightPalette;
            lightPalette.setColor(QPalette::Window, QColor("#F0F7FF"));
            lightPalette.setColor(QPalette::WindowText, QColor("#1E293B"));
            lightPalette.setColor(QPalette::Base, QColor("#FFFFFF"));
            lightPalette.setColor(QPalette::AlternateBase, QColor("#E0F2FE"));
            lightPalette.setColor(QPalette::ToolTipBase, QColor("#1E293B"));
            lightPalette.setColor(QPalette::ToolTipText, QColor("#FFFFFF"));
            lightPalette.setColor(QPalette::Text, QColor("#1E293B"));
            lightPalette.setColor(QPalette::Button, QColor("#E2E8F0"));
            lightPalette.setColor(QPalette::ButtonText, QColor("#1E293B"));
            lightPalette.setColor(QPalette::BrightText, Qt::red);
            lightPalette.setColor(QPalette::Link, QColor("#0284C7"));
            lightPalette.setColor(QPalette::Highlight, QColor("#0284C7"));
            lightPalette.setColor(QPalette::HighlightedText, Qt::white);

            lightPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#94A3B8"));
            lightPalette.setColor(QPalette::Disabled, QPalette::Text, QColor("#94A3B8"));
            lightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#CBD5E1"));

            qApp->setPalette(lightPalette);

        } else if (theme == Theme::Red) {
            // Paleta Red (Tons de vermelho corporativo)
            QPalette redPalette;
            redPalette.setColor(QPalette::Window, QColor("#1C1315"));
            redPalette.setColor(QPalette::WindowText, QColor("#FDEDEC"));
            redPalette.setColor(QPalette::Base, QColor("#150E10"));
            redPalette.setColor(QPalette::AlternateBase, QColor("#281A1D"));
            redPalette.setColor(QPalette::ToolTipBase, QColor("#FDEDEC"));
            redPalette.setColor(QPalette::ToolTipText, QColor("#1C1315"));
            redPalette.setColor(QPalette::Text, QColor("#FDEDEC"));
            redPalette.setColor(QPalette::Button, QColor("#381A20"));
            redPalette.setColor(QPalette::ButtonText, QColor("#FDEDEC"));
            redPalette.setColor(QPalette::BrightText, Qt::white);
            redPalette.setColor(QPalette::Link, QColor("#E74C3C"));
            redPalette.setColor(QPalette::Highlight, QColor("#C0392B"));
            redPalette.setColor(QPalette::HighlightedText, Qt::white);

            redPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#8D6E72"));
            redPalette.setColor(QPalette::Disabled, QPalette::Text, QColor("#8D6E72"));
            redPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#5A3E42"));

            qApp->setPalette(redPalette);
        } else if (theme == Theme::Win2000) {
            QPalette win2kPalette;
            win2kPalette.setColor(QPalette::Window, QColor("#D4D0C8"));
            win2kPalette.setColor(QPalette::WindowText, Qt::black);
            win2kPalette.setColor(QPalette::Base, Qt::white);
            win2kPalette.setColor(QPalette::AlternateBase, QColor("#ECEAE4"));
            win2kPalette.setColor(QPalette::ToolTipBase, QColor("#FFFFE1"));
            win2kPalette.setColor(QPalette::ToolTipText, Qt::black);
            win2kPalette.setColor(QPalette::Text, Qt::black);
            win2kPalette.setColor(QPalette::Button, QColor("#D4D0C8"));
            win2kPalette.setColor(QPalette::ButtonText, Qt::black);
            win2kPalette.setColor(QPalette::Highlight, QColor("#0A246A"));
            win2kPalette.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(win2kPalette);
        } else if (theme == Theme::WinXP) {
            QPalette winXpPalette;
            winXpPalette.setColor(QPalette::Window, QColor("#ECE9D8"));
            winXpPalette.setColor(QPalette::WindowText, Qt::black);
            winXpPalette.setColor(QPalette::Base, Qt::white);
            winXpPalette.setColor(QPalette::AlternateBase, QColor("#F5F4EC"));
            winXpPalette.setColor(QPalette::ToolTipBase, QColor("#FFFFE1"));
            winXpPalette.setColor(QPalette::ToolTipText, Qt::black);
            winXpPalette.setColor(QPalette::Text, Qt::black);
            winXpPalette.setColor(QPalette::Button, QColor("#ECE9D8"));
            winXpPalette.setColor(QPalette::ButtonText, Qt::black);
            winXpPalette.setColor(QPalette::Highlight, QColor("#316AC5"));
            winXpPalette.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(winXpPalette);
        } else if (theme == Theme::Win7) {
            QPalette win7Palette;
            win7Palette.setColor(QPalette::Window, QColor("#EEF4FB"));
            win7Palette.setColor(QPalette::WindowText, QColor("#1E293B"));
            win7Palette.setColor(QPalette::Base, Qt::white);
            win7Palette.setColor(QPalette::AlternateBase, QColor("#E4ECF7"));
            win7Palette.setColor(QPalette::ToolTipBase, QColor("#F2F6FB"));
            win7Palette.setColor(QPalette::ToolTipText, QColor("#1E293B"));
            win7Palette.setColor(QPalette::Text, QColor("#1E293B"));
            win7Palette.setColor(QPalette::Button, QColor("#EAF1FA"));
            win7Palette.setColor(QPalette::ButtonText, QColor("#1E293B"));
            win7Palette.setColor(QPalette::Highlight, QColor("#3399FF"));
            win7Palette.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(win7Palette);
        } else if (theme == Theme::CinnamonLight) {
            QPalette cLightPalette;
            cLightPalette.setColor(QPalette::Window, QColor("#F5F5F5"));
            cLightPalette.setColor(QPalette::WindowText, QColor("#2E3436"));
            cLightPalette.setColor(QPalette::Base, Qt::white);
            cLightPalette.setColor(QPalette::AlternateBase, QColor("#EBEBEB"));
            cLightPalette.setColor(QPalette::ToolTipBase, Qt::white);
            cLightPalette.setColor(QPalette::ToolTipText, QColor("#2E3436"));
            cLightPalette.setColor(QPalette::Text, QColor("#2E3436"));
            cLightPalette.setColor(QPalette::Button, QColor("#ECECEC"));
            cLightPalette.setColor(QPalette::ButtonText, QColor("#2E3436"));
            cLightPalette.setColor(QPalette::Highlight, QColor("#5D9B47"));
            cLightPalette.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(cLightPalette);
        } else if (theme == Theme::CinnamonDark) {
            QPalette cDarkPalette;
            cDarkPalette.setColor(QPalette::Window, QColor("#2F2F2F"));
            cDarkPalette.setColor(QPalette::WindowText, QColor("#EAEAEA"));
            cDarkPalette.setColor(QPalette::Base, QColor("#212121"));
            cDarkPalette.setColor(QPalette::AlternateBase, QColor("#3A3A3A"));
            cDarkPalette.setColor(QPalette::ToolTipBase, QColor("#353535"));
            cDarkPalette.setColor(QPalette::ToolTipText, Qt::white);
            cDarkPalette.setColor(QPalette::Text, QColor("#EAEAEA"));
            cDarkPalette.setColor(QPalette::Button, QColor("#3A3A3A"));
            cDarkPalette.setColor(QPalette::ButtonText, QColor("#EAEAEA"));
            cDarkPalette.setColor(QPalette::Highlight, QColor("#78B159"));
            cDarkPalette.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(cDarkPalette);
        }

        if (!qss.isEmpty()) {
            qApp->setStyleSheet(qss);
        }
        emit themeChanged(m_currentTheme);
    }
}

ThemeManager::Theme ThemeManager::currentTheme() const {
    return m_currentTheme;
}

void ThemeManager::toggleTheme() {
    switch (m_currentTheme) {
    case Theme::Dark:          setTheme(Theme::Light); break;
    case Theme::Light:         setTheme(Theme::Purple); break;
    case Theme::Purple:        setTheme(Theme::Red); break;
    case Theme::Red:           setTheme(Theme::Win2000); break;
    case Theme::Win2000:       setTheme(Theme::WinXP); break;
    case Theme::WinXP:         setTheme(Theme::Win7); break;
    case Theme::Win7:          setTheme(Theme::CinnamonLight); break;
    case Theme::CinnamonLight: setTheme(Theme::CinnamonDark); break;
    case Theme::CinnamonDark:  setTheme(Theme::Dark); break;
    }
}

bool ThemeManager::isLightTheme(Theme theme) {
    return theme == Theme::Light ||
           theme == Theme::Win2000 ||
           theme == Theme::WinXP ||
           theme == Theme::Win7 ||
           theme == Theme::CinnamonLight;
}

QString ThemeManager::themeName(Theme theme) const {
    switch (theme) {
    case Theme::Dark:          return "Dark (Azul Escuro)";
    case Theme::Light:         return "Light (Azul Claro)";
    case Theme::Purple:        return "Purple (Roxo Corporativo)";
    case Theme::Red:           return "Red (Vermelho Corporativo)";
    case Theme::Win2000:       return "Windows 2000 (Clássico)";
    case Theme::WinXP:         return "Windows XP (Luna)";
    case Theme::Win7:          return "Windows 7 (Aero)";
    case Theme::CinnamonLight: return "Cinnamon Light (Mint-Y)";
    case Theme::CinnamonDark:  return "Cinnamon Dark (Mint-Y)";
    }
    return "Purple (Roxo Corporativo)";
}

QString ThemeManager::themeToString(Theme theme) {
    switch (theme) {
    case Theme::Dark:          return "Dark";
    case Theme::Light:         return "Light";
    case Theme::Purple:        return "Purple";
    case Theme::Red:           return "Red";
    case Theme::Win2000:       return "Win2000";
    case Theme::WinXP:         return "WinXP";
    case Theme::Win7:          return "Win7";
    case Theme::CinnamonLight: return "CinnamonLight";
    case Theme::CinnamonDark:  return "CinnamonDark";
    }
    return "Purple";
}

ThemeManager::Theme ThemeManager::themeFromString(const QString &themeStr) {
    QString t = themeStr.trimmed().toLower();
    if (t == "dark") return Theme::Dark;
    if (t == "light") return Theme::Light;
    if (t == "red") return Theme::Red;
    if (t == "win2000") return Theme::Win2000;
    if (t == "winxp") return Theme::WinXP;
    if (t == "win7") return Theme::Win7;
    if (t == "cinnamonlight" || t == "cinnamon_light" || t == "cinnamon light") return Theme::CinnamonLight;
    if (t == "cinnamondark" || t == "cinnamon_dark" || t == "cinnamon dark") return Theme::CinnamonDark;
    return Theme::Purple;
}

QString ThemeManager::loadStyleSheet(const QString &resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ThemeManager] Falha ao carregar stylesheet:" << resourcePath;
        return QString();
    }
    QTextStream in(&file);
    return in.readAll();
}
