#pragma once

#include "gamestate.h"

#include <QColor>
#include <QIcon>
#include <QSize>
#include <QString>
#include <QWidget>

class QCloseEvent;
class QEvent;
class QFrame;
class QLabel;
class QObject;
class QPushButton;
class QTimer;

class Clicker : public QWidget {
    Q_OBJECT

public:
    explicit Clicker(QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void makeClick();
    void buyClickUpgrade();
    void buyIncomeUpgrade();
    void addPassiveIncome();
    void resetGame();
    void showSettings();
    void showChangelog();

private:
    void showTux();
    void setupWindow();
    void buildUi();
    QFrame *createUpgradesBox();
    void startIncomeTimer();
    void applyThemeIcons();
    void refreshUi();
    void loadGame();
    void saveGame() const;
    int nextCost(int currentCost, int extra) const;
    QIcon tintedSvgIcon(const QString &path, const QColor &color, const QSize &size) const;
    QString tintedSvgDataUri(const QString &path, const QColor &color, const QSize &size) const;
    QString changelogHtml() const;
    void setFont(QWidget *widget, int pointSize, bool bold);

    GameState game;

    QLabel *scoreLabel = nullptr;
    QLabel *statsLabel = nullptr;
    QPushButton *changelogButton = nullptr;
    QPushButton *settingsButton = nullptr;
    QPushButton *clickButton = nullptr;
    QPushButton *clickUpgradeButton = nullptr;
    QPushButton *incomeUpgradeButton = nullptr;
    QTimer *incomeTimer = nullptr;
};
