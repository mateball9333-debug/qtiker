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

inline int nextUpgradeCost(int currentCost, int extra) {
    return currentCost * 3 / 2 + extra;
}

inline int nextSoftenedUpgradeCost(int currentCost, int extra, int growthPercent) {
    const int increase = nextUpgradeCost(currentCost, extra) - currentCost;
    return currentCost + (increase * growthPercent + 50) / 100;
}

inline int nextClickUpgradeCost(int currentCost) {
    return nextSoftenedUpgradeCost(currentCost, 10, 90);
}

inline int nextIncomeUpgradeCost(int currentCost) {
    return nextSoftenedUpgradeCost(currentCost, 15, 67);
}

inline qint64 nextArchThreshold(qint64 currentThreshold) {
    return currentThreshold * 5 / 2;
}

inline qint64 applyMultiplier(qint64 value, int numerator, int denominator) {
    return value * numerator / denominator;
}

inline qint64 applyMultiplierRoundedUp(qint64 value, int numerator, int denominator) {
    return (value * numerator + denominator - 1) / denominator;
}

inline int effectiveCardCopies(int ownedCount) {
    if (ownedCount < 1) {
        return 1;
    }

    return ownedCount > 6 ? 6 : ownedCount;
}

inline qint64 applyStackedMultiplier(qint64 value, int numerator, int denominator, int ownedCount) {
    const int extraTenths = effectiveCardCopies(ownedCount) - 1;
    const int stackedNumerator = numerator * 10 + extraTenths * denominator;
    const int stackedDenominator = denominator * 10;
    return applyMultiplier(value, stackedNumerator, stackedDenominator);
}
