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
    } else if (theme == Theme::BlueBaby) {
        path = ":/styles/blue_baby.qss";
    } else if (theme == Theme::BlueCapri) {
        path = ":/styles/blue_capri.qss";
    } else if (theme == Theme::RosaAmaranth) {
        path = ":/styles/rosa_amaranth.qss";
    } else if (theme == Theme::RosaHotMagenta) {
        path = ":/styles/rosa_hot_magenta.qss";
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
        } else if (theme == Theme::BlueBaby) {
            QPalette p;
            p.setColor(QPalette::Window, QColor("#EBF6FC"));
            p.setColor(QPalette::WindowText, QColor("#0F2942"));
            p.setColor(QPalette::Base, QColor("#F5FBFF"));
            p.setColor(QPalette::AlternateBase, QColor("#D9EFFB"));
            p.setColor(QPalette::Text, QColor("#0F2942"));
            p.setColor(QPalette::Button, QColor("#C5E4F6"));
            p.setColor(QPalette::ButtonText, QColor("#0F2942"));
            p.setColor(QPalette::Link, QColor("#0284C7"));
            p.setColor(QPalette::Highlight, QColor("#0284C7"));
            p.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(p);
        } else if (theme == Theme::BlueCapri) {
            QPalette p;
            p.setColor(QPalette::Window, QColor("#082032"));
            p.setColor(QPalette::WindowText, QColor("#E8F8FF"));
            p.setColor(QPalette::Base, QColor("#04121C"));
            p.setColor(QPalette::AlternateBase, QColor("#0C2D48"));
            p.setColor(QPalette::Text, QColor("#E8F8FF"));
            p.setColor(QPalette::Button, QColor("#0D2D47"));
            p.setColor(QPalette::ButtonText, QColor("#E8F8FF"));
            p.setColor(QPalette::Link, QColor("#00BFFF"));
            p.setColor(QPalette::Highlight, QColor("#0099CC"));
            p.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(p);
        } else if (theme == Theme::RosaAmaranth) {
            QPalette p;
            p.setColor(QPalette::Window, QColor("#260E18"));
            p.setColor(QPalette::WindowText, QColor("#FDF0F4"));
            p.setColor(QPalette::Base, QColor("#14060C"));
            p.setColor(QPalette::AlternateBase, QColor("#3A1525"));
            p.setColor(QPalette::Text, QColor("#FDF0F4"));
            p.setColor(QPalette::Button, QColor("#381424"));
            p.setColor(QPalette::ButtonText, QColor("#FDF0F4"));
            p.setColor(QPalette::Link, QColor("#E52B50"));
            p.setColor(QPalette::Highlight, QColor("#C21E56"));
            p.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(p);
        } else if (theme == Theme::RosaHotMagenta) {
            QPalette p;
            p.setColor(QPalette::Window, QColor("#21091B"));
            p.setColor(QPalette::WindowText, QColor("#FFF0FB"));
            p.setColor(QPalette::Base, QColor("#12040F"));
            p.setColor(QPalette::AlternateBase, QColor("#340E2B"));
            p.setColor(QPalette::Text, QColor("#FFF0FB"));
            p.setColor(QPalette::Button, QColor("#36102D"));
            p.setColor(QPalette::ButtonText, QColor("#FFF0FB"));
            p.setColor(QPalette::Link, QColor("#FF1DCE"));
            p.setColor(QPalette::Highlight, QColor("#D900A7"));
            p.setColor(QPalette::HighlightedText, Qt::white);
            qApp->setPalette(p);
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
    case Theme::Dark:           setTheme(Theme::Light); break;
    case Theme::Light:          setTheme(Theme::Purple); break;
    case Theme::Purple:         setTheme(Theme::Red); break;
    case Theme::Red:            setTheme(Theme::BlueBaby); break;
    case Theme::BlueBaby:       setTheme(Theme::BlueCapri); break;
    case Theme::BlueCapri:      setTheme(Theme::RosaAmaranth); break;
    case Theme::RosaAmaranth:   setTheme(Theme::RosaHotMagenta); break;
    case Theme::RosaHotMagenta: setTheme(Theme::Dark); break;
    }
}

QString ThemeManager::themeName(Theme theme) const {
    switch (theme) {
    case Theme::Dark:           return "Dark (Azul Escuro)";
    case Theme::Light:          return "Light (Azul Claro)";
    case Theme::Purple:         return "Purple (Roxo Corporativo)";
    case Theme::Red:            return "Red (Vermelho Corporativo)";
    case Theme::BlueBaby:       return "Blue Baby";
    case Theme::BlueCapri:      return "Blue Capri";
    case Theme::RosaAmaranth:   return "Rosa Amaranth";
    case Theme::RosaHotMagenta: return "Rosa Hot Magenta";
    }
    return "Purple (Roxo Corporativo)";
}

QString ThemeManager::themeToString(Theme theme) {
    switch (theme) {
    case Theme::Dark:           return "Dark";
    case Theme::Light:          return "Light";
    case Theme::Purple:         return "Purple";
    case Theme::Red:            return "Red";
    case Theme::BlueBaby:       return "BlueBaby";
    case Theme::BlueCapri:      return "BlueCapri";
    case Theme::RosaAmaranth:   return "RosaAmaranth";
    case Theme::RosaHotMagenta: return "RosaHotMagenta";
    }
    return "Purple";
}

ThemeManager::Theme ThemeManager::themeFromString(const QString &themeStr) {
    QString t = themeStr.trimmed().toLower();
    if (t == "dark") return Theme::Dark;
    if (t == "light") return Theme::Light;
    if (t == "red") return Theme::Red;
    if (t == "bluebaby" || t == "blue baby") return Theme::BlueBaby;
    if (t == "bluecapri" || t == "blue capri") return Theme::BlueCapri;
    if (t == "rosaamaranth" || t == "rosa amaranth") return Theme::RosaAmaranth;
    if (t == "rosahotmagenta" || t == "rosa hot magenta") return Theme::RosaHotMagenta;
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
