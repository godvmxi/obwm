#include <QtGui/QApplication>
#include "desktopapp.h"
#include "iconpushbutton.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DesktopApp w;


    w.show();


    return a.exec();
}
