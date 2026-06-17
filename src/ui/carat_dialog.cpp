// SPDX-License-Identifier: GPL-2.0-or-later
#include "carat_dialog.h"

#include "clicker.h"
#include "core/game_rules.h"
#include "core/utils.h"

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

    auto *multRow = new QHBoxLayout();
    multRow->setContentsMargins(0, 0, 0, 0);
    multRow->setSpacing(0);

    mult1Btn = new QPushButton("x1", this);
    mult10Btn = new QPushButton("x10", this);
    mult100Btn = new QPushButton("x100", this);

    auto *multGroup = new QButtonGroup(this);
    multGroup->setExclusive(true);
    multGroup->addButton(mult1Btn, 1);
    multGroup->addButton(mult10Btn, 10);
    multGroup->addButton(mult100Btn, 100);

    const auto styleMultBtn = [](QPushButton *b, bool first, bool last) {
        b->setCheckable(true);
        b->setFixedHeight(28);
        QString radius;
        if (first && last)
            radius = QStringLiteral("border-radius: 4px;");
        else if (first)
            radius = QStringLiteral("border-top-left-radius: 4px; border-bottom-left-radius: 4px; border-top-right-radius: 0; border-bottom-right-radius: 0;");
        else if (last)
            radius = QStringLiteral("border-top-right-radius: 4px; border-bottom-right-radius: 4px; border-top-left-radius: 0; border-bottom-left-radius: 0;");
        else
            radius = QStringLiteral("border-radius: 0;");
        b->setStyleSheet(QString(
            "QPushButton {"
            "  %1"
            "  background: palette(button);"
            "  color: palette(button-text);"
            "  border: 1px solid palette(mid);"
            "  padding: 2px 12px;"
            "}"
            "QPushButton:checked {"
            "  background: palette(highlight);"
            "  color: palette(highlighted-text);"
            "  border-color: palette(highlight);"
            "}"
        ).arg(radius));
    };
    styleMultBtn(mult1Btn, true, false);
    styleMultBtn(mult10Btn, false, false);
    styleMultBtn(mult100Btn, false, true);

    mult1Btn->setChecked(true);

    connect(multGroup, &QButtonGroup::idClicked, this, [this](int id) {
        setPurchaseMult(id);
    });

    multRow->addWidget(mult1Btn);
    multRow->addWidget(mult10Btn);
    multRow->addWidget(mult100Btn);

    burnButton = new QPushButton(this);
    burnButton->setIcon(QIcon(":/assets/ui/carat.png"));
    burnButton->setIconSize(CaratIconSize);

    connect(burnButton, &QPushButton::clicked, this, [this]() {
        const qint64 cost = CaratBurnCost * purchaseMult;
        const qint64 reward = CaratBurnReward * purchaseMult;
        if (clicker->game.score < cost)
            return;
        clicker->game.score -= cost;
        clicker->game.carats += reward;
        if (burnSound) burnSound->play();
        clicker->saveGame();
        clicker->refreshUi();
        updateUi();
    });

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

        auto *buffHeaderRow = new QHBoxLayout();
        buffHeaderRow->setContentsMargins(0, 0, 0, 0);
        buffHeaderRow->setSpacing(DialogSpacing);
        buffHeaderRow->addWidget(buffTitle);
        buffHeaderRow->addWidget(buffStatusLabels[index], 1);

        buffLayout->addLayout(buffHeaderRow);
        buffLayout->addWidget(buyBuffButtons[index]);
    }

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    slot2BuyButton = new QPushButton(this);
    penaltyUpgradeButton = new QPushButton(this);

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
            const qint64 cost = rule.caratCost * purchaseMult;
            const int duration = rule.durationSeconds * purchaseMult;
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
    layout->addLayout(multRow);
    layout->addWidget(burnButton);
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

void CaratDialog::setPurchaseMult(int mult) {
    purchaseMult = mult;
    updateUi();
}

void CaratDialog::updateUi() {
    caratBalanceLabel->setText(QString("Carat %1").arg(clicker->formatNumber(clicker->game.carats)));
    clickBalanceLabel->setText(QString("%1 clicks").arg(clicker->formatNumber(clicker->game.score)));

    const qint64 burnCost = CaratBurnCost * purchaseMult;
    const qint64 burnReward = CaratBurnReward * purchaseMult;
    burnButton->setText(QString("Burn %1 clicks  +%2 Carat")
                            .arg(clicker->formatNumber(burnCost))
                            .arg(clicker->formatNumber(burnReward)));
    burnButton->setEnabled(clicker->game.score >= burnCost);

    for (int index = 0; index < TimedBuffCount; ++index) {
        const auto &rule = TimedBuffRules[index];
        const int secondsLeft = clicker->timedBuffSecondsLeft(rule.buff);

        auto fmtTime = [](int s) -> QString {
            if (s <= 0) return QStringLiteral("0s");
            const int h = s / 3600, m = (s % 3600) / 60, sec = s % 60;
            if (h > 0) return QString("%1h %2m %3s").arg(h).arg(m).arg(sec);
            if (m > 0) return sec > 0 ? QString("%1m %2s").arg(m).arg(sec) : QString("%1m").arg(m);
            return QString("%1s").arg(sec);
        };

        buffStatusLabels[index]->setText(secondsLeft > 0
            ? QString("Active: %1 left").arg(fmtTime(secondsLeft))
            : QString("Inactive"));

        const int duration = rule.durationSeconds * purchaseMult;
        const qint64 cost = rule.caratCost * purchaseMult;
        buyBuffButtons[index]->setText(QString("%1 for %2  %3 Carat")
                                           .arg(rule.name)
                                           .arg(fmtTime(duration))
                                           .arg(clicker->formatNumber(cost)));
        buyBuffButtons[index]->setEnabled(clicker->game.carats >= cost);
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
