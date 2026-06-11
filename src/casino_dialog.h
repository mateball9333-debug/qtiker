#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

#include <array>

class Clicker;

class CasinoDialog : public QDialog {
    Q_OBJECT

public:
    explicit CasinoDialog(Clicker *clicker);

private:
    void updateUi();
    void spin();
    void tickReel(int reel);
    int evaluatePayout();
    void showResult();
    QString formatBet(qint64 amount) const;

    Clicker *clicker;

    QLabel *balanceLabel = nullptr;
    QLabel *betLabel = nullptr;
    QLabel *resultLabel = nullptr;
    std::array<QLabel *, 3> reelLabels = {};
    QPushButton *spinButton = nullptr;
    QPushButton *betUpButton = nullptr;
    QPushButton *betDownButton = nullptr;
    QPushButton *maxBetButton = nullptr;

    QTimer *spinTimer = nullptr;
    std::array<int, 3> reelValues = {0, 0, 0};
    std::array<int, 3> spinTicks = {0, 0, 0};
    qint64 betAmount = 100;
    bool spinning = false;
};
