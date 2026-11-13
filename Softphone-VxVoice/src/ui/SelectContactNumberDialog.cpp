#include "SelectContactNumberDialog.h"
#include "IconHelper.h"
#include "ThemeManager.h"

SelectContactNumberDialog::SelectContactNumberDialog(const ContactEntry &contact, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QString("Ligar para %1").arg(contact.name));
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    setModal(true);
    setFixedSize(380, 260);
    setupUi(contact);
}

void SelectContactNumberDialog::setupUi(const ContactEntry &contact) {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    // Card com dados do contato
    auto card = new QFrame(this);
    card->setObjectName("callInfoFrame");
    auto cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(4);

    auto lblName = new QLabel(QString("<b>%1</b>").arg(contact.name), card);
    lblName->setStyleSheet("font-size: 16px; font-weight: bold; background: transparent;");
    lblName->setAlignment(Qt::AlignCenter);

    QString subText = contact.company.isEmpty() ? "Contato Cadastrado" : contact.company;
    auto lblSub = new QLabel(subText, card);
    lblSub->setStyleSheet("font-size: 12px; color: #7F8C8D; background: transparent;");
    lblSub->setAlignment(Qt::AlignCenter);

    auto lblPrompt = new QLabel("Este contato possui ramal e telefone cadastrados.<br><b>Para qual deseja ligar?</b>", card);
    lblPrompt->setStyleSheet("font-size: 13px; margin-top: 4px; background: transparent;");
    lblPrompt->setAlignment(Qt::AlignCenter);

    cardLayout->addWidget(lblName);
    cardLayout->addWidget(lblSub);
    cardLayout->addWidget(lblPrompt);
    mainLayout->addWidget(card);

    // Botões de escolha com estilo proeminente
    auto btnRamal = new QPushButton(QString("📞 Ramal SIP:  %1").arg(contact.extension.trimmed()), this);
    btnRamal->setObjectName("btnSelectRamal");
    btnRamal->setFixedHeight(42);
    btnRamal->setCursor(Qt::PointingHandCursor);
    btnRamal->setStyleSheet(
        "QPushButton#btnSelectRamal {"
        "  background-color: #27AE60;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  border-radius: 6px;"
        "  border: 1px solid #2ECC71;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton#btnSelectRamal:hover { background-color: #2ECC71; }"
        "QPushButton#btnSelectRamal:pressed { background-color: #1E8449; }"
    );
    connect(btnRamal, &QPushButton::clicked, this, [this, contact]() {
        m_selectedNumber = contact.extension.trimmed();
        accept();
    });

    auto btnNumero = new QPushButton(QString("📱 Telefone:  %1").arg(contact.number.trimmed()), this);
    btnNumero->setObjectName("btnSelectNumero");
    btnNumero->setFixedHeight(42);
    btnNumero->setCursor(Qt::PointingHandCursor);
    btnNumero->setStyleSheet(
        "QPushButton#btnSelectNumero {"
        "  background-color: #2980B9;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  border-radius: 6px;"
        "  border: 1px solid #3498DB;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton#btnSelectNumero:hover { background-color: #3498DB; }"
        "QPushButton#btnSelectNumero:pressed { background-color: #1F618D; }"
    );
    connect(btnNumero, &QPushButton::clicked, this, [this, contact]() {
        m_selectedNumber = contact.number.trimmed();
        accept();
    });

    auto btnCancel = new QPushButton("Cancelar", this);
    btnCancel->setFixedHeight(34);
    btnCancel->setCursor(Qt::PointingHandCursor);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    mainLayout->addWidget(btnRamal);
    mainLayout->addWidget(btnNumero);
    mainLayout->addWidget(btnCancel);
}
