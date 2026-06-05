#include "save_slots_dialog.h"

#include "clicker.h"
#include "utils.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

SaveSlotsDialog::SaveSlotsDialog(Clicker *parentClicker)
    : QDialog(parentClicker), clicker(parentClicker)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Save Slots");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    setMinimumWidth(380);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(DialogMargin, DialogMargin, DialogMargin, DialogMargin);
    layout->setSpacing(WindowSpacing);

    auto *title = new QLabel("Save Slots", this);
    setWidgetFont(title, 13, true);

    for (int slot = 0; slot < 3; ++slot) {
        auto *box = new QFrame(this);
        box->setFrameShape(QFrame::StyledPanel);
        auto *boxLayout = new QVBoxLayout(box);
        boxLayout->setContentsMargins(PanelMargin, DialogSpacing, PanelMargin, DialogSpacing);
        boxLayout->setSpacing(DialogSpacing);

        auto *headerRow = new QHBoxLayout();
        headerRow->setContentsMargins(0, 0, 0, 0);

        auto *slotLabel = new QLabel(QString("Slot %1").arg(slot + 1), box);
        setWidgetFont(slotLabel, slotLabel->font().pointSize(), true);
        headerRow->addWidget(slotLabel);

        infoLabels[slot] = new QLabel(box);
        infoLabels[slot]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        headerRow->addWidget(infoLabels[slot], 1);

        boxLayout->addLayout(headerRow);

        auto *buttonRow = new QHBoxLayout();
        buttonRow->setContentsMargins(0, 0, 0, 0);
        buttonRow->setSpacing(DialogSpacing);

        loadButtons[slot] = new QPushButton(box);
        buttonRow->addWidget(loadButtons[slot], 1);

        resetButtons[slot] = new QPushButton("Reset", box);
        buttonRow->addWidget(resetButtons[slot]);

        boxLayout->addLayout(buttonRow);

        layout->addWidget(box);

        const int s = slot;
        connect(loadButtons[slot], &QPushButton::clicked, this, [this, s]() {
            clicker->switchToSlot(s);
            refresh();
            emit slotLoaded();
        });
        connect(resetButtons[slot], &QPushButton::clicked, this, [this, s]() {
            const bool wasCurrent = (s == clicker->slotForSettings());
            auto answer = QMessageBox::question(
                this,
                "Reset Slot",
                QString("Reset slot %1?\nThis cannot be undone.").arg(s + 1),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            if (answer != QMessageBox::Yes)
                return;
            if (wasCurrent) {
                clicker->resetSlot();
            } else {
                QSettings settings("qtiker", "qtiker");
                settings.beginGroup(QString("slot%1").arg(s));
                settings.remove("");
                settings.endGroup();
            }
            refresh();
        });
    }

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    layout->addStretch();
    layout->addWidget(closeButton);

    refresh();
}

void SaveSlotsDialog::refresh() {
    const int current = clicker->slotForSettings();
    for (int slot = 0; slot < 3; ++slot) {
        QSettings settings("qtiker", "qtiker");
        settings.beginGroup(QString("slot%1").arg(slot));

        const qint64 score = settings.value("score", qint64{0}).toLongLong();
        const int arches = settings.value("arches", 0).toInt();
        const qint64 playSecs = settings.value("totalPlaySeconds", qint64{0}).toLongLong();
        const bool hasData = score > 0 || arches > 0;

        settings.endGroup();

        if (slot == current) {
            infoLabels[slot]->setText(QString("CURRENT  %1  %2 arches")
                .arg(clicker->formatNumber(score))
                .arg(clicker->formatNumber(arches)));
            loadButtons[slot]->setText("Active");
            loadButtons[slot]->setEnabled(false);
            resetButtons[slot]->setEnabled(true);
        } else if (hasData) {
            infoLabels[slot]->setText(QString("%1  %2 arches")
                .arg(clicker->formatNumber(score))
                .arg(clicker->formatNumber(arches)));
            loadButtons[slot]->setText("Load");
            loadButtons[slot]->setEnabled(true);
            resetButtons[slot]->setEnabled(true);
        } else {
            infoLabels[slot]->setText("Empty");
            loadButtons[slot]->setText("New Game");
            loadButtons[slot]->setEnabled(true);
            resetButtons[slot]->setEnabled(false);
        }
    }
}
