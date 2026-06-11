#pragma once

#include "game_rules.h"

#include <array>

#include <QtGlobal>

inline constexpr int GachaCardCount = 6;

struct GameState {
    static constexpr qint64 SaveMagic1 = 333;
    static constexpr qint64 SaveMagic2 = 33;
    static constexpr qint64 SaveMagic3 = 3;

    qint64 score = 0;
    qint64 archProgress = 0;
    qint64 nextArchAt = 10;
    int perClick = 1;
    int perSecond = 0;
    int arches = 0;
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
    int clickCost = 25;
    int incomeCost = 60;
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
        h = mix(h, static_cast<qint64>(clickCost));
        h = mix(h, static_cast<qint64>(incomeCost));
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
        clickCost = 25;
        incomeCost = 60;
        incomeBuffEasterEgg = false;
    }
};
