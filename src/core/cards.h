// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "gamestate.h"

#include <QColor>
#include <QString>

enum class GachaEffect {
    Click,
    Income,
    CritChance,
    CritPower,
    ArchHopper,
    Hoarder,
    Speedrun,
};

inline bool isNormalEffect(GachaEffect effect) {
    return effect == GachaEffect::Click || effect == GachaEffect::Income;
}

struct GachaCard {
    QString name;
    QString suit;
    QColor color;
    GachaEffect effect;
    int multiplierNumerator;
    int multiplierDenominator;
    int dropWeight;
    int specialBase = 0;
    int specialPerCopy = 0;
};

inline const GachaCard DebugGachaCards[] = {
    {"Normal", "Common", QColor("#777777"), GachaEffect::Click, 3, 2, 47},
    {"Uncommon", "Uncommon", QColor("#2e7d32"), GachaEffect::Income, 3, 2, 23},
    {"Rare", "Rare", QColor("#1565c0"), GachaEffect::Click, 5, 2, 13},
    {"Epic", "Epic", QColor("#6a1b9a"), GachaEffect::Income, 3, 1, 6},
    {"Legendary", "Legendary", QColor("#f9a825"), GachaEffect::Click, 6, 1, 2},
    {"Mythic", "Mythic", QColor("#c62828"), GachaEffect::Income, 10, 1, 1},
    {"CritEye", "Rare", QColor("#ff6f00"), GachaEffect::CritChance, 1, 1, 3, 5, 1},
    {"CritBlade", "Epic", QColor("#b71c1c"), GachaEffect::CritPower, 1, 1, 2, 20, 0},
    {"ArchMage", "Legendary", QColor("#4a148c"), GachaEffect::ArchHopper, 1, 1, 2, 10, 2},
    {"Hoarder", "Rare", QColor("#ffab00"), GachaEffect::Hoarder, 1, 1, 3, 10, 2},
    {"SpeedDemon", "Mythic", QColor("#00e5ff"), GachaEffect::Speedrun, 1, 1, 1, 2, 0},
};

inline int debugGachaCardCount() {
    return GachaCardCount;
}

inline GachaCard debugGachaCardAt(int index) {
    return DebugGachaCards[index];
}

inline int debugGachaTotalWeight() {
    int total = 0;
    for (const auto &card : DebugGachaCards) {
        total += card.dropWeight;
    }
    return total;
}
