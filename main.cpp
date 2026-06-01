#include <QApplication>
#include <QCloseEvent>
#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

struct GameState {
    qint64 score = 0;
    int perClick = 1;
    int perSecond = 0;
    int clickCost = 25;
    int incomeCost = 60;

    void reset() {
        score = 0;
        perClick = 1;
        perSecond = 0;
        clickCost = 25;
        incomeCost = 60;
    }
};

class Clicker : public QWidget {
    Q_OBJECT

    GameState game;

    QLabel *scoreLabel = nullptr;
    QLabel *statsLabel = nullptr;
    QPushButton *clickButton = nullptr;
    QPushButton *clickUpgradeButton = nullptr;
    QPushButton *incomeUpgradeButton = nullptr;
    QTimer *incomeTimer = nullptr;

public:
    Clicker() {
        setupWindow();
        loadGame();
        buildUi();
        startIncomeTimer();
        refreshUi();
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        saveGame();
        QWidget::closeEvent(event);
    }

    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == clickButton && event->type() == QEvent::MouseButtonRelease) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::MiddleButton) {
                showTux();
                return true;
            }
        }

        return QWidget::eventFilter(watched, event);
    }

private slots:
    void makeClick() {
        game.score += game.perClick;
        refreshUi();
    }

    void buyClickUpgrade() {
        if (game.score < game.clickCost) {
            return;
        }

        game.score -= game.clickCost;
        game.perClick += 1;
        game.clickCost = nextCost(game.clickCost, 10);

        saveGame();
        refreshUi();
    }

    void buyIncomeUpgrade() {
        if (game.score < game.incomeCost) {
            return;
        }

        game.score -= game.incomeCost;
        game.perSecond += 1;
        game.incomeCost = nextCost(game.incomeCost, 15);

        saveGame();
        refreshUi();
    }

    void addPassiveIncome() {
        if (game.perSecond == 0) {
            return;
        }

        game.score += game.perSecond;
        saveGame();
        refreshUi();
    }

    void resetGame() {
        game.reset();
        saveGame();
        refreshUi();
    }

private:
    void showTux() {
        auto *window = new QWidget(this, Qt::Dialog);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->setWindowTitle("Tux");
        window->setWindowIcon(QIcon(":/assets/tux.png"));

        auto *layout = new QVBoxLayout(window);
        layout->setContentsMargins(12, 12, 12, 12);

        auto *image = new QLabel(window);
        image->setAlignment(Qt::AlignCenter);
        image->setPixmap(QPixmap(":/assets/tux.png"));

        layout->addWidget(image);

        window->setFixedSize(290, 350);
        window->show();
    }

    void setupWindow() {
        setWindowTitle("Qtiker");
        setWindowIcon(QIcon(":/assets/qtiker-64.png"));
        setMinimumSize(340, 360);
        resize(360, 420);
    }

    void buildUi() {
        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(14, 14, 14, 14);
        mainLayout->setSpacing(10);

        scoreLabel = new QLabel(this);
        scoreLabel->setAlignment(Qt::AlignCenter);
        setFont(scoreLabel, 30, true);

        statsLabel = new QLabel(this);
        statsLabel->setAlignment(Qt::AlignCenter);
        statsLabel->setStyleSheet("font-weight: 600;");

        clickButton = new QPushButton("Click", this);
        clickButton->setMinimumHeight(76);
        setFont(clickButton, 18, true);
        clickButton->installEventFilter(this);
        connect(clickButton, &QPushButton::clicked, this, &Clicker::makeClick);

        auto *upgradesBox = createUpgradesBox();

        auto *resetButton = new QPushButton("Reset", this);
        connect(resetButton, &QPushButton::clicked, this, &Clicker::resetGame);

        mainLayout->addWidget(scoreLabel);
        mainLayout->addWidget(statsLabel);
        mainLayout->addWidget(clickButton, 1);
        mainLayout->addWidget(upgradesBox);
        mainLayout->addWidget(resetButton);
    }

    QFrame *createUpgradesBox() {
        auto *box = new QFrame(this);
        box->setFrameShape(QFrame::StyledPanel);

        auto *layout = new QVBoxLayout(box);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(8);

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

    void startIncomeTimer() {
        incomeTimer = new QTimer(this);
        incomeTimer->setInterval(1000);
        connect(incomeTimer, &QTimer::timeout, this, &Clicker::addPassiveIncome);
        incomeTimer->start();
    }

    void refreshUi() {
        scoreLabel->setText(QString::number(game.score));
        statsLabel->setText(QString("%1 per click · %2/sec")
                                .arg(game.perClick)
                                .arg(game.perSecond));

        clickUpgradeButton->setText(QString("Click +1       %1").arg(game.clickCost));
        incomeUpgradeButton->setText(QString("Income +1/sec  %1").arg(game.incomeCost));

        clickUpgradeButton->setEnabled(game.score >= game.clickCost);
        incomeUpgradeButton->setEnabled(game.score >= game.incomeCost);
    }

    void loadGame() {
        QSettings settings("qtiker", "qtiker");
        game.score = settings.value("score", game.score).toLongLong();
        game.perClick = settings.value("perClick", game.perClick).toInt();
        game.perSecond = settings.value("perSecond", game.perSecond).toInt();
        game.clickCost = settings.value("clickCost", game.clickCost).toInt();
        game.incomeCost = settings.value("incomeCost", game.incomeCost).toInt();

        // Compatibility with early test builds.
        game.perClick = settings.value("clickPower", game.perClick).toInt();
        game.perSecond = settings.value("autoPower", game.perSecond).toInt();
        game.clickCost = settings.value("clickUpgradeCost", game.clickCost).toInt();
        game.incomeCost = settings.value("autoUpgradeCost", game.incomeCost).toInt();
    }

    void saveGame() const {
        QSettings settings("qtiker", "qtiker");
        settings.setValue("score", game.score);
        settings.setValue("perClick", game.perClick);
        settings.setValue("perSecond", game.perSecond);
        settings.setValue("clickCost", game.clickCost);
        settings.setValue("incomeCost", game.incomeCost);
    }

    int nextCost(int currentCost, int extra) const {
        return currentCost * 3 / 2 + extra;
    }

    void setFont(QWidget *widget, int pointSize, bool bold) {
        auto font = widget->font();
        if (pointSize > 0) {
            font.setPointSize(pointSize);
        }
        font.setBold(bold);
        widget->setFont(font);
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Qtiker");
    QApplication::setOrganizationName("qtiker");
    QApplication::setWindowIcon(QIcon(":/assets/qtiker-64.png"));

    Clicker window;
    window.show();

    return app.exec();
}

#include "main.moc"
