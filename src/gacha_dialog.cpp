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
            if (activeSlot == 1) {
                emit cardSelected(index);
            } else {
                emit cardSelectedForSlot2(index);
            }
        });
    }

    rollButton = new QPushButton("Roll - 1 Arch", this);
    connect(rollButton, &QPushButton::clicked, this, &GachaDialog::rollRequested);

    slot1Button = new QPushButton("Slot 1: —", this);
    slot1Button->setMinimumHeight(28);
    connect(slot1Button, &QPushButton::clicked, this, [this]() {
        activeSlot = 1;
        emit slotChanged(1);
    });

    slot2Button = new QPushButton("Slot 2: —", this);
    slot2Button->setMinimumHeight(28);
    slot2Button->setIcon(QIcon(":/assets/ui/stat-down.svg"));
    slot2Button->setVisible(false);
    connect(slot2Button, &QPushButton::clicked, this, [this]() {
        activeSlot = 2;
        emit slotChanged(2);
    });

    auto *slotsLayout = new QHBoxLayout();
    slotsLayout->setSpacing(8);
    slotsLayout->addWidget(slot1Button, 1);
    slotsLayout->addWidget(slot2Button, 1);

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    layout->addLayout(topLayout);
    layout->addLayout(slotsLayout);
    layout->addWidget(cardLabel);
    for (auto *button : cardButtons) {
        layout->addWidget(button);
    }
    layout->addWidget(messageLabel);
    layout->addWidget(rollButton);
    layout->addWidget(closeButton);

    setArchCount(0);
    setInventory({}, -1, -1);
}

void GachaDialog::setArchCount(int arches) {
    archLabel->setText(QString("Arch's: %1").arg(arches));
    rollButton->setEnabled(arches >= 1);
}

void GachaDialog::setInventory(const std::array<int, GachaCardCount> &cardCounts, int selectedCard, int selectedCard2) {
    for (int index = 0; index < debugGachaCardCount(); ++index) {
        updateCardButton(index, cardCounts[index], selectedCard == index, selectedCard2 == index);
    }

    const auto fmtSlot = [&](int slotCard) -> QString {
        if (slotCard < 0 || slotCard >= debugGachaCardCount()) {
            return "---";
        }
        const auto c = debugGachaCardAt(slotCard);
        return QString("%1 x%2").arg(c.name).arg(cardCounts[slotCard]);
    };

    const auto effMultText = [&](int slotCard) -> QString {
        if (slotCard < 0 || slotCard >= debugGachaCardCount()) {
            return QString();
        }
        const auto card = debugGachaCardAt(slotCard);
        const int extraTenths = effectiveCardCopies(cardCounts[slotCard]) - 1;
        const int stackedNum = card.multiplierNumerator * 10 + extraTenths * card.multiplierDenominator;
        const int stackedDen = card.multiplierDenominator * 10;
        const int penalty = penaltyUpgraded ? 33 : 55;
        const int penNum = stackedDen * penalty + stackedNum * (100 - penalty);
        const int penDen = stackedDen * 100;
        const double effective = static_cast<double>(penNum) / static_cast<double>(penDen);
        if (qFuzzyCompare(effective, static_cast<double>(static_cast<int>(effective)))) {
            return QString("x%1").arg(static_cast<int>(effective));
        }
        return QString("x%1").arg(effective, 0, 'f', 2);
    };

    slot1Button->setText(QString("Slot 1: %1").arg(fmtSlot(selectedCard)));
    if (selectedCard2 >= 0) {
        const auto eff = effMultText(selectedCard2);
        slot2Button->setText(QString("Slot 2: %1 > %2").arg(fmtSlot(selectedCard2), eff));
        slot2Button->setIcon(QIcon(penaltyUpgraded
            ? ":/assets/ui/stat-down.svg"
            : ":/assets/ui/stat-down-double.svg"));
    } else {
        slot2Button->setText("Slot 2: ---");
        slot2Button->setIcon(QIcon());
    }

    const auto highlight = [&](QPushButton *btn, bool active) {
        btn->setStyleSheet(active
            ? "QPushButton { border: 2px solid palette(highlight); padding: 4px 8px; }"
            : "QPushButton { border: 1px solid palette(mid); padding: 4px 8px; }");
    };
    highlight(slot1Button, activeSlot == 1);
    highlight(slot2Button, activeSlot == 2);
}

void GachaDialog::setSecondCardSlotEnabled(bool unlocked) {
    slot2Button->setVisible(unlocked);
    if (!unlocked) {
        activeSlot = 1;
    }
}

void GachaDialog::setSecondCardPenaltyUpgraded(bool upgraded) {
    penaltyUpgraded = upgraded;
}

void GachaDialog::setActiveSlot(int slot) {
    activeSlot = slot;
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

void GachaDialog::updateCardButton(int index, int ownedCount, bool selected, bool selected2) {
    const auto card = debugGachaCardAt(index);
    const auto marker = selected ? (selected2 ? "*⁑" : "* ") : (selected2 ? "⁑ " : "  ");
    cardButtons[index]->setText(QString("%1%2  %3  x%4")
                                    .arg(marker, card.name, gachaEffectText(card, ownedCount))
                                    .arg(ownedCount));
    cardButtons[index]->setStyleSheet(buttonStyle(card.color, selected || selected2));
    cardButtons[index]->setEnabled(ownedCount > 0);
}
