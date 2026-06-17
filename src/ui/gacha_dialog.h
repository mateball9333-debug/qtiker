// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "core/cards.h"

#include <QColor>
#include <QDialog>
#include <QString>
#include <QTabWidget>

class QVBoxLayout;

#include <array>

class QLabel;
class QPushButton;
class Clicker;

QString gachaEffectText(const GachaCard &card);
QString gachaEffectText(const GachaCard &card, int ownedCount);

class GachaDialog : public QDialog {
    Q_OBJECT

public:
    explicit GachaDialog(Clicker *clicker, QWidget *parent = nullptr);

    void setArchCount(int arches);
    void setInventory(const std::array<int, GachaCardCount> &cardCounts,
                      const std::array<int, GachaCardCount> &cardUpgradeLevel,
                      const std::array<int, GachaCardCount> &cardUpgradePath,
                      int selectedCard, int selectedCard2);
    void setSecondCardSlotEnabled(bool unlocked);
    void setSecondCardPenaltyUpgraded(bool upgraded);
    void setActiveSlot(int slot);
    void showCard(const GachaCard &card, int ownedCount);
    void showMessage(const QString &message);

signals:
    void rollRequested();
    void cardSelected(int index);
    void cardSelectedForSlot2(int index);
    void slotChanged(int slot);
    void upgradeRequested(int index);
    void upgradePathRequested(int index, int path);

private:
    void updateCardButton(int index, int ownedCount, bool selected, bool selected2);

    Clicker *clicker;
    QLabel *archLabel = nullptr;
    QLabel *collectedLabel = nullptr;
    QLabel *cardLabel = nullptr;
    QLabel *messageLabel = nullptr;
    std::array<QPushButton *, GachaCardCount> cardButtons = {};
    std::array<QPushButton *, GachaCardCount> upgradeButtons = {};
    std::array<int, GachaCardCount> cardUpgradeLevels_ = {};
    std::array<int, GachaCardCount> cardUpgradePaths_ = {};
    std::array<int, GachaCardCount> cardOwned_ = {};
    QPushButton *rollButton = nullptr;
    QPushButton *slot1Button = nullptr;
    QPushButton *slot2Button = nullptr;
    QTabWidget *cardTabs = nullptr;
    int activeSlot = 1;
    bool penaltyUpgraded = false;
};
