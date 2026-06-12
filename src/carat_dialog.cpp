#include "carat_dialog.h"

#include "clicker.h"
#include "game_rules.h"
#include "utils.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QUrl>
#include <QVBoxLayout>

CaratDialog::CaratDialog(Clicker *parentClicker)
    : QDialog(parentClicker), clicker(parentClicker)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Carat");
    setWindowIcon(QIcon(":/assets/ui/carat.png"));
    resize(340, 380);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *balanceBox = new QFrame(this);
    balanceBox->setFrameShape(QFrame::StyledPanel);

    auto *balanceLayout = new QHBoxLayout(balanceBox);
    balanceLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    balanceLayout->setSpacing(DialogSpacing);

    auto *caratIcon = new QLabel(balanceBox);
    caratIcon->setPixmap(QIcon(":/assets/ui/carat.png").pixmap(32, 32));

    caratBalanceLabel = new QLabel(balanceBox);
    setWidgetFont(caratBalanceLabel, 14, true);

    clickBalanceLabel = new QLabel(balanceBox);
    clickBalanceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    balanceLayout->addWidget(caratIcon);
    balanceLayout->addWidget(caratBalanceLabel);
    balanceLayout->addStretch();
    balanceLayout->addWidget(clickBalanceLabel);

    burnButton = new QPushButton(this);
    burnButton->setIcon(QIcon(":/assets/ui/carat.png"));
    burnButton->setIconSize(CaratIconSize);

    burnX10Button = new QPushButton(this);
    burnX10Button->setIcon(QIcon(":/assets/ui/carat.png"));
    burnX10Button->setIconSize(CaratIconSize);

    auto *burnRow = new QHBoxLayout();
    burnRow->setContentsMargins(0, 0, 0, 0);
    burnRow->setSpacing(DialogSpacing);
    burnRow->addWidget(burnButton, 1);
    burnRow->addWidget(burnX10Button);

    auto *buffBox = new QFrame(this);
    buffBox->setFrameShape(QFrame::StyledPanel);

    auto *buffLayout = new QVBoxLayout(buffBox);
    buffLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    buffLayout->setSpacing(DialogSpacing);

    for (int index = 0; index < TimedBuffCount; ++index) {
        const auto &rule = TimedBuffRules[index];

        auto *buffTitle = new QLabel(rule.name, buffBox);
        setWidgetFont(buffTitle, buffTitle->font().pointSize(), true);

        buffStatusLabels[index] = new QLabel(buffBox);
        buffStatusLabels[index]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        buyBuffButtons[index] = new QPushButton(buffBox);
        buyBuffX10Buttons[index] = new QPushButton(buffBox);

        auto *buffHeaderRow = new QHBoxLayout();
        buffHeaderRow->setContentsMargins(0, 0, 0, 0);
        buffHeaderRow->setSpacing(DialogSpacing);
        buffHeaderRow->addWidget(buffTitle);
        buffHeaderRow->addWidget(buffStatusLabels[index], 1);

        auto *buffButtonRow = new QHBoxLayout();
        buffButtonRow->setContentsMargins(0, 0, 0, 0);
        buffButtonRow->setSpacing(DialogSpacing);
        buffButtonRow->addWidget(buyBuffButtons[index], 1);
        buffButtonRow->addWidget(buyBuffX10Buttons[index]);

        buffLayout->addLayout(buffHeaderRow);
        buffLayout->addLayout(buffButtonRow);
    }

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    slot2BuyButton = new QPushButton(this);
    penaltyUpgradeButton = new QPushButton(this);

    connect(burnButton, &QPushButton::clicked, this, [this]() {
        if (clicker->game.score < CaratBurnCost)
            return;
        clicker->game.score -= CaratBurnCost;
        clicker->game.carats += CaratBurnReward;
        if (burnSound) burnSound->play();
        clicker->saveGame();
        clicker->refreshUi();
        updateUi();
    });

    connect(burnX10Button, &QPushButton::clicked, this, [this]() {
        constexpr qint64 cost = CaratBurnCost * 10;
        constexpr qint64 reward = CaratBurnReward * 10;
        if (clicker->game.score < cost)
            return;
        clicker->game.score -= cost;
        clicker->game.carats += reward;
        if (burnSound) burnSound->play();
        clicker->saveGame();
        clicker->refreshUi();
        updateUi();
    });

    connect(slot2BuyButton, &QPushButton::clicked, this, [this]() {
        if (clicker->game.carats < SecondCardSlotCost || clicker->game.secondCardSlotUnlocked)
            return;
        clicker->game.carats -= SecondCardSlotCost;
        clicker->game.secondCardSlotUnlocked = true;
        if (unlockSound) unlockSound->play();
        clicker->saveGame();
        clicker->refreshUi();
        updateUi();
    });

    connect(penaltyUpgradeButton, &QPushButton::clicked, this, [this]() {
        if (clicker->game.carats < PenaltyUpgradeCost || clicker->game.secondCardPenaltyUpgraded)
            return;
        clicker->game.carats -= PenaltyUpgradeCost;
        clicker->game.secondCardPenaltyUpgraded = true;
        if (unlockSound) unlockSound->play();
        clicker->saveGame();
        clicker->refreshUi();
        updateUi();
    });

    for (int index = 0; index < TimedBuffCount; ++index) {
        const auto rule = TimedBuffRules[index];
        connect(buyBuffButtons[index], &QPushButton::clicked, this, [this, rule]() {
            if (clicker->game.carats < rule.caratCost)
                return;
            clicker->game.carats -= rule.caratCost;
            clicker->activateTimedBuff(rule.buff, rule.durationSeconds);
            if (buffSound) buffSound->play();
            clicker->saveGame();
            clicker->refreshUi();
            updateUi();
        });
        connect(buyBuffX10Buttons[index], &QPushButton::clicked, this, [this, rule]() {
            const qint64 cost = rule.caratCost * 10;
            const int duration = rule.durationSeconds * 10;
            if (clicker->game.carats < cost)
                return;
            clicker->game.carats -= cost;
            clicker->activateTimedBuff(rule.buff, duration);
            if (buffSound) buffSound->play();
            clicker->saveGame();
            clicker->refreshUi();
            updateUi();
        });
    }

    layout->addWidget(balanceBox);
    layout->addLayout(burnRow);
    layout->addWidget(buffBox);
    layout->addWidget(slot2BuyButton);
    layout->addWidget(penaltyUpgradeButton);

    clickMultButton = new QPushButton(this);
    connect(clickMultButton, &QPushButton::clicked, this, [this]() {
        if (clicker->game.clickMultLevel >= ClickMultMaxLevel) return;
        const qint64 cost = permanentMultCost(clicker->game.clickMultLevel, ClickMultBaseCost);
        if (clicker->game.carats < cost) return;
        clicker->game.carats -= cost;
        ++clicker->game.clickMultLevel;
        if (unlockSound) unlockSound->play();
        clicker->saveGame();
        clicker->refreshUi();
        updateUi();
    });
    layout->addWidget(clickMultButton);

    incomeMultButton = new QPushButton(this);
    connect(incomeMultButton, &QPushButton::clicked, this, [this]() {
        if (clicker->game.incomeMultLevel >= IncomeMultMaxLevel) return;
        const qint64 cost = permanentMultCost(clicker->game.incomeMultLevel, IncomeMultBaseCost);
        if (clicker->game.carats < cost) return;
        clicker->game.carats -= cost;
        ++clicker->game.incomeMultLevel;
        if (unlockSound) unlockSound->play();
        clicker->saveGame();
        clicker->refreshUi();
        updateUi();
    });
    layout->addWidget(incomeMultButton);

    layout->addStretch();
    layout->addWidget(closeButton);

    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &CaratDialog::updateUi);
    refreshTimer->start();

    initSounds();
    updateUi();
}

void CaratDialog::initSounds() {
    burnSound = new QSoundEffect(this);
    burnSound->setSource(QUrl("qrc:/assets/sound/burn.wav"));
    burnSound->setVolume(clicker->masterVolume());

    buffSound = new QSoundEffect(this);
    buffSound->setSource(QUrl("qrc:/assets/sound/buff.wav"));
    buffSound->setVolume(clicker->masterVolume());

    unlockSound = new QSoundEffect(this);
    unlockSound->setSource(QUrl("qrc:/assets/sound/unlock.wav"));
    unlockSound->setVolume(clicker->masterVolume());
}

void CaratDialog::updateUi() {
    caratBalanceLabel->setText(QString("Carat %1").arg(clicker->formatNumber(clicker->game.carats)));
    clickBalanceLabel->setText(QString("%1 clicks").arg(clicker->formatNumber(clicker->game.score)));

    burnButton->setText(QString("Burn %1 clicks  +%2 Carat")
                            .arg(clicker->formatNumber(CaratBurnCost))
                            .arg(clicker->formatNumber(CaratBurnReward)));
    burnButton->setEnabled(clicker->game.score >= CaratBurnCost);

    constexpr qint64 burnX10Cost = CaratBurnCost * 10;
    constexpr qint64 burnX10Reward = CaratBurnReward * 10;
    burnX10Button->setText(QString("x10  +%1").arg(clicker->formatNumber(burnX10Reward)));
    burnX10Button->setEnabled(clicker->game.score >= burnX10Cost);
    burnX10Button->setToolTip(QString("Burn %1 clicks for %2 Carat")
                                  .arg(clicker->formatNumber(burnX10Cost))
                                  .arg(clicker->formatNumber(burnX10Reward)));

    for (int index = 0; index < TimedBuffCount; ++index) {
        const auto &rule = TimedBuffRules[index];
        const int secondsLeft = clicker->timedBuffSecondsLeft(rule.buff);

        buffStatusLabels[index]->setText(secondsLeft > 0
            ? QString("Active: %1s left").arg(secondsLeft)
            : QString("Inactive"));
        buyBuffButtons[index]->setText(QString("%1 for %2s  %3 Carat")
                                           .arg(rule.name)
                                           .arg(rule.durationSeconds)
                                           .arg(clicker->formatNumber(rule.caratCost)));
        buyBuffButtons[index]->setEnabled(clicker->game.carats >= rule.caratCost);

        const qint64 x10Cost = rule.caratCost * 10;
        const int x10Duration = rule.durationSeconds * 10;
        buyBuffX10Buttons[index]->setText(QString("x10  +%1").arg(clicker->formatNumber(x10Cost)));
        buyBuffX10Buttons[index]->setEnabled(clicker->game.carats >= x10Cost);
        buyBuffX10Buttons[index]->setToolTip(QString("%1 for %2s  %3 Carat")
                                                 .arg(rule.name)
                                                 .arg(x10Duration)
                                                 .arg(clicker->formatNumber(x10Cost)));
    }

    if (clicker->game.secondCardSlotUnlocked) {
        slot2BuyButton->setText("Second card slot: unlocked");
        slot2BuyButton->setEnabled(false);
    } else {
        slot2BuyButton->setText(QString("Unlock second card slot  %1 Carat")
                                    .arg(clicker->formatNumber(SecondCardSlotCost)));
        slot2BuyButton->setEnabled(clicker->game.carats >= SecondCardSlotCost);
    }

    if (clicker->game.secondCardSlotUnlocked) {
        if (clicker->game.secondCardPenaltyUpgraded) {
            penaltyUpgradeButton->setText("Penalty reduced to 33%");
            penaltyUpgradeButton->setEnabled(false);
        } else {
            penaltyUpgradeButton->setText(clicker->compat(QString("Reduce penalty 55% \u2192 33%  %1 Carat")
                                              .arg(clicker->formatNumber(PenaltyUpgradeCost))));
            penaltyUpgradeButton->setEnabled(clicker->game.carats >= PenaltyUpgradeCost);
        }
        penaltyUpgradeButton->setVisible(true);
    } else {
        penaltyUpgradeButton->setVisible(false);
    }

    {
        const int lvl = clicker->game.clickMultLevel;
        if (lvl >= ClickMultMaxLevel) {
            clickMultButton->setText(clicker->compat(QString("Click multiplier: \u00D7%1 (max)")).arg(1 << lvl));
            clickMultButton->setEnabled(false);
        } else {
            const qint64 cost = permanentMultCost(lvl, ClickMultBaseCost);
            clickMultButton->setText(clicker->compat(QString("Click multiplier: \u00D7%1 \u2192 \u00D7%2  %3 Carat"))
                                         .arg(1 << lvl)
                                         .arg(1 << (lvl + 1))
                                         .arg(clicker->formatNumber(cost)));
            clickMultButton->setEnabled(clicker->game.carats >= cost);
        }
    }

    {
        const int lvl = clicker->game.incomeMultLevel;
        if (lvl >= IncomeMultMaxLevel) {
            incomeMultButton->setText(clicker->compat(QString("Income multiplier: \u00D7%1 (max)")).arg(1 << lvl));
            incomeMultButton->setEnabled(false);
        } else {
            const qint64 cost = permanentMultCost(lvl, IncomeMultBaseCost);
            incomeMultButton->setText(clicker->compat(QString("Income multiplier: \u00D7%1 \u2192 \u00D7%2  %3 Carat"))
                                           .arg(1 << lvl)
                                           .arg(1 << (lvl + 1))
                                           .arg(clicker->formatNumber(cost)));
            incomeMultButton->setEnabled(clicker->game.carats >= cost);
        }
    }
}
