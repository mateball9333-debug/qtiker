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
    require(game.selectedCard2 == -1);
    require(game.secondCardSlotUnlocked == false);
    require(game.secondCardPenaltyUpgraded == false);
    for (const auto count : game.cardCounts) {
        require(count == 0);
    }
    require(game.clickCost == 25);
    require(game.incomeCost == 60);
    require(game.incomeBuffEasterEgg == false);
    require(SecondCardSlotCost == 25);

    const auto defaultChecksum = game.computeChecksum();
    game.score = 100;
    const auto modifiedChecksum = game.computeChecksum();
    require(defaultChecksum != modifiedChecksum);

    GameState copy;
    copy.score = 100;
    require(copy.computeChecksum() == modifiedChecksum);

    return 0;
}
