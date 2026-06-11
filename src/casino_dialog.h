#pragma once

#include <QColor>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVector>

class Clicker;

struct PayLine {
    int cols[5];   // row index per column (0=top, 1=mid, 2=bot)
    QColor color;
};

class CasinoDialog : public QDialog {
    Q_OBJECT

public:
    explicit CasinoDialog(Clicker *clicker, QWidget *parent = nullptr);

private:
    void updateUi();
    void spin();
    void stopReel(int reel);
    void forceWin();
    void evaluateAndShow();

    Clicker *clicker;

    QLabel *balanceLabel = nullptr;
    QLabel *betLabel = nullptr;
    QLabel *resultLabel = nullptr;
    QPushButton *spinButton = nullptr;
    QPushButton *betUpButton = nullptr;
    QPushButton *betDownButton = nullptr;
    QPushButton *maxBetButton = nullptr;

    QLabel *gridLabels[5][3] = {};

    static constexpr int ReelSize = 12;
    static constexpr int VisibleRows = 3;
    static constexpr int Cols = 5;

    int reels[Cols][ReelSize] = {};
    int grid[Cols][VisibleRows] = {};

    QTimer *spinTimer = nullptr;
    int stoppingReel = -1;
    int stopTick = 0;
    qint64 betAmount = 100;
    bool spinning = false;

    QVector<PayLine> payLines;
    QVector<int> winningLines;
};
