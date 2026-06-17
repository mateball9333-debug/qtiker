// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>

#include <QtGlobal>

enum class TimedBuff {
    IncomeGain = 0,
    ClickGain = 1,
};

enum class TimedBuffEffect {
    Click,
    Income,
};

inline constexpr int TimedBuffCount = 2;
inline constexpr qint64 CaratBurnCost = 1000;
inline constexpr qint64 CaratBurnReward = 1;
inline constexpr qint64 SecondCardSlotCost = 25;
inline constexpr qint64 PenaltyUpgradeCost = 10;
inline constexpr int ClickMultBaseCost = 50;
inline constexpr int ClickMultMaxLevel = 5;
inline constexpr int IncomeMultBaseCost = 50;
inline constexpr int IncomeMultMaxLevel = 5;

inline int permanentMultCost(int level, int baseCost) {
    int c = baseCost;
    for (int i = 0; i < level; ++i)
        c *= 3;
    return c;
}

inline constexpr int GachaPityThreshold = 10;
inline constexpr int RarePlusMaxWeight = 14;

struct TimedBuffRule {
    TimedBuff buff;
    TimedBuffEffect effect;
    const char *name;
    qint64 caratCost;
    int durationSeconds;
    int multiplierNumerator;
    int multiplierDenominator;
};

inline constexpr TimedBuffRule IncomeGainBuffRule = {
    TimedBuff::IncomeGain,
    TimedBuffEffect::Income,
    "Income gain 1.5x",
    1,
    30,
    3,
    2,
};

inline constexpr TimedBuffRule ClickGainBuffRule = {
    TimedBuff::ClickGain,
    TimedBuffEffect::Click,
    "Click gain 1.25x",
    1,
    30,
    5,
    4,
};

inline constexpr std::array<TimedBuffRule, TimedBuffCount> TimedBuffRules = {
    IncomeGainBuffRule,
    ClickGainBuffRule,
};

inline int timedBuffIndex(TimedBuff buff) {
    return static_cast<int>(buff);
}

inline qint64 nextUpgradeCost(qint64 currentCost, qint64 extra) {
    return currentCost * 3 / 2 + extra;
}

inline qint64 nextSoftenedUpgradeCost(qint64 currentCost, qint64 extra, int growthPercent) {
    const qint64 increase = nextUpgradeCost(currentCost, extra) - currentCost;
    return currentCost + (increase * growthPercent + 50) / 100;
}

inline qint64 nextClickUpgradeCost(qint64 currentCost) {
    return nextSoftenedUpgradeCost(currentCost, 10, 90);
}

inline qint64 nextIncomeUpgradeCost(qint64 currentCost) {
    return nextSoftenedUpgradeCost(currentCost, 15, 67);
}

inline qint64 nextArchThreshold(qint64 currentThreshold) {
    return currentThreshold * 5 / 2;
}

inline qint64 applyMultiplier(qint64 value, qint64 numerator, qint64 denominator) {
    return value * numerator / denominator;
}

inline qint64 applyMultiplierRoundedUp(qint64 value, qint64 numerator, qint64 denominator) {
    return (value * numerator + denominator - 1) / denominator;
}

inline int normalCardUpgradeCost(int cardIndex, int toLevel) {
    constexpr int baseCosts[] = {10, 20, 40, 80, 120, 200};
    return baseCosts[cardIndex] * (toLevel == 1 ? 1 : 3);
}

inline int specialCardUpgradeCost(int toLevel) {
    constexpr int costs[] = {0, 30, 100, 250};
    return costs[toLevel];
}

inline double normalCardMult(int upgradeLevel) {
    return 1.0 + 0.25 * upgradeLevel;
}

inline double specialCardMult(int upgradeLevel) {
    return 1.0 + 0.20 * upgradeLevel;
}

inline constexpr int DupeCarats = 5;

inline constexpr qint64 HoardPathB_Cap = 10'000'000;
inline constexpr int HoardPathB_PerArch = 20;   // specialBase * 20 = per-arch % for path B
inline constexpr int CritEyePathA_Chance = 3;     // +3% extra crit chance
inline constexpr int CritBladePathB_Chance = 5;   // +5% crit chance
inline constexpr int RepathCost = 400;
