#include "autocontrollerwindow.h"

#include <QApplication>

#include "autocontrollerdefines.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    autocontrollerwindow w;
    w.show();
    return a.exec();
}
