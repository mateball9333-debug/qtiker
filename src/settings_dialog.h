#pragma once

#include <QDialog>

class Clicker;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(Clicker *clicker);

private:
    Clicker *clicker;
};
