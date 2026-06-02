#include "clicker.h"

#include "appversion.h"
#include "game_rules.h"

#include <QApplication>
#include <QBuffer>
#include <QCloseEvent>
#include <QDialog>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIODevice>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QSvgRenderer>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

Clicker::Clicker(QWidget *parent) : QWidget(parent) {
    setupWindow();
    loadGame();
    buildUi();
    startIncomeTimer();
    refreshUi();
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

bool Clicker::eventFilter(QObject *watched, QEvent *event) {
    if (watched == clickButton && event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::MiddleButton) {
            showTux();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void Clicker::makeClick() {
    game.score += game.perClick;
    refreshUi();
}

void Clicker::buyClickUpgrade() {
    if (game.score < game.clickCost) {
        return;
    }

    game.score -= game.clickCost;
    game.perClick += 1;
    game.clickCost = nextCost(game.clickCost, 10);

    saveGame();
    refreshUi();
}

void Clicker::buyIncomeUpgrade() {
    if (game.score < game.incomeCost) {
        return;
    }

    game.score -= game.incomeCost;
    game.perSecond += 1;
    game.incomeCost = nextCost(game.incomeCost, 15);

    saveGame();
    refreshUi();
}

void Clicker::addPassiveIncome() {
    if (game.perSecond == 0) {
        return;
    }

    game.score += game.perSecond;
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
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto *title = new QLabel("Settings", dialog);
    setFont(title, 13, true);

    auto *resetBox = new QFrame(dialog);
    resetBox->setFrameShape(QFrame::StyledPanel);

    auto *resetLayout = new QHBoxLayout(resetBox);
    resetLayout->setContentsMargins(10, 8, 10, 8);
    resetLayout->setSpacing(8);

    auto *resetText = new QLabel("Reset saved progress", resetBox);
    setFont(resetText, resetText->font().pointSize(), true);

    auto *resetButton = new QPushButton("Reset", resetBox);
    connect(resetButton, &QPushButton::clicked, this, [this, dialog]() {
        auto answer = QMessageBox::question(
            dialog,
            "Reset",
            "Are you sure you want to delete everything?",
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

    auto *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    layout->addWidget(title);
    layout->addWidget(resetBox);
    layout->addStretch();
    layout->addWidget(closeButton);

    dialog->show();
}

void Clicker::showChangelog() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Qtiker Release Notes");
    dialog->setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    dialog->resize(340, 260);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *titleLayout = new QHBoxLayout();
    titleLayout->setSpacing(8);

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

void Clicker::showTux() {
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

void Clicker::setupWindow() {
    setWindowTitle("Qtiker");
    setWindowIcon(QIcon(":/assets/qtiker-64.png"));
    setMinimumSize(340, 360);
    resize(360, 420);
}

void Clicker::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(10);

    changelogButton = new QPushButton(QString("v%1").arg(AppVersion), this);
    changelogButton->setIconSize(QSize(16, 16));
    changelogButton->setToolTip("Release notes");
    changelogButton->setFixedSize(86, 28);
    setFont(changelogButton, changelogButton->font().pointSize(), true);
    connect(changelogButton, &QPushButton::clicked, this, &Clicker::showChangelog);

    settingsButton = new QPushButton(this);
    settingsButton->setIconSize(QSize(16, 16));
    settingsButton->setToolTip("Settings");
    settingsButton->setFixedSize(32, 28);
    connect(settingsButton, &QPushButton::clicked, this, &Clicker::showSettings);
    applyThemeIcons();

    auto *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(6);
    topLayout->addStretch();
    topLayout->addWidget(changelogButton);
    topLayout->addWidget(settingsButton);

    scoreLabel = new QLabel(this);
    scoreLabel->setAlignment(Qt::AlignCenter);
    setFont(scoreLabel, 30, true);

    statsLabel = new QLabel(this);
    statsLabel->setAlignment(Qt::AlignCenter);
    setFont(statsLabel, statsLabel->font().pointSize(), true);

    clickButton = new QPushButton("Click", this);
    clickButton->setMinimumHeight(76);
    setFont(clickButton, 18, true);
    clickButton->installEventFilter(this);
    connect(clickButton, &QPushButton::clicked, this, &Clicker::makeClick);

    auto *upgradesBox = createUpgradesBox();

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(scoreLabel);
    mainLayout->addWidget(statsLabel);
    mainLayout->addWidget(clickButton, 1);
    mainLayout->addWidget(upgradesBox);
}

QFrame *Clicker::createUpgradesBox() {
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

void Clicker::startIncomeTimer() {
    incomeTimer = new QTimer(this);
    incomeTimer->setInterval(1000);
    connect(incomeTimer, &QTimer::timeout, this, &Clicker::addPassiveIncome);
    incomeTimer->start();
}

void Clicker::applyThemeIcons() {
    if (changelogButton != nullptr) {
        const auto iconColor = changelogButton->palette().color(QPalette::ButtonText);
        changelogButton->setIcon(tintedSvgIcon(":/assets/ui/inbox.svg", iconColor, QSize(16, 16)));
    }

    if (settingsButton != nullptr) {
        const auto iconColor = settingsButton->palette().color(QPalette::ButtonText);
        settingsButton->setIcon(tintedSvgIcon(":/assets/ui/settings.svg", iconColor, QSize(16, 16)));
    }
}

void Clicker::refreshUi() {
    scoreLabel->setText(QString::number(game.score));
    statsLabel->setText(QString("Click +%1 · Income +%2/sec")
                            .arg(game.perClick)
                            .arg(game.perSecond));

    clickUpgradeButton->setText(QString("Click +1       %1").arg(game.clickCost));
    incomeUpgradeButton->setText(QString("Income +1/sec  %1").arg(game.incomeCost));

    clickUpgradeButton->setEnabled(game.score >= game.clickCost);
    incomeUpgradeButton->setEnabled(game.score >= game.incomeCost);
}

void Clicker::loadGame() {
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

void Clicker::saveGame() const {
    QSettings settings("qtiker", "qtiker");
    settings.setValue("score", game.score);
    settings.setValue("perClick", game.perClick);
    settings.setValue("perSecond", game.perSecond);
    settings.setValue("clickCost", game.clickCost);
    settings.setValue("incomeCost", game.incomeCost);
}

int Clicker::nextCost(int currentCost, int extra) const {
    return nextUpgradeCost(currentCost, extra);
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

    return QStringLiteral(R"(
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
        <h3><span class="version">0.1.1</span> <span class="date">2026-06-02</span></h3>
        <table>
            <tr>
                <td class="icon"><img src="%1" width="14" height="14"></td>
                <td class="entry">Added compact in-game release notes.</td>
            </tr>
            <tr>
                <td class="icon"><img src="%1" width="14" height="14"></td>
                <td class="entry">Added colored release note markers.</td>
            </tr>
            <tr>
                <td class="icon"><img src="%1" width="14" height="14"></td>
                <td class="entry">Added a settings dialog.</td>
            </tr>
            <tr>
                <td class="icon"><img src="%1" width="14" height="14"></td>
                <td class="entry">Added reset confirmation.</td>
            </tr>
            <tr>
                <td class="icon"><img src="%2" width="13" height="13"></td>
                <td class="entry">Moved reset into settings.</td>
            </tr>
            <tr>
                <td class="icon"><img src="%2" width="13" height="13"></td>
                <td class="entry">Updated 0.1.1 metadata.</td>
            </tr>
        </table>
        <h3><span class="version">0.1.0</span> <span class="date">2026-06-01</span></h3>
        <table>
            <tr>
                <td class="icon"><img src="%2" width="13" height="13"></td>
                <td class="entry">Initial packaged version.</td>
            </tr>
        </table>
    )").arg(addIcon, changedIcon);
}

void Clicker::setFont(QWidget *widget, int pointSize, bool bold) {
    auto font = widget->font();
    if (pointSize > 0) {
        font.setPointSize(pointSize);
    }
    font.setBold(bold);
    widget->setFont(font);
}
