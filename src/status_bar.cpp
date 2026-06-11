#include "status_bar.h"

#include <QFrame>
#include <QStyle>

namespace {
constexpr int BarVerticalMargin = 3;
constexpr int BarHorizontalMargin = 4;
constexpr int BarFontDelta = -2;
constexpr int SectionSpacing = 6;

QString fmtDuration(qint64 secs) {
    const qint64 h = secs / 3600;
    secs %= 3600;
    const qint64 m = secs / 60;
    secs %= 60;

    if (h > 0) {
        return QString("%1h %2m %3s").arg(h).arg(m).arg(secs);
    }
    if (m > 0) {
        return QString("%1m %2s").arg(m).arg(secs);
    }
    return QString("%1s").arg(secs);
}
}

StatusBar::StatusBar(QWidget *parent)
    : QWidget(parent)
{

    setFixedHeight(style()->pixelMetric(QStyle::PM_SmallIconSize) + BarVerticalMargin * 2);

    barLayout = new QHBoxLayout(this);
    barLayout->setContentsMargins(BarHorizontalMargin, BarVerticalMargin, BarHorizontalMargin, BarVerticalMargin);
    barLayout->setSpacing(SectionSpacing);
    barLayout->addStretch();

    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &StatusBar::refresh);
}

void StatusBar::enableSaveTimer() {
    if (saveTimerLabel) {
        return;
    }

    if (hasContent) {
        addSeparator();
    }

    saveTimerLabel = makeLabel("Saved \u2022 --");
    barLayout->insertWidget(barLayout->count() - 1, saveTimerLabel);

    hasContent = true;
    timeSinceSave.start();
    refreshSaveTimer();
    ensureRefreshRunning();
}

void StatusBar::enableSessionTimer() {
    if (sessionLabel) {
        return;
    }

    if (hasContent) {
        addSeparator();
    }

    sessionLabel = makeLabel("Session \u2022 --");
    barLayout->insertWidget(barLayout->count() - 1, sessionLabel);

    hasContent = true;
    sessionTimer.start();
    refreshSessionTimer();
    ensureRefreshRunning();
}

void StatusBar::onSaveCompleted() {
    if (!saveTimerLabel) {
        return;
    }
    timeSinceSave.start();
    refreshSaveTimer();
}

void StatusBar::addSeparator() {
    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setFixedWidth(2);
    sep->setMaximumHeight(fontMetrics().height());
    barLayout->insertWidget(barLayout->count() - 1, sep);
}

QLabel *StatusBar::makeLabel(const QString &text) {
    auto *label = new QLabel(text, this);
    auto f = label->font();
    const int sz = f.pointSize() + BarFontDelta;
    if (sz > 0) {
        f.setPointSize(sz);
    }
    f.setBold(false);
    label->setFont(f);
    return label;
}

void StatusBar::refresh() {
    refreshSaveTimer();
    refreshSessionTimer();
}

void StatusBar::refreshSaveTimer() {
    if (!saveTimerLabel || !timeSinceSave.isValid()) {
        return;
    }

    const qint64 secs = timeSinceSave.elapsed() / 1000;
    if (secs < 5) {
        saveTimerLabel->setText(QStringLiteral("Saved \u2022 now"));
    } else {
        saveTimerLabel->setText(QString("Saved \u2022 %1 ago").arg(fmtDuration(secs)));
    }
}

void StatusBar::refreshSessionTimer() {
    if (!sessionLabel || !sessionTimer.isValid()) {
        return;
    }

    const qint64 secs = sessionTimer.elapsed() / 1000;
    sessionLabel->setText(QString("Session \u2022 %1").arg(fmtDuration(secs)));
}

void StatusBar::ensureRefreshRunning() {
    if (!refreshTimer->isActive()) {
        refreshTimer->start();
    }
}
