// SPDX-License-Identifier: GPL-2.0-or-later
#include "statistics_dialog.h"

#include "clicker.h"
#include "core/utils.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

StatisticsDialog::StatisticsDialog(Clicker *parentClicker)
    : QDialog(parentClicker), clicker(parentClicker)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Qtiker Statistics");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    resize(340, 380);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel("Statistics", this);
    setWidgetFont(title, 13, true);

    auto *statsBox = new QFrame(this);
    statsBox->setFrameShape(QFrame::StyledPanel);

    auto *statsLayout = new QVBoxLayout(statsBox);
    statsLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    statsLayout->setSpacing(DialogSpacing);

    auto *totalClicksLabel = new QLabel(
        QString("Total clicks: %1").arg(clicker->formatNumber(clicker->game.totalClicks)),
        statsBox
    );
    auto *earnedScoreLabel = new QLabel(
        QString("Total score earned: %1").arg(clicker->formatNumber(clicker->game.totalScoreEarned)),
        statsBox
    );
    auto *playTimeLabel = new QLabel(
        QString("Total play time: %1").arg(clicker->formatDuration(clicker->currentTotalPlaySeconds())),
        statsBox
    );
    auto *archesLabel = new QLabel(
        QString("Total Arch's earned: %1").arg(clicker->formatNumber(clicker->game.totalArchesEarned)),
        statsBox
    );
    auto *archesSpentLabel = new QLabel(
        QString("Arch's spent on gacha: %1").arg(clicker->formatNumber(clicker->game.totalArchesSpent)),
        statsBox
    );

    statsLayout->addWidget(totalClicksLabel);
    statsLayout->addWidget(earnedScoreLabel);
    statsLayout->addWidget(playTimeLabel);
    statsLayout->addWidget(archesLabel);
    statsLayout->addWidget(archesSpentLabel);

    auto *uselessBox = new QFrame(this);
    uselessBox->setFrameShape(QFrame::StyledPanel);

    auto *uselessLayout = new QVBoxLayout(uselessBox);
    uselessLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    uselessLayout->setSpacing(DialogSpacing);

    auto *uselessTitle = new QLabel("Questionable statistic", uselessBox);
    setWidgetFont(uselessTitle, uselessTitle->font().pointSize(), true);

    auto *rightClicksLabel = new QLabel(
        QString("Right-clicks on Click button: %1").arg(clicker->formatNumber(clicker->game.clickButtonRightClicks)),
        uselessBox
    );

    uselessLayout->addWidget(uselessTitle);
    uselessLayout->addWidget(rightClicksLabel);

    auto *casinoBox = new QFrame(this);
    casinoBox->setFrameShape(QFrame::StyledPanel);

    auto *casinoLayout = new QVBoxLayout(casinoBox);
    casinoLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    casinoLayout->setSpacing(DialogSpacing);

    auto *casinoTitle = new QLabel("Casino", casinoBox);
    setWidgetFont(casinoTitle, casinoTitle->font().pointSize(), true);

    auto *casinoWonLabel = new QLabel(
        QString("Score won: %1").arg(clicker->formatNumber(clicker->game.casinoTotalWon)),
        casinoBox
    );
    auto *casinoSpinsLabel = new QLabel(
        QString("Spins: %1").arg(clicker->formatNumber(clicker->game.casinoTotalSpins)),
        casinoBox
    );
    auto *casinoArchesLabel = new QLabel(
        QString("Arch's from casino: %1").arg(clicker->formatNumber(clicker->game.archesFromCasino)),
        casinoBox
    );

    casinoLayout->addWidget(casinoTitle);
    casinoLayout->addWidget(casinoWonLabel);
    casinoLayout->addWidget(casinoSpinsLabel);
    casinoLayout->addWidget(casinoArchesLabel);

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    layout->addWidget(title);
    layout->addWidget(statsBox);
    layout->addWidget(casinoBox);
    layout->addWidget(uselessBox);
    layout->addStretch();
    layout->addWidget(closeButton);
}
