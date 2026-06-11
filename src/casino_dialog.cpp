#include "casino_dialog.h"

#include "clicker.h"
#include "utils.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTimer>
#include <QVBoxLayout>

namespace {

constexpr int SymbolCount = 7;
const QString Symbols[SymbolCount] = {
    QStringLiteral("\u2605"),   // ★  jackpot
    QStringLiteral("\u2665"),   // ♥
    QStringLiteral("\u2666"),   // ♦
    QStringLiteral("\u2663"),   // ♣
    QStringLiteral("\u2660"),   // ♠
    QStringLiteral("\u25C6"),   // ◆
    QStringLiteral("\u25CF"),   // ●
};
constexpr int WildIndex = 0;   // ★ is wild

const int Payouts3[SymbolCount] = {5, 3, 2, 2, 1, 1, 1};
const int Payouts4[SymbolCount] = {25, 10, 8, 6, 5, 4, 3};
const int Payouts5[SymbolCount] = {100, 40, 30, 20, 15, 10, 8};

constexpr qint64 MinBet = 10;
constexpr qint64 BetStep = 10;

QString formatScore(qint64 v) {
    if (v < 10000)
        return QString::number(v);
    double d = static_cast<double>(v);
    int s = 0;
    while (d >= 1000.0 && s < 3) { d /= 1000.0; ++s; }
    const QChar sfx[4] = {QChar(0), 'K', 'M', 'B'};
    return QString("%1%2").arg(d, 0, 'f', 1).arg(sfx[s]);
}

} // namespace

CasinoDialog::CasinoDialog(Clicker *parentClicker)
    : QDialog(parentClicker), clicker(parentClicker)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QStringLiteral("\u8CED\u3051"));
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    resize(340, 430);

    payLines = {
        {{0,0,0,0,0}, QColor("#f9a825")},   // top
        {{1,1,1,1,1}, QColor("#ef6c00")},   // middle
        {{2,2,2,2,2}, QColor("#6a1b9a")},   // bottom
        {{0,1,2,1,0}, QColor("#2e7d32")},   // V
        {{2,1,0,1,2}, QColor("#1565c0")},   // ^
    };

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel(QStringLiteral("\u8CED\u3051 \u30DE\u30B7\u30FC\u30F3"), this);
    setWidgetFont(title, 14, true);
    title->setAlignment(Qt::AlignCenter);

    balanceLabel = new QLabel(this);
    balanceLabel->setAlignment(Qt::AlignCenter);
    setWidgetFont(balanceLabel, balanceLabel->font().pointSize(), true);

    auto *gridBox = new QFrame(this);
    gridBox->setFrameShape(QFrame::StyledPanel);
    gridBox->setStyleSheet("QFrame { background: palette(dark); border-radius: 6px; }");
    auto *gridLayout = new QGridLayout(gridBox);
    gridLayout->setSpacing(3);
    gridLayout->setContentsMargins(6, 6, 6, 6);

    for (int col = 0; col < Cols; ++col) {
        for (int row = 0; row < VisibleRows; ++row) {
            auto *cell = new QLabel(Symbols[0], gridBox);
            cell->setAlignment(Qt::AlignCenter);
            cell->setFixedSize(52, 52);
            auto f = cell->font();
            f.setPointSize(22);
            cell->setFont(f);
            cell->setStyleSheet("QLabel { background: palette(base); border-radius: 6px; }");
            gridLabels[col][row] = cell;
            gridLayout->addWidget(cell, row, col);
        }
    }

    resultLabel = new QLabel(this);
    resultLabel->setAlignment(Qt::AlignCenter);
    setWidgetFont(resultLabel, resultLabel->font().pointSize(), true);
    resultLabel->setMinimumHeight(24);

    auto *betRow = new QHBoxLayout();
    betRow->setSpacing(DialogSpacing);

    betDownButton = new QPushButton("-", this);
    betDownButton->setFixedWidth(36);
    betUpButton = new QPushButton("+", this);
    betUpButton->setFixedWidth(36);
    maxBetButton = new QPushButton("Max", this);
    maxBetButton->setFixedWidth(50);
    betLabel = new QLabel(this);
    betLabel->setAlignment(Qt::AlignCenter);
    setWidgetFont(betLabel, betLabel->font().pointSize(), true);

    betRow->addWidget(betDownButton);
    betRow->addWidget(betLabel, 1);
    betRow->addWidget(betUpButton);
    betRow->addWidget(maxBetButton);

    spinButton = new QPushButton(QStringLiteral("\u30B9\u30D4\u30F3"), this);
    spinButton->setMinimumHeight(40);
    auto sf = spinButton->font();
    sf.setPointSize(16);
    sf.setBold(true);
    spinButton->setFont(sf);

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    connect(betDownButton, &QPushButton::clicked, this, [this]() {
        if (spinning) return;
        betAmount = qMax(MinBet, betAmount - BetStep);
        updateUi();
    });
    connect(betUpButton, &QPushButton::clicked, this, [this]() {
        if (spinning) return;
        betAmount += BetStep;
        updateUi();
    });
    connect(maxBetButton, &QPushButton::clicked, this, [this]() {
        if (spinning) return;
        betAmount = qMax(MinBet, clicker->game.score);
        updateUi();
    });
    connect(spinButton, &QPushButton::clicked, this, &CasinoDialog::spin);

    spinTimer = new QTimer(this);
    spinTimer->setInterval(70);
    connect(spinTimer, &QTimer::timeout, this, [this]() {
        for (int col = 0; col < Cols; ++col) {
            if (col < stoppingReel) continue;
            for (int r = ReelSize - 1; r > 0; --r)
                reels[col][r] = reels[col][r - 1];
            reels[col][0] = QRandomGenerator::global()->bounded(SymbolCount);
        }
        ++stopTick;
        if (stoppingReel >= 0 && stopTick >= stoppingReel * 4 + 4) {
            stopReel(stoppingReel);
        }
        updateUi();
    });

    auto *rg = QRandomGenerator::global();
    for (int col = 0; col < Cols; ++col)
        for (int r = 0; r < ReelSize; ++r)
            reels[col][r] = rg->bounded(SymbolCount);

    updateUi();
}

void CasinoDialog::updateUi() {
    balanceLabel->setText(QStringLiteral("Balance: %1").arg(formatScore(clicker->game.score)));
    betLabel->setText(QStringLiteral("Bet: %1").arg(formatScore(betAmount)));

    for (int col = 0; col < Cols; ++col)
        for (int row = 0; row < VisibleRows; ++row)
            gridLabels[col][row]->setText(Symbols[reels[col][row]]);

    const bool canSpin = !spinning && clicker->game.score >= betAmount;
    spinButton->setEnabled(canSpin);
    betDownButton->setEnabled(!spinning);
    betUpButton->setEnabled(!spinning);
    maxBetButton->setEnabled(!spinning);

    if (spinning)
        spinButton->setText(QStringLiteral("..."));
    else
        spinButton->setText(QStringLiteral("\u30B9\u30D4\u30F3"));
}

void CasinoDialog::spin() {
    if (spinning || clicker->game.score < betAmount)
        return;

    clicker->game.score -= betAmount;
    spinning = true;
    stoppingReel = -1;
    stopTick = 0;
    winningLines.clear();
    resultLabel->setText(QString());

    for (int col = 0; col < Cols; ++col)
        for (int row = 0; row < VisibleRows; ++row)
            gridLabels[col][row]->setStyleSheet("QLabel { background: palette(base); border-radius: 6px; }");

    updateUi();
    spinTimer->start();
}

void CasinoDialog::stopReel(int reel) {
    if (reel == Cols) {
        spinTimer->stop();
        spinning = false;
        evaluateAndShow();
        clicker->saveGame();
        clicker->refreshUi();
        updateUi();
        return;
    }
    ++stoppingReel;
    stopTick = 0;
}

void CasinoDialog::evaluateAndShow() {
    // Build grid from reel top 3 rows
    for (int col = 0; col < Cols; ++col)
        for (int row = 0; row < VisibleRows; ++row)
            grid[col][row] = reels[col][row];

    qint64 totalWin = 0;
    const int w = WildIndex;

    for (int li = 0; li < payLines.size(); ++li) {
        const auto &line = payLines[li];

        // Find the matching symbol (skip wild, wilds count as that symbol)
        int sym = -1;
        bool allMatch = true;
        for (int c = 0; c < Cols; ++c) {
            const int s = grid[c][line.cols[c]];
            if (s != w) {
                if (sym < 0) sym = s;
                else if (sym != s) { allMatch = false; break; }
            }
        }
        if (!allMatch || sym < 0) continue;

        // Count how many match (including wilds)
        int count = 0;
        for (int c = 0; c < Cols; ++c) {
            const int s = grid[c][line.cols[c]];
            if (s == sym || s == w) ++count;
            else break;
        }

        int payout = 0;
        if (count == 5) payout = Payouts5[sym];
        else if (count == 4) payout = Payouts4[sym];
        else if (count == 3) payout = Payouts3[sym];

        if (payout > 0) {
            totalWin += betAmount * payout;
            winningLines.append(li);
        }
    }

    // Highlight winning cells
    for (int li : winningLines) {
        const auto &line = payLines[li];
        for (int c = 0; c < Cols; ++c) {
            auto *cell = gridLabels[c][line.cols[c]];
            cell->setStyleSheet(QString("QLabel { background: %1; border-radius: 6px; color: white; font-weight: bold; }")
                                    .arg(line.color.name()));
        }
    }

    if (totalWin > 0) {
        clicker->game.score += totalWin;
        resultLabel->setText(QStringLiteral("\u2728 Won %1 \u2728").arg(formatScore(totalWin)));
        resultLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(payLines[winningLines.first()].color.name()));
    } else {
        resultLabel->setText(QStringLiteral("No luck..."));
        resultLabel->setStyleSheet("");
    }
}
