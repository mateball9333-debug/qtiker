#include "casino_dialog.h"

#include "clicker.h"
#include "utils.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTimer>
#include <QVBoxLayout>

namespace {

constexpr int SymbolCount = 8;
const QString Symbols[SymbolCount] = {
    QStringLiteral("\u2605"),   // ★
    QStringLiteral("\u2665"),   // ♥
    QStringLiteral("\u2666"),   // ♦
    QStringLiteral("\u2663"),   // ♣
    QStringLiteral("\u2660"),   // ♠
    QStringLiteral("\u25C6"),   // ◆
    QStringLiteral("\u2606"),   // ☆
    QStringLiteral("\u25CF"),   // ●
};

constexpr int Payouts[SymbolCount] = {50, 10, 8, 6, 5, 4, 3, 2};

constexpr int SpinTicksPerReel = 12;
constexpr int SpinTickMs = 60;
constexpr qint64 MinBet = 10;
constexpr qint64 BetStep = 10;

QString formatScore(qint64 v) {
    if (v < 10000)
        return QString::number(v);
    double d = static_cast<double>(v);
    int suffix = 0;
    while (d >= 1000.0 && suffix < 3) {
        d /= 1000.0;
        ++suffix;
    }
    const QChar sfx[4] = {QChar(0), 'K', 'M', 'B'};
    return QString("%1%2").arg(d, 0, 'f', 1).arg(sfx[suffix]);
}

} // namespace

CasinoDialog::CasinoDialog(Clicker *parentClicker)
    : QDialog(parentClicker), clicker(parentClicker)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QStringLiteral("\u8CED\u3051"));  // 賭け
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    resize(320, 330);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel(QStringLiteral("\u8CED\u3051 \u30DE\u30B7\u30FC\u30F3"), this);
    setWidgetFont(title, 14, true);
    title->setAlignment(Qt::AlignCenter);

    balanceLabel = new QLabel(this);
    balanceLabel->setAlignment(Qt::AlignCenter);
    setWidgetFont(balanceLabel, balanceLabel->font().pointSize(), true);

    auto *reelBox = new QFrame(this);
    reelBox->setFrameShape(QFrame::StyledPanel);
    auto *reelLayout = new QHBoxLayout(reelBox);
    reelLayout->setContentsMargins(PanelMargin, PanelMargin, PanelMargin, PanelMargin);
    reelLayout->setSpacing(12);

    for (int i = 0; i < 3; ++i) {
        reelLabels[i] = new QLabel(Symbols[0], reelBox);
        reelLabels[i]->setAlignment(Qt::AlignCenter);
        reelLabels[i]->setFixedSize(72, 72);
        auto f = reelLabels[i]->font();
        f.setPointSize(36);
        reelLabels[i]->setFont(f);
        reelLabels[i]->setFrameShape(QFrame::StyledPanel);
        reelLabels[i]->setStyleSheet("QLabel { background: palette(base); border-radius: 8px; }");
        reelLayout->addWidget(reelLabels[i]);
    }
    reelLayout->addStretch();

    resultLabel = new QLabel(this);
    resultLabel->setAlignment(Qt::AlignCenter);
    setWidgetFont(resultLabel, resultLabel->font().pointSize(), true);

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
        betAmount = clicker->game.score;
        updateUi();
    });
    connect(spinButton, &QPushButton::clicked, this, &CasinoDialog::spin);

    spinTimer = new QTimer(this);
    spinTimer->setInterval(SpinTickMs);

    layout->addWidget(title);
    layout->addWidget(balanceLabel);
    layout->addWidget(reelBox);
    layout->addWidget(resultLabel);
    layout->addLayout(betRow);
    layout->addWidget(spinButton);
    layout->addWidget(closeButton);

    for (int i = 0; i < 3; ++i)
        reelValues[i] = QRandomGenerator::global()->bounded(SymbolCount);

    updateUi();
}

void CasinoDialog::updateUi() {
    balanceLabel->setText(QStringLiteral("Balance: %1").arg(formatScore(clicker->game.score)));
    betLabel->setText(QStringLiteral("Bet: %1").arg(formatScore(betAmount)));

    for (int i = 0; i < 3; ++i)
        reelLabels[i]->setText(Symbols[reelValues[i]]);

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

    for (int i = 0; i < 3; ++i) {
        spinTicks[i] = SpinTicksPerReel + i * 6;
        reelValues[i] = QRandomGenerator::global()->bounded(SymbolCount);
    }

    int tick = 0;
    auto *ticker = new QTimer(this);
    ticker->setInterval(SpinTickMs);
    connect(ticker, &QTimer::timeout, this, [this, ticker, tick]() mutable {
        for (int i = 0; i < 3; ++i) {
            if (tick < spinTicks[i]) {
                reelValues[i] = QRandomGenerator::global()->bounded(SymbolCount);
                reelLabels[i]->setText(Symbols[reelValues[i]]);
            }
        }

        ++tick;
        if (tick > spinTicks[2] + 3) {
            ticker->stop();
            ticker->deleteLater();
            spinning = false;
            showResult();
            clicker->saveGame();
            clicker->refreshUi();
            updateUi();
        }
    });
    ticker->start();

    updateUi();
}

void CasinoDialog::showResult() {
    const int payout = evaluatePayout();
    if (payout > 0) {
        const qint64 win = betAmount * payout;
        clicker->game.score += win;
        resultLabel->setText(QStringLiteral("\u2728 Won %1! \u2728").arg(formatScore(win)));
        resultLabel->setStyleSheet("color: #f9a825; font-weight: bold;");
    } else {
        resultLabel->setText(QStringLiteral("No luck..."));
        resultLabel->setStyleSheet("");
    }
}

int CasinoDialog::evaluatePayout() {
    const int a = reelValues[0];
    const int b = reelValues[1];
    const int c = reelValues[2];

    if (a == b && b == c)
        return Payouts[a];

    if (a == b || b == c) {
        const int match = (a == b) ? a : b;
        return qMax(1, Payouts[match] / 5);
    }

    return 0;
}
