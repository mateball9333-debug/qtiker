#pragma once

#include "game_rules.h"

#include <array>

#include <QtGlobal>

inline constexpr int GachaCardCount = 6;

struct GameState {
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
    std::array<int, GachaCardCount> cardCounts = {};
    int clickCost = 25;
    int incomeCost = 60;

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
        cardCounts.fill(0);
        clickCost = 25;
        incomeCost = 60;
    }
};
