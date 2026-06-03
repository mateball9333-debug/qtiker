#include "gacha_dialog.h"

#include "game_rules.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <iterator>

namespace {
const GachaCard DebugGachaCards[] = {
    {"Normal", "Common", QColor("#777777"), GachaEffect::Click, 3, 2, 50},
    {"Uncommon", "Uncommon", QColor("#2e7d32"), GachaEffect::Income, 3, 2, 25},
    {"Rare", "Rare", QColor("#1565c0"), GachaEffect::Click, 5, 2, 14},
    {"Epic", "Epic", QColor("#6a1b9a"), GachaEffect::Income, 3, 1, 7},
    {"Legendary", "Legendary", QColor("#f9a825"), GachaEffect::Click, 6, 1, 3},
    {"Mythic", "Mythic", QColor("#c62828"), GachaEffect::Income, 10, 1, 1},
};

QString multiplierText(const GachaCard &card, int ownedCount) {
    const int extraTenths = effectiveCardCopies(ownedCount) - 1;
    const double multiplier = static_cast<double>(card.multiplierNumerator)
        / static_cast<double>(card.multiplierDenominator)
        + static_cast<double>(extraTenths) / 10.0;

    if (qFuzzyCompare(multiplier, static_cast<double>(static_cast<int>(multiplier)))) {
        return QString("x%1").arg(static_cast<int>(multiplier));
    }

    return QString("x%1").arg(multiplier, 0, 'f', 1);
}

QString buttonStyle(const QColor &color, bool selected) {
    return QString(
        "QPushButton {"
        "  text-align: left;"
        "  padding: 6px 10px;"
        "  border: %1px solid %2;"
        "  border-radius: 5px;"
        "  background: qlineargradient("
        "    x1: 0, y1: 0, x2: 1, y2: 0,"
        "    stop: 0 palette(button),"
        "    stop: 0.74 palette(button),"
        "    stop: 0.75 %3,"
        "    stop: 1 %3"
        "  );"
        "}"
        "QPushButton:disabled {"
        "  color: palette(mid);"
        "}"
    ).arg(
        QString::number(selected ? 2 : 1),
        selected ? color.name() : QString("palette(mid)"),
        color.name()
    );
}
}

int debugGachaCardCount() {
    return static_cast<int>(std::size(DebugGachaCards));
}

GachaCard debugGachaCardAt(int index) {
    return DebugGachaCards[index];
}

int debugGachaTotalWeight() {
    int total = 0;
    for (const auto &card : DebugGachaCards) {
        total += card.dropWeight;
    }
    return total;
}

QString gachaEffectText(const GachaCard &card) {
    return gachaEffectText(card, 1);
}

QString gachaEffectText(const GachaCard &card, int ownedCount) {
    const auto target = card.effect == GachaEffect::Click ? "Click" : "Income";
    return QString("%1 %2").arg(target, multiplierText(card, ownedCount));
}

GachaDialog::GachaDialog(QWidget *parent) : QDialog(parent) {
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Gacha");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    resize(340, 450);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *topLayout = new QHBoxLayout();
    topLayout->setSpacing(8);

    auto *title = new QLabel("Gacha", this);
    auto titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title->setFont(titleFont);

    archLabel = new QLabel(this);
    archLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    topLayout->addWidget(title);
    topLayout->addStretch();
    topLayout->addWidget(archLabel);

    cardLabel = new QLabel("Last drop\nNone", this);
    cardLabel->setAlignment(Qt::AlignCenter);
    cardLabel->setMinimumHeight(54);

    messageLabel = new QLabel("Roll costs 1 Arch.", this);
    messageLabel->setWordWrap(true);

    for (int index = 0; index < debugGachaCardCount(); ++index) {
        cardButtons[index] = new QPushButton(this);
        cardButtons[index]->setMinimumHeight(34);
        connect(cardButtons[index], &QPushButton::clicked, this, [this, index]() {
            emit cardSelected(index);
        });
    }

    rollButton = new QPushButton("Roll - 1 Arch", this);
    connect(rollButton, &QPushButton::clicked, this, &GachaDialog::rollRequested);

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    layout->addLayout(topLayout);
    layout->addWidget(cardLabel);
    for (auto *button : cardButtons) {
        layout->addWidget(button);
    }
    layout->addWidget(messageLabel);
    layout->addWidget(rollButton);
    layout->addWidget(closeButton);

    setArchCount(0);
    setInventory({}, -1);
}

void GachaDialog::setArchCount(int arches) {
    archLabel->setText(QString("Arch's: %1").arg(arches));
    rollButton->setEnabled(arches >= 1);
}

void GachaDialog::setInventory(const std::array<int, GachaCardCount> &cardCounts, int selectedCard) {
    for (int index = 0; index < debugGachaCardCount(); ++index) {
        updateCardButton(index, cardCounts[index], selectedCard == index);
    }
}

void GachaDialog::showCard(const GachaCard &card, int ownedCount) {
    cardLabel->setText(QString("Last drop\n■ %1  %2\n%3   x%4")
                           .arg(card.name, card.suit, gachaEffectText(card, ownedCount))
                           .arg(ownedCount));
    auto palette = cardLabel->palette();
    palette.setColor(QPalette::WindowText, card.color);
    cardLabel->setPalette(palette);
    messageLabel->setText("Card rolled.");
}

void GachaDialog::showMessage(const QString &message) {
    messageLabel->setText(message);
}

void GachaDialog::updateCardButton(int index, int ownedCount, bool selected) {
    const auto card = debugGachaCardAt(index);
    const auto marker = selected ? "* " : "";
    cardButtons[index]->setText(QString("%1%2  %3  x%4")
                                    .arg(marker, card.name, gachaEffectText(card, ownedCount))
                                    .arg(ownedCount));
    cardButtons[index]->setStyleSheet(buttonStyle(card.color, selected));
    cardButtons[index]->setEnabled(ownedCount > 0);
}
