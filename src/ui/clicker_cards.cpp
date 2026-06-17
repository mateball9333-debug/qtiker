// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui/clicker.h"
#include "ui/gacha_dialog.h"
#include "core/game_rules.h"
#include <QRandomGenerator>
#include <QVector>

void Clicker::rollGacha(GachaDialog *dialog) {
    if (game.arches < 1) {
        dialog->setArchCount(game.arches);
        dialog->showMessage("Need 1 Arch to roll.");
        return;
    }

    game.arches -= 1;
    game.totalArchesSpent += 1;
    int index;

    if (game.gachaPityCounter >= GachaPityThreshold) {
        QVector<int> candidates;
        for (int i = 0; i < debugGachaCardCount(); ++i) {
            if (debugGachaCardAt(i).dropWeight <= RarePlusMaxWeight)
                candidates.append(i);
        }
        index = candidates[QRandomGenerator::global()->bounded(candidates.size())];
    } else {
        int roll = QRandomGenerator::global()->bounded(debugGachaTotalWeight());
        index = 0;
        for (; index < debugGachaCardCount(); ++index) {
            roll -= debugGachaCardAt(index).dropWeight;
            if (roll < 0) break;
        }
        if (index >= debugGachaCardCount())
            index = debugGachaCardCount() - 1;
    }

    const bool pulledRare = debugGachaCardAt(index).dropWeight <= RarePlusMaxWeight;
    if (pulledRare)
        game.gachaPityCounter = 0;
    else
        game.gachaPityCounter += 1;

    const bool wasDuplicate = game.cardCounts[index] > 0;
    if (wasDuplicate) {
        game.carats += DupeCarats;
    } else {
        game.cardCounts[index] = 1;
    }

    if (!wasDuplicate && game.selectedCard == -1) {
        game.selectedCard = index;
    } else if (!wasDuplicate && game.secondCardSlotUnlocked && game.selectedCard2 == -1 && index != game.selectedCard) {
        game.selectedCard2 = index;
    }

    dialog->setArchCount(game.arches);
    dialog->setInventory(game.cardCounts, game.cardUpgradeLevel, game.cardUpgradePath, game.selectedCard, game.selectedCard2);
    dialog->showCard(debugGachaCardAt(index), wasDuplicate ? 0 : 1);
    if (wasDuplicate)
        dialog->showMessage(QString("Duplicate! +%1 carats.").arg(DupeCarats));
    else
        dialog->showMessage("Card rolled.");

    saveGame();
    refreshUi();
}

void Clicker::selectGachaCard(GachaDialog *dialog, int index) {
    if (index < 0 || index >= GachaCardCount || game.cardCounts[index] <= 0) {
        dialog->showMessage("You do not own this card.");
        return;
    }

    if (index == game.selectedCard) {
        game.selectedCard = -1;
        dialog->showMessage("Deselected slot 1.");
    } else if (index == game.selectedCard2) {
        std::swap(game.selectedCard, game.selectedCard2);
        dialog->showMessage("Swapped slots.");
    } else {
        game.selectedCard = index;
        dialog->showMessage(QString("Selected %1.").arg(debugGachaCardAt(index).name));
    }

    dialog->setInventory(game.cardCounts, game.cardUpgradeLevel, game.cardUpgradePath, game.selectedCard, game.selectedCard2);
    saveGame();
    refreshUi();
}

void Clicker::selectGachaCard2(GachaDialog *dialog, int index) {
    if (index < 0 || index >= GachaCardCount || game.cardCounts[index] <= 0) {
        dialog->showMessage("You do not own this card.");
        return;
    }

    if (index == game.selectedCard2) {
        game.selectedCard2 = -1;
        dialog->showMessage("Deselected slot 2.");
    } else if (index == game.selectedCard) {
        std::swap(game.selectedCard, game.selectedCard2);
        dialog->showMessage("Swapped slots.");
    } else {
        game.selectedCard2 = index;
        dialog->showMessage(QString("Selected %1 for slot 2.").arg(debugGachaCardAt(index).name));
    }

    dialog->setInventory(game.cardCounts, game.cardUpgradeLevel, game.cardUpgradePath, game.selectedCard, game.selectedCard2);
    saveGame();
    refreshUi();
}

void Clicker::upgradeCard(int index) {
    if (index < 0 || index >= GachaCardCount || game.cardCounts[index] <= 0) return;
    const bool isSpecial = !isNormalEffect(debugGachaCardAt(index).effect);
    const int lvl = game.cardUpgradeLevel[index];
    const int nextLvl = lvl + 1;

    if (isSpecial && lvl >= 3) return;
    if (!isSpecial && lvl >= 2) return;
    if (isSpecial && lvl >= 2 && game.cardUpgradePath[index] <= 0) return;

    const int cost = isSpecial ? specialCardUpgradeCost(nextLvl) : normalCardUpgradeCost(index, nextLvl);
    if (game.carats < cost) return;

    game.carats -= cost;
    game.cardUpgradeLevel[index] = nextLvl;
    saveGame();
}

void Clicker::upgradeCardPath(int index, int path) {
    if (index < 0 || index >= GachaCardCount || game.cardCounts[index] <= 0) return;
    const bool isSpecial = !isNormalEffect(debugGachaCardAt(index).effect);
    if (!isSpecial) return;
    if (game.cardUpgradeLevel[index] < 2) return;

    const bool isRepath = game.cardUpgradePath[index] > 0;
    const int cost = isRepath ? RepathCost : specialCardUpgradeCost(3);
    if (game.carats < cost) return;

    game.carats -= cost;
    if (!isRepath) game.cardUpgradeLevel[index] = 3;
    game.cardUpgradePath[index] = path;
    saveGame();
}

qint64 Clicker::applyActiveCardBonus(qint64 value, GachaEffect effect) const {
    auto applySlot = [&](int slotIdx, bool penalized) -> qint64 {
        if (slotIdx < 0 || slotIdx >= GachaCardCount) return value;
        if (game.cardCounts[slotIdx] <= 0) return value;
        const auto card = debugGachaCardAt(slotIdx);
        if (card.effect != effect) return value;
        if (card.multiplierNumerator == card.multiplierDenominator) return value;

        const int lvl = game.cardUpgradeLevel[slotIdx];
        const qint64 num = card.multiplierNumerator * (4 + lvl);
        const qint64 den = card.multiplierDenominator * 4;
        qint64 result = value * num / den;

        if (penalized) {
            const int penalty = game.secondCardPenaltyUpgraded ? 33 : 55;
            result = value + (result - value) * (100 - penalty) / 100;
        }

        return result;
    };

    value = applySlot(game.selectedCard, false);
    if (game.secondCardSlotUnlocked)
        value = applySlot(game.selectedCard2, true);

    return value;
}

int Clicker::totalSpecialEffectValue(GachaEffect effect) const {
    int result = 0;
    auto add = [&](int slotIdx) {
        if (slotIdx < 0 || slotIdx >= GachaCardCount) return;
        if (game.cardCounts[slotIdx] <= 0) return;
        const auto card = debugGachaCardAt(slotIdx);
        if (card.effect != effect) return;

        const int lvl = game.cardUpgradeLevel[slotIdx];
        const int path = game.cardUpgradePath[slotIdx];
        double val = card.specialBase * specialCardMult(lvl);

        if (lvl >= 3) {
            if (card.effect == GachaEffect::Hoarder && path == 2) return;
            if (card.effect == GachaEffect::Hoarder && path == 1) val *= 1.5;
            if (card.effect == GachaEffect::ArchHopper && path == 1) val *= 1.5;
        }

        int intVal = static_cast<int>(val);
        if (slotIdx == game.selectedCard2) {
            const int penalty = game.secondCardPenaltyUpgraded ? 33 : 55;
            intVal = intVal * (100 - penalty) / 100;
        }
        if (intVal > 0) result += intVal;
    };
    add(game.selectedCard);
    add(game.selectedCard2);

    if (effect == GachaEffect::CritChance) {
        for (int slotIdx : {game.selectedCard, game.selectedCard2}) {
            if (slotIdx < 0 || slotIdx >= GachaCardCount) continue;
            if (game.cardCounts[slotIdx] <= 0) continue;
            const auto card = debugGachaCardAt(slotIdx);
            if (card.effect == GachaEffect::CritPower
                && game.cardUpgradeLevel[slotIdx] >= 3
                && game.cardUpgradePath[slotIdx] == 2) {
                result += CritBladePathB_Chance;
            }
        }
    }

    return result;
}

bool Clicker::hasSpecialEffect(GachaEffect effect) const {
    if (game.selectedCard >= 0 && game.selectedCard < GachaCardCount) {
        if (debugGachaCardAt(game.selectedCard).effect == effect) return true;
    }
    if (game.secondCardSlotUnlocked && game.selectedCard2 >= 0 && game.selectedCard2 < GachaCardCount) {
        if (debugGachaCardAt(game.selectedCard2).effect == effect) return true;
    }
    return false;
}

bool Clicker::isPathActive(GachaEffect effect, int path) const {
    for (int s : {game.selectedCard, game.selectedCard2}) {
        if (s >= 0 && s < GachaCardCount && game.cardCounts[s] > 0
            && debugGachaCardAt(s).effect == effect
            && game.cardUpgradeLevel[s] >= 3 && game.cardUpgradePath[s] == path)
            return true;
    }
    return false;
}
