#pragma once

#include "gamestate.h"

#include <QColor>
#include <QDialog>
#include <QString>

#include <array>

class QLabel;
class QPushButton;

enum class GachaEffect {
    Click,
    Income
};

struct GachaCard {
    QString name;
    QString suit;
    QColor color;
    GachaEffect effect;
    int multiplierNumerator;
    int multiplierDenominator;
    int dropWeight;
};

int debugGachaCardCount();
GachaCard debugGachaCardAt(int index);
int debugGachaTotalWeight();
QString gachaEffectText(const GachaCard &card);
QString gachaEffectText(const GachaCard &card, int ownedCount);

class GachaDialog : public QDialog {
    Q_OBJECT

public:
    explicit GachaDialog(QWidget *parent = nullptr);

    void setArchCount(int arches);
    void setInventory(const std::array<int, GachaCardCount> &cardCounts, int selectedCard);
    void showCard(const GachaCard &card, int ownedCount);
    void showMessage(const QString &message);

signals:
    void rollRequested();
    void cardSelected(int index);

private:
    void updateCardButton(int index, int ownedCount, bool selected);

    QLabel *archLabel = nullptr;
    QLabel *cardLabel = nullptr;
    QLabel *messageLabel = nullptr;
    std::array<QPushButton *, GachaCardCount> cardButtons = {};
    QPushButton *rollButton = nullptr;
};
