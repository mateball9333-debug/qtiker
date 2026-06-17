// SPDX-License-Identifier: GPL-2.0-or-later
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
