// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QSoundEffect>
#include <QTimer>

#include <array>

class Clicker;

class CaratDialog : public QDialog {
    Q_OBJECT

public:
    explicit CaratDialog(Clicker *clicker);

private:
    void updateUi();
    void initSounds();
    void setPurchaseMult(int mult);

    Clicker *clicker;
    QTimer *refreshTimer = nullptr;
    QSoundEffect *burnSound = nullptr;
    QSoundEffect *buffSound = nullptr;
    QSoundEffect *unlockSound = nullptr;
    QLabel *caratBalanceLabel = nullptr;
    QLabel *clickBalanceLabel = nullptr;
    QPushButton *burnButton = nullptr;
    std::array<QLabel *, 2> buffStatusLabels = {};
    std::array<QPushButton *, 2> buyBuffButtons = {};
    QPushButton *slot2BuyButton = nullptr;
    QPushButton *penaltyUpgradeButton = nullptr;
    QPushButton *clickMultButton = nullptr;
    QPushButton *incomeMultButton = nullptr;
    QPushButton *mult1Btn = nullptr;
    QPushButton *mult10Btn = nullptr;
    QPushButton *mult100Btn = nullptr;
    int purchaseMult = 1;
};

