#include "gamestate.h"
#include "game_rules.h"

#include <cstdlib>

namespace {
void require(bool condition) {
    if (!condition) {
        std::exit(1);
    }
}
}

int main() {
    GameState game;

    require(game.score == 0);
    require(game.archProgress == 0);
    require(game.nextArchAt == 10);
    require(game.perClick == 1);
    require(game.perSecond == 0);
    require(game.arches == 0);
    require(game.carats == 0);
    require(game.totalClicks == 0);
    require(game.totalScoreEarned == 0);
    require(game.totalPlaySeconds == 0);
    require(game.totalArchesEarned == 0);
    require(game.clickButtonRightClicks == 0);
    require(game.buffExpiresAtMs[timedBuffIndex(TimedBuff::IncomeGain)] == 0);
    require(game.selectedCard == -1);
    for (const auto count : game.cardCounts) {
        require(count == 0);
    }
    require(game.clickCost == 25);
    require(game.incomeCost == 60);
    require(nextUpgradeCost(25, 10) == 47);
    require(nextUpgradeCost(60, 15) == 105);
    require(nextClickUpgradeCost(25) == 45);
    require(nextIncomeUpgradeCost(60) == 90);
    require(nextArchThreshold(10) == 25);
    require(nextArchThreshold(25) == 62);
    require(applyMultiplier(6, 3, 2) == 9);
    require(applyMultiplierRoundedUp(1, 3, 2) == 2);
    require(CaratBurnCost == 1000);
    require(CaratBurnReward == 1);
    require(TimedBuffRules.size() == TimedBuffCount);
    require(TimedBuffRules[0].buff == TimedBuff::IncomeGain);
    require(TimedBuffRules[0].effect == TimedBuffEffect::Income);
    require(TimedBuffRules[0].caratCost == 1);
    require(TimedBuffRules[0].durationSeconds == 30);
    require(effectiveCardCopies(0) == 1);
    require(effectiveCardCopies(6) == 6);
    require(effectiveCardCopies(100) == 6);
    require(applyStackedMultiplier(10, 3, 2, 1) == 15);
    require(applyStackedMultiplier(10, 3, 2, 6) == 20);
    require(applyStackedMultiplier(10, 3, 2, 100) == 20);

    game.score = 500;
    game.archProgress = 64;
    game.nextArchAt = 156;
    game.perClick = 8;
    game.perSecond = 4;
    game.arches = 3;
    game.carats = 7;
    game.totalClicks = 99;
    game.totalScoreEarned = 321;
    game.totalPlaySeconds = 123;
    game.totalArchesEarned = 4;
    game.clickButtonRightClicks = 8;
    game.buffExpiresAtMs[timedBuffIndex(TimedBuff::IncomeGain)] = 123456;
    game.selectedCard = 1;
    game.cardCounts[0] = 2;
    game.cardCounts[1] = 1;
    game.cardCounts[5] = 1;
    game.clickCost = 120;
    game.incomeCost = 180;
    game.reset();

    require(game.score == 0);
    require(game.archProgress == 0);
    require(game.nextArchAt == 10);
    require(game.perClick == 1);
    require(game.perSecond == 0);
    require(game.arches == 0);
    require(game.carats == 0);
    require(game.totalClicks == 0);
    require(game.totalScoreEarned == 0);
    require(game.totalPlaySeconds == 0);
    require(game.totalArchesEarned == 0);
    require(game.clickButtonRightClicks == 0);
    require(game.buffExpiresAtMs[timedBuffIndex(TimedBuff::IncomeGain)] == 0);
    require(game.selectedCard == -1);
    for (const auto count : game.cardCounts) {
        require(count == 0);
    }
    require(game.clickCost == 25);
    require(game.incomeCost == 60);

    return 0;
}
