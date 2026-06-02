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
    require(game.perClick == 1);
    require(game.perSecond == 0);
    require(game.clickCost == 25);
    require(game.incomeCost == 60);
    require(nextUpgradeCost(25, 10) == 47);
    require(nextUpgradeCost(60, 15) == 105);

    game.score = 500;
    game.perClick = 8;
    game.perSecond = 4;
    game.clickCost = 120;
    game.incomeCost = 180;
    game.reset();

    require(game.score == 0);
    require(game.perClick == 1);
    require(game.perSecond == 0);
    require(game.clickCost == 25);
    require(game.incomeCost == 60);

    return 0;
}
