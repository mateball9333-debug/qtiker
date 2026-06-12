#pragma once

#include <QElapsedTimer>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QWidget>

class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);

    void enableSaveTimer();
    void enableSessionTimer();
    void setCompatMode(bool enabled);

public slots:
    void onSaveCompleted();

private:
    void addSeparator();
    QLabel *makeLabel(const QString &text);
    void refresh();
    void refreshSaveTimer();
    void refreshSessionTimer();
    void updateRainbowGlow();
    void ensureRefreshRunning();

    QHBoxLayout *barLayout;
    QLabel *saveTimerLabel = nullptr;
    QLabel *sessionLabel = nullptr;
    QElapsedTimer timeSinceSave;
    QElapsedTimer sessionTimer;
    QTimer *refreshTimer;
    QTimer *glowTimer = nullptr;
    QGraphicsDropShadowEffect *sessionGlow = nullptr;
    int glowHue = 0;
    bool hasContent = false;
    bool compatMode = false;
};
