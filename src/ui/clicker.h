// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "core/gamestate.h"

#include <QColor>
#include <QElapsedTimer>
#include <QIcon>
#include <QSize>
#include <QSoundEffect>
#include <QString>
#include <QWidget>

inline constexpr int ButtonTextChangeChance = 35;
inline constexpr int ClickEffectChance = 10;
inline constexpr int ClickEffectCheckMs = 10000;
inline constexpr int ClickEffectFrameMs = 120;
inline constexpr int ClickEffectFrames = 34;
inline constexpr int ChangelogHighlightFrameMs = 140;

inline const char *NumberSuffixes[] = {
    "",
    "K",
    "M",
    "B",
    "T",
    "Qa",
    "Qi",
    "Sx",
    "Sp",
    "Oc",
    "No",
    "Dc",
};

namespace SettingsKeys {
inline constexpr auto Score = "score";
inline constexpr auto ArchProgress = "archProgress";
inline constexpr auto NextArchAt = "nextArchAt";
inline constexpr auto PerClick = "perClick";
inline constexpr auto PerSecond = "perSecond";
inline constexpr auto Arches = "arches";
inline constexpr auto Carats = "carats";
inline constexpr auto TotalClicks = "statTotalClicks";
inline constexpr auto TotalScoreEarned = "totalScoreEarned";
inline constexpr auto TotalPlaySeconds = "totalPlaySeconds";
inline constexpr auto TotalArchesEarned = "totalArchesEarned";
inline constexpr auto ClickButtonRightClicks = "clickButtonRightClicks";
inline constexpr auto SelectedCard = "selectedCard";
inline constexpr auto SelectedCard2 = "selectedCard2";
inline constexpr auto SecondCardSlotUnlocked = "secondCardSlotUnlocked";
inline constexpr auto SecondCardPenaltyUpgraded = "secondCardPenaltyUpgraded";
inline constexpr auto ClickCost = "clickCost";
inline constexpr auto IncomeCost = "incomeCost";
inline constexpr auto IncomeBuffEasterEgg = "incomeBuffEasterEgg";
inline constexpr auto ClickMultLevel = "clickMultLevel";
inline constexpr auto IncomeMultLevel = "incomeMultLevel";
inline constexpr auto GachaPityCounter = "gachaPityCounter";
inline constexpr auto CasinoTotalWon = "casinoTotalWon";
inline constexpr auto CasinoTotalSpins = "casinoTotalSpins";
inline constexpr auto ArchesFromCasino = "archesFromCasino";
inline constexpr auto TotalArchesSpent = "totalArchesSpent";
inline constexpr auto BuffExpiresAtPrefix = "buffExpiresAt";
inline constexpr auto Checksum = "checksum";
inline constexpr auto LastSeenChangelogVersion = "lastSeenChangelogVersion";
inline constexpr auto CurrentSlot = "currentSlot";
inline constexpr auto LegacyTotalClicks = "totalClicks";
inline constexpr auto LegacyClickPower = "clickPower";
inline constexpr auto LegacyAutoPower = "autoPower";
inline constexpr auto LegacyTotalClickScoreEarned = "totalClickScoreEarned";
inline constexpr auto LegacyClickUpgradeCost = "clickUpgradeCost";
inline constexpr auto LegacyAutoUpgradeCost = "autoUpgradeCost";
}

class QCloseEvent;
class QEvent;
class QFrame;
class QResizeEvent;
class QHideEvent;
class QShowEvent;
class GachaDialog;
class QGraphicsDropShadowEffect;
class QLabel;
class QObject;
class ParticleOverlay;
class QPushButton;
class QTimer;
class StatusBar;
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
    void setMasterVolume(qreal volume);
    qreal masterVolume() const;
    void setClickSoundMuted(bool muted);
    bool isClickSoundMuted() const;
    void setCritSoundMuted(bool muted);
    bool isCritSoundMuted() const;
    bool isCompatibilityMode() const { return m_compatibilityMode; }
    void setCompatibilityMode(bool enabled);
    QString compat(const QString &text) const;

    GameState game;

signals:
    void switchToLegacyRequested();
    void saveCompleted();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

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
    void showCasino();
    void showLore();

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
    bool loadGame();
    void addArchProgress(qint64 amount);
    void maybeStartClickEffect();
    void updateClickEffect();
    void stopClickEffect();
    void triggerCritBurst(bool big);
    void rollGacha(GachaDialog *dialog);
    qint64 applyActiveCardBonus(qint64 value, GachaEffect effect) const;
    qint64 applyTimedBuffBonuses(qint64 value, TimedBuffEffect effect) const;
    bool isTimedBuffActive(TimedBuff buff) const;
    QString formatUpgradeText(const QString &label, int ownedCount, qint64 cost) const;
    QString changelogHtml() const;
    void buttonChange();
    int totalSpecialEffectValue(GachaEffect effect) const;
    bool hasSpecialEffect(GachaEffect effect) const;
    bool isPathActive(GachaEffect effect, int path) const;
    void upgradeCard(int index);
    void upgradeCardPath(int index, int path);

    QElapsedTimer playTimer;
    int currentSlot = 0;
    qreal m_masterVolume = 1.0;
    bool m_compatibilityMode = false;

    StatusBar *statusBar = nullptr;
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
    QPushButton *casinoButton = nullptr;
    QPushButton *clickUpgradeButton = nullptr;
    QPushButton *incomeUpgradeButton = nullptr;
    QTimer *incomeTimer = nullptr;
    QTimer *clickEffectCheckTimer = nullptr;
    QTimer *clickEffectTimer = nullptr;
    QTimer *changelogHighlightTimer = nullptr;
    QTimer *statsBuffGlowTimer = nullptr;
    QGraphicsDropShadowEffect *clickGlowEffect = nullptr;
    QGraphicsDropShadowEffect *changelogGlowEffect = nullptr;
    QSoundEffect *clickSound = nullptr;
    QSoundEffect *buySound = nullptr;
    QSoundEffect *critSound = nullptr;
    ParticleOverlay *particleOverlay = nullptr;
    int clickEffectFramesLeft = 0;
    int clickEffectHue = 0;
    TextEffectState clickStatsBuffGlowState;
    TextEffectState incomeStatsBuffGlowState;
    int changelogHighlightHue = 0;
    bool changelogSeenForVersion = true;
};
