#pragma once

#include <QColor>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSoundEffect>
#include <QTimer>
#include <QVector>

class Clicker;

struct PayLine {
    int cols[5];
    QColor color;
};

class CasinoDialog : public QDialog {
    Q_OBJECT

public:
    explicit CasinoDialog(Clicker *clicker, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateUi();
    void spin();
    void stopReel(int reel);
    void forceWin();
    void evaluateAndShow();
    void highlightLine(int lineIdx, bool on);

    Clicker *clicker;

    QLabel *balanceLabel = nullptr;
    QLabel *betLabel = nullptr;
    QLabel *resultLabel = nullptr;
    QPushButton *spinButton = nullptr;
    QPushButton *betUpButton = nullptr;
    QPushButton *betDownButton = nullptr;
    QPushButton *maxBetButton = nullptr;
    QList<QPushButton *> presetButtons;

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
    QVector<int> winStarts;
    QVector<int> winCounts;

    QSoundEffect *reelStopSound = nullptr;
    QSoundEffect *winSound = nullptr;
};
