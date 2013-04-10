#ifndef DESKTOPAPP_H
#define DESKTOPAPP_H

#include <QMainWindow>
#include "appmanager.h"
#include <QLineEdit>
#include <QMovie>
#include "home.h"
#include <QCloseEvent>
#include "obcall.h"
#include  <QTimer>

namespace Ui {
    class DesktopApp;
}

class DesktopApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit DesktopApp(QWidget *parent = 0);
    ~DesktopApp();
public slots:
    void setSelfLayer(void);

private slots:


    void on_pushButton_clicked();

private:
    Ui::DesktopApp *ui;
    Home *home_back;
    AppManager *appManager;
    void closeEvent(QCloseEvent *);
    ObCall *obCall;
    QTimer *timer;


};

#endif // DESKTOPAPP_H
