#pragma once

#include "gamestate.h"

#include <QColor>
#include <QElapsedTimer>
#include <QIcon>
#include <QSize>
#include <QString>
#include <QWidget>

class QCloseEvent;
class QEvent;
class QFrame;
class GachaDialog;
class QLabel;
class QObject;
class QPushButton;
class QTimer;
enum class GachaEffect;

class Clicker : public QWidget {
    Q_OBJECT

public:
    explicit Clicker(QWidget *parent = nullptr);

signals:
    void switchToLegacyRequested();

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
    void showInfo();
    void showGacha();
    void showCarat();
    void showStatistics();

private:
    void showTux();
    void setupWindow();
    void buildUi();
    QPushButton *createTopIconButton(const QString &toolTip);
    QFrame *createUpgradesBox();
    void startIncomeTimer();
    void applyThemeIcons();
    void setThemeIcon(QPushButton *button, const QString &path);
    void refreshUi();
    void loadGame();
    void saveGame();
    qint64 currentTotalPlaySeconds() const;
    void addArchProgress(qint64 amount);
    void maybeStartClickEffect();
    void updateClickEffect();
    void stopClickEffect();
    void rollGacha(GachaDialog *dialog);
    void selectGachaCard(GachaDialog *dialog, int index);
    qint64 applyActiveCardBonus(qint64 value, GachaEffect effect) const;
    qint64 applyTimedBuffBonuses(qint64 value, TimedBuffEffect effect) const;
    bool isTimedBuffActive(TimedBuff buff) const;
    int timedBuffSecondsLeft(TimedBuff buff) const;
    void activateTimedBuff(TimedBuff buff, int durationSeconds);
    QString formatNumber(qint64 value) const;
    QString formatDuration(qint64 seconds) const;
    QString formatUpgradeText(const QString &label, int cost) const;
    QIcon tintedSvgIcon(const QString &path, const QColor &color, const QSize &size) const;
    QString tintedSvgDataUri(const QString &path, const QColor &color, const QSize &size) const;
    QString changelogHtml() const;
    void buttonChange();
    void setFont(QWidget *widget, int pointSize, bool bold);

    GameState game;
    QElapsedTimer playTimer;

    QLabel *scoreLabel = nullptr;
    QLabel *statsLabel = nullptr;
    QLabel *archLabel = nullptr;
    QPushButton *caratButton = nullptr;
    QPushButton *changelogButton = nullptr;
    QPushButton *infoButton = nullptr;
    QPushButton *settingsButton = nullptr;
    QPushButton *clickButton = nullptr;
    QPushButton *gachaButton = nullptr;
    QPushButton *clickUpgradeButton = nullptr;
    QPushButton *incomeUpgradeButton = nullptr;
    QTimer *incomeTimer = nullptr;
    QTimer *clickEffectCheckTimer = nullptr;
    QTimer *clickEffectTimer = nullptr;
    int clickEffectFramesLeft = 0;
    int clickEffectHue = 0;
};
