#include <QApplication>
#include "App.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    App app("../client/config.json");
    app.start();

    return a.exec();
}
