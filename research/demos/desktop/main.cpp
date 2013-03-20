#include <QtGui/QApplication>
#include "desktop.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    desktop w;
    w.showMaximized();

    //w.showFullScreen();
    w.show();

    return a.exec();
}
