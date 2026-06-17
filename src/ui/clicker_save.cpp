// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui/clicker.h"
#include "core/appversion.h"
#include "core/game_rules.h"
#include <QSettings>
#include <QString>

bool Clicker::loadGame() {
    QSettings settings("qtiker", "qtiker");
    settings.beginGroup(QString("slot%1").arg(currentSlot));

    const bool hasIntegrity = settings.contains("integrity1")
        && settings.contains("integrity2")
        && settings.contains("integrity3");
    if (hasIntegrity) {
        const qint64 m1 = settings.value("integrity1", qint64{0}).toLongLong();
        const qint64 m2 = settings.value("integrity2", qint64{0}).toLongLong();
        const qint64 m3 = settings.value("integrity3", qint64{0}).toLongLong();
        if (m1 != GameState::SaveMagic1 || m2 != GameState::SaveMagic2 || m3 != GameState::SaveMagic3) {
            settings.endGroup();
            game.reset();
            return false;
        }
    }

    game.score = settings.value(SettingsKeys::Score,
        settings.value(SettingsKeys::LegacyTotalClicks, game.score)).toLongLong();
    game.archProgress = settings.value(SettingsKeys::ArchProgress, game.archProgress).toLongLong();
    game.nextArchAt = settings.value(SettingsKeys::NextArchAt, game.nextArchAt).toLongLong();
    game.perClick = settings.value(SettingsKeys::PerClick,
        settings.value(SettingsKeys::LegacyClickPower, game.perClick)).toLongLong();
    game.perSecond = settings.value(SettingsKeys::PerSecond,
        settings.value(SettingsKeys::LegacyAutoPower, game.perSecond)).toLongLong();
    game.arches = settings.value(SettingsKeys::Arches, game.arches).toLongLong();
    game.carats = settings.value(SettingsKeys::Carats, game.carats).toLongLong();
    game.totalClicks = settings.value(SettingsKeys::TotalClicks, game.totalClicks).toLongLong();
    game.totalScoreEarned = settings.value(SettingsKeys::TotalScoreEarned,
        settings.value(SettingsKeys::LegacyTotalClickScoreEarned, game.totalScoreEarned)).toLongLong();
    game.totalPlaySeconds = settings.value(SettingsKeys::TotalPlaySeconds, game.totalPlaySeconds).toLongLong();
    game.totalArchesEarned = settings.value(SettingsKeys::TotalArchesEarned, game.totalArchesEarned).toLongLong();
    game.clickButtonRightClicks = settings.value(SettingsKeys::ClickButtonRightClicks, game.clickButtonRightClicks).toLongLong();
    for (int index = 0; index < TimedBuffCount; ++index) {
        game.buffExpiresAtMs[index] = settings.value(
            QString("%1%2").arg(SettingsKeys::BuffExpiresAtPrefix).arg(index),
            game.buffExpiresAtMs[index]
        ).toLongLong();
    }
    game.selectedCard = settings.value(SettingsKeys::SelectedCard, game.selectedCard).toInt();
    game.selectedCard2 = settings.value(SettingsKeys::SelectedCard2, game.selectedCard2).toInt();
    game.secondCardSlotUnlocked = settings.value(SettingsKeys::SecondCardSlotUnlocked, game.secondCardSlotUnlocked).toBool();
    game.secondCardPenaltyUpgraded = settings.value(SettingsKeys::SecondCardPenaltyUpgraded, game.secondCardPenaltyUpgraded).toBool();
    for (int index = 0; index < GachaCardCount; ++index) {
        game.cardCounts[index] = settings.value(QString("card%1").arg(index), game.cardCounts[index]).toInt();
    }
    for (int index = 0; index < GachaCardCount; ++index) {
        game.cardUpgradeLevel[index] = settings.value(QString("cardUp%1").arg(index), game.cardUpgradeLevel[index]).toInt();
    }
    for (int index = 0; index < GachaCardCount; ++index) {
        game.cardUpgradePath[index] = settings.value(QString("cardPath%1").arg(index), game.cardUpgradePath[index]).toInt();
    }
    game.clickCost = settings.value(SettingsKeys::ClickCost,
        settings.value(SettingsKeys::LegacyClickUpgradeCost, game.clickCost)).toLongLong();
    game.incomeCost = settings.value(SettingsKeys::IncomeCost,
        settings.value(SettingsKeys::LegacyAutoUpgradeCost, game.incomeCost)).toLongLong();
    game.incomeBuffEasterEgg = settings.value(SettingsKeys::IncomeBuffEasterEgg, game.incomeBuffEasterEgg).toBool();
    game.clickMultLevel = settings.value(SettingsKeys::ClickMultLevel, game.clickMultLevel).toInt();
    game.incomeMultLevel = settings.value(SettingsKeys::IncomeMultLevel, game.incomeMultLevel).toInt();
    if (game.clickMultLevel < 0) game.clickMultLevel = 0;
    if (game.clickMultLevel > ClickMultMaxLevel) game.clickMultLevel = ClickMultMaxLevel;
    if (game.incomeMultLevel < 0) game.incomeMultLevel = 0;
    if (game.incomeMultLevel > IncomeMultMaxLevel) game.incomeMultLevel = IncomeMultMaxLevel;
    game.gachaPityCounter = settings.value(SettingsKeys::GachaPityCounter, game.gachaPityCounter).toInt();
    if (game.gachaPityCounter < 0) game.gachaPityCounter = 0;
    game.casinoTotalWon = settings.value(SettingsKeys::CasinoTotalWon, game.casinoTotalWon).toLongLong();
    game.casinoTotalSpins = settings.value(SettingsKeys::CasinoTotalSpins, game.casinoTotalSpins).toLongLong();
    game.archesFromCasino = settings.value(SettingsKeys::ArchesFromCasino, game.archesFromCasino).toLongLong();
    game.totalArchesSpent = settings.value(SettingsKeys::TotalArchesSpent, game.totalArchesSpent).toLongLong();

    bool didMigrate = false;
    for (int i = 0; i < GachaCardCount; ++i) {
        if (game.cardCounts[i] > 1) {
            game.carats += (game.cardCounts[i] - 1) * DupeCarats;
            game.cardCounts[i] = 1;
            didMigrate = true;
        }
    }

    if (hasIntegrity && !didMigrate) {
        const qint64 storedChecksum = settings.value(SettingsKeys::Checksum, qint64{0}).toLongLong();
        const qint64 actualChecksum = game.computeChecksum();
        if (storedChecksum != actualChecksum) {
            settings.endGroup();
            game.reset();
            return false;
        }
    }

    settings.endGroup();

    {
        QSettings meta("qtiker", "qtiker");
        changelogSeenForVersion = meta.value(
            SettingsKeys::LastSeenChangelogVersion, QString()
        ).toString() == AppVersion;
    }

    if (game.selectedCard < 0
        || game.selectedCard >= GachaCardCount
        || game.cardCounts[game.selectedCard] <= 0) {
        game.selectedCard = -1;
    }
    if (game.selectedCard2 < 0
        || game.selectedCard2 >= GachaCardCount
        || game.cardCounts[game.selectedCard2] <= 0) {
        game.selectedCard2 = -1;
    }
    if (!game.secondCardSlotUnlocked) {
        game.selectedCard2 = -1;
    }

    return true;
}

void Clicker::saveGame() {
    QSettings settings("qtiker", "qtiker");
    settings.beginGroup(QString("slot%1").arg(currentSlot));

    game.totalPlaySeconds = currentTotalPlaySeconds();
    playTimer.restart();

    settings.setValue(SettingsKeys::Score, game.score);
    settings.setValue(SettingsKeys::ArchProgress, game.archProgress);
    settings.setValue(SettingsKeys::NextArchAt, game.nextArchAt);
    settings.setValue(SettingsKeys::PerClick, game.perClick);
    settings.setValue(SettingsKeys::PerSecond, game.perSecond);
    settings.setValue(SettingsKeys::Arches, game.arches);
    settings.setValue(SettingsKeys::Carats, game.carats);
    settings.setValue(SettingsKeys::TotalClicks, game.totalClicks);
    settings.setValue(SettingsKeys::TotalScoreEarned, game.totalScoreEarned);
    settings.setValue(SettingsKeys::TotalPlaySeconds, currentTotalPlaySeconds());
    settings.setValue(SettingsKeys::TotalArchesEarned, game.totalArchesEarned);
    settings.setValue(SettingsKeys::ClickButtonRightClicks, game.clickButtonRightClicks);
    for (int index = 0; index < TimedBuffCount; ++index) {
        settings.setValue(
            QString("%1%2").arg(SettingsKeys::BuffExpiresAtPrefix).arg(index),
            game.buffExpiresAtMs[index]
        );
    }
    settings.setValue(SettingsKeys::SelectedCard, game.selectedCard);
    settings.setValue(SettingsKeys::SelectedCard2, game.selectedCard2);
    settings.setValue(SettingsKeys::SecondCardSlotUnlocked, game.secondCardSlotUnlocked);
    settings.setValue(SettingsKeys::SecondCardPenaltyUpgraded, game.secondCardPenaltyUpgraded);
    for (int index = 0; index < GachaCardCount; ++index) {
        settings.setValue(QString("card%1").arg(index), game.cardCounts[index]);
    }
    for (int index = 0; index < GachaCardCount; ++index) {
        settings.setValue(QString("cardUp%1").arg(index), game.cardUpgradeLevel[index]);
    }
    for (int index = 0; index < GachaCardCount; ++index) {
        settings.setValue(QString("cardPath%1").arg(index), game.cardUpgradePath[index]);
    }
    settings.setValue(SettingsKeys::ClickCost, game.clickCost);
    settings.setValue(SettingsKeys::IncomeCost, game.incomeCost);
    settings.setValue(SettingsKeys::IncomeBuffEasterEgg, game.incomeBuffEasterEgg);
    settings.setValue(SettingsKeys::ClickMultLevel, game.clickMultLevel);
    settings.setValue(SettingsKeys::IncomeMultLevel, game.incomeMultLevel);
    settings.setValue(SettingsKeys::GachaPityCounter, game.gachaPityCounter);
    settings.setValue(SettingsKeys::CasinoTotalWon, game.casinoTotalWon);
    settings.setValue(SettingsKeys::CasinoTotalSpins, game.casinoTotalSpins);
    settings.setValue(SettingsKeys::ArchesFromCasino, game.archesFromCasino);
    settings.setValue(SettingsKeys::TotalArchesSpent, game.totalArchesSpent);

    settings.setValue("integrity1", GameState::SaveMagic1);
    settings.setValue("integrity2", GameState::SaveMagic2);
    settings.setValue("integrity3", GameState::SaveMagic3);
    settings.setValue(SettingsKeys::Checksum, game.computeChecksum());

    settings.endGroup();

    emit saveCompleted();
}

