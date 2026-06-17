// SPDX-License-Identifier: GPL-2.0-or-later
#include "gacha_dialog.h"

#include "clicker.h"
#include "core/game_rules.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

QString dropRateText(const GachaCard &card) {
    const int total = debugGachaTotalWeight();
    const double pct = 100.0 * card.dropWeight / total;
    if (pct < 1.0) return QString("%1%").arg(pct, 0, 'f', 1);
    return QString("%1%").arg(static_cast<int>(pct + 0.5));
}

QString cardButtonStyle(const QColor &color, bool selected) {
    return QString(
        "QPushButton { text-align: left; padding: 6px 8px 6px 10px;"
        "  border: %1px solid %2; border-radius: 4px;"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 %3, stop:0.06 %3, stop:0.061 palette(button), stop:1 palette(button)); }"
        "QPushButton:disabled { color: palette(mid);"
        "  border: 1px solid palette(mid);"
        "  background: qlineargradient(stop:0 %3, stop:0.06 %3,"
        "    stop:0.061 palette(button), stop:1 palette(button)); }"
    ).arg(QString::number(selected ? 2 : 1),
          selected ? color.name() : QString("palette(mid)"),
          color.name());
}

QString starsForLevel(int lvl, int path) {
    if (lvl <= 0) return QString();
    QString s;
    for (int i = 0; i < qMin(lvl, 3); ++i) s += "\u2605";
    if (lvl >= 3 && path > 0) s += path == 1 ? "A" : "B";
    return s;
}

QString normalEffectText(const GachaCard &card, int upgradeLevel) {
    const double up = static_cast<double>(card.multiplierNumerator) / card.multiplierDenominator * normalCardMult(upgradeLevel);
    if (qFuzzyCompare(up, static_cast<double>(static_cast<int>(up))))
        return QString("x%1").arg(static_cast<int>(up));
    return QString("x%1").arg(up, 0, 'f', 1);
}

QString specialEffectTextUpgraded(const GachaCard &card, int upgradeLevel) {
    const int val = static_cast<int>(card.specialBase * specialCardMult(upgradeLevel));
    switch (card.effect) {
    case GachaEffect::CritChance: return QString("+%1% crit").arg(val);
    case GachaEffect::CritPower:  return QString("x%1.%2 crit").arg(val / 10).arg(val % 10);
    case GachaEffect::ArchHopper: return QString("+%1%/arch").arg(val);
    case GachaEffect::Hoarder:    return QString("+%1%/10K").arg(val);
    case GachaEffect::Speedrun:   return QString("x%1 tick").arg(val);
    default: return "";
    }
}

} // namespace

// leftover stubs for old API compatibility (used by clicker.cpp showCard)
QString multiplierText(const GachaCard &card, int) {
    const double m = static_cast<double>(card.multiplierNumerator) / card.multiplierDenominator;
    return qFuzzyCompare(m, static_cast<double>(static_cast<int>(m)))
        ? QString("x%1").arg(static_cast<int>(m)) : QString("x%1").arg(m, 0, 'f', 2);
}
QString specialCardEffectText(const GachaCard &card, int) { return specialEffectTextUpgraded(card, 0); }
QString cardDescription(const GachaCard &card, int) {
    if (isNormalEffect(card.effect)) {
        const auto t = card.effect == GachaEffect::Click ? "Click" : "Income";
        return QString("%1 %2").arg(t, multiplierText(card, 0));
    }
    return specialCardEffectText(card, 0);
}
QString gachaEffectText(const GachaCard &card) { return gachaEffectText(card, 1); }
QString gachaEffectText(const GachaCard &card, int) {
    if (isNormalEffect(card.effect)) {
        const auto t = card.effect == GachaEffect::Click ? "Click" : "Income";
        return QString("%1 %2").arg(t, multiplierText(card, 0));
    }
    return specialCardEffectText(card, 0);
}

GachaDialog::GachaDialog(Clicker *parentClicker, QWidget *parent) : QDialog(parent), clicker(parentClicker) {
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Gacha");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    setFixedSize(380, 520);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    auto *topLayout = new QHBoxLayout();
    auto *title = new QLabel("Gacha", this);
    auto tf = title->font(); tf.setPointSize(14); tf.setBold(true); title->setFont(tf);
    archLabel = new QLabel(this);
    archLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    collectedLabel = new QLabel(this);
    collectedLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto clf = collectedLabel->font(); clf.setPointSize(clf.pointSize() - 1);
    collectedLabel->setFont(clf);
    topLayout->addWidget(title);
    topLayout->addStretch();
    topLayout->addWidget(archLabel);
    topLayout->addWidget(collectedLabel);

    cardLabel = new QLabel("Last: ---", this);
    cardLabel->setAlignment(Qt::AlignCenter);
    auto cardf = cardLabel->font(); cardf.setPointSize(cardf.pointSize() - 1);
    cardLabel->setFont(cardf);

    messageLabel = new QLabel("Roll costs 1 Arch.", this);
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);
    auto msgf = messageLabel->font(); msgf.setPointSize(msgf.pointSize() - 1);
    msgf.setItalic(true);
    messageLabel->setFont(msgf);

    cardTabs = new QTabWidget(this);

    auto mkGrid = [&](int start, int count) -> QWidget * {
        auto *w = new QWidget(cardTabs);
        auto *g = new QGridLayout(w);
        g->setContentsMargins(2, 2, 2, 2);
        g->setSpacing(3);
        for (int i = 0; i < count; ++i) {
            int idx = start + i;

            auto *row = new QHBoxLayout();
            row->setContentsMargins(0, 0, 0, 0);
            row->setSpacing(0);

            auto *btn = new QPushButton(cardTabs);
            btn->setMinimumHeight(42);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            connect(btn, &QPushButton::clicked, this, [this, idx]() {
                if (activeSlot == 1) emit cardSelected(idx);
                else emit cardSelectedForSlot2(idx);
            });
            cardButtons[idx] = btn;

            auto *upBtn = new QPushButton(cardTabs);
            upBtn->setFixedSize(24, 42);
            upBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            upBtn->setToolTip("Upgrade / pick path");
            connect(upBtn, &QPushButton::clicked, this, [this, idx, upBtn]() {
                if (cardOwned_[idx] <= 0) return;
                const auto card = debugGachaCardAt(idx);
                const bool isSpecial = !isNormalEffect(card.effect);
                const int lvl = cardUpgradeLevels_[idx];
                const int path = cardUpgradePaths_[idx];

                if (isSpecial && lvl >= 3 && path > 0) {
                    const int other = path == 1 ? 2 : 1;
                    emit upgradePathRequested(idx, other);
                } else if (isSpecial && lvl >= 2 && path <= 0) {
                    QMenu menu;
                    QAction *a = nullptr, *b = nullptr;
                    const int cost = specialCardUpgradeCost(3);
                    switch (card.effect) {
                    case GachaEffect::Hoarder:
                        a = menu.addAction(QString("A: +50%  (%1C)").arg(cost));
                        b = menu.addAction(QString("B: +200%/arch  (%1C)").arg(cost)); break;
                    case GachaEffect::ArchHopper:
                        a = menu.addAction(QString("A: +50%  (%1C)").arg(cost));
                        b = menu.addAction(QString("B: On income  (%1C)").arg(cost)); break;
                    case GachaEffect::CritChance:
                        a = menu.addAction(QString("A: +3% crit  (%1C)").arg(cost));
                        b = menu.addAction(QString("B: Arch on crit  (%1C)").arg(cost)); break;
                    case GachaEffect::CritPower:
                        a = menu.addAction(QString("A: Bigger crit  (%1C)").arg(cost));
                        b = menu.addAction(QString("B: +5% chance  (%1C)").arg(cost)); break;
                    case GachaEffect::Speedrun:
                        a = menu.addAction(QString("A: x3 income  (%1C)").arg(cost));
                        b = menu.addAction(QString("B: Clicks x2  (%1C)").arg(cost)); break;
                    default: break;
                    }
                    auto *chosen = menu.exec(upBtn->mapToGlobal(QPoint(0, upBtn->height())));
                    if (chosen == a) emit upgradePathRequested(idx, 1);
                    else if (chosen == b) emit upgradePathRequested(idx, 2);
                } else if ((isSpecial && lvl < 2) || (!isSpecial && lvl < 2)) {
                    emit upgradeRequested(idx);
                }
            });
            upgradeButtons[idx] = upBtn;

            row->addWidget(btn);
            row->addWidget(upBtn);
            g->addLayout(row, i / 2, i % 2);
        }
        return w;
    };

    cardTabs->addTab(mkGrid(0, 6), "Normal");
    cardTabs->addTab(mkGrid(6, 5), "Special");

    slot1Button = new QPushButton("Slot 1\n---", this);
    slot2Button = new QPushButton("Slot 2\n---", this);
    slot1Button->setMinimumHeight(44);
    slot2Button->setMinimumHeight(44);
    slot1Button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    slot2Button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(slot1Button, &QPushButton::clicked, this, [this]() { activeSlot = 1; emit slotChanged(1); });
    connect(slot2Button, &QPushButton::clicked, this, [this]() { activeSlot = 2; emit slotChanged(2); });

    rollButton = new QPushButton("Roll for 1 Arch!", this);
    rollButton->setFixedHeight(38);
    auto rf = rollButton->font(); rf.setBold(true); rollButton->setFont(rf);
    connect(rollButton, &QPushButton::clicked, this, [this]() { emit rollRequested(); });

    layout->addLayout(topLayout);
    layout->addWidget(slot1Button);
    layout->addWidget(slot2Button);
    layout->addWidget(cardLabel);
    layout->addWidget(messageLabel);
    layout->addWidget(cardTabs);
    layout->addWidget(rollButton);

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeButton);
}

void GachaDialog::setArchCount(int arches) {
    archLabel->setText(QString("Arch's %1").arg(arches));
    rollButton->setEnabled(arches >= 1);
}

void GachaDialog::setInventory(const std::array<int, GachaCardCount> &cardCounts,
                              const std::array<int, GachaCardCount> &cardUpgradeLevel,
                              const std::array<int, GachaCardCount> &cardUpgradePath,
                              int selectedCard, int selectedCard2) {
    cardOwned_ = cardCounts;
    cardUpgradeLevels_ = cardUpgradeLevel;
    cardUpgradePaths_ = cardUpgradePath;

    for (int i = 0; i < debugGachaCardCount(); ++i)
        updateCardButton(i, cardCounts[i], selectedCard == i, selectedCard2 == i);

    int collected = 0;
    for (int i = 0; i < debugGachaCardCount(); ++i)
        if (cardCounts[i] > 0) ++collected;
    collectedLabel->setText(QString("%1/%2").arg(collected).arg(debugGachaCardCount()));

    const auto slotText = [&](int s) -> QString {
        if (s < 0 || s >= debugGachaCardCount() || cardCounts[s] <= 0) return "---";
        const auto &card = debugGachaCardAt(s);
        const int lvl = cardUpgradeLevel[s];
        const int path = cardUpgradePath[s];
        const bool isSpecial = !isNormalEffect(card.effect);
        const auto eff = isSpecial ? specialEffectTextUpgraded(card, lvl)
                                   : normalEffectText(card, lvl);
        return QString("%1 %2\n%3 %4")
            .arg(card.name, starsForLevel(lvl, path), card.suit, eff);
    };
    slot1Button->setText(QString("Slot 1\n%1").arg(slotText(selectedCard)));
    slot1Button->setStyleSheet(activeSlot == 1
        ? "QPushButton { border: 2px solid palette(highlight); padding: 4px 8px; }"
        : "QPushButton { border: 1px solid palette(mid); padding: 4px 8px; }");
    if (selectedCard2 >= 0 && cardCounts[selectedCard2] > 0) {
        slot2Button->setText(QString("Slot 2\n%1").arg(slotText(selectedCard2)));
        slot2Button->setIcon(QIcon(penaltyUpgraded ? ":/assets/ui/stat-down.svg" : ":/assets/ui/stat-down-double.svg"));
    } else {
        slot2Button->setText("Slot 2\n---");
        slot2Button->setIcon(QIcon());
    }
    slot2Button->setStyleSheet(activeSlot == 2
        ? "QPushButton { border: 2px solid palette(highlight); padding: 4px 8px; }"
        : "QPushButton { border: 1px solid palette(mid); padding: 4px 8px; }");
}

void GachaDialog::setSecondCardSlotEnabled(bool unlocked) { slot2Button->setVisible(unlocked); if (!unlocked) activeSlot = 1; }
void GachaDialog::setSecondCardPenaltyUpgraded(bool upgraded) { penaltyUpgraded = upgraded; }
void GachaDialog::setActiveSlot(int slot) { activeSlot = slot; }

void GachaDialog::showCard(const GachaCard &card, int) {
    cardLabel->setText(clicker->compat(QString("Last: %1 %2").arg(card.name, card.suit)));
    auto p = cardLabel->palette(); p.setColor(QPalette::WindowText, card.color); cardLabel->setPalette(p);
}
void GachaDialog::showMessage(const QString &message) { messageLabel->setText(message); }

void GachaDialog::updateCardButton(int index, int ownedCount, bool selected, bool selected2) {
    const auto card = debugGachaCardAt(index);
    const auto marker = selected ? (selected2 ? clicker->compat("*\u2051") : clicker->compat("* "))
                                 : (selected2 ? clicker->compat("\u2051 ") : "");
    const int lvl = cardUpgradeLevels_[index];
    const int path = cardUpgradePaths_[index];
    const bool isSpecial = !isNormalEffect(card.effect);

    const auto eff = ownedCount > 0
        ? (isSpecial ? specialEffectTextUpgraded(card, lvl) : normalEffectText(card, lvl))
        : cardDescription(card, 0);
    const auto stars = starsForLevel(lvl, path);

    cardButtons[index]->setText(QString("%1%2%3\n%4   %5")
        .arg(marker, card.name, stars.isEmpty() ? QString() : " " + stars)
        .arg(eff, dropRateText(card)));
    cardButtons[index]->setStyleSheet(cardButtonStyle(card.color, selected || selected2));
    cardButtons[index]->setEnabled(ownedCount > 0);
    cardButtons[index]->setToolTip(QString("%1\n%2\nDrop: %3").arg(card.name, cardDescription(card, 0), dropRateText(card)));

    if (ownedCount <= 0) {
        upgradeButtons[index]->setText(QString());
        upgradeButtons[index]->setEnabled(false);
        return;
    }

    if (isSpecial && lvl >= 3) {
        const int other = path == 1 ? 2 : 1;
        upgradeButtons[index]->setText(QString::fromUtf8("\u21C4"));
        upgradeButtons[index]->setToolTip(QString("Switch to path %1 (%2C)").arg(other == 1 ? "A" : "B").arg(RepathCost));
        upgradeButtons[index]->setEnabled(clicker->game.carats >= RepathCost);
    } else if (isSpecial && lvl >= 2 && path <= 0) {
        upgradeButtons[index]->setText("?");
        upgradeButtons[index]->setToolTip(QString("Pick path (%1C)").arg(specialCardUpgradeCost(3)));
        upgradeButtons[index]->setEnabled(clicker->game.carats >= specialCardUpgradeCost(3));
    } else if ((!isSpecial && lvl >= 2) || (isSpecial && lvl >= 3)) {
        upgradeButtons[index]->setText(QString());
        upgradeButtons[index]->setEnabled(false);
    } else {
        const int nextLvl = lvl + 1;
        const int cost = isSpecial ? specialCardUpgradeCost(nextLvl) : normalCardUpgradeCost(index, nextLvl);
        upgradeButtons[index]->setText(QString::fromUtf8("\u2191"));
        upgradeButtons[index]->setToolTip(QString("Upgrade to L%1 (%2C)").arg(nextLvl).arg(cost));
        upgradeButtons[index]->setEnabled(clicker->game.carats >= cost);
    }
}
