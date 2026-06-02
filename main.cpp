#include "clicker.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Qtiker");
    QApplication::setOrganizationName("qtiker");
    QApplication::setWindowIcon(QIcon(":/assets/qtiker-64.png"));

    Clicker window;
    window.show();

    return app.exec();
}
