#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
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

    Clicker *clicker;
    QTimer *refreshTimer = nullptr;
    QSoundEffect *burnSound = nullptr;
    QSoundEffect *buffSound = nullptr;
    QSoundEffect *unlockSound = nullptr;
    QLabel *caratBalanceLabel = nullptr;
    QLabel *clickBalanceLabel = nullptr;
    QPushButton *burnButton = nullptr;
    QPushButton *burnX10Button = nullptr;
    std::array<QLabel *, 2> buffStatusLabels = {};
    std::array<QPushButton *, 2> buyBuffButtons = {};
    std::array<QPushButton *, 2> buyBuffX10Buttons = {};
    QPushButton *slot2BuyButton = nullptr;
    QPushButton *penaltyUpgradeButton = nullptr;
};
