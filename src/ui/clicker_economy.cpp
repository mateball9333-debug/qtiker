// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui/clicker.h"
#include "core/game_rules.h"
#include "core/cards.h"
#include "ui/particle_overlay.h"
#include <QRandomGenerator>
#include <QDateTime>
#include <QPushButton>
#include <QTimer>

void Clicker::makeClick() {
    const auto cardEarned = applyActiveCardBonus(game.perClick, GachaEffect::Click);
    const auto buffed = applyTimedBuffBonuses(cardEarned, TimedBuffEffect::Click);

    const int archBonus = totalSpecialEffectValue(GachaEffect::ArchHopper);
    const auto archBoosted = (archBonus > 0 && game.arches > 0)
        ? applyMultiplier(buffed, 100 + archBonus * game.arches, 100)
        : buffed;

    const int bonusCrit = totalSpecialEffectValue(GachaEffect::CritChance);
    const int critPowerTenths = totalSpecialEffectValue(GachaEffect::CritPower);
    const int bigCritThreshold = 1 + bonusCrit / 5;
    const int totalCritThreshold = 6 + bonusCrit;

    const int roll = QRandomGenerator::global()->bounded(100);
    qint64 earned;
    bool critted = false;
    if (roll < bigCritThreshold) {
        const qint64 critMult = critPowerTenths > 10 ? 5 * (critPowerTenths / 10) : 5;
        earned = archBoosted * critMult;
        critted = true;
        triggerCritBurst(true);
        if (critSound && !isCritSoundMuted()) critSound->play();
    } else if (roll < totalCritThreshold) {
        const qint64 critMult = critPowerTenths > 10 ? 2 * (critPowerTenths / 10) : 2;
        earned = archBoosted * critMult;
        critted = true;
        triggerCritBurst(false);
        if (critSound && !isCritSoundMuted()) critSound->play();
    } else {
        earned = archBoosted;
    }

    if (isPathActive(GachaEffect::Speedrun, 2))
        earned *= 2;

    if (critted && isPathActive(GachaEffect::CritChance, 2))
        game.archProgress += 1;

    earned <<= game.clickMultLevel;

    game.score += earned;
    game.totalClicks += 1;
    game.totalScoreEarned += earned;
    addArchProgress(earned);

    if (clickSound && !isClickSoundMuted()) {
        clickSound->play();
    }

    buttonChange();
    saveGame();
    refreshUi();
}

void Clicker::buyClickUpgrade() {
    if (game.score < game.clickCost) {
        return;
    }

    game.score -= game.clickCost;
    game.perClick += 1;
    game.clickCost = nextClickUpgradeCost(game.clickCost);

    if (buySound) buySound->play();
    saveGame();
    refreshUi();
}

void Clicker::buyIncomeUpgrade() {
    if (game.score < game.incomeCost) {
        return;
    }

    game.score -= game.incomeCost;
    game.perSecond += 1;
    game.incomeCost = nextIncomeUpgradeCost(game.incomeCost);

    if (buySound) buySound->play();
    saveGame();
    refreshUi();
}

void Clicker::addPassiveIncome() {
    if (game.perSecond == 0) {
        return;
    }

    const auto cardEarned = applyActiveCardBonus(game.perSecond, GachaEffect::Income);
    const auto buffed = applyTimedBuffBonuses(cardEarned, TimedBuffEffect::Income);
    const auto egg = game.incomeBuffEasterEgg ? applyMultiplierRoundedUp(buffed, 11, 10) : buffed;

    const int hoardBonus = totalSpecialEffectValue(GachaEffect::Hoarder);
    const auto hoarded = (hoardBonus > 0 && game.score > 0)
        ? applyMultiplier(egg, 100 + hoardBonus * qMin(game.score / 10000, Q_INT64_C(400)), 100)
        : egg;

    auto finalPassive = hoarded;

    if (isPathActive(GachaEffect::Hoarder, 2) && game.arches > 0) {
        const int pct = debugGachaCardAt(9).specialBase
            * specialCardMult(game.cardUpgradeLevel[9])
            * HoardPathB_PerArch;
        finalPassive = applyMultiplier(finalPassive, 100 + pct * game.arches, 100);
    }

    if (hasSpecialEffect(GachaEffect::Speedrun))
        finalPassive *= 2;

    if (isPathActive(GachaEffect::ArchHopper, 2)) {
        const int archBonus = totalSpecialEffectValue(GachaEffect::ArchHopper);
        if (archBonus > 0 && game.arches > 0)
            finalPassive = applyMultiplier(finalPassive, 100 + archBonus * game.arches / 3, 100);
    }

    const auto finalEarned = finalPassive << game.incomeMultLevel;
    game.score += finalEarned;
    game.totalScoreEarned += finalEarned;
    addArchProgress(finalEarned);
    saveGame();
    refreshUi();
}

void Clicker::addArchProgress(qint64 amount) {
    if (amount <= 0) {
        return;
    }

    game.archProgress += amount;
    while (game.archProgress >= game.nextArchAt) {
        game.arches += 1;
        game.totalArchesEarned += 1;
        game.nextArchAt = nextArchThreshold(game.nextArchAt);
    }
}

void Clicker::maybeStartClickEffect() {
    if (clickEffectTimer->isActive()) {
        return;
    }

    if (QRandomGenerator::global()->bounded(100) >= ClickEffectChance) {
        return;
    }

    clickEffectFramesLeft = ClickEffectFrames;
    clickEffectHue = QRandomGenerator::global()->bounded(360);
    clickEffectTimer->start();
    updateClickEffect();
}

void Clicker::updateClickEffect() {
    if (clickEffectFramesLeft <= 0) {
        stopClickEffect();
        return;
    }

    const auto color = QColor::fromHsv(clickEffectHue, 180, 230);
    clickGlowEffect = setButtonGlow(clickButton, clickGlowEffect, color);
    clickEffectHue = (clickEffectHue + 18) % 360;
    clickEffectFramesLeft -= 1;
}

void Clicker::stopClickEffect() {
    clickEffectTimer->stop();
    clickEffectFramesLeft = 0;
    clearButtonGlow(clickButton, clickGlowEffect);
    clickGlowEffect = nullptr;
}

void Clicker::triggerCritBurst(bool big) {
    if (!particleOverlay) return;
    particleOverlay->resize(size());
    const QRect btnRect = clickButton->geometry();
    const QPointF center(
        btnRect.x() + btnRect.width() / 2.0,
        btnRect.y() + btnRect.height() / 2.0
    );
    particleOverlay->burstAt(center, big ? 60 : 30, big);
}

qint64 Clicker::applyTimedBuffBonuses(qint64 value, TimedBuffEffect effect) const {
    qint64 result = value;

    for (const auto &rule : TimedBuffRules) {
        if (rule.effect != effect || !isTimedBuffActive(rule.buff)) {
            continue;
        }

        result = applyMultiplierRoundedUp(
            result,
            rule.multiplierNumerator,
            rule.multiplierDenominator
        );
    }

    return result;
}

bool Clicker::isTimedBuffActive(TimedBuff buff) const {
    return timedBuffSecondsLeft(buff) > 0;
}

int Clicker::timedBuffSecondsLeft(TimedBuff buff) const {
    const int index = timedBuffIndex(buff);
    if (index < 0 || index >= TimedBuffCount) {
        return 0;
    }

    const qint64 remainingMs = game.buffExpiresAtMs[index] - QDateTime::currentMSecsSinceEpoch();
    if (remainingMs <= 0) {
        return 0;
    }

    return static_cast<int>((remainingMs + 999) / 1000);
}

void Clicker::activateTimedBuff(TimedBuff buff, int durationSeconds) {
    const int index = timedBuffIndex(buff);
    if (index < 0 || index >= TimedBuffCount || durationSeconds <= 0) {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 startAt = game.buffExpiresAtMs[index] > now ? game.buffExpiresAtMs[index] : now;
    game.buffExpiresAtMs[index] = startAt + qint64{durationSeconds} * 1000;
}

