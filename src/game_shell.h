#pragma once

#include <QWidget>

class Clicker;
class LegacyClicker;
class QStackedWidget;

class GameShell : public QWidget {
    Q_OBJECT

public:
    explicit GameShell(QWidget *parent = nullptr);

private:
    enum class GameMode {
        Modern,
        Legacy012,
    };

    void loadMode();
    void saveMode() const;
    void setMode(GameMode nextMode);

    GameMode mode = GameMode::Modern;
    QStackedWidget *stack = nullptr;
    Clicker *modernClicker = nullptr;
    LegacyClicker *legacyClicker = nullptr;
};
