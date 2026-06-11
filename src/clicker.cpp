#include "clicker.h"

#include "appversion.h"
#include "carat_dialog.h"
#include "gacha_dialog.h"
#include "game_rules.h"
#include "particle_overlay.h"
#include "release_notes.h"
#include "settings_dialog.h"
#include "statistics_dialog.h"
#include "status_bar.h"
#include "svg_utils.h"
#include "utils.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QImage>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QSettings>
#include <QShortcut>
#include <QSoundEffect>
#include <QSvgRenderer>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

#include <array>
#include <iterator>

namespace {
constexpr int ButtonTextChangeChance = 35;
constexpr int ClickEffectChance = 10;
constexpr int ClickEffectCheckMs = 10000;
constexpr int ClickEffectFrameMs = 120;
constexpr int ClickEffectFrames = 34;
constexpr int ChangelogHighlightFrameMs = 140;

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
constexpr auto CurrentSlot = "currentSlot";
constexpr auto Checksum = "checksum";
}
}

Clicker::Clicker(QWidget *parent) : QWidget(parent) {
    setupWindow();
    {
        QSettings meta("qtiker", "qtiker");
        m_masterVolume = meta.value("masterVolume", 1.0).toDouble();
        currentSlot = meta.value(SettingsKeys::CurrentSlot, 0).toInt();
        const bool hasRootData = meta.contains(SettingsKeys::Score);
        const bool hasSlotData = meta.contains(QString("slot0/%1").arg(SettingsKeys::Score));
        if (hasRootData && !hasSlotData) {
            for (int s = 0; s < 3; ++s) {
                const bool occupied = meta.contains(QString("slot%1/%2").arg(s).arg(SettingsKeys::Score));
                if (!occupied) {
                    currentSlot = s;
                    break;
                }
            }
        }
        meta.setValue(SettingsKeys::CurrentSlot, currentSlot);
    }
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

void Clicker::hideEvent(QHideEvent *event) {
    if (incomeTimer) {
        incomeTimer->stop();
    }
    QWidget::hideEvent(event);
}

void Clicker::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (incomeTimer) {
        incomeTimer->start();
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
        if (critSound) critSound->play();
    } else if (roll < 6) {
        earned = buffed * 2;
        triggerCritBurst(false);
        if (critSound) critSound->play();
    } else {
        earned = buffed;
    }

    game.score += earned;
    game.totalClicks += 1;
    game.totalScoreEarned += earned;
    addArchProgress(earned);

    if (clickSound) {
        clickSound->play();
    }

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

    if (buySound) buySound->play();
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

    if (buySound) buySound->play();
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

void Clicker::switchToSlot(int slot) {
    if (slot < 0 || slot > 2 || slot == currentSlot)
        return;

    saveGame();
    game.reset();
    currentSlot = slot;
    {
        QSettings meta("qtiker", "qtiker");
        meta.setValue("currentSlot", currentSlot);
    }
    loadGame();
    refreshUi();
}

void Clicker::resetSlot() {
    game.reset();
    saveGame();
    refreshUi();
}

void Clicker::showSettings() {
    (new SettingsDialog(this))->show();
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
    setWidgetFont(title, 13, true);

    auto *version = new QLabel(QString("v%1").arg(AppVersion), dialog);
    version->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    setWidgetFont(version, version->font().pointSize(), true);

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
    setWidgetFont(title, 14, true);

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
    (new CaratDialog(this))->show();
}

void Clicker::showStatistics() {
    (new StatisticsDialog(this))->show();
}

void Clicker::showAssets() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Assets");
    dialog->setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    dialog->resize(420, 480);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel("Game Assets", dialog);
    setWidgetFont(title, 13, true);

    auto *scrollArea = new QScrollArea(dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::StyledPanel);

    auto *scrollContent = new QWidget(scrollArea);
    auto *flow = new QVBoxLayout(scrollContent);
    flow->setSpacing(2);
    flow->setContentsMargins(4, 4, 4, 4);

    struct AssetEntry {
        QString name;
        QString path;
        qint64 size;
        bool isSvg;
        bool isPng;
        bool isWav;
    };
    QList<AssetEntry> entries;

    const auto collectAssets = [&](const QString &dir) {
        QDir d(dir);
        for (const auto &file : d.entryList({"*.svg", "*.png", "*.wav"}, QDir::Files, QDir::Name)) {
            AssetEntry e;
            e.name = file;
            e.path = QString("%1/%2").arg(dir, file);
            e.size = QFileInfo(e.path).size();
            e.isSvg = file.endsWith(".svg", Qt::CaseInsensitive);
            e.isPng = file.endsWith(".png", Qt::CaseInsensitive);
            e.isWav = file.endsWith(".wav", Qt::CaseInsensitive);
            entries.append(e);
        }
    };
    collectAssets(":/assets/ui");
    collectAssets(":/assets/sound");

    for (const auto &entry : entries) {
        auto *row = new QFrame(scrollContent);
        row->setFrameShape(QFrame::StyledPanel);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(6, 3, 6, 3);
        rowLayout->setSpacing(8);

        auto *icon = new QLabel(row);
        icon->setFixedSize(24, 24);
        icon->setAlignment(Qt::AlignCenter);

        if (entry.isSvg) {
            QPixmap pixmap(24, 24);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            QSvgRenderer renderer(entry.path);
            renderer.render(&painter, QRectF(0, 0, 24, 24));
            painter.end();
            icon->setPixmap(pixmap);
        } else if (entry.isPng) {
            QPixmap pm(entry.path);
            icon->setPixmap(pm.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            icon->setText("\u266B");
        }

        auto *name = new QLabel(entry.name, row);
        name->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        setWidgetFont(name, name->font().pointSize(), false);

        auto *sizeLabel = new QLabel(QString("%1 B").arg(entry.size), row);
        sizeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        setWidgetFont(sizeLabel, 8, false);

        rowLayout->addWidget(icon);
        rowLayout->addWidget(name, 1);
        rowLayout->addWidget(sizeLabel);

        if (entry.isWav) {
            auto *playBtn = new QPushButton("\u25B6", row);
            playBtn->setFixedSize(28, 24);
            playBtn->setCheckable(true);
            playBtn->setToolTip("Play");

            auto *sound = new QSoundEffect(dialog);
            sound->setSource(QUrl("qrc:" + entry.path.mid(1)));
            sound->setVolume(0.8);

            connect(playBtn, &QPushButton::clicked, dialog, [sound, playBtn]() {
                if (sound->isPlaying()) {
                    sound->stop();
                    playBtn->setChecked(false);
                } else {
                    sound->play();
                    playBtn->setChecked(true);
                    QTimer::singleShot(2000, sound, [playBtn]() { playBtn->setChecked(false); });
                }
            });
            connect(sound, &QSoundEffect::playingChanged, dialog, [playBtn]() {
                playBtn->setChecked(false);
            });

            rowLayout->addWidget(playBtn);
        }

        flow->addWidget(row);
    }

    scrollArea->setWidget(scrollContent);

    auto *countLabel = new QLabel(QString("Total assets: %1").arg(entries.size()), dialog);
    setWidgetFont(countLabel, countLabel->font().pointSize(), true);

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
    setMinimumSize(340, 420);
    resize(360, 500);
}

void Clicker::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(WindowMargin, WindowMargin, WindowMargin, WindowMargin);
    mainLayout->setSpacing(WindowSpacing);

    changelogButton = new QPushButton(QString("  v%1").arg(AppVersion), this);
    changelogButton->setIconSize(TopIconSize);
    changelogButton->setToolTip("Release notes");
    changelogButton->setFixedSize(ChangelogButtonSize);
    setWidgetFont(changelogButton, changelogButton->font().pointSize(), true);
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
    setWidgetFont(scoreLabel, 30, true);

    auto *statsLayout = new QHBoxLayout();
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(0);

    clickStatsLabel = new QLabel(this);
    clickStatsLabel->setAlignment(Qt::AlignCenter);
    setWidgetFont(clickStatsLabel, clickStatsLabel->font().pointSize(), true);

    auto *separator = new QLabel(" · ", this);
    separator->setAlignment(Qt::AlignCenter);
    setWidgetFont(separator, separator->font().pointSize(), true);

    incomeStatsLabel = new QLabel(this);
    incomeStatsLabel->setAlignment(Qt::AlignCenter);
    setWidgetFont(incomeStatsLabel, incomeStatsLabel->font().pointSize(), true);

    statsLayout->addStretch();
    statsLayout->addWidget(clickStatsLabel);
    statsLayout->addWidget(separator);
    statsLayout->addWidget(incomeStatsLabel);
    statsLayout->addStretch();

    archLabel = new QLabel(this);
    archLabel->setAlignment(Qt::AlignCenter);
    archLabel->setFrameShape(QFrame::StyledPanel);
    archLabel->setMargin(4);
    setWidgetFont(archLabel, archLabel->font().pointSize(), true);

    clickButton = new QPushButton("Click", this);
    clickButton->setFocusPolicy(Qt::NoFocus);
    clickButton->setMinimumHeight(76);
    setWidgetFont(clickButton, 18, true);
    clickButton->installEventFilter(this);
    connect(clickButton, &QPushButton::clicked, this, &Clicker::makeClick);

    particleOverlay = new ParticleOverlay(this);

    clickSound = new QSoundEffect(this);
    clickSound->setSource(QUrl("qrc:/assets/sound/click.wav"));
    clickSound->setVolume(0.6 * m_masterVolume);

    buySound = new QSoundEffect(this);
    buySound->setSource(QUrl("qrc:/assets/sound/buy.wav"));
    buySound->setVolume(1.0);

    critSound = new QSoundEffect(this);
    critSound->setSource(QUrl("qrc:/assets/sound/crit.wav"));
    critSound->setVolume(1.0);

    for (auto *s : {buySound, critSound}) {
        s->setVolume(m_masterVolume);
        connect(s, &QSoundEffect::statusChanged, this, [s]() {
            if (s->status() == QSoundEffect::Error) {
                qWarning("SoundEffect error: %s", qPrintable(s->source().toString()));
            }
        });
    }

    gachaButton = new QPushButton("Gacha", this);
    gachaButton->setMinimumHeight(32);
    connect(gachaButton, &QPushButton::clicked, this, &Clicker::showGacha);

    auto *upgradesBox = createUpgradesBox();

    auto *statusSeparator = new QFrame(this);
    statusSeparator->setFrameShape(QFrame::HLine);
    statusSeparator->setFrameShadow(QFrame::Sunken);

    statusBar = new StatusBar(this);
    statusBar->enableSaveTimer();
    statusBar->enableSessionTimer();
    connect(this, &Clicker::saveCompleted, statusBar, &StatusBar::onSaveCompleted);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(scoreLabel);
    mainLayout->addLayout(statsLayout);
    mainLayout->addWidget(archLabel);
    mainLayout->addWidget(clickButton, 1);
    mainLayout->addWidget(gachaButton);
    mainLayout->addWidget(upgradesBox);
    mainLayout->addWidget(statusSeparator);
    mainLayout->addWidget(statusBar);
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
    setWidgetFont(title, title->font().pointSize(), true);

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
    scoreLabel->setToolTip(QLocale::system().toString(game.score));
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
    caratButton->setToolTip(QString("Carats: %1").arg(QLocale::system().toString(game.carats)));

    const int clickUpgradesBought = game.perClick > 1 ? game.perClick - 1 : 0;
    const int incomeUpgradesBought = game.perSecond > 0 ? game.perSecond : 0;
    clickUpgradeButton->setText(formatUpgradeText("Click +1", clickUpgradesBought, game.clickCost));
    incomeUpgradeButton->setText(formatUpgradeText("Income +1/sec", incomeUpgradesBought, game.incomeCost));
    gachaButton->setText("Gacha");

    clickUpgradeButton->setEnabled(game.score >= game.clickCost);
    incomeUpgradeButton->setEnabled(game.score >= game.incomeCost);
}

bool Clicker::loadGame() {
    QSettings settings("qtiker", "qtiker");
    settings.beginGroup(QString("slot%1").arg(currentSlot));

    const bool hasIntegrity = settings.contains("integrity1")
        && settings.contains("integrity2")
        && settings.contains("integrity3");
    if (hasIntegrity) {
        const qint64 m1 = settings.value("integrity1", qint64{0}).toLongLong();
        const qint64 m2 = settings.value("integrity2", qint64{0}).toLongLong();
        const qint64 m3 = settings.value("integrity3", qint64{0}).toLongLong();
        if (m1 != GameState::SaveMagic1 || m2 != GameState::SaveMagic2 || m3 != GameState::SaveMagic3) {
            settings.endGroup();
            game.reset();
            return false;
        }
    }

    game.score = settings.value(SettingsKeys::Score,
        settings.value(SettingsKeys::LegacyTotalClicks, game.score)).toLongLong();
    game.archProgress = settings.value(SettingsKeys::ArchProgress, game.archProgress).toLongLong();
    game.nextArchAt = settings.value(SettingsKeys::NextArchAt, game.nextArchAt).toLongLong();
    game.perClick = settings.value(SettingsKeys::PerClick,
        settings.value(SettingsKeys::LegacyClickPower, game.perClick)).toInt();
    game.perSecond = settings.value(SettingsKeys::PerSecond,
        settings.value(SettingsKeys::LegacyAutoPower, game.perSecond)).toInt();
    game.arches = settings.value(SettingsKeys::Arches, game.arches).toInt();
    game.carats = settings.value(SettingsKeys::Carats, game.carats).toLongLong();
    game.totalClicks = settings.value(SettingsKeys::TotalClicks, game.totalClicks).toLongLong();
    game.totalScoreEarned = settings.value(SettingsKeys::TotalScoreEarned,
        settings.value(SettingsKeys::LegacyTotalClickScoreEarned, game.totalScoreEarned)).toLongLong();
    game.totalPlaySeconds = settings.value(SettingsKeys::TotalPlaySeconds, game.totalPlaySeconds).toLongLong();
    game.totalArchesEarned = settings.value(SettingsKeys::TotalArchesEarned, game.totalArchesEarned).toLongLong();
    game.clickButtonRightClicks = settings.value(SettingsKeys::ClickButtonRightClicks, game.clickButtonRightClicks).toLongLong();
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
    game.clickCost = settings.value(SettingsKeys::ClickCost,
        settings.value(SettingsKeys::LegacyClickUpgradeCost, game.clickCost)).toInt();
    game.incomeCost = settings.value(SettingsKeys::IncomeCost,
        settings.value(SettingsKeys::LegacyAutoUpgradeCost, game.incomeCost)).toInt();
    game.incomeBuffEasterEgg = settings.value(SettingsKeys::IncomeBuffEasterEgg, game.incomeBuffEasterEgg).toBool();

    if (hasIntegrity) {
        const qint64 storedChecksum = settings.value(SettingsKeys::Checksum, qint64{0}).toLongLong();
        const qint64 actualChecksum = game.computeChecksum();
        if (storedChecksum != actualChecksum) {
            settings.endGroup();
            game.reset();
            return false;
        }
    }

    settings.endGroup();

    {
        QSettings meta("qtiker", "qtiker");
        changelogSeenForVersion = meta.value(
            SettingsKeys::LastSeenChangelogVersion, QString()
        ).toString() == AppVersion;
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

    return true;
}

void Clicker::saveGame() {
    QSettings settings("qtiker", "qtiker");
    settings.beginGroup(QString("slot%1").arg(currentSlot));

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

    settings.setValue("integrity1", GameState::SaveMagic1);
    settings.setValue("integrity2", GameState::SaveMagic2);
    settings.setValue("integrity3", GameState::SaveMagic3);
    settings.setValue(SettingsKeys::Checksum, game.computeChecksum());

    settings.endGroup();

    emit saveCompleted();
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

void Clicker::setMasterVolume(qreal volume) {
    m_masterVolume = qBound(0.0, volume, 1.0);
    QSettings("qtiker", "qtiker").setValue("masterVolume", m_masterVolume);
    for (auto *s : {clickSound, buySound, critSound}) {
        if (s) s->setVolume(m_masterVolume);
    }
}

qreal Clicker::masterVolume() const {
    return m_masterVolume;
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
