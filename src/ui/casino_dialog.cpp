// SPDX-License-Identifier: GPL-2.0-or-later
#include "casino_dialog.h"

#include "clicker.h"
#include "core/utils.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSoundEffect>
#include <QTimer>
#include <QVBoxLayout>

static QString winCellStyle(const QColor &bg) {
    const int bright = (bg.red() * 299 + bg.green() * 587 + bg.blue() * 114) / 1000;
    return QString("QLabel { background: %1; color: %2; font-weight: bold; }")
        .arg(bg.name(), bright > 128 ? QStringLiteral("black") : QStringLiteral("white"));
}

namespace {

constexpr int SymbolCount = 7;
const QString Symbols[SymbolCount] = {
    QStringLiteral("\u2605"), QStringLiteral("\u2665"), QStringLiteral("\u2666"),
    QStringLiteral("\u2663"), QStringLiteral("\u2660"), QStringLiteral("\u25C6"),
    QStringLiteral("\u25CF"),
};
constexpr int WildIndex = 0;
const int Payouts3[SymbolCount] = {1, 1, 1, 0, 0, 0, 0};
const int Payouts4[SymbolCount] = {5, 3, 1, 1, 0, 0, 0};
const int Payouts5[SymbolCount] = {25, 11, 7, 5, 4, 3, 2};
constexpr qint64 MinBet = 10;
constexpr qint64 BetStep = 10;

QString fmtScore(qint64 v) {
    if (v < 10000) return QString::number(v);
    double d = static_cast<double>(v); int s = 0;
    while (d >= 1000.0 && s < 3) { d /= 1000.0; ++s; }
    return QString("%1%2").arg(d,0,'f',1).arg(QChar::fromLatin1(" KMB"[s]));
}

}

CasinoDialog::CasinoDialog(Clicker *parentClicker, QWidget *parent)
    : QDialog(parent), clicker(parentClicker)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(clicker->compat(QStringLiteral("\u8CED\u3051")));
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    setFixedSize(320, 500);

    payLines = {
        {{0,0,0,0,0}, QColor("#f9a825")},
        {{1,1,1,1,1}, QColor("#ef6c00")},
        {{2,2,2,2,2}, QColor("#6a1b9a")},
        {{0,0,1,2,2}, QColor("#2e7d32")},
        {{2,2,1,0,0}, QColor("#1565c0")},
        {{1,0,0,0,1}, QColor("#c62828")},
        {{1,2,2,2,1}, QColor("#00838f")},
        {{0,1,2,1,0}, QColor("#6d4c41")},
    };

    reelStopSound = new QSoundEffect(this);
    reelStopSound->setSource(QUrl("qrc:/assets/sound/reel_stop.wav"));
    reelStopSound->setVolume(clicker->masterVolume());

    winSound = new QSoundEffect(this);
    winSound->setSource(QUrl("qrc:/assets/sound/win.wav"));
    winSound->setVolume(clicker->masterVolume());

    auto *L = new QVBoxLayout(this);
    L->setContentsMargins(12, 12, 12, 12);
    L->setSpacing(6);

    auto *title = new QLabel(clicker->compat(QStringLiteral("\u8CED\u3051 \u30DE\u30B7\u30FC\u30F3")), this);
    title->setAlignment(Qt::AlignCenter);
    title->setFixedHeight(24);
    L->addWidget(title);

    balanceLabel = new QLabel("Balance: 0", this);
    balanceLabel->setAlignment(Qt::AlignCenter);
    balanceLabel->setFixedHeight(20);
    L->addWidget(balanceLabel);

    auto *grid = new QFrame(this);
    grid->setFrameShape(QFrame::Box);
    auto *GL = new QVBoxLayout(grid);
    GL->setSpacing(2);
    GL->setContentsMargins(8, 8, 8, 8);

    for (int row = 0; row < 3; ++row) {
        auto *RL = new QHBoxLayout();
        RL->setSpacing(4);
        for (int col = 0; col < 5; ++col) {
            auto *c = new QLabel(Symbols[0], grid);
            c->setAlignment(Qt::AlignCenter);
            c->setFixedSize(46, 46);
            c->setFrameShape(QFrame::Box);
            c->setStyleSheet("QLabel { background: palette(base); }");
            c->setMouseTracking(true);
            c->installEventFilter(this);
            gridLabels[col][row] = c;
            RL->addWidget(c);
        }
        GL->addLayout(RL);
    }
    L->addWidget(grid);

    resultLabel = new QLabel(this);
    resultLabel->setAlignment(Qt::AlignCenter);
    resultLabel->setMinimumHeight(36);
    L->addWidget(resultLabel);

    auto *betRow = new QHBoxLayout();
    betRow->setSpacing(6);
    betDownButton = new QPushButton("-", this); betDownButton->setFixedWidth(36);
    betLabel = new QLabel("Bet: 100", this); betLabel->setAlignment(Qt::AlignCenter);
    betUpButton = new QPushButton("+", this); betUpButton->setFixedWidth(36);
    maxBetButton = new QPushButton("Max", this); maxBetButton->setFixedWidth(48);
    betRow->addWidget(betDownButton);
    betRow->addWidget(betLabel, 1);
    betRow->addWidget(betUpButton);
    betRow->addWidget(maxBetButton);
    L->addLayout(betRow);

    auto *presetRow = new QHBoxLayout();
    presetRow->setSpacing(4);
    for (qint64 v : {10, 50, 100, 500, 1000, 5000}) {
        auto *btn = new QPushButton(fmtScore(v), this);
        btn->setFixedHeight(26);
        connect(btn, &QPushButton::clicked, this, [this, v]() {
            if (spinning) return;
            betAmount = v;
            updateUi();
        });
        presetButtons.append(btn);
        presetRow->addWidget(btn);
    }
    L->addLayout(presetRow);

    spinButton = new QPushButton(clicker->compat(QStringLiteral("\u30B9\u30D4\u30F3")), this);
    spinButton->setMinimumHeight(36);
    L->addWidget(spinButton);

    auto *closeButton = new QPushButton("Close", this);
    L->addWidget(closeButton);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(spinButton, &QPushButton::clicked, this, [this]() {
        if (spinning) {
            // Skip to end
            spinTimer->stop();
            spinning = false;
            for (int col = stoppingReel; col < 5; ++col) {
                for (int r = 11; r > 0; --r)
                    reels[col][r] = reels[col][r - 1];
                reels[col][0] = QRandomGenerator::global()->bounded(SymbolCount);
            }
            evaluateAndShow();
            clicker->saveGame();
            clicker->refreshUi();
            updateUi();
        } else {
            spin();
        }
    });

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

    spinTimer = new QTimer(this);
    spinTimer->setInterval(50);
    connect(spinTimer, &QTimer::timeout, this, [this]() {
        for (int col = 0; col < 5; ++col) {
            if (col < stoppingReel) continue;
            for (int r = 11; r > 0; --r) reels[col][r] = reels[col][r-1];
            reels[col][0] = QRandomGenerator::global()->bounded(SymbolCount);
        }
        ++stopTick;
        if (stopTick >= stoppingReel * 3 + 4)
            stopReel(stoppingReel);
        updateUi();
    });

    auto *rg = QRandomGenerator::global();
    for (int col = 0; col < 5; ++col)
        for (int r = 0; r < 12; ++r)
            reels[col][r] = rg->bounded(SymbolCount);

    updateUi();
}

void CasinoDialog::updateUi() {
    balanceLabel->setText("Balance: " + fmtScore(clicker->game.score));
    betLabel->setText("Bet: " + fmtScore(betAmount));
    for (int col = 0; col < 5; ++col)
        for (int row = 0; row < 3; ++row)
            gridLabels[col][row]->setText(clicker->compat(Symbols[reels[col][row]]));
    bool can = !spinning && clicker->game.score >= betAmount;
    spinButton->setEnabled(can || spinning);
    betDownButton->setEnabled(!spinning);
    betUpButton->setEnabled(!spinning);
    maxBetButton->setEnabled(!spinning);
    for (auto *b : presetButtons)
        b->setEnabled(!spinning);
    spinButton->setText(spinning ? clicker->compat("Skip \u00BB") : clicker->compat(QStringLiteral("\u30B9\u30D4\u30F3")));
}

void CasinoDialog::spin() {
    if (spinning || clicker->game.score < betAmount) return;
    clicker->game.score -= betAmount;
    clicker->game.casinoTotalSpins += 1;
    spinning = true;
    stoppingReel = 0;
    stopTick = 0;
    winningLines.clear();
    winStarts.clear();
    winCounts.clear();
    resultLabel->setText(QString());
    for (int col = 0; col < 5; ++col)
        for (int row = 0; row < 3; ++row)
            gridLabels[col][row]->setStyleSheet("QLabel { background: palette(base); }");
    updateUi();
    spinTimer->start();
}

void CasinoDialog::stopReel(int reel) {
    if (reel >= 5) {
        spinTimer->stop();
        spinning = false;
        evaluateAndShow();
        clicker->saveGame();
        clicker->refreshUi();
        updateUi();
        return;
    }
    if (reelStopSound) reelStopSound->play();
    ++stoppingReel;
    stopTick = 0;
}

void CasinoDialog::forceWin() {
    auto *rg = QRandomGenerator::global();
    const int lineIdx = rg->bounded(payLines.size());
    const auto &line = payLines[lineIdx];
    const int sym = rg->bounded(1, SymbolCount);
    const int length = rg->bounded(3, 6);

    for (int c = 0; c < length; ++c) {
        reels[c][line.cols[c]] = sym;
        if (length >= 4 && c > 0 && rg->bounded(100) < 20)
            reels[c][line.cols[c]] = WildIndex;
    }
}

void CasinoDialog::evaluateAndShow() {
    for (int col = 0; col < 5; ++col)
        for (int row = 0; row < 3; ++row)
            grid[col][row] = reels[col][row];

    qint64 totalWin = 0;

    for (int li = 0; li < payLines.size(); ++li) {
        const auto &line = payLines[li];

        // Count consecutive matches from left (incl. wilds)
        int baseSym = -1;
        int count = 0;
        for (int c = 0; c < 5; ++c) {
            int s = grid[c][line.cols[c]];
            if (s == WildIndex) {
                ++count;
            } else if (baseSym < 0) {
                baseSym = s;
                ++count;
            } else if (s == baseSym) {
                ++count;
            } else {
                break;
            }
        }
        if (baseSym < 0) baseSym = WildIndex;

        if (count >= 3) {
            int payout = 0;
            if (count == 5) payout = Payouts5[baseSym];
            else if (count == 4) payout = Payouts4[baseSym];
            else payout = Payouts3[baseSym];
            if (payout > 0) {
                totalWin += betAmount * payout;
                winningLines.append(li);
                winStarts.append(0);
                winCounts.append(count);
            }
        }
    }

    // Highlight winning cells
    for (int wi = 0; wi < winningLines.size(); ++wi) {
        const auto &line = payLines[winningLines[wi]];
        const int start = winStarts[wi];
        const int count = winCounts[wi];
        for (int c = start; c < start + count; ++c)
            gridLabels[c][line.cols[c]]->setStyleSheet(winCellStyle(line.color));
    }

    if (totalWin > 0) {
        clicker->game.score += totalWin;
        clicker->game.casinoTotalWon += totalWin;
        resultLabel->setText(clicker->compat(QStringLiteral("\u2605 Won %1 \u2605")).arg(fmtScore(totalWin)));

        if (totalWin > betAmount * 2 && betAmount >= 5000
            && QRandomGenerator::global()->bounded(100) < 15) {
            clicker->game.arches += 1;
            clicker->game.totalArchesEarned += 1;
            clicker->game.archesFromCasino += 1;
            resultLabel->setText(resultLabel->text() + clicker->compat(QStringLiteral("\n+\u2605 1 Arch!")));
        }

        if (winSound) winSound->play();
    } else {
        resultLabel->setText("No luck...");
    }
}

void CasinoDialog::highlightLine(int lineIdx, bool on) {
    const int wi = winningLines.indexOf(lineIdx);
    if (wi < 0) return;
    const auto &line = payLines[lineIdx];
    const int start = winStarts[wi];
    const int count = winCounts[wi];
    for (int c = start; c < start + count; ++c) {
        auto *cell = gridLabels[c][line.cols[c]];
        if (on)
            cell->setStyleSheet(winCellStyle(line.color));
        else
            cell->setStyleSheet("QLabel { background: palette(button); font-weight: bold; }");
    }
}

bool CasinoDialog::eventFilter(QObject *watched, QEvent *event) {
    for (int col = 0; col < 5; ++col)
        for (int row = 0; row < 3; ++row)
            if (watched == gridLabels[col][row]) {
                if (event->type() == QEvent::Enter) {
                    // Dim all winning lines, highlight only those containing this cell
                    for (int li : winningLines)
                        highlightLine(li, false);

                    for (int li : winningLines) {
                        const auto &line = payLines[li];
                        if (line.cols[col] == row)
                            highlightLine(li, true);
                    }
                } else if (event->type() == QEvent::Leave) {
                    // Restore all winning lines
                    for (int li : winningLines)
                        highlightLine(li, true);
                }
                return false;
            }
    return QDialog::eventFilter(watched, event);
}
