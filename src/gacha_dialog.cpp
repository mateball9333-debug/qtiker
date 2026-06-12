#include "gacha_dialog.h"

#include "clicker.h"
#include "game_rules.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <iterator>

namespace {
const GachaCard DebugGachaCards[] = {
    {"Normal", "Common", QColor("#777777"), GachaEffect::Click, 3, 2, 47},
    {"Uncommon", "Uncommon", QColor("#2e7d32"), GachaEffect::Income, 3, 2, 23},
    {"Rare", "Rare", QColor("#1565c0"), GachaEffect::Click, 5, 2, 13},
    {"Epic", "Epic", QColor("#6a1b9a"), GachaEffect::Income, 3, 1, 6},
    {"Legendary", "Legendary", QColor("#f9a825"), GachaEffect::Click, 6, 1, 2},
    {"Mythic", "Mythic", QColor("#c62828"), GachaEffect::Income, 10, 1, 1},
    {"CritEye", "Rare", QColor("#ff6f00"), GachaEffect::CritChance, 1, 1, 3, 5, 1},
    {"CritBlade", "Epic", QColor("#b71c1c"), GachaEffect::CritPower, 1, 1, 2, 2, 0},
    {"ArchMage", "Legendary", QColor("#4a148c"), GachaEffect::ArchHopper, 1, 1, 2, 10, 2},
    {"Hoarder", "Rare", QColor("#ffab00"), GachaEffect::Hoarder, 1, 1, 3, 10, 2},
    {"SpeedDemon", "Mythic", QColor("#00e5ff"), GachaEffect::Speedrun, 1, 1, 1, 2, 0},
};

int totalCardCount() {
    return static_cast<int>(std::size(DebugGachaCards));
}

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

QString specialCardEffectText(const GachaCard &card, int ownedCount) {
    const int copies = effectiveCardCopies(ownedCount);
    const int val = card.specialBase + (copies - 1) * card.specialPerCopy;
    switch (card.effect) {
    case GachaEffect::CritChance:
        return QString("+%1% crit chance").arg(val);
    case GachaEffect::CritPower:
        return QString("x%1 crit dmg").arg(val);
    case GachaEffect::ArchHopper:
        return QString("+%1%/arch").arg(val);
    case GachaEffect::Hoarder:
        return QString("+%1%/10K").arg(val);
    case GachaEffect::Speedrun:
        return QString("x%1 tick").arg(val);
    default:
        return QString();
    }
}

QString cardDescription(const GachaCard &card, int ownedCount) {
    const int copies = effectiveCardCopies(ownedCount);
    switch (card.effect) {
    case GachaEffect::Click:
    case GachaEffect::Income: {
        const auto target = card.effect == GachaEffect::Click ? "Click" : "Income";
        return QString("%1 %2").arg(target, multiplierText(card, ownedCount));
    }
    case GachaEffect::CritChance:
        return QString("+%1% crits").arg(card.specialBase + (copies - 1) * card.specialPerCopy);
    case GachaEffect::CritPower:
        return QString("x%1 crit dmg").arg(card.specialBase);
    case GachaEffect::ArchHopper:
        return QString("+%1%/arch click").arg(card.specialBase + (copies - 1) * card.specialPerCopy);
    case GachaEffect::Hoarder:
        return QString("+%1%/10K income").arg(card.specialBase + (copies - 1) * card.specialPerCopy);
    case GachaEffect::Speedrun:
        return QString("x%1 income tick").arg(card.specialBase);
    }
    return QString();
}

QString dropRateText(const GachaCard &card) {
    const int total = debugGachaTotalWeight();
    const double pct = 100.0 * card.dropWeight / total;
    if (pct < 1.0)
        return QString("%1%").arg(pct, 0, 'f', 1);
    return QString("%1%").arg(static_cast<int>(pct + 0.5));
}

QString cardButtonStyle(const QColor &color, bool selected) {
    return QString(
        "QPushButton {"
        "  text-align: left;"
        "  padding: 6px 8px 6px 10px;"
        "  border: %1px solid %2;"
        "  border-radius: 4px;"
        "  background: qlineargradient("
        "    x1: 0, y1: 0, x2: 1, y2: 0,"
        "    stop: 0 %3,"
        "    stop: 0.06 %3,"
        "    stop: 0.061 palette(button),"
        "    stop: 1 palette(button)"
        "  );"
        "}"
        "QPushButton:disabled {"
        "  color: palette(mid);"
        "  background: qlineargradient("
        "    x1: 0, y1: 0, x2: 1, y2: 0,"
        "    stop: 0 palette(mid),"
        "    stop: 0.06 palette(mid),"
        "    stop: 0.061 palette(button),"
        "    stop: 1 palette(button)"
        "  );"
        "}"
    ).arg(
        QString::number(selected ? 2 : 1),
        selected ? color.name() : QString("palette(mid)"),
        color.name()
    );
}
}

bool isNormalEffect(GachaEffect effect) {
    return effect == GachaEffect::Click || effect == GachaEffect::Income;
}

QString specialEffectName(GachaEffect effect) {
    switch (effect) {
    case GachaEffect::CritChance: return "Crit chance";
    case GachaEffect::CritPower:  return "Crit power";
    case GachaEffect::ArchHopper: return "Click/arch";
    case GachaEffect::Hoarder:    return "Income/10K";
    case GachaEffect::Speedrun:   return "Income tick";
    default: return "";
    }
}

int debugGachaCardCount() {
    return totalCardCount();
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
    if (isNormalEffect(card.effect)) {
        const auto target = card.effect == GachaEffect::Click ? "Click" : "Income";
        return QString("%1 %2").arg(target, multiplierText(card, ownedCount));
    }
    return specialCardEffectText(card, ownedCount);
}

GachaDialog::GachaDialog(Clicker *parentClicker, QWidget *parent) : QDialog(parent), clicker(parentClicker) {
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Gacha");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    setFixedSize(360, 530);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

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
    cardLabel->setMinimumHeight(50);

    messageLabel = new QLabel("Roll costs 1 Arch.", this);
    messageLabel->setWordWrap(true);

    cardTabs = new QTabWidget(this);

    auto mkGrid = [&](int start, int count) -> QWidget * {
        auto *w = new QWidget(cardTabs);
        auto *g = new QGridLayout(w);
        g->setContentsMargins(4, 4, 4, 4);
        g->setSpacing(4);
        for (int i = 0; i < count; ++i) {
            int idx = start + i;
            auto *btn = new QPushButton(cardTabs);
            btn->setMinimumHeight(46);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            btn->setToolTip(QString("%1\n%2\nDrop: %3")
                                .arg(DebugGachaCards[idx].name,
                                     cardDescription(DebugGachaCards[idx], 0),
                                     dropRateText(DebugGachaCards[idx])));
            connect(btn, &QPushButton::clicked, this, [this, idx]() {
                if (activeSlot == 1)
                    emit cardSelected(idx);
                else
                    emit cardSelectedForSlot2(idx);
            });
            cardButtons[idx] = btn;
            g->addWidget(btn, i / 2, i % 2);
        }
        g->setRowStretch((count + 1) / 2, 1);
        return w;
    };

    cardTabs->addTab(mkGrid(0, 6), "Normal");
    cardTabs->addTab(mkGrid(6, 5), "Special");

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
    layout->addWidget(cardTabs, 1);
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
    for (int index = 0; index < totalCardCount(); ++index) {
        updateCardButton(index, cardCounts[index], selectedCard == index, selectedCard2 == index);
    }

    const auto fmtSlot = [&](int slotCard) -> QString {
        if (slotCard < 0 || slotCard >= totalCardCount()) return "---";
        const auto c = debugGachaCardAt(slotCard);
        return QString("%1 x%2").arg(c.name).arg(cardCounts[slotCard]);
    };

    const auto effMultText = [&](int slotCard) -> QString {
        if (slotCard < 0 || slotCard >= totalCardCount()) return QString();
        const auto card = debugGachaCardAt(slotCard);
        if (isNormalEffect(card.effect)) {
            const int extraTenths = effectiveCardCopies(cardCounts[slotCard]) - 1;
            const int stackedNum = card.multiplierNumerator * 10 + extraTenths * card.multiplierDenominator;
            const int stackedDen = card.multiplierDenominator * 10;
            const int penalty = penaltyUpgraded ? 33 : 55;
            const int penNum = stackedDen * penalty + stackedNum * (100 - penalty);
            const int penDen = stackedDen * 100;
            const double effective = static_cast<double>(penNum) / static_cast<double>(penDen);
            if (qFuzzyCompare(effective, static_cast<double>(static_cast<int>(effective))))
                return QString("x%1").arg(static_cast<int>(effective));
            return QString("x%1").arg(effective, 0, 'f', 2);
        }
        return cardCounts[slotCard] > 0 ? specialCardEffectText(card, cardCounts[slotCard]) : QString();
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
    if (!unlocked) activeSlot = 1;
}

void GachaDialog::setSecondCardPenaltyUpgraded(bool upgraded) {
    penaltyUpgraded = upgraded;
}

void GachaDialog::setActiveSlot(int slot) {
    activeSlot = slot;
}

void GachaDialog::showCard(const GachaCard &card, int ownedCount) {
    const auto eff = isNormalEffect(card.effect)
        ? gachaEffectText(card, ownedCount)
        : specialCardEffectText(card, ownedCount);
    cardLabel->setText(clicker->compat(QString("Last drop\n\u25A0 %1  %2\n%3  x%4"))
                           .arg(card.name, card.suit, eff)
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
    const auto marker = selected ? (selected2 ? clicker->compat("*\u2051") : clicker->compat("* ")) : (selected2 ? clicker->compat("\u2051 ") : "");
    const auto desc = cardDescription(card, ownedCount);
    const auto eff = ownedCount > 0
        ? (isNormalEffect(card.effect) ? gachaEffectText(card, ownedCount) : specialCardEffectText(card, ownedCount))
        : desc;
    cardButtons[index]->setText(QString("%1%2\n%3   %4  x%5")
                                    .arg(marker, card.name, eff, dropRateText(card))
                                    .arg(ownedCount));
    cardButtons[index]->setStyleSheet(cardButtonStyle(card.color, selected || selected2));
    cardButtons[index]->setEnabled(ownedCount > 0);
    cardButtons[index]->setToolTip(QString("%1\n%2\nDrop: %3")
                                       .arg(card.name, cardDescription(card, ownedCount), dropRateText(card)));
}
