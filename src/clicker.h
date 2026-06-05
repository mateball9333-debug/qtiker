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
class QResizeEvent;
class GachaDialog;
class QGraphicsDropShadowEffect;
class QLabel;
class QObject;
class ParticleOverlay;
class QPushButton;
class QTimer;
enum class GachaEffect;

class Clicker : public QWidget {
    Q_OBJECT

public:
    explicit Clicker(QWidget *parent = nullptr);

    void resetGame();
    void showStatistics();
    void showAssets();
    void saveGame();
    void refreshUi();
    void activateTimedBuff(TimedBuff buff, int durationSeconds);
    int timedBuffSecondsLeft(TimedBuff buff) const;
    QString formatNumber(qint64 value) const;
    QString formatDuration(qint64 seconds) const;
    qint64 currentTotalPlaySeconds() const;

    int slotForSettings() const { return currentSlot; }
    void switchToSlot(int slot);
    void resetSlot();

    GameState game;

signals:
    void switchToLegacyRequested();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void makeClick();
    void buyClickUpgrade();
    void buyIncomeUpgrade();
    void addPassiveIncome();
    void showSettings();
    void showChangelog();
    void showInfo();
    void showGacha();
    void selectGachaCard(GachaDialog *dialog, int index);
    void selectGachaCard2(GachaDialog *dialog, int index);
    void showCarat();

private:
    enum class TextEffectMode {
        RainbowGlow,
        RainbowFill,
        SolidFill
    };

    struct TextEffectState {
        QGraphicsDropShadowEffect *glowEffect = nullptr;
        QColor originalTextColor;
        int hue = 0;
        bool hasOriginalTextColor = false;
    };

    void showTux();
    void setupWindow();
    void buildUi();
    QPushButton *createTopIconButton(const QString &toolTip);
    QFrame *createUpgradesBox();
    void startIncomeTimer();
    void applyThemeIcons();
    void setThemeIcon(QPushButton *button, const QString &path);
    QGraphicsDropShadowEffect *setButtonGlow(
        QPushButton *button,
        QGraphicsDropShadowEffect *effect,
        const QColor &color
    );
    void clearButtonGlow(QPushButton *button, QGraphicsDropShadowEffect *effect);
    void applyTextEffect(
        QLabel *label,
        TextEffectState &state,
        TextEffectMode mode,
        const QColor &fillColor = QColor()
    );
    void clearTextEffect(QLabel *label, TextEffectState &state);
    QColor activeCardColorForEffect(GachaEffect effect) const;
    void applyStatsTextColor(QLabel *label, TextEffectState &state, const QColor &accentColor);
    void startChangelogHighlightIfNeeded();
    void updateChangelogHighlight();
    void stopChangelogHighlight();
    void updateStatsBuffGlow();
    void markChangelogSeen();
    void loadGame();
    void addArchProgress(qint64 amount);
    void maybeStartClickEffect();
    void updateClickEffect();
    void stopClickEffect();
    void triggerCritBurst(bool big);
    void rollGacha(GachaDialog *dialog);
    qint64 applyActiveCardBonus(qint64 value, GachaEffect effect) const;
    qint64 applyTimedBuffBonuses(qint64 value, TimedBuffEffect effect) const;
    bool isTimedBuffActive(TimedBuff buff) const;
    QString formatUpgradeText(const QString &label, int ownedCount, int cost) const;
    QString changelogHtml() const;
    void buttonChange();

    QElapsedTimer playTimer;
    int currentSlot = 0;

    QLabel *scoreLabel = nullptr;
    QLabel *clickStatsLabel = nullptr;
    QLabel *incomeStatsLabel = nullptr;
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
    QTimer *changelogHighlightTimer = nullptr;
    QTimer *statsBuffGlowTimer = nullptr;
    QGraphicsDropShadowEffect *clickGlowEffect = nullptr;
    QGraphicsDropShadowEffect *changelogGlowEffect = nullptr;
    ParticleOverlay *particleOverlay = nullptr;
    int clickEffectFramesLeft = 0;
    int clickEffectHue = 0;
    TextEffectState clickStatsBuffGlowState;
    TextEffectState incomeStatsBuffGlowState;
    int changelogHighlightHue = 0;
    bool changelogSeenForVersion = true;
};
