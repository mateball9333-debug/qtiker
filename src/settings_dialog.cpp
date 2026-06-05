#include "settings_dialog.h"

#include "clicker.h"
#include "game_rules.h"
#include "svg_utils.h"
#include "utils.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(Clicker *parentClicker)
    : QDialog(parentClicker), clicker(parentClicker)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Qtiker Settings");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    resize(320, 170);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel("Settings", this);
    setWidgetFont(title, 13, true);

    auto *resetBox = new QFrame(this);
    resetBox->setFrameShape(QFrame::StyledPanel);

    auto *resetLayout = new QHBoxLayout(resetBox);
    resetLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    resetLayout->setSpacing(DialogSpacing);

    auto *resetText = new QLabel("Reset saved progress", resetBox);
    setWidgetFont(resetText, resetText->font().pointSize(), true);

    auto *resetButton = new QPushButton("Reset", resetBox);
    resetButton->setIcon(tintedSvgIcon(
        ":/assets/ui/reset.svg",
        resetButton->palette().color(QPalette::ButtonText),
        TopIconSize
    ));
    resetButton->setIconSize(TopIconSize);
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        auto answer = QMessageBox::question(
            this,
            "Reset",
            "Are you sure you want to delete saved progress?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (answer == QMessageBox::Yes) {
            clicker->resetGame();
            accept();
        }
    });

    resetLayout->addWidget(resetText);
    resetLayout->addStretch();
    resetLayout->addWidget(resetButton);

    auto *statisticsBox = new QFrame(this);
    statisticsBox->setFrameShape(QFrame::StyledPanel);

    auto *statisticsLayout = new QHBoxLayout(statisticsBox);
    statisticsLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    statisticsLayout->setSpacing(DialogSpacing);

    auto *statisticsText = new QLabel(statisticsBox);
    setWidgetFont(statisticsText, statisticsText->font().pointSize(), true);
    statisticsText->setTextFormat(Qt::RichText);
    statisticsText->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    statisticsText->setProperty("role", "statsLink");
    statisticsText->installEventFilter(clicker);
    const auto refreshLink = [](QLabel *label) {
        if (!label) return;
        const auto c = label->palette().color(QPalette::WindowText).name();
        label->setText(QStringLiteral(
            "<a href='#' style='text-decoration:none; color:%1;'>S</a>tatistics"
        ).arg(c));
    };
    refreshLink(statisticsText);
    connect(statisticsText, &QLabel::linkActivated, clicker, &Clicker::showAssets);

    auto *statisticsButton = new QPushButton("Show statistics", statisticsBox);
    statisticsButton->setIcon(tintedSvgIcon(
        ":/assets/ui/statistics.svg",
        statisticsButton->palette().color(QPalette::ButtonText),
        TopIconSize
    ));
    statisticsButton->setIconSize(TopIconSize);
    connect(statisticsButton, &QPushButton::clicked, clicker, &Clicker::showStatistics);

    statisticsLayout->addWidget(statisticsText);
    statisticsLayout->addStretch();
    statisticsLayout->addWidget(statisticsButton);

    auto *modeBox = new QFrame(this);
    modeBox->setFrameShape(QFrame::StyledPanel);

    auto *modeLayout = new QHBoxLayout(modeBox);
    modeLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    modeLayout->setSpacing(DialogSpacing);

    auto *modeText = new QLabel("Game version", modeBox);
    setWidgetFont(modeText, modeText->font().pointSize(), true);

    auto *legacyButton = new QPushButton("Legacy 0.1.2", modeBox);
    connect(legacyButton, &QPushButton::clicked, this, [this]() {
        emit clicker->switchToLegacyRequested();
        accept();
    });

    modeLayout->addWidget(modeText);
    modeLayout->addStretch();
    modeLayout->addWidget(legacyButton);

    QSettings eeSettings("qtiker", "qtiker");
    const bool easterEggFound = eeSettings.value("easterEggFound", false).toBool();

    QFrame *buffBox = nullptr;
    if (easterEggFound) {
        buffBox = new QFrame(this);
        buffBox->setFrameShape(QFrame::StyledPanel);

        auto *buffLayout = new QHBoxLayout(buffBox);
        buffLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, PanelMargin);
        buffLayout->setSpacing(DialogSpacing);

        auto *buffText = new QLabel("Income +10% buff", buffBox);
        setWidgetFont(buffText, buffText->font().pointSize(), true);

        auto *buffButton = new QPushButton(
            clicker->game.incomeBuffEasterEgg ? "ON" : "OFF", buffBox);
        connect(buffButton, &QPushButton::clicked, this,
                [this, buffButton]() {
                    clicker->game.incomeBuffEasterEgg = !clicker->game.incomeBuffEasterEgg;
                    buffButton->setText(clicker->game.incomeBuffEasterEgg ? "ON" : "OFF");
                    clicker->saveGame();
                    clicker->refreshUi();
                });

        buffLayout->addWidget(buffText);
        buffLayout->addStretch();
        buffLayout->addWidget(buffButton);
    }

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    layout->addWidget(title);
    layout->addWidget(resetBox);
    layout->addWidget(statisticsBox);
    layout->addWidget(modeBox);
    if (buffBox) {
        layout->addWidget(buffBox);
    }
    layout->addStretch();
    layout->addWidget(closeButton);
}
