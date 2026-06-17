// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QPushButton>

#include <array>

class Clicker;

class SaveSlotsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SaveSlotsDialog(Clicker *clicker);

signals:
    void slotLoaded();

private:
    void refresh();

    Clicker *clicker;
    std::array<QLabel *, 3> infoLabels = {};
    std::array<QPushButton *, 3> loadButtons = {};
    std::array<QPushButton *, 3> resetButtons = {};
};
