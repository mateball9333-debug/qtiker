#include "clicker.h"

#include "appversion.h"
#include "gacha_dialog.h"
#include "game_rules.h"

#include <QApplication>
#include <QBuffer>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIODevice>
#include <QImage>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
#include <QStringList>
#include <QSvgRenderer>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

#include <array>
#include <iterator>

namespace {
constexpr QSize TopIconSize(16, 16);
constexpr QSize TopIconButtonSize(32, 28);
constexpr QSize ChangelogButtonSize(86, 28);
constexpr int WindowMargin = 14;
constexpr int WindowSpacing = 10;
constexpr int DialogMargin = 12;
constexpr int DialogSpacing = 8;
constexpr int PanelMargin = 10;
constexpr int TopBarSpacing = 6;
constexpr int ButtonTextChangeChance = 35;
constexpr int ClickEffectChance = 10;
constexpr int ClickEffectCheckMs = 10000;
constexpr int ClickEffectFrameMs = 120;
constexpr int ClickEffectFrames = 34;
constexpr QSize CaratIconSize(18, 18);

const char *NumberSuffixes[] = {
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
constexpr auto Score = "score";
constexpr auto ArchProgress = "archProgress";
constexpr auto NextArchAt = "nextArchAt";
constexpr auto PerClick = "perClick";
constexpr auto PerSecond = "perSecond";
constexpr auto Arches = "arches";
constexpr auto Carats = "carats";
constexpr auto TotalClicks = "statTotalClicks";
constexpr auto TotalScoreEarned = "totalScoreEarned";
constexpr auto TotalPlaySeconds = "totalPlaySeconds";
constexpr auto TotalArchesEarned = "totalArchesEarned";
constexpr auto ClickButtonRightClicks = "clickButtonRightClicks";
constexpr auto SelectedCard = "selectedCard";
constexpr auto ClickCost = "clickCost";
constexpr auto IncomeCost = "incomeCost";
constexpr auto BuffExpiresAtPrefix = "buffExpiresAt";
constexpr auto LegacyClickPower = "clickPower";
constexpr auto LegacyAutoPower = "autoPower";
constexpr auto LegacyClickUpgradeCost = "clickUpgradeCost";
constexpr auto LegacyAutoUpgradeCost = "autoUpgradeCost";
constexpr auto LegacyTotalClicks = "totalClicks";
constexpr auto LegacyTotalClickScoreEarned = "totalClickScoreEarned";
}

enum class ChangelogIcon {
    Added,
    Changed
};

struct ChangelogEntry {
    ChangelogIcon icon;
    const char *text;
};

struct ChangelogRelease {
    const char *version;
    const char *date;
    const ChangelogEntry *entries;
    qsizetype entryCount;
};

constexpr ChangelogEntry Changelog013[] = {
    {ChangelogIcon::Added, "Added clicks show strange text sometimes."},
    {ChangelogIcon::Added, "Added an info button.."},
    {ChangelogIcon::Added, "Added Arch's and gacha cards."},
    {ChangelogIcon::Added, "Added card inventory, selection, and stacked bonuses."},
    {ChangelogIcon::Added, "Added Carat currency with a small top-bar vault."},
    {ChangelogIcon::Added, "Added a Carat window with click burning and timed buffs."},
    {ChangelogIcon::Added, "Added Legacy 0.1.2 mode for the old clean click loop."},
    {ChangelogIcon::Added, "Added statistics for progress, time, and a few questionable gestures."},
    {ChangelogIcon::Changed, "Improved number formatting and incremental progression."},
    {ChangelogIcon::Changed, "Added small visual polish for click interactions."},
    {ChangelogIcon::Changed, "Stats now tell the whole truth after cards quietly tip the scales."},
};

constexpr ChangelogEntry Changelog012[] = {
    {ChangelogIcon::Changed, "Optimized PNG assets - binary ~1 MB lighter."},
};

constexpr ChangelogEntry Changelog011[] = {
    {ChangelogIcon::Added, "Added compact in-game release notes."},
    {ChangelogIcon::Added, "Added colored release note markers."},
    {ChangelogIcon::Added, "Added a settings dialog."},
    {ChangelogIcon::Added, "Added reset confirmation."},
    {ChangelogIcon::Changed, "Moved reset into settings."},
    {ChangelogIcon::Changed, "Updated 0.1.1 metadata."},
};

constexpr ChangelogEntry Changelog010[] = {
    {ChangelogIcon::Changed, "Initial packaged version."},
};

constexpr ChangelogRelease ChangelogReleases[] = {
    {"0.2.0", "2026-06-03", Changelog013, std::size(Changelog013)},
    {"0.1.2", "2026-06-02", Changelog012, std::size(Changelog012)},
    {"0.1.1", "2026-06-02", Changelog011, std::size(Changelog011)},
    {"0.1.0", "2026-06-01", Changelog010, std::size(Changelog010)},
};
}

Clicker::Clicker(QWidget *parent) : QWidget(parent) {
    setupWindow();
    loadGame();
    playTimer.start();
    buildUi();
    startIncomeTimer();
    refreshUi();
}

void Clicker::changeEvent(QEvent *event) {
    QWidget::changeEvent(event);

    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::StyleChange) {
        QTimer::singleShot(0, this, &Clicker::applyThemeIcons);
        QTimer::singleShot(100, this, &Clicker::applyThemeIcons);
    }
}

void Clicker::closeEvent(QCloseEvent *event) {
    saveGame();
    QWidget::closeEvent(event);
}

bool Clicker::eventFilter(QObject *watched, QEvent *event) {
    if (watched == clickButton && event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::MiddleButton) {
            showTux();
            return true;
        }
        if (mouseEvent->button() == Qt::RightButton) {
            game.clickButtonRightClicks += 1;
            saveGame();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void Clicker::makeClick() {
    const auto earned = applyActiveCardBonus(game.perClick, GachaEffect::Click);
    game.score += earned;
    game.totalClicks += 1;
    game.totalScoreEarned += earned;
    addArchProgress(earned);

    buttonChange();
    saveGame();
    refreshUi();
}

void Clicker::buyClickUpgrade() {
    if (game.score < game.clickCost) {
        return;
    }

    game.score -= game.clickCost;
    game.perClick += 1;
    game.clickCost = nextClickUpgradeCost(game.clickCost);

    saveGame();
    refreshUi();
}

void Clicker::buyIncomeUpgrade() {
    if (game.score < game.incomeCost) {
        return;
    }

    game.score -= game.incomeCost;
    game.perSecond += 1;
    game.incomeCost = nextIncomeUpgradeCost(game.incomeCost);

    saveGame();
    refreshUi();
}

void Clicker::addPassiveIncome() {
    if (game.perSecond == 0) {
        return;
    }

    const auto cardEarned = applyActiveCardBonus(game.perSecond, GachaEffect::Income);
    const auto earned = applyTimedBuffBonuses(cardEarned, TimedBuffEffect::Income);
    game.score += earned;
    game.totalScoreEarned += earned;
    addArchProgress(earned);
    saveGame();
    refreshUi();
}

void Clicker::resetGame() {
    game.reset();
    saveGame();
    refreshUi();
}

void Clicker::showSettings() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Qtiker Settings");
    dialog->setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    dialog->resize(320, 170);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel("Settings", dialog);
    setFont(title, 13, true);

    auto *resetBox = new QFrame(dialog);
    resetBox->setFrameShape(QFrame::StyledPanel);

    auto *resetLayout = new QHBoxLayout(resetBox);
    resetLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    resetLayout->setSpacing(DialogSpacing);

    auto *resetText = new QLabel("Reset saved progress", resetBox);
    setFont(resetText, resetText->font().pointSize(), true);

    auto *resetButton = new QPushButton("Reset", resetBox);
    resetButton->setIcon(tintedSvgIcon(
        ":/assets/ui/reset.svg",
        resetButton->palette().color(QPalette::ButtonText),
        TopIconSize
    ));
    resetButton->setIconSize(TopIconSize);
    connect(resetButton, &QPushButton::clicked, this, [this, dialog]() {
        auto answer = QMessageBox::question(
            dialog,
            "Reset",
            "Are you sure you want to delete saved progress?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (answer == QMessageBox::Yes) {
            resetGame();
            dialog->accept();
        }
    });

    resetLayout->addWidget(resetText);
    resetLayout->addStretch();
    resetLayout->addWidget(resetButton);

    auto *statisticsBox = new QFrame(dialog);
    statisticsBox->setFrameShape(QFrame::StyledPanel);

    auto *statisticsLayout = new QHBoxLayout(statisticsBox);
    statisticsLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    statisticsLayout->setSpacing(DialogSpacing);

    auto *statisticsText = new QLabel("Statistics", statisticsBox);
    setFont(statisticsText, statisticsText->font().pointSize(), true);

    auto *statisticsButton = new QPushButton("Show statistics", statisticsBox);
    statisticsButton->setIcon(tintedSvgIcon(
        ":/assets/ui/statistics.svg",
        statisticsButton->palette().color(QPalette::ButtonText),
        TopIconSize
    ));
    statisticsButton->setIconSize(TopIconSize);
    connect(statisticsButton, &QPushButton::clicked, this, &Clicker::showStatistics);

    statisticsLayout->addWidget(statisticsText);
    statisticsLayout->addStretch();
    statisticsLayout->addWidget(statisticsButton);

    auto *modeBox = new QFrame(dialog);
    modeBox->setFrameShape(QFrame::StyledPanel);

    auto *modeLayout = new QHBoxLayout(modeBox);
    modeLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    modeLayout->setSpacing(DialogSpacing);

    auto *modeText = new QLabel("Game version", modeBox);
    setFont(modeText, modeText->font().pointSize(), true);

    auto *legacyButton = new QPushButton("Legacy 0.1.2", modeBox);
    connect(legacyButton, &QPushButton::clicked, this, [this, dialog]() {
        emit switchToLegacyRequested();
        dialog->accept();
    });

    modeLayout->addWidget(modeText);
    modeLayout->addStretch();
    modeLayout->addWidget(legacyButton);

    auto *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    layout->addWidget(title);
    layout->addWidget(resetBox);
    layout->addWidget(statisticsBox);
    layout->addWidget(modeBox);
    layout->addStretch();
    layout->addWidget(closeButton);

    dialog->show();
}

void Clicker::buttonChange() {
    static const QStringList texts = {
        "Click",
        "Bash",
        "kcilC",
        "Stop!",
        "/home",
        "rm -rf /",
        "Click",
        "Click"
    };

    if (QRandomGenerator::global()->bounded(100) >= ButtonTextChangeChance) {
        return;
    }

    const int index = QRandomGenerator::global()->bounded(texts.size());
    clickButton->setText(texts.at(index));
}

void Clicker::showChangelog() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Qtiker Release Notes");
    dialog->setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    dialog->resize(340, 260);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(DialogSpacing);

    auto *titleLayout = new QHBoxLayout();
    titleLayout->setSpacing(DialogSpacing);

    auto *title = new QLabel("Release notes", dialog);
    setFont(title, 13, true);

    auto *version = new QLabel(QString("v%1").arg(AppVersion), dialog);
    version->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    setFont(version, version->font().pointSize(), true);

    titleLayout->addWidget(title);
    titleLayout->addStretch();
    titleLayout->addWidget(version);

    auto *changes = new QTextBrowser(dialog);
    changes->setOpenExternalLinks(false);
    changes->setHtml(changelogHtml());
    changes->setStyleSheet(
        "QTextBrowser {"
        "  border: 1px solid palette(midlight);"
        "  border-radius: 6px;"
        "  padding: 6px;"
        "  background: palette(base);"
        "}"
    );

    auto *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    layout->addLayout(titleLayout);
    layout->addWidget(changes, 1);
    layout->addWidget(closeButton);

    dialog->show();
}

void Clicker::showInfo() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("About Qtiker");
    dialog->setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    dialog->resize(340, 330);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(WindowMargin, WindowMargin, WindowMargin, WindowMargin);
    layout->setSpacing(DialogSpacing);

    auto *logo = new QLabel(dialog);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QPixmap(":/assets/qtiker-64.png"));

    auto *title = new QLabel(QString("Qtiker v%1").arg(AppVersion), dialog);
    title->setAlignment(Qt::AlignCenter);
    setFont(title, 14, true);

    auto *description = new QLabel(
        "A small Qt Widgets clicker game for Linux desktops.",
        dialog
    );
    description->setWordWrap(true);
    description->setAlignment(Qt::AlignCenter);

    auto *details = new QLabel(
        "Author: G3B33\n"
        "Source code license: GPL-2.0-or-later\n"
        "Material Symbols: Apache-2.0\n"
        "Tux artwork: Larry Ewing and The GIMP",
        dialog
    );
    details->setWordWrap(true);
    details->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    layout->addWidget(logo);
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addSpacing(6);
    layout->addWidget(details);
    layout->addStretch();
    layout->addWidget(closeButton);

    dialog->show();
}

void Clicker::showGacha() {
    auto *dialog = new GachaDialog(this);
    dialog->setArchCount(game.arches);
    dialog->setInventory(game.cardCounts, game.selectedCard);
    connect(dialog, &GachaDialog::rollRequested, this, [this, dialog]() {
        rollGacha(dialog);
    });
    connect(dialog, &GachaDialog::cardSelected, this, [this, dialog](int index) {
        selectGachaCard(dialog, index);
    });
    dialog->show();
}

void Clicker::showCarat() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Carat");
    dialog->setWindowIcon(QIcon(":/assets/ui/carat.png"));
    dialog->resize(340, 230);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *balanceBox = new QFrame(dialog);
    balanceBox->setFrameShape(QFrame::StyledPanel);

    auto *balanceLayout = new QHBoxLayout(balanceBox);
    balanceLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    balanceLayout->setSpacing(DialogSpacing);

    auto *caratIcon = new QLabel(balanceBox);
    caratIcon->setPixmap(QIcon(":/assets/ui/carat.png").pixmap(32, 32));

    auto *caratBalanceLabel = new QLabel(balanceBox);
    setFont(caratBalanceLabel, 14, true);

    auto *clickBalanceLabel = new QLabel(balanceBox);
    clickBalanceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    balanceLayout->addWidget(caratIcon);
    balanceLayout->addWidget(caratBalanceLabel);
    balanceLayout->addStretch();
    balanceLayout->addWidget(clickBalanceLabel);

    auto *burnButton = new QPushButton(dialog);
    burnButton->setIcon(QIcon(":/assets/ui/carat.png"));
    burnButton->setIconSize(CaratIconSize);

    auto *buffBox = new QFrame(dialog);
    buffBox->setFrameShape(QFrame::StyledPanel);

    auto *buffLayout = new QVBoxLayout(buffBox);
    buffLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    buffLayout->setSpacing(DialogSpacing);

    std::array<QLabel *, TimedBuffCount> buffStatusLabels = {};
    std::array<QPushButton *, TimedBuffCount> buyBuffButtons = {};

    for (int index = 0; index < TimedBuffCount; ++index) {
        const auto &rule = TimedBuffRules[index];

        auto *buffTitle = new QLabel(rule.name, buffBox);
        setFont(buffTitle, buffTitle->font().pointSize(), true);

        buffStatusLabels[index] = new QLabel(buffBox);
        buyBuffButtons[index] = new QPushButton(buffBox);

        buffLayout->addWidget(buffTitle);
        buffLayout->addWidget(buffStatusLabels[index]);
        buffLayout->addWidget(buyBuffButtons[index]);
    }

    auto *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    const auto updateDialog = [this, caratBalanceLabel, clickBalanceLabel, burnButton, buffStatusLabels, buyBuffButtons]() {
        caratBalanceLabel->setText(QString("Carat %1").arg(formatNumber(game.carats)));
        clickBalanceLabel->setText(QString("%1 clicks").arg(formatNumber(game.score)));

        burnButton->setText(QString("Burn %1 clicks  +%2 Carat")
                                .arg(formatNumber(CaratBurnCost))
                                .arg(formatNumber(CaratBurnReward)));
        burnButton->setEnabled(game.score >= CaratBurnCost);

        for (int index = 0; index < TimedBuffCount; ++index) {
            const auto &rule = TimedBuffRules[index];
            const int secondsLeft = timedBuffSecondsLeft(rule.buff);

            buffStatusLabels[index]->setText(secondsLeft > 0
                ? QString("Active: %1s left").arg(secondsLeft)
                : QString("Inactive"));
            buyBuffButtons[index]->setText(QString("%1 for %2s  %3 Carat")
                                               .arg(rule.name)
                                               .arg(rule.durationSeconds)
                                               .arg(formatNumber(rule.caratCost)));
            buyBuffButtons[index]->setEnabled(game.carats >= rule.caratCost);
        }
    };

    connect(burnButton, &QPushButton::clicked, this, [this, updateDialog]() {
        if (game.score < CaratBurnCost) {
            return;
        }

        game.score -= CaratBurnCost;
        game.carats += CaratBurnReward;
        saveGame();
        refreshUi();
        updateDialog();
    });

    for (int index = 0; index < TimedBuffCount; ++index) {
        const auto rule = TimedBuffRules[index];
        connect(buyBuffButtons[index], &QPushButton::clicked, this, [this, updateDialog, rule]() {
            if (game.carats < rule.caratCost) {
                return;
            }

            game.carats -= rule.caratCost;
            activateTimedBuff(rule.buff, rule.durationSeconds);
            saveGame();
            refreshUi();
            updateDialog();
        });
    }

    layout->addWidget(balanceBox);
    layout->addWidget(burnButton);
    layout->addWidget(buffBox);
    layout->addStretch();
    layout->addWidget(closeButton);

    auto *dialogTimer = new QTimer(dialog);
    dialogTimer->setInterval(1000);
    connect(dialogTimer, &QTimer::timeout, dialog, updateDialog);
    dialogTimer->start();

    updateDialog();
    dialog->show();
}

void Clicker::showStatistics() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Qtiker Statistics");
    dialog->setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    dialog->resize(340, 230);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel("Statistics", dialog);
    setFont(title, 13, true);

    auto *statsBox = new QFrame(dialog);
    statsBox->setFrameShape(QFrame::StyledPanel);

    auto *statsLayout = new QVBoxLayout(statsBox);
    statsLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    statsLayout->setSpacing(DialogSpacing);

    auto *totalClicksLabel = new QLabel(
        QString("Total clicks: %1").arg(formatNumber(game.totalClicks)),
        statsBox
    );
    auto *earnedScoreLabel = new QLabel(
        QString("Total score earned: %1").arg(formatNumber(game.totalScoreEarned)),
        statsBox
    );
    auto *playTimeLabel = new QLabel(
        QString("Total play time: %1").arg(formatDuration(currentTotalPlaySeconds())),
        statsBox
    );
    auto *archesLabel = new QLabel(
        QString("Total Arch's earned: %1").arg(formatNumber(game.totalArchesEarned)),
        statsBox
    );

    statsLayout->addWidget(totalClicksLabel);
    statsLayout->addWidget(earnedScoreLabel);
    statsLayout->addWidget(playTimeLabel);
    statsLayout->addWidget(archesLabel);

    auto *uselessBox = new QFrame(dialog);
    uselessBox->setFrameShape(QFrame::StyledPanel);

    auto *uselessLayout = new QVBoxLayout(uselessBox);
    uselessLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    uselessLayout->setSpacing(DialogSpacing);

    auto *uselessTitle = new QLabel("Questionable statistic", uselessBox);
    setFont(uselessTitle, uselessTitle->font().pointSize(), true);

    auto *rightClicksLabel = new QLabel(
        QString("Right-clicks on Click button: %1").arg(formatNumber(game.clickButtonRightClicks)),
        uselessBox
    );

    uselessLayout->addWidget(uselessTitle);
    uselessLayout->addWidget(rightClicksLabel);

    auto *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    layout->addWidget(title);
    layout->addWidget(statsBox);
    layout->addWidget(uselessBox);
    layout->addStretch();
    layout->addWidget(closeButton);

    dialog->show();
}

void Clicker::showTux() {
    auto *window = new QWidget(this, Qt::Dialog);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->setWindowTitle("Tux");
    window->setWindowIcon(QIcon(":/assets/tux.png"));

    auto *layout = new QVBoxLayout(window);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);

    auto *image = new QLabel(window);
    image->setAlignment(Qt::AlignCenter);
    image->setPixmap(QPixmap(":/assets/tux.png"));

    layout->addWidget(image);

    window->setFixedSize(290, 350);
    window->show();
}

void Clicker::setupWindow() {
    setWindowTitle("Qtiker");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    setMinimumSize(340, 360);
    resize(360, 420);
}

void Clicker::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(WindowMargin, WindowMargin, WindowMargin, WindowMargin);
    mainLayout->setSpacing(WindowSpacing);

    changelogButton = new QPushButton(QString("v%1").arg(AppVersion), this);
    changelogButton->setIconSize(TopIconSize);
    changelogButton->setToolTip("Release notes");
    changelogButton->setFixedSize(ChangelogButtonSize);
    setFont(changelogButton, changelogButton->font().pointSize(), true);
    connect(changelogButton, &QPushButton::clicked, this, &Clicker::showChangelog);

    infoButton = createTopIconButton("About Qtiker");
    connect(infoButton, &QPushButton::clicked, this, &Clicker::showInfo);

    settingsButton = createTopIconButton("Settings");
    connect(settingsButton, &QPushButton::clicked, this, &Clicker::showSettings);
    applyThemeIcons();

    caratButton = new QPushButton(this);
    caratButton->setIcon(QIcon(":/assets/ui/carat.png"));
    caratButton->setIconSize(CaratIconSize);
    caratButton->setMinimumSize(86, TopIconButtonSize.height());
    caratButton->setToolTip("Carat");
    caratButton->setFocusPolicy(Qt::NoFocus);
    connect(caratButton, &QPushButton::clicked, this, &Clicker::showCarat);

    auto *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(TopBarSpacing);
    topLayout->addWidget(caratButton);
    topLayout->addStretch();
    topLayout->addWidget(changelogButton);
    topLayout->addWidget(infoButton);
    topLayout->addWidget(settingsButton);

    scoreLabel = new QLabel(this);
    scoreLabel->setAlignment(Qt::AlignCenter);
    setFont(scoreLabel, 30, true);

    statsLabel = new QLabel(this);
    statsLabel->setAlignment(Qt::AlignCenter);
    setFont(statsLabel, statsLabel->font().pointSize(), true);

    archLabel = new QLabel(this);
    archLabel->setAlignment(Qt::AlignCenter);
    archLabel->setFrameShape(QFrame::StyledPanel);
    archLabel->setMargin(4);
    setFont(archLabel, archLabel->font().pointSize(), true);

    clickButton = new QPushButton("Click", this);
    clickButton->setMinimumHeight(76);
    setFont(clickButton, 18, true);
    clickButton->installEventFilter(this);
    connect(clickButton, &QPushButton::clicked, this, &Clicker::makeClick);

    gachaButton = new QPushButton("Gacha", this);
    gachaButton->setMinimumHeight(32);
    connect(gachaButton, &QPushButton::clicked, this, &Clicker::showGacha);

    auto *upgradesBox = createUpgradesBox();

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(scoreLabel);
    mainLayout->addWidget(statsLabel);
    mainLayout->addWidget(archLabel);
    mainLayout->addWidget(clickButton, 1);
    mainLayout->addWidget(gachaButton);
    mainLayout->addWidget(upgradesBox);
}

QPushButton *Clicker::createTopIconButton(const QString &toolTip) {
    auto *button = new QPushButton(this);
    button->setIconSize(TopIconSize);
    button->setToolTip(toolTip);
    button->setFixedSize(TopIconButtonSize);
    return button;
}

QFrame *Clicker::createUpgradesBox() {
    auto *box = new QFrame(this);
    box->setFrameShape(QFrame::StyledPanel);

    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(PanelMargin, PanelMargin, PanelMargin, PanelMargin);
    layout->setSpacing(DialogSpacing);

    auto *title = new QLabel("Upgrades", box);
    setFont(title, title->font().pointSize(), true);

    clickUpgradeButton = new QPushButton(box);
    incomeUpgradeButton = new QPushButton(box);

    connect(clickUpgradeButton, &QPushButton::clicked, this, &Clicker::buyClickUpgrade);
    connect(incomeUpgradeButton, &QPushButton::clicked, this, &Clicker::buyIncomeUpgrade);

    layout->addWidget(title);
    layout->addWidget(clickUpgradeButton);
    layout->addWidget(incomeUpgradeButton);

    return box;
}

void Clicker::startIncomeTimer() {
    incomeTimer = new QTimer(this);
    incomeTimer->setInterval(1000);
    connect(incomeTimer, &QTimer::timeout, this, &Clicker::addPassiveIncome);
    incomeTimer->start();

    clickEffectCheckTimer = new QTimer(this);
    clickEffectCheckTimer->setInterval(ClickEffectCheckMs);
    connect(clickEffectCheckTimer, &QTimer::timeout, this, &Clicker::maybeStartClickEffect);
    clickEffectCheckTimer->start();

    clickEffectTimer = new QTimer(this);
    clickEffectTimer->setInterval(ClickEffectFrameMs);
    connect(clickEffectTimer, &QTimer::timeout, this, &Clicker::updateClickEffect);
}

void Clicker::applyThemeIcons() {
    setThemeIcon(changelogButton, ":/assets/ui/inbox.svg");
    setThemeIcon(settingsButton, ":/assets/ui/settings.svg");
    setThemeIcon(infoButton, ":/assets/ui/info.svg");
}

void Clicker::setThemeIcon(QPushButton *button, const QString &path) {
    if (button == nullptr) {
        return;
    }

    const auto iconColor = button->palette().color(QPalette::ButtonText);
    button->setIcon(tintedSvgIcon(path, iconColor, TopIconSize));
}

void Clicker::refreshUi() {
    const auto displayedClick = applyTimedBuffBonuses(
        applyActiveCardBonus(game.perClick, GachaEffect::Click),
        TimedBuffEffect::Click
    );
    const auto displayedIncome = applyTimedBuffBonuses(
        applyActiveCardBonus(game.perSecond, GachaEffect::Income),
        TimedBuffEffect::Income
    );

    scoreLabel->setText(formatNumber(game.score));
    statsLabel->setText(QString("Click +%1 · Income +%2/sec")
                            .arg(formatNumber(displayedClick))
                            .arg(formatNumber(displayedIncome)));

    archLabel->setText(QString("Arch's %1").arg(formatNumber(game.arches)));
    const auto progressToNextArch = game.nextArchAt > game.archProgress
        ? game.nextArchAt - game.archProgress
        : qint64{0};
    archLabel->setToolTip(QString("Next Arch in %1 score (%2/%3)")
                               .arg(formatNumber(progressToNextArch))
                               .arg(formatNumber(game.archProgress))
                               .arg(formatNumber(game.nextArchAt)));
    caratButton->setText(QString("- %1").arg(formatNumber(game.carats)));

    clickUpgradeButton->setText(formatUpgradeText("Click +1", game.clickCost));
    incomeUpgradeButton->setText(formatUpgradeText("Income +1/sec", game.incomeCost));
    gachaButton->setText(QString("Gacha  %1 Arch").arg(formatNumber(1)));

    clickUpgradeButton->setEnabled(game.score >= game.clickCost);
    incomeUpgradeButton->setEnabled(game.score >= game.incomeCost);
}

void Clicker::loadGame() {
    QSettings settings("qtiker", "qtiker");
    game.score = settings.value(SettingsKeys::Score, game.score).toLongLong();
    game.archProgress = settings.value(
        SettingsKeys::ArchProgress,
        settings.value(SettingsKeys::LegacyTotalClicks, game.archProgress)
    ).toLongLong();
    game.nextArchAt = settings.value(SettingsKeys::NextArchAt, game.nextArchAt).toLongLong();
    game.perClick = settings.value(
        SettingsKeys::PerClick,
        settings.value(SettingsKeys::LegacyClickPower, game.perClick)
    ).toInt();
    game.perSecond = settings.value(
        SettingsKeys::PerSecond,
        settings.value(SettingsKeys::LegacyAutoPower, game.perSecond)
    ).toInt();
    game.arches = settings.value(SettingsKeys::Arches, game.arches).toInt();
    game.carats = settings.value(SettingsKeys::Carats, game.carats).toLongLong();
    game.totalClicks = settings.value(SettingsKeys::TotalClicks, game.totalClicks).toLongLong();
    game.totalScoreEarned = settings.value(
        SettingsKeys::TotalScoreEarned,
        settings.value(SettingsKeys::LegacyTotalClickScoreEarned, game.totalScoreEarned)
    ).toLongLong();
    game.totalPlaySeconds = settings.value(SettingsKeys::TotalPlaySeconds, game.totalPlaySeconds).toLongLong();
    game.totalArchesEarned = settings.value(SettingsKeys::TotalArchesEarned, game.totalArchesEarned).toLongLong();
    game.clickButtonRightClicks = settings.value(
        SettingsKeys::ClickButtonRightClicks,
        game.clickButtonRightClicks
    ).toLongLong();
    for (int index = 0; index < TimedBuffCount; ++index) {
        game.buffExpiresAtMs[index] = settings.value(
            QString("%1%2").arg(SettingsKeys::BuffExpiresAtPrefix).arg(index),
            game.buffExpiresAtMs[index]
        ).toLongLong();
    }
    game.selectedCard = settings.value(SettingsKeys::SelectedCard, game.selectedCard).toInt();
    for (int index = 0; index < GachaCardCount; ++index) {
        game.cardCounts[index] = settings.value(QString("card%1").arg(index), game.cardCounts[index]).toInt();
    }

    if (game.selectedCard < 0
        || game.selectedCard >= GachaCardCount
        || game.cardCounts[game.selectedCard] <= 0) {
        game.selectedCard = -1;
    }

    game.clickCost = settings.value(
        SettingsKeys::ClickCost,
        settings.value(SettingsKeys::LegacyClickUpgradeCost, game.clickCost)
    ).toInt();
    game.incomeCost = settings.value(
        SettingsKeys::IncomeCost,
        settings.value(SettingsKeys::LegacyAutoUpgradeCost, game.incomeCost)
    ).toInt();
}

void Clicker::saveGame() {
    QSettings settings("qtiker", "qtiker");
    game.totalPlaySeconds = currentTotalPlaySeconds();
    playTimer.restart();

    settings.setValue(SettingsKeys::Score, game.score);
    settings.setValue(SettingsKeys::ArchProgress, game.archProgress);
    settings.setValue(SettingsKeys::NextArchAt, game.nextArchAt);
    settings.setValue(SettingsKeys::PerClick, game.perClick);
    settings.setValue(SettingsKeys::PerSecond, game.perSecond);
    settings.setValue(SettingsKeys::Arches, game.arches);
    settings.setValue(SettingsKeys::Carats, game.carats);
    settings.setValue(SettingsKeys::TotalClicks, game.totalClicks);
    settings.setValue(SettingsKeys::TotalScoreEarned, game.totalScoreEarned);
    settings.setValue(SettingsKeys::TotalPlaySeconds, currentTotalPlaySeconds());
    settings.setValue(SettingsKeys::TotalArchesEarned, game.totalArchesEarned);
    settings.setValue(SettingsKeys::ClickButtonRightClicks, game.clickButtonRightClicks);
    for (int index = 0; index < TimedBuffCount; ++index) {
        settings.setValue(
            QString("%1%2").arg(SettingsKeys::BuffExpiresAtPrefix).arg(index),
            game.buffExpiresAtMs[index]
        );
    }
    settings.setValue(SettingsKeys::SelectedCard, game.selectedCard);
    for (int index = 0; index < GachaCardCount; ++index) {
        settings.setValue(QString("card%1").arg(index), game.cardCounts[index]);
    }
    settings.setValue(SettingsKeys::ClickCost, game.clickCost);
    settings.setValue(SettingsKeys::IncomeCost, game.incomeCost);
}

qint64 Clicker::currentTotalPlaySeconds() const {
    if (!playTimer.isValid()) {
        return game.totalPlaySeconds;
    }

    return game.totalPlaySeconds + playTimer.elapsed() / 1000;
}

void Clicker::addArchProgress(qint64 amount) {
    if (amount <= 0) {
        return;
    }

    game.archProgress += amount;
    while (game.archProgress >= game.nextArchAt) {
        game.arches += 1;
        game.totalArchesEarned += 1;
        game.nextArchAt = nextArchThreshold(game.nextArchAt);
    }
}

void Clicker::maybeStartClickEffect() {
    if (clickEffectTimer->isActive()) {
        return;
    }

    if (QRandomGenerator::global()->bounded(100) >= ClickEffectChance) {
        return;
    }

    clickEffectFramesLeft = ClickEffectFrames;
    clickEffectHue = QRandomGenerator::global()->bounded(360);
    clickEffectTimer->start();
    updateClickEffect();
}

void Clicker::updateClickEffect() {
    if (clickEffectFramesLeft <= 0) {
        stopClickEffect();
        return;
    }

    const auto color = QColor::fromHsv(clickEffectHue, 180, 230);
    clickButton->setStyleSheet(
        QString(
            "QPushButton {"
            "  border: 2px solid rgb(%1, %2, %3);"
            "  border-radius: 6px;"
            "}"
        ).arg(color.red()).arg(color.green()).arg(color.blue())
    );
    clickEffectHue = (clickEffectHue + 18) % 360;
    clickEffectFramesLeft -= 1;
}

void Clicker::stopClickEffect() {
    clickEffectTimer->stop();
    clickEffectFramesLeft = 0;
    clickButton->setStyleSheet(QString());
}

void Clicker::rollGacha(GachaDialog *dialog) {
    if (game.arches < 1) {
        dialog->setArchCount(game.arches);
        dialog->showMessage("Need 1 Arch to roll.");
        return;
    }

    game.arches -= 1;
    int roll = QRandomGenerator::global()->bounded(debugGachaTotalWeight());
    int index = 0;
    for (; index < debugGachaCardCount(); ++index) {
        roll -= debugGachaCardAt(index).dropWeight;
        if (roll < 0) {
            break;
        }
    }
    if (index >= debugGachaCardCount()) {
        index = debugGachaCardCount() - 1;
    }

    game.cardCounts[index] += 1;
    if (game.selectedCard == -1) {
        game.selectedCard = index;
    }

    dialog->setArchCount(game.arches);
    dialog->setInventory(game.cardCounts, game.selectedCard);
    dialog->showCard(debugGachaCardAt(index), game.cardCounts[index]);

    saveGame();
    refreshUi();
}

void Clicker::selectGachaCard(GachaDialog *dialog, int index) {
    if (index < 0 || index >= GachaCardCount || game.cardCounts[index] <= 0) {
        dialog->showMessage("You do not own this card.");
        return;
    }

    game.selectedCard = index;
    dialog->setInventory(game.cardCounts, game.selectedCard);
    dialog->showMessage(QString("Selected %1.").arg(debugGachaCardAt(index).name));
    saveGame();
}

qint64 Clicker::applyActiveCardBonus(qint64 value, GachaEffect effect) const {
    if (game.selectedCard < 0 || game.selectedCard >= GachaCardCount) {
        return value;
    }

    const auto card = debugGachaCardAt(game.selectedCard);
    if (card.effect != effect) {
        return value;
    }

    return applyStackedMultiplier(
        value,
        card.multiplierNumerator,
        card.multiplierDenominator,
        game.cardCounts[game.selectedCard]
    );
}

qint64 Clicker::applyTimedBuffBonuses(qint64 value, TimedBuffEffect effect) const {
    qint64 result = value;

    for (const auto &rule : TimedBuffRules) {
        if (rule.effect != effect || !isTimedBuffActive(rule.buff)) {
            continue;
        }

        result = applyMultiplierRoundedUp(
            result,
            rule.multiplierNumerator,
            rule.multiplierDenominator
        );
    }

    return result;
}

bool Clicker::isTimedBuffActive(TimedBuff buff) const {
    return timedBuffSecondsLeft(buff) > 0;
}

int Clicker::timedBuffSecondsLeft(TimedBuff buff) const {
    const int index = timedBuffIndex(buff);
    if (index < 0 || index >= TimedBuffCount) {
        return 0;
    }

    const qint64 remainingMs = game.buffExpiresAtMs[index] - QDateTime::currentMSecsSinceEpoch();
    if (remainingMs <= 0) {
        return 0;
    }

    return static_cast<int>((remainingMs + 999) / 1000);
}

void Clicker::activateTimedBuff(TimedBuff buff, int durationSeconds) {
    const int index = timedBuffIndex(buff);
    if (index < 0 || index >= TimedBuffCount || durationSeconds <= 0) {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 startAt = game.buffExpiresAtMs[index] > now ? game.buffExpiresAtMs[index] : now;
    game.buffExpiresAtMs[index] = startAt + qint64{durationSeconds} * 1000;
}

QString Clicker::formatNumber(qint64 value) const {
    if (value < 10000) {
        return QLocale::system().toString(value);
    }

    double scaled = static_cast<double>(value);
    int suffixIndex = 0;
    const int maxSuffixIndex = static_cast<int>(std::size(NumberSuffixes)) - 1;

    while (scaled >= 1000.0 && suffixIndex < maxSuffixIndex) {
        scaled /= 1000.0;
        suffixIndex += 1;
    }

    const int precision = scaled >= 100.0 ? 0 : 1;
    return QString("%1%2").arg(scaled, 0, 'f', precision).arg(NumberSuffixes[suffixIndex]);
}

QString Clicker::formatDuration(qint64 seconds) const {
    const qint64 hours = seconds / 3600;
    seconds %= 3600;
    const qint64 minutes = seconds / 60;
    seconds %= 60;

    if (hours > 0) {
        return QString("%1h %2m %3s").arg(hours).arg(minutes).arg(seconds);
    }
    if (minutes > 0) {
        return QString("%1m %2s").arg(minutes).arg(seconds);
    }

    return QString("%1s").arg(seconds);
}

QString Clicker::formatUpgradeText(const QString &label, int cost) const {
    return QString("%1  %2").arg(label, formatNumber(cost));
}

QIcon Clicker::tintedSvgIcon(const QString &path, const QColor &color, const QSize &size) const {
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    QSvgRenderer renderer(path);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    painter.end();

    return QIcon(pixmap);
}

QString Clicker::tintedSvgDataUri(const QString &path, const QColor &color, const QSize &size) const {
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    QSvgRenderer renderer(path);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(image.rect(), color);
    painter.end();

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    return QString("data:image/png;base64,%1").arg(QString::fromLatin1(bytes.toBase64()));
}

QString Clicker::changelogHtml() const {
    const auto addIcon = tintedSvgDataUri(":/assets/ui/add.svg", QColor("#2e7d32"), QSize(14, 14));
    const auto changedIcon = tintedSvgDataUri(":/assets/ui/changed.svg", QColor("#ef6c00"), QSize(13, 13));

    QString html = QStringLiteral(R"(
        <style>
            body { font-family: sans-serif; font-size: 10pt; }
            h3 { margin: 8px 0 5px 0; }
            table { border-collapse: collapse; margin-bottom: 4px; }
            td { padding: 2px 0; vertical-align: middle; }
            .icon { width: 22px; }
            .version { font-weight: 700; }
            .date { color: #777; font-size: 9pt; }
            .entry { padding-left: 2px; }
        </style>
    )");

    for (const auto &release : ChangelogReleases) {
        html += QString(
            R"(<h3><span class="version">%1</span> <span class="date">%2</span></h3><table>)"
        ).arg(release.version, release.date);

        for (qsizetype index = 0; index < release.entryCount; ++index) {
            const auto &entry = release.entries[index];
            const bool isAdded = entry.icon == ChangelogIcon::Added;
            html += QString(
                R"(<tr><td class="icon"><img src="%1" width="%2" height="%2"></td><td class="entry">%3</td></tr>)"
            ).arg(
                isAdded ? addIcon : changedIcon,
                QString::number(isAdded ? 14 : 13),
                QString::fromUtf8(entry.text)
            );
        }

        html += QStringLiteral("</table>");
    }

    return html;
}

void Clicker::setFont(QWidget *widget, int pointSize, bool bold) {
    auto font = widget->font();
    if (pointSize > 0) {
        font.setPointSize(pointSize);
    }
    font.setBold(bold);
    widget->setFont(font);
}
