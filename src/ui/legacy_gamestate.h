// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QtGlobal>

struct LegacyGameState {
    qint64 score = 0;
    int perClick = 1;
    int perSecond = 0;
    int clickCost = 25;
    int incomeCost = 60;

    void reset() {
        score = 0;
        perClick = 1;
        perSecond = 0;
        clickCost = 25;
        incomeCost = 60;
    }
};
