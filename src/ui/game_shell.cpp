// SPDX-License-Identifier: GPL-2.0-or-later
#include "game_shell.h"

#include "clicker.h"
#include "legacy_clicker.h"

#include <QCloseEvent>
#include <QIcon>
#include <QSettings>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {
constexpr auto GameModeKey = "gameMode";
constexpr auto ModernModeValue = "modern";
constexpr auto Legacy012ModeValue = "legacy-0.1.2";
}

GameShell::GameShell(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Qtiker");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    setMinimumSize(340, 420);
    resize(360, 500);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    stack = new QStackedWidget(this);
    modernClicker = new Clicker(stack);
    legacyClicker = new LegacyClicker(stack);

    stack->addWidget(modernClicker);
    stack->addWidget(legacyClicker);
    layout->addWidget(stack);

    connect(modernClicker, &Clicker::switchToLegacyRequested, this, [this]() {
        setMode(GameMode::Legacy012);
    });
    connect(legacyClicker, &LegacyClicker::switchToModernRequested, this, [this]() {
        setMode(GameMode::Modern);
    });

    loadMode();
    setMode(mode);
}

void GameShell::loadMode() {
    QSettings settings("qtiker", "qtiker");
    const auto value = settings.value(GameModeKey, ModernModeValue).toString();
    mode = value == Legacy012ModeValue ? GameMode::Legacy012 : GameMode::Modern;
}

void GameShell::saveMode() const {
    QSettings settings("qtiker", "qtiker");
    settings.setValue(GameModeKey, mode == GameMode::Legacy012 ? Legacy012ModeValue : ModernModeValue);
}

void GameShell::setMode(GameMode nextMode) {
    mode = nextMode;
    stack->setCurrentWidget(mode == GameMode::Legacy012 ? static_cast<QWidget *>(legacyClicker)
                                                        : static_cast<QWidget *>(modernClicker));
    setWindowTitle(mode == GameMode::Legacy012 ? "Qtiker Legacy 0.1.2" : "Qtiker");
    saveMode();
}

void GameShell::closeEvent(QCloseEvent *event) {
    modernClicker->saveGame();
    legacyClicker->saveGame();
    QWidget::closeEvent(event);
}
