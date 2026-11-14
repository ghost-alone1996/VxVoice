#include "SelectContactNumberDialog.h"
#include "IconHelper.h"
#include "ThemeManager.h"
#include <QScrollArea>

SelectContactNumberDialog::SelectContactNumberDialog(const ContactEntry &contact, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QString("Ligar para %1").arg(contact.name));
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setModal(true);
    setMinimumWidth(460);
    setupUi(contact);
    adjustSize();
}

void SelectContactNumberDialog::setupUi(const ContactEntry &contact) {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(12);
    mainLayout->setSizeConstraint(QLayout::SetMinimumSize);

    // Card com dados do contato
    auto card = new QFrame(this);
    card->setObjectName("callInfoFrame");
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(6);

    const ThemeManager::Theme currentTheme = ThemeManager::instance().currentTheme();
    const bool isLight = ThemeManager::isLightTheme(currentTheme);
    const bool isClassicWinTheme = (currentTheme == ThemeManager::Theme::Win2000 ||
                                    currentTheme == ThemeManager::Theme::WinXP ||
                                    currentTheme == ThemeManager::Theme::Win7);

    auto lblName = new QLabel(QString("<b>%1</b>").arg(contact.name.toHtmlEscaped()), card);
    lblName->setStyleSheet("font-size: 17px; font-weight: bold; background: transparent;");
    lblName->setAlignment(Qt::AlignCenter);
    lblName->setWordWrap(true);

    QString subText = contact.company.isEmpty() ? "Contato Corporativo" : contact.company;
    auto lblSub = new QLabel(subText, card);
    lblSub->setStyleSheet(
        QString("font-size: 13px; color: %1; background: transparent;")
            .arg(isLight ? "#475569" : "#9CA3AF")
    );
    lblSub->setAlignment(Qt::AlignCenter);
    lblSub->setWordWrap(true);

    auto lblPrompt = new QLabel("Este contato possui múltiplos números cadastrados.\nSelecione para qual destino deseja ligar:", card);
    lblPrompt->setWordWrap(true);
    lblPrompt->setStyleSheet("font-size: 13px; margin-top: 4px; background: transparent;");
    lblPrompt->setAlignment(Qt::AlignCenter);

    cardLayout->addWidget(lblName);
    cardLayout->addWidget(lblSub);
    cardLayout->addWidget(lblPrompt);
    mainLayout->addWidget(card);

    // Lista dinâmica de botões para cada número salvo
    auto buttonsWidget = new QWidget(this);
    auto buttonsLayout = new QVBoxLayout(buttonsWidget);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(8);

    const QStringList numbers = contact.allNumbers();
    const QString extTrimmed = contact.extension.trimmed();
    int phoneIndex = 1;

    for (const QString &num : numbers) {
        const bool isExt = (!extTrimmed.isEmpty() && num == extTrimmed);
        QString btnLabel;
        QString bgNormal, bgHover, bgPressed, borderColor;

        if (isExt) {
            btnLabel = QString("📞 Ramal SIP:  %1").arg(num);
            bgNormal = "#27AE60"; bgHover = "#2ECC71"; bgPressed = "#1E8449"; borderColor = "#2ECC71";
        } else {
            btnLabel = (phoneIndex == 1)
                ? QString("📱 Telefone:  %1").arg(num)
                : QString("📱 Telefone %1:  %2").arg(phoneIndex).arg(num);
            ++phoneIndex;
            bgNormal = "#2980B9"; bgHover = "#3498DB"; bgPressed = "#1F618D"; borderColor = "#3498DB";
        }

        auto btn = new QPushButton(btnLabel, buttonsWidget);
        btn->setFixedHeight(42);
        btn->setCursor(Qt::PointingHandCursor);
        if (isClassicWinTheme) {
            // Mantém o estilo nativo do tema Windows ativo (Win2000, WinXP, Win7)
            btn->setStyleSheet("QPushButton { font-weight: bold; font-size: 13px; padding: 6px 14px; }");
        } else {
            btn->setStyleSheet(
                QString("QPushButton {"
                        "  background-color: %1;"
                        "  color: #FFFFFF;"
                        "  font-weight: bold;"
                        "  font-size: 14px;"
                        "  border-radius: 6px;"
                        "  border: 1px solid %4;"
                        "  padding: 6px 14px;"
                        "}"
                        "QPushButton:hover { background-color: %2; }"
                        "QPushButton:pressed { background-color: %3; }")
                    .arg(bgNormal, bgHover, bgPressed, borderColor)
            );
        }
        connect(btn, &QPushButton::clicked, this, [this, num]() {
            m_selectedNumber = num;
            accept();
        });
        buttonsLayout->addWidget(btn);
    }

    if (numbers.size() > 5) {
        auto scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidget(buttonsWidget);
        scrollArea->setMinimumHeight(240);
        scrollArea->setMaximumHeight(320);
        mainLayout->addWidget(scrollArea);
    } else {
        mainLayout->addWidget(buttonsWidget);
    }

    auto btnCancel = new QPushButton("Cancelar", this);
    btnCancel->setFixedHeight(36);
    btnCancel->setCursor(Qt::PointingHandCursor);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    mainLayout->addWidget(btnCancel);
}
