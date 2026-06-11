#include "settings_dialog.h"

#include "clicker.h"
#include "game_rules.h"
#include "save_slots_dialog.h"
#include "svg_utils.h"
#include "utils.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(Clicker *parentClicker)
    : QDialog(parentClicker), clicker(parentClicker)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Qtiker Settings");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    resize(320, 240);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel("Settings", this);
    setWidgetFont(title, 13, true);

    auto *savesBox = new QFrame(this);
    savesBox->setFrameShape(QFrame::StyledPanel);

    auto *savesLayout = new QHBoxLayout(savesBox);
    savesLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    savesLayout->setSpacing(DialogSpacing);

    auto *savesText = new QLabel("Save slots", savesBox);
    setWidgetFont(savesText, savesText->font().pointSize(), true);

    auto *savesButton = new QPushButton(QString("Slot %1").arg(clicker->slotForSettings() + 1), savesBox);
    savesButton->setIcon(tintedSvgIcon(
        ":/assets/ui/save.svg",
        savesButton->palette().color(QPalette::ButtonText),
        TopIconSize
    ));
    savesButton->setIconSize(TopIconSize);
    connect(savesButton, &QPushButton::clicked, this, [this, savesButton]() {
        auto *dlg = new SaveSlotsDialog(clicker);
        connect(dlg, &SaveSlotsDialog::slotLoaded, this, [this, savesButton]() {
            savesButton->setText(QString("Slot %1").arg(clicker->slotForSettings() + 1));
        });
        dlg->show();
    });

    savesLayout->addWidget(savesText);
    savesLayout->addStretch();
    savesLayout->addWidget(savesButton);

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
    layout->addWidget(savesBox);
    layout->addWidget(statisticsBox);
    layout->addWidget(modeBox);
    if (buffBox) {
        layout->addWidget(buffBox);
    }

    auto *volumeBox = new QFrame(this);
    volumeBox->setFrameShape(QFrame::StyledPanel);

    auto *volumeLayout = new QVBoxLayout(volumeBox);
    volumeLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
    volumeLayout->setSpacing(DialogSpacing);

    auto *volumeRow = new QHBoxLayout();
    volumeRow->setContentsMargins(0, 0, 0, 0);
    volumeRow->setSpacing(DialogSpacing);

    auto *volumeText = new QLabel("Volume", volumeBox);
    setWidgetFont(volumeText, volumeText->font().pointSize(), true);

    volumeSlider = new QSlider(Qt::Horizontal, volumeBox);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(static_cast<int>(clicker->masterVolume() * 100));
    connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        clicker->setMasterVolume(value / 100.0);
    });

    volumeRow->addWidget(volumeText);
    volumeRow->addWidget(volumeSlider, 1);

    muteClicksCheck = new QCheckBox("Mute click sound", volumeBox);
    muteClicksCheck->setChecked(clicker->isClickSoundMuted());
    connect(muteClicksCheck, &QCheckBox::toggled, this, [this](bool checked) {
        clicker->setClickSoundMuted(checked);
    });

    volumeLayout->addLayout(volumeRow);
    volumeLayout->addWidget(muteClicksCheck);

    layout->addWidget(volumeBox);
    layout->addStretch();
    layout->addWidget(closeButton);
}
