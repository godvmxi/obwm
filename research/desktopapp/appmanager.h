#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QWidget>
#include <QPalette>
#include <QRect>
#include <QString>
#include "filetype.h"
#include <QDebug>
#include <QPushButton>
#include "iconpushbutton.h"
#include <QMessageBox>
#include "home.h"
#include <QTimer>
#include "obcall.h"

typedef struct{
    QList<APP *> running;
    QList<APP *>top;
    QList<APP *>bottom;
    QStack<APP *>stack;
    APP *main;
    APP *extend;
}SCREEN;

namespace Ui {
    class AppManager;
}

class AppManager : public QWidget
{
    Q_OBJECT

public:
    explicit AppManager(QWidget *parent,QList<APP> apps);

    ~AppManager();
    bool setAppList(QList<APP> apps,int size);
    bool placeAppIcons(void);
    QList<APP_COM> getAppLists(void);

    APP_COM appCom;
    bool timeout;
   //APP_COM execObCmd();
    Home *homeBack;

    bool setSelfLayer(int desktopWinid,int homeWinid);
    void cleanProcess(void);

public slots:
    //bool update_app(APP_COM app);
    //bool update_apps_list(QList<APP_COM> apps);
    bool iconClick(void *ptr);
    bool appProcessChanged(QProcess::ProcessState newState);
    bool homeButtonHomeMsgSlot(void* app);
    bool homeButtonBackMsgSlot(void* app);
    bool homeButtonResizeMsgSlot(void* app);
    bool homeButtonScreenMsgSlot(void* app);
    bool execObAppsCmdSlot(QList<APP_COM>coms,QString state,QString error);
    bool execObAppCmdSlot(APP_COM com,QString state,QString error);
    bool execObCmdTimeout();
signals:

    bool execObCmdSignal(APP_COM com);

private:
    Ui::AppManager *ui;
    SCREEN main;
    //SCREEN extend;
    QList<APP *> apps;


    int udpId;
    QTimer *timer;
    ObCall *obCall;



    bool showRunningApp(APP *app);
    bool startAppFromButton(APP *app);

    bool execObCmd(APP_COM *);
    bool execObCmdAndWait(APP_COM *,int timeoutMs);

    bool getAppInfoFromWinid(APP_COM *app);
};

#endif // APPMANAGER_H
