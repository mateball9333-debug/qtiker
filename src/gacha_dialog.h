#pragma once

#include "gamestate.h"

#include <QColor>
#include <QDialog>
#include <QString>
#include <QTabWidget>

class QVBoxLayout;

#include <array>

class QLabel;
class QPushButton;
class Clicker;

enum class GachaEffect {
    Click,
    Income,
    CritChance,
    CritPower,
    ArchHopper,
    Hoarder,
    Speedrun,
};

bool isNormalEffect(GachaEffect effect);
QString specialEffectName(GachaEffect effect);
QString specialEffectValueText(GachaEffect effect, int specialBase, int specialPerCopy, int ownedCount);

struct GachaCard {
    QString name;
    QString suit;
    QColor color;
    GachaEffect effect;
    int multiplierNumerator;
    int multiplierDenominator;
    int dropWeight;
    int specialBase = 0;
    int specialPerCopy = 0;
};

int debugGachaCardCount();
GachaCard debugGachaCardAt(int index);
int debugGachaTotalWeight();
QString gachaEffectText(const GachaCard &card);
QString gachaEffectText(const GachaCard &card, int ownedCount);

class GachaDialog : public QDialog {
    Q_OBJECT

public:
    explicit GachaDialog(Clicker *clicker, QWidget *parent = nullptr);

    void setArchCount(int arches);
    void setInventory(const std::array<int, GachaCardCount> &cardCounts, int selectedCard, int selectedCard2);
    void setSecondCardSlotEnabled(bool unlocked);
    void setSecondCardPenaltyUpgraded(bool upgraded);
    void setActiveSlot(int slot);
    void showCard(const GachaCard &card, int ownedCount);
    void showMessage(const QString &message);

signals:
    void rollRequested();
    void cardSelected(int index);
    void cardSelectedForSlot2(int index);
    void slotChanged(int slot);

private:
    void updateCardButton(int index, int ownedCount, bool selected, bool selected2);

    Clicker *clicker;
    QLabel *archLabel = nullptr;
    QLabel *cardLabel = nullptr;
    QLabel *messageLabel = nullptr;
    std::array<QPushButton *, GachaCardCount> cardButtons = {};
    QPushButton *rollButton = nullptr;
    QPushButton *slot1Button = nullptr;
    QPushButton *slot2Button = nullptr;
    QTabWidget *cardTabs = nullptr;
    int activeSlot = 1;
    bool penaltyUpgraded = false;
};
