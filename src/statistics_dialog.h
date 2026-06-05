#pragma once

#include <QDialog>

class Clicker;

class StatisticsDialog : public QDialog {
    Q_OBJECT

public:
    explicit StatisticsDialog(Clicker *clicker);

private:
    Clicker *clicker;
};
