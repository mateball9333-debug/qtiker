#include "clicker.h"

#include "appversion.h"
#include "gacha_dialog.h"
#include "game_rules.h"
#include "particle_overlay.h"
#include "release_notes.h"

#include <QApplication>
#include <QBuffer>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
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
#include <QScrollArea>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QSettings>
#include <QShortcut>
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
constexpr QSize ChangelogButtonSize(92, 28);
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
constexpr int ChangelogHighlightFrameMs = 140;
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
constexpr auto SelectedCard2 = "selectedCard2";
constexpr auto SecondCardSlotUnlocked = "secondCardSlotUnlocked";
constexpr auto SecondCardPenaltyUpgraded = "secondCardPenaltyUpgraded";
constexpr auto ClickCost = "clickCost";
constexpr auto IncomeCost = "incomeCost";
constexpr auto IncomeBuffEasterEgg = "incomeBuffEasterEgg";
constexpr auto LastSeenChangelogVersion = "lastSeenChangelogVersion";
constexpr auto BuffExpiresAtPrefix = "buffExpiresAt";
constexpr auto LegacyClickPower = "clickPower";
constexpr auto LegacyAutoPower = "autoPower";
constexpr auto LegacyClickUpgradeCost = "clickUpgradeCost";
constexpr auto LegacyAutoUpgradeCost = "autoUpgradeCost";
constexpr auto LegacyTotalClicks = "totalClicks";
constexpr auto LegacyTotalClickScoreEarned = "totalClickScoreEarned";
}

}

Clicker::Clicker(QWidget *parent) : QWidget(parent) {
    setupWindow();
    loadGame();
    playTimer.start();
    buildUi();
    startIncomeTimer();
    refreshUi();

    auto *spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    spaceShortcut->setAutoRepeat(false);
    connect(spaceShortcut, &QShortcut::activated, this, &Clicker::makeClick);
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

void Clicker::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (particleOverlay) {
        particleOverlay->resize(event->size());
    }
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

    if (watched->property("role") == "statsLink"
        && (event->type() == QEvent::ApplicationPaletteChange
            || event->type() == QEvent::PaletteChange
            || event->type() == QEvent::StyleChange)) {
        auto *label = qobject_cast<QLabel *>(watched);
        if (label) {
            const auto c = label->palette().color(QPalette::WindowText).name();
            label->setText(QStringLiteral(
                "<a href='#' style='text-decoration:none; color:%1;'>S</a>tatistics"
            ).arg(c));
            label->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        }
        return false;
    }

    if (watched->property("role") == "tuxLogo"
        && event->type() == QEvent::MouseButtonRelease) {
        auto *win = new QWidget(nullptr, Qt::Dialog);
        win->setAttribute(Qt::WA_DeleteOnClose);
        win->setWindowTitle("Tux");
        win->setWindowIcon(QIcon(":/assets/qtiker-64.png"));

        auto *layout = new QVBoxLayout(win);
        layout->setContentsMargins(0, 0, 0, 0);

        auto *img = new QLabel(win);
        img->setAlignment(Qt::AlignCenter);
        img->setPixmap(QPixmap(":/assets/qtiker.png"));
        img->setCursor(Qt::ArrowCursor);

        layout->addWidget(img);
        win->show();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void Clicker::makeClick() {
    const auto cardEarned = applyActiveCardBonus(game.perClick, GachaEffect::Click);
    const auto buffed = applyTimedBuffBonuses(cardEarned, TimedBuffEffect::Click);

    const int roll = QRandomGenerator::global()->bounded(100);
    qint64 earned;
    if (roll < 1) {
        earned = buffed * 5;
        triggerCritBurst(true);
    } else if (roll < 6) {
        earned = buffed * 2;
        triggerCritBurst(false);
    } else {
        earned = buffed;
    }

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
    const auto buffed = applyTimedBuffBonuses(cardEarned, TimedBuffEffect::Income);
    const auto earned = game.incomeBuffEasterEgg ? applyMultiplierRoundedUp(buffed, 11, 10) : buffed;
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

    auto *statisticsText = new QLabel(statisticsBox);
    setFont(statisticsText, statisticsText->font().pointSize(), true);
    statisticsText->setTextFormat(Qt::RichText);
    statisticsText->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    statisticsText->setProperty("role", "statsLink");
    statisticsText->installEventFilter(this);
    const auto refreshLink = [](QLabel *label) {
        if (!label) return;
        const auto c = label->palette().color(QPalette::WindowText).name();
        label->setText(QStringLiteral(
            "<a href='#' style='text-decoration:none; color:%1;'>S</a>tatistics"
        ).arg(c));
    };
    refreshLink(statisticsText);
    connect(statisticsText, &QLabel::linkActivated, this, &Clicker::showAssets);

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

    QSettings eeSettings("qtiker", "qtiker");
    const bool easterEggFound = eeSettings.value("easterEggFound", false).toBool();

    QFrame *buffBox = nullptr;
    if (easterEggFound) {
        buffBox = new QFrame(dialog);
        buffBox->setFrameShape(QFrame::StyledPanel);

        auto *buffLayout = new QHBoxLayout(buffBox);
        buffLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
        buffLayout->setSpacing(DialogSpacing);

        auto *buffText = new QLabel("Income +10% buff", buffBox);
        setFont(buffText, buffText->font().pointSize(), true);

        auto *buffButton = new QPushButton(
            game.incomeBuffEasterEgg ? "ON" : "OFF", buffBox);
        connect(buffButton, &QPushButton::clicked, this,
                [this, buffButton]() {
                    game.incomeBuffEasterEgg = !game.incomeBuffEasterEgg;
                    buffButton->setText(game.incomeBuffEasterEgg ? "ON" : "OFF");
                    saveGame();
                    refreshUi();
                });

        buffLayout->addWidget(buffText);
        buffLayout->addStretch();
        buffLayout->addWidget(buffButton);
    }

    auto *justBox = new QFrame(dialog);
    justBox->setFrameShape(QFrame::StyledPanel);

    auto *justLayout = new QHBoxLayout(justBox);
    justLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    justLayout->setSpacing(DialogSpacing);

    auto *justText = new QLabel("Just text", justBox);
    setFont(justText, justText->font().pointSize(), true);

    auto *justButton = new QPushButton("Just button", justBox);
    auto *justGlowEffect = setButtonGlow(justButton, nullptr, QColor::fromHsv(0, 210, 245));
    auto *justTextState = new TextEffectState();
    auto *justMode = new int(0);
    connect(justBox, &QObject::destroyed, this, [justTextState, justMode]() {
        delete justTextState;
        delete justMode;
    });

    auto *justButtonTimer = new QTimer(justButton);
    justButtonTimer->setInterval(ChangelogHighlightFrameMs);
    connect(justButtonTimer, &QTimer::timeout, this, [this, justButton, justText, justTextState, justGlowEffect, justMode, justButtonHue = 18]() mutable {
        const auto color = QColor::fromHsv(justButtonHue, 210, 245);
        if (*justMode == 0) {
            justGlowEffect->setColor(color);
        } else if (*justMode == 1) {
            applyTextEffect(justText, *justTextState, TextEffectMode::RainbowGlow);
        } else if (*justMode == 2) {
            applyTextEffect(justText, *justTextState, TextEffectMode::RainbowFill);
        }
        justButtonHue = (justButtonHue + 18) % 360;
    });
    justButtonTimer->start();

    connect(justButton, &QPushButton::clicked, this, [this, justButton, justText, justTextState, justGlowEffect, justMode]() {
        *justMode = QRandomGenerator::global()->bounded(5);

        clearTextEffect(justText, *justTextState);
        justGlowEffect->setEnabled(false);

        if (*justMode == 0) {
            justButton->setText("Button glow");
            justGlowEffect->setColor(QColor::fromHsv(0, 210, 245));
            justGlowEffect->setEnabled(true);
        } else if (*justMode == 1) {
            justButton->setText("Text glow");
            applyTextEffect(justText, *justTextState, TextEffectMode::RainbowGlow);
        } else if (*justMode == 2) {
            justButton->setText("Text rainbow");
            applyTextEffect(justText, *justTextState, TextEffectMode::RainbowFill);
        } else if (*justMode == 3) {
            justButton->setText("Text fill");
            applyTextEffect(justText, *justTextState, TextEffectMode::SolidFill, QColor("#2e7d32"));
        } else {
            justButton->setText("Just button");
        }
    });

    justLayout->addWidget(justText);
    justLayout->addStretch();
    justLayout->addWidget(justButton);

    auto *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    layout->addWidget(title);
    layout->addWidget(resetBox);
    layout->addWidget(statisticsBox);
    layout->addWidget(modeBox);
    if (buffBox) {
        layout->addWidget(buffBox);
    }
    layout->addWidget(justBox);
    layout->addStretch();
    layout->addWidget(closeButton);

    dialog->show();

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
    markChangelogSeen();

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
    logo->setCursor(Qt::PointingHandCursor);
    logo->installEventFilter(this);
    logo->setProperty("role", "tuxLogo");

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
    dialog->setSecondCardSlotEnabled(game.secondCardSlotUnlocked);
    dialog->setSecondCardPenaltyUpgraded(game.secondCardPenaltyUpgraded);
    dialog->setInventory(game.cardCounts, game.selectedCard, game.selectedCard2);
    connect(dialog, &GachaDialog::rollRequested, this, [this, dialog]() {
        rollGacha(dialog);
    });
    connect(dialog, &GachaDialog::cardSelected, this, [this, dialog](int index) {
        selectGachaCard(dialog, index);
    });
    connect(dialog, &GachaDialog::cardSelectedForSlot2, this, [this, dialog](int index) {
        selectGachaCard2(dialog, index);
    });
    connect(dialog, &GachaDialog::slotChanged, this, [this, dialog](int) {
        dialog->setInventory(game.cardCounts, game.selectedCard, game.selectedCard2);
        dialog->setSecondCardPenaltyUpgraded(game.secondCardPenaltyUpgraded);
    });
    dialog->show();
}

void Clicker::showCarat() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Carat");
    dialog->setWindowIcon(QIcon(":/assets/ui/carat.png"));
    dialog->resize(340, 280);

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

    auto *burnX10Button = new QPushButton(dialog);
    burnX10Button->setIcon(QIcon(":/assets/ui/carat.png"));
    burnX10Button->setIconSize(CaratIconSize);

    auto *burnRow = new QHBoxLayout();
    burnRow->setContentsMargins(0, 0, 0, 0);
    burnRow->setSpacing(DialogSpacing);
    burnRow->addWidget(burnButton, 1);
    burnRow->addWidget(burnX10Button);

    auto *buffBox = new QFrame(dialog);
    buffBox->setFrameShape(QFrame::StyledPanel);

    auto *buffLayout = new QVBoxLayout(buffBox);
    buffLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    buffLayout->setSpacing(DialogSpacing);

    std::array<QLabel *, TimedBuffCount> buffStatusLabels = {};
    std::array<QPushButton *, TimedBuffCount> buyBuffButtons = {};
    std::array<QPushButton *, TimedBuffCount> buyBuffX10Buttons = {};

    for (int index = 0; index < TimedBuffCount; ++index) {
        const auto &rule = TimedBuffRules[index];

        auto *buffTitle = new QLabel(rule.name, buffBox);
        setFont(buffTitle, buffTitle->font().pointSize(), true);

        buffStatusLabels[index] = new QLabel(buffBox);
        buffStatusLabels[index]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        buyBuffButtons[index] = new QPushButton(buffBox);
        buyBuffX10Buttons[index] = new QPushButton(buffBox);

        auto *buffHeaderRow = new QHBoxLayout();
        buffHeaderRow->setContentsMargins(0, 0, 0, 0);
        buffHeaderRow->setSpacing(DialogSpacing);
        buffHeaderRow->addWidget(buffTitle);
        buffHeaderRow->addWidget(buffStatusLabels[index], 1);

        auto *buffButtonRow = new QHBoxLayout();
        buffButtonRow->setContentsMargins(0, 0, 0, 0);
        buffButtonRow->setSpacing(DialogSpacing);
        buffButtonRow->addWidget(buyBuffButtons[index], 1);
        buffButtonRow->addWidget(buyBuffX10Buttons[index]);

        buffLayout->addLayout(buffHeaderRow);
        buffLayout->addLayout(buffButtonRow);
    }

    auto *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    auto *slot2BuyButton = new QPushButton(dialog);
    auto *penaltyUpgradeButton = new QPushButton(dialog);

    const auto updateDialog = [this, caratBalanceLabel, clickBalanceLabel, burnButton, burnX10Button, buffStatusLabels, buyBuffButtons, buyBuffX10Buttons, slot2BuyButton, penaltyUpgradeButton]() {
        caratBalanceLabel->setText(QString("Carat %1").arg(formatNumber(game.carats)));
        clickBalanceLabel->setText(QString("%1 clicks").arg(formatNumber(game.score)));

        burnButton->setText(QString("Burn %1 clicks  +%2 Carat")
                                .arg(formatNumber(CaratBurnCost))
                                .arg(formatNumber(CaratBurnReward)));
        burnButton->setEnabled(game.score >= CaratBurnCost);

        constexpr qint64 burnX10Cost = CaratBurnCost * 10;
        constexpr qint64 burnX10Reward = CaratBurnReward * 10;
        burnX10Button->setText(QString("x10  +%1").arg(formatNumber(burnX10Reward)));
        burnX10Button->setEnabled(game.score >= burnX10Cost);
        burnX10Button->setToolTip(QString("Burn %1 clicks for %2 Carat")
                                      .arg(formatNumber(burnX10Cost))
                                      .arg(formatNumber(burnX10Reward)));

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

            const qint64 x10Cost = rule.caratCost * 10;
            const int x10Duration = rule.durationSeconds * 10;
            buyBuffX10Buttons[index]->setText(QString("x10  +%1").arg(formatNumber(x10Cost)));
            buyBuffX10Buttons[index]->setEnabled(game.carats >= x10Cost);
            buyBuffX10Buttons[index]->setToolTip(QString("%1 for %2s  %3 Carat")
                                                     .arg(rule.name)
                                                     .arg(x10Duration)
                                                     .arg(formatNumber(x10Cost)));
        }

        if (game.secondCardSlotUnlocked) {
            slot2BuyButton->setText("Second card slot: unlocked");
            slot2BuyButton->setEnabled(false);
        } else {
            slot2BuyButton->setText(QString("Unlock second card slot  %1 Carat")
                                        .arg(formatNumber(SecondCardSlotCost)));
            slot2BuyButton->setEnabled(game.carats >= SecondCardSlotCost);
        }

        if (game.secondCardSlotUnlocked) {
            if (game.secondCardPenaltyUpgraded) {
                penaltyUpgradeButton->setText("Penalty reduced to 33%");
                penaltyUpgradeButton->setEnabled(false);
            } else {
                penaltyUpgradeButton->setText(QString("Reduce penalty 55% → 33%  %1 Carat")
                                                  .arg(formatNumber(PenaltyUpgradeCost)));
                penaltyUpgradeButton->setEnabled(game.carats >= PenaltyUpgradeCost);
            }
            penaltyUpgradeButton->setVisible(true);
        } else {
            penaltyUpgradeButton->setVisible(false);
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

    connect(burnX10Button, &QPushButton::clicked, this, [this, updateDialog]() {
        constexpr qint64 cost = CaratBurnCost * 10;
        constexpr qint64 reward = CaratBurnReward * 10;
        if (game.score < cost) {
            return;
        }

        game.score -= cost;
        game.carats += reward;
        saveGame();
        refreshUi();
        updateDialog();
    });

    connect(slot2BuyButton, &QPushButton::clicked, this, [this, updateDialog]() {
        if (game.carats < SecondCardSlotCost || game.secondCardSlotUnlocked) {
            return;
        }

        game.carats -= SecondCardSlotCost;
        game.secondCardSlotUnlocked = true;
        saveGame();
        refreshUi();
        updateDialog();
    });

    connect(penaltyUpgradeButton, &QPushButton::clicked, this, [this, updateDialog]() {
        if (game.carats < PenaltyUpgradeCost || game.secondCardPenaltyUpgraded) {
            return;
        }

        game.carats -= PenaltyUpgradeCost;
        game.secondCardPenaltyUpgraded = true;
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
        connect(buyBuffX10Buttons[index], &QPushButton::clicked, this, [this, updateDialog, rule]() {
            const qint64 cost = rule.caratCost * 10;
            const int duration = rule.durationSeconds * 10;
            if (game.carats < cost) {
                return;
            }

            game.carats -= cost;
            activateTimedBuff(rule.buff, duration);
            saveGame();
            refreshUi();
            updateDialog();
        });
    }

    layout->addWidget(balanceBox);
    layout->addLayout(burnRow);
    layout->addWidget(buffBox);
    layout->addWidget(slot2BuyButton);
    layout->addWidget(penaltyUpgradeButton);
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

void Clicker::showAssets() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Assets");
    dialog->setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    dialog->resize(400, 440);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel("Game Assets", dialog);
    setFont(title, 13, true);

    auto *scrollArea = new QScrollArea(dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::StyledPanel);

    auto *scrollContent = new QWidget(scrollArea);
    auto *flow = new QVBoxLayout(scrollContent);
    flow->setSpacing(2);
    flow->setContentsMargins(4, 4, 4, 4);

    QStringList filters = {"*.svg", "*.png"};
    QDir assetsDir(":/assets/ui");
    const auto files = assetsDir.entryList(filters, QDir::Files, QDir::Name);

    for (const auto &file : files) {
        const QString path = QString(":/assets/ui/%1").arg(file);
        const bool isSvg = file.endsWith(".svg", Qt::CaseInsensitive);

        auto *row = new QFrame(scrollContent);
        row->setFrameShape(QFrame::StyledPanel);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(6, 3, 6, 3);
        rowLayout->setSpacing(8);

        auto *icon = new QLabel(row);
        icon->setFixedSize(24, 24);
        icon->setAlignment(Qt::AlignCenter);

        if (isSvg) {
            QPixmap pixmap(24, 24);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            QSvgRenderer renderer(path);
            renderer.render(&painter, QRectF(0, 0, 24, 24));
            painter.end();
            icon->setPixmap(pixmap);
        } else {
            QPixmap pm(path);
            icon->setPixmap(pm.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        auto *name = new QLabel(file, row);
        name->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        setFont(name, name->font().pointSize(), false);

        QFileInfo fi(path);
        auto *sizeLabel = new QLabel(
            QString("%1 B").arg(fi.size()), row);
        sizeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        setFont(sizeLabel, 8, false);

        rowLayout->addWidget(icon);
        rowLayout->addWidget(name, 1);
        rowLayout->addWidget(sizeLabel);

        flow->addWidget(row);
    }

    scrollArea->setWidget(scrollContent);

    auto *countLabel = new QLabel(QString("Total assets: %1").arg(files.size()), dialog);
    setFont(countLabel, countLabel->font().pointSize(), true);

    auto *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    layout->addWidget(title);
    layout->addWidget(scrollArea, 1);
    layout->addWidget(countLabel);
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

    changelogButton = new QPushButton(QString("  v%1").arg(AppVersion), this);
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

    auto *statsLayout = new QHBoxLayout();
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(0);

    clickStatsLabel = new QLabel(this);
    clickStatsLabel->setAlignment(Qt::AlignCenter);
    setFont(clickStatsLabel, clickStatsLabel->font().pointSize(), true);

    auto *separator = new QLabel(" · ", this);
    separator->setAlignment(Qt::AlignCenter);
    setFont(separator, separator->font().pointSize(), true);

    incomeStatsLabel = new QLabel(this);
    incomeStatsLabel->setAlignment(Qt::AlignCenter);
    setFont(incomeStatsLabel, incomeStatsLabel->font().pointSize(), true);

    statsLayout->addStretch();
    statsLayout->addWidget(clickStatsLabel);
    statsLayout->addWidget(separator);
    statsLayout->addWidget(incomeStatsLabel);
    statsLayout->addStretch();

    archLabel = new QLabel(this);
    archLabel->setAlignment(Qt::AlignCenter);
    archLabel->setFrameShape(QFrame::StyledPanel);
    archLabel->setMargin(4);
    setFont(archLabel, archLabel->font().pointSize(), true);

    clickButton = new QPushButton("Click", this);
    clickButton->setFocusPolicy(Qt::NoFocus);
    clickButton->setMinimumHeight(76);
    setFont(clickButton, 18, true);
    clickButton->installEventFilter(this);
    connect(clickButton, &QPushButton::clicked, this, &Clicker::makeClick);

    particleOverlay = new ParticleOverlay(this);

    gachaButton = new QPushButton("Gacha", this);
    gachaButton->setMinimumHeight(32);
    connect(gachaButton, &QPushButton::clicked, this, &Clicker::showGacha);

    auto *upgradesBox = createUpgradesBox();

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(scoreLabel);
    mainLayout->addLayout(statsLayout);
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

    changelogHighlightTimer = new QTimer(this);
    changelogHighlightTimer->setInterval(ChangelogHighlightFrameMs);
    connect(changelogHighlightTimer, &QTimer::timeout, this, &Clicker::updateChangelogHighlight);
    startChangelogHighlightIfNeeded();

    statsBuffGlowTimer = new QTimer(this);
    statsBuffGlowTimer->setInterval(ChangelogHighlightFrameMs);
    connect(statsBuffGlowTimer, &QTimer::timeout, this, &Clicker::updateStatsBuffGlow);
}

void Clicker::applyThemeIcons() {
    setThemeIcon(changelogButton, ":/assets/ui/release-notes.svg");
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

QGraphicsDropShadowEffect *Clicker::setButtonGlow(
    QPushButton *button,
    QGraphicsDropShadowEffect *effect,
    const QColor &color
) {
    if (button == nullptr) {
        return nullptr;
    }

    if (effect == nullptr) {
        effect = new QGraphicsDropShadowEffect(button);
        effect->setBlurRadius(12);
        effect->setOffset(0, 0);
    }
    if (button->graphicsEffect() != effect) {
        button->setGraphicsEffect(effect);
    }

    effect->setColor(color);
    effect->setEnabled(true);
    return effect;
}

void Clicker::clearButtonGlow(QPushButton *button, QGraphicsDropShadowEffect *effect) {
    if (button != nullptr && button->graphicsEffect() == effect) {
        button->setGraphicsEffect(nullptr);
    }
}

void Clicker::applyTextEffect(
    QLabel *label,
    TextEffectState &state,
    TextEffectMode mode,
    const QColor &fillColor
) {
    if (label == nullptr) {
        return;
    }

    if (!state.hasOriginalTextColor) {
        state.originalTextColor = label->palette().color(QPalette::WindowText);
        state.hasOriginalTextColor = true;
    }

    const bool useRainbow = mode == TextEffectMode::RainbowGlow || mode == TextEffectMode::RainbowFill;
    const auto color = useRainbow ? QColor::fromHsv(state.hue, 210, 245) : fillColor;

    if (mode == TextEffectMode::RainbowGlow) {
        auto palette = label->palette();
        palette.setColor(QPalette::WindowText, state.originalTextColor);
        label->setPalette(palette);

        if (state.glowEffect == nullptr) {
            state.glowEffect = new QGraphicsDropShadowEffect(label);
            state.glowEffect->setBlurRadius(12);
            state.glowEffect->setOffset(0, 0);
            label->setGraphicsEffect(state.glowEffect);
        }
        state.glowEffect->setColor(color);
        state.glowEffect->setEnabled(true);
    } else {
        if (label->graphicsEffect() == state.glowEffect) {
            label->setGraphicsEffect(nullptr);
        }
        state.glowEffect = nullptr;

        auto palette = label->palette();
        palette.setColor(QPalette::WindowText, color.isValid() ? color : state.originalTextColor);
        label->setPalette(palette);
    }

    if (useRainbow) {
        state.hue = (state.hue + 18) % 360;
    }
}

void Clicker::clearTextEffect(QLabel *label, TextEffectState &state) {
    if (label == nullptr) {
        return;
    }

    if (label->graphicsEffect() == state.glowEffect) {
        label->setGraphicsEffect(nullptr);
    }
    state.glowEffect = nullptr;

    if (state.hasOriginalTextColor) {
        auto palette = label->palette();
        palette.setColor(QPalette::WindowText, state.originalTextColor);
        label->setPalette(palette);
    }
}

QColor Clicker::activeCardColorForEffect(GachaEffect effect) const {
    const auto colorForCard = [this, effect](int index) -> QColor {
        if (index < 0 || index >= GachaCardCount || game.cardCounts[index] <= 0) {
            return {};
        }

        const auto card = debugGachaCardAt(index);
        return card.effect == effect ? card.color : QColor();
    };

    auto color = colorForCard(game.selectedCard);
    if (color.isValid()) {
        return color;
    }

    if (game.secondCardSlotUnlocked) {
        color = colorForCard(game.selectedCard2);
    }

    return color;
}

void Clicker::applyStatsTextColor(QLabel *label, TextEffectState &state, const QColor &accentColor) {
    if (label == nullptr) {
        return;
    }

    const auto textColor = accentColor.isValid()
        ? accentColor
        : QApplication::palette(label).color(QPalette::WindowText);

    state.originalTextColor = textColor;
    state.hasOriginalTextColor = true;

    auto palette = label->palette();
    palette.setColor(QPalette::WindowText, textColor);
    label->setPalette(palette);
}

void Clicker::startChangelogHighlightIfNeeded() {
    if (changelogSeenForVersion || changelogButton == nullptr || changelogHighlightTimer == nullptr) {
        return;
    }

    changelogHighlightHue = 0;
    changelogHighlightTimer->start();
    updateChangelogHighlight();
}

void Clicker::updateChangelogHighlight() {
    if (changelogButton == nullptr) {
        return;
    }

    const auto color = QColor::fromHsv(changelogHighlightHue, 210, 245);
    changelogGlowEffect = setButtonGlow(changelogButton, changelogGlowEffect, color);
    changelogHighlightHue = (changelogHighlightHue + 18) % 360;
}

void Clicker::stopChangelogHighlight() {
    if (changelogHighlightTimer != nullptr) {
        changelogHighlightTimer->stop();
    }
    clearButtonGlow(changelogButton, changelogGlowEffect);
    changelogGlowEffect = nullptr;
}

void Clicker::updateStatsBuffGlow() {
    const bool clickActive = isTimedBuffActive(TimedBuff::ClickGain);
    const bool incomeActive = isTimedBuffActive(TimedBuff::IncomeGain);

    applyStatsTextColor(clickStatsLabel, clickStatsBuffGlowState, activeCardColorForEffect(GachaEffect::Click));
    applyStatsTextColor(incomeStatsLabel, incomeStatsBuffGlowState, activeCardColorForEffect(GachaEffect::Income));

    if (clickActive) {
        applyTextEffect(clickStatsLabel, clickStatsBuffGlowState, TextEffectMode::RainbowGlow);
    } else {
        clearTextEffect(clickStatsLabel, clickStatsBuffGlowState);
    }

    if (incomeActive) {
        applyTextEffect(incomeStatsLabel, incomeStatsBuffGlowState, TextEffectMode::RainbowGlow);
    } else {
        clearTextEffect(incomeStatsLabel, incomeStatsBuffGlowState);
    }

    if (!clickActive && !incomeActive) {
        statsBuffGlowTimer->stop();
    }
}

void Clicker::markChangelogSeen() {
    if (changelogSeenForVersion) {
        return;
    }

    changelogSeenForVersion = true;
    QSettings settings("qtiker", "qtiker");
    settings.setValue(SettingsKeys::LastSeenChangelogVersion, AppVersion);
    stopChangelogHighlight();
}

void Clicker::refreshUi() {
    const auto displayedClick = applyTimedBuffBonuses(
        applyActiveCardBonus(game.perClick, GachaEffect::Click),
        TimedBuffEffect::Click
    );
    auto displayedIncome = applyTimedBuffBonuses(
        applyActiveCardBonus(game.perSecond, GachaEffect::Income),
        TimedBuffEffect::Income
    );
    if (game.incomeBuffEasterEgg) {
        displayedIncome = applyMultiplierRoundedUp(displayedIncome, 11, 10);
    }

    scoreLabel->setText(formatNumber(game.score));
    clickStatsLabel->setText(QString("Click +%1").arg(formatNumber(displayedClick)));
    incomeStatsLabel->setText(QString("Income +%1/sec").arg(formatNumber(displayedIncome)));
    applyStatsTextColor(clickStatsLabel, clickStatsBuffGlowState, activeCardColorForEffect(GachaEffect::Click));
    applyStatsTextColor(incomeStatsLabel, incomeStatsBuffGlowState, activeCardColorForEffect(GachaEffect::Income));

    const bool anyBuffActive = isTimedBuffActive(TimedBuff::IncomeGain)
        || isTimedBuffActive(TimedBuff::ClickGain);
    if (anyBuffActive && !statsBuffGlowTimer->isActive()) {
        statsBuffGlowTimer->start();
    } else if (!anyBuffActive && statsBuffGlowTimer->isActive()) {
        statsBuffGlowTimer->stop();
        clearTextEffect(clickStatsLabel, clickStatsBuffGlowState);
        clearTextEffect(incomeStatsLabel, incomeStatsBuffGlowState);
    }

    archLabel->setText(QString("Arch's %1").arg(formatNumber(game.arches)));
    const auto progressToNextArch = game.nextArchAt > game.archProgress
        ? game.nextArchAt - game.archProgress
        : qint64{0};
    archLabel->setToolTip(QString("Next Arch in %1 score (%2/%3)")
                               .arg(formatNumber(progressToNextArch))
                               .arg(formatNumber(game.archProgress))
                               .arg(formatNumber(game.nextArchAt)));
    caratButton->setText(QString("- %1").arg(formatNumber(game.carats)));

    const int clickUpgradesBought = game.perClick > 1 ? game.perClick - 1 : 0;
    const int incomeUpgradesBought = game.perSecond > 0 ? game.perSecond : 0;
    clickUpgradeButton->setText(formatUpgradeText("Click +1", clickUpgradesBought, game.clickCost));
    incomeUpgradeButton->setText(formatUpgradeText("Income +1/sec", incomeUpgradesBought, game.incomeCost));
    gachaButton->setText("Gacha");

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
    game.selectedCard2 = settings.value(SettingsKeys::SelectedCard2, game.selectedCard2).toInt();
    game.secondCardSlotUnlocked = settings.value(SettingsKeys::SecondCardSlotUnlocked, game.secondCardSlotUnlocked).toBool();
    game.secondCardPenaltyUpgraded = settings.value(SettingsKeys::SecondCardPenaltyUpgraded, game.secondCardPenaltyUpgraded).toBool();
    for (int index = 0; index < GachaCardCount; ++index) {
        game.cardCounts[index] = settings.value(QString("card%1").arg(index), game.cardCounts[index]).toInt();
    }

    if (game.selectedCard < 0
        || game.selectedCard >= GachaCardCount
        || game.cardCounts[game.selectedCard] <= 0) {
        game.selectedCard = -1;
    }

    if (game.selectedCard2 < 0
        || game.selectedCard2 >= GachaCardCount
        || game.cardCounts[game.selectedCard2] <= 0) {
        game.selectedCard2 = -1;
    }

    if (!game.secondCardSlotUnlocked) {
        game.selectedCard2 = -1;
    }

    game.clickCost = settings.value(
        SettingsKeys::ClickCost,
        settings.value(SettingsKeys::LegacyClickUpgradeCost, game.clickCost)
    ).toInt();
    game.incomeCost = settings.value(
        SettingsKeys::IncomeCost,
        settings.value(SettingsKeys::LegacyAutoUpgradeCost, game.incomeCost)
    ).toInt();
    game.incomeBuffEasterEgg = settings.value(
        SettingsKeys::IncomeBuffEasterEgg, game.incomeBuffEasterEgg
    ).toBool();
    changelogSeenForVersion = settings.value(
        SettingsKeys::LastSeenChangelogVersion,
        QString()
    ).toString() == AppVersion;
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
    settings.setValue(SettingsKeys::SelectedCard2, game.selectedCard2);
    settings.setValue(SettingsKeys::SecondCardSlotUnlocked, game.secondCardSlotUnlocked);
    settings.setValue(SettingsKeys::SecondCardPenaltyUpgraded, game.secondCardPenaltyUpgraded);
    for (int index = 0; index < GachaCardCount; ++index) {
        settings.setValue(QString("card%1").arg(index), game.cardCounts[index]);
    }
    settings.setValue(SettingsKeys::ClickCost, game.clickCost);
    settings.setValue(SettingsKeys::IncomeCost, game.incomeCost);
    settings.setValue(SettingsKeys::IncomeBuffEasterEgg, game.incomeBuffEasterEgg);
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
    clickGlowEffect = setButtonGlow(clickButton, clickGlowEffect, color);
    clickEffectHue = (clickEffectHue + 18) % 360;
    clickEffectFramesLeft -= 1;
}

void Clicker::stopClickEffect() {
    clickEffectTimer->stop();
    clickEffectFramesLeft = 0;
    clearButtonGlow(clickButton, clickGlowEffect);
    clickGlowEffect = nullptr;
}

void Clicker::triggerCritBurst(bool big) {
    if (!particleOverlay) return;
    particleOverlay->resize(size());
    const QRect btnRect = clickButton->geometry();
    const QPointF center(
        btnRect.x() + btnRect.width() / 2.0,
        btnRect.y() + btnRect.height() / 2.0
    );
    particleOverlay->burstAt(center, big ? 60 : 30, big);
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
    } else if (game.secondCardSlotUnlocked && game.selectedCard2 == -1 && index != game.selectedCard) {
        game.selectedCard2 = index;
    }

    dialog->setArchCount(game.arches);
    dialog->setInventory(game.cardCounts, game.selectedCard, game.selectedCard2);
    dialog->showCard(debugGachaCardAt(index), game.cardCounts[index]);

    saveGame();
    refreshUi();
}

void Clicker::selectGachaCard(GachaDialog *dialog, int index) {
    if (index < 0 || index >= GachaCardCount || game.cardCounts[index] <= 0) {
        dialog->showMessage("You do not own this card.");
        return;
    }

    if (index == game.selectedCard) {
        game.selectedCard = -1;
        dialog->showMessage("Deselected slot 1.");
    } else if (index == game.selectedCard2) {
        std::swap(game.selectedCard, game.selectedCard2);
        dialog->showMessage("Swapped slots.");
    } else {
        game.selectedCard = index;
        dialog->showMessage(QString("Selected %1.").arg(debugGachaCardAt(index).name));
    }

    dialog->setInventory(game.cardCounts, game.selectedCard, game.selectedCard2);
    saveGame();
    refreshUi();
}

void Clicker::selectGachaCard2(GachaDialog *dialog, int index) {
    if (index < 0 || index >= GachaCardCount || game.cardCounts[index] <= 0) {
        dialog->showMessage("You do not own this card.");
        return;
    }

    if (index == game.selectedCard2) {
        game.selectedCard2 = -1;
        dialog->showMessage("Deselected slot 2.");
    } else if (index == game.selectedCard) {
        std::swap(game.selectedCard, game.selectedCard2);
        dialog->showMessage("Swapped slots.");
    } else {
        game.selectedCard2 = index;
        dialog->showMessage(QString("Selected %1 for slot 2.").arg(debugGachaCardAt(index).name));
    }

    dialog->setInventory(game.cardCounts, game.selectedCard, game.selectedCard2);
    saveGame();
    refreshUi();
}

qint64 Clicker::applyActiveCardBonus(qint64 value, GachaEffect effect) const {
    if (game.selectedCard >= 0 && game.selectedCard < GachaCardCount) {
        const auto card = debugGachaCardAt(game.selectedCard);
        if (card.effect == effect) {
            value = applyStackedMultiplier(
                value, card.multiplierNumerator, card.multiplierDenominator,
                game.cardCounts[game.selectedCard]
            );
        }
    }

    if (game.secondCardSlotUnlocked && game.selectedCard2 >= 0 && game.selectedCard2 < GachaCardCount) {
        const auto card = debugGachaCardAt(game.selectedCard2);
        if (card.effect == effect) {
            const int extraTenths = effectiveCardCopies(game.cardCounts[game.selectedCard2]) - 1;
            const int stackedNum = card.multiplierNumerator * 10 + extraTenths * card.multiplierDenominator;
            const int stackedDen = card.multiplierDenominator * 10;
            const int penalty = game.secondCardPenaltyUpgraded ? 33 : 55;
            const int penNum = stackedDen * penalty + stackedNum * (100 - penalty);
            const int penDen = stackedDen * 100;
            value = applyMultiplier(value, penNum, penDen);
        }
    }

    return value;
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

QString Clicker::formatUpgradeText(const QString &label, int ownedCount, int cost) const {
    return QString("x%1  %2  %3")
        .arg(formatNumber(ownedCount), label, formatNumber(cost));
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
