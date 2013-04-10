#ifndef DESKTOPAPP_H
#define DESKTOPAPP_H

#include <QMainWindow>
#include "appmanager.h"
#include <QLineEdit>
#include <QMovie>
#include "home.h"
#include <QCloseEvent>
#include "obcall.h"

namespace Ui {
    class DesktopApp;
}

class DesktopApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit DesktopApp(QWidget *parent = 0);
    ~DesktopApp();

private:
    Ui::DesktopApp *ui;
    Home *home_back;
    AppManager *appManager;
    void closeEvent(QCloseEvent *);
    ObCall *obCall;

};

#endif // DESKTOPAPP_H
