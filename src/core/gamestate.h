// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "game_rules.h"

#include <array>

#include <QtGlobal>

inline constexpr int GachaCardCount = 11;

struct GameState {
    static constexpr qint64 SaveMagic1 = 333;
    static constexpr qint64 SaveMagic2 = 33;
    static constexpr qint64 SaveMagic3 = 3;

    qint64 score = 0;
    qint64 archProgress = 0;
    qint64 nextArchAt = 10;
    qint64 perClick = 1;
    qint64 perSecond = 0;
    qint64 arches = 0;
    qint64 carats = 0;
    qint64 totalClicks = 0;
    qint64 totalScoreEarned = 0;
    qint64 totalPlaySeconds = 0;
    qint64 totalArchesEarned = 0;
    qint64 clickButtonRightClicks = 0;
    std::array<qint64, TimedBuffCount> buffExpiresAtMs = {};
    int selectedCard = -1;
    int selectedCard2 = -1;
    bool secondCardSlotUnlocked = false;
    bool secondCardPenaltyUpgraded = false;
    std::array<int, GachaCardCount> cardCounts = {};
    std::array<int, GachaCardCount> cardUpgradeLevel = {};
    std::array<int, GachaCardCount> cardUpgradePath = {};
    qint64 clickCost = 25;
    qint64 incomeCost = 60;
    int clickMultLevel = 0;
    int incomeMultLevel = 0;
    int gachaPityCounter = 0;
    qint64 casinoTotalWon = 0;
    qint64 casinoTotalSpins = 0;
    qint64 archesFromCasino = 0;
    qint64 totalArchesSpent = 0;
    bool incomeBuffEasterEgg = false;

    qint64 computeChecksum() const {
        auto mix = [](qint64 h, qint64 v) -> qint64 {
            return ((h << 5) - h) ^ v;
        };

        qint64 h = SaveMagic1;
        h = mix(h, score);
        h = mix(h, archProgress);
        h = mix(h, nextArchAt);
        h = mix(h, static_cast<qint64>(perClick));
        h = mix(h, static_cast<qint64>(perSecond));
        h = mix(h, static_cast<qint64>(arches));
        h = mix(h, carats);
        h = mix(h, totalClicks);
        h = mix(h, totalScoreEarned);
        h = mix(h, totalPlaySeconds);
        h = mix(h, totalArchesEarned);
        h = mix(h, clickButtonRightClicks);
        for (auto v : buffExpiresAtMs) h = mix(h, v);
        h = mix(h, static_cast<qint64>(selectedCard));
        h = mix(h, static_cast<qint64>(selectedCard2));
        h = mix(h, secondCardSlotUnlocked ? 1 : 0);
        h = mix(h, secondCardPenaltyUpgraded ? 1 : 0);
        for (auto v : cardCounts) h = mix(h, static_cast<qint64>(v));
        for (auto v : cardUpgradeLevel) h = mix(h, static_cast<qint64>(v));
        for (auto v : cardUpgradePath) h = mix(h, static_cast<qint64>(v));
        h = mix(h, static_cast<qint64>(clickCost));
        h = mix(h, static_cast<qint64>(incomeCost));
        if (clickMultLevel) h = mix(h, static_cast<qint64>(clickMultLevel));
        if (incomeMultLevel) h = mix(h, static_cast<qint64>(incomeMultLevel));
        if (gachaPityCounter) h = mix(h, static_cast<qint64>(gachaPityCounter));
        h = mix(h, incomeBuffEasterEgg ? 1 : 0);
        return h;
    }

    void reset() {
        score = 0;
        archProgress = 0;
        nextArchAt = 10;
        perClick = 1;
        perSecond = 0;
        arches = 0;
        carats = 0;
        totalClicks = 0;
        totalScoreEarned = 0;
        totalPlaySeconds = 0;
        totalArchesEarned = 0;
        clickButtonRightClicks = 0;
        buffExpiresAtMs.fill(0);
        selectedCard = -1;
        selectedCard2 = -1;
        secondCardSlotUnlocked = false;
        secondCardPenaltyUpgraded = false;
        cardCounts.fill(0);
        cardUpgradeLevel.fill(0);
        cardUpgradePath.fill(0);
        clickCost = 25;
        incomeCost = 60;
        clickMultLevel = 0;
        incomeMultLevel = 0;
        gachaPityCounter = 0;
        casinoTotalWon = 0;
        casinoTotalSpins = 0;
        archesFromCasino = 0;
        totalArchesSpent = 0;
        incomeBuffEasterEgg = false;
    }
};
