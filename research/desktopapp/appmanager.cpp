#include "appmanager.h"
#include "ui_appmanager.h"

AppManager::AppManager(QWidget *parent,QList<APP> apps) :
    QWidget(parent),
    ui(new Ui::AppManager)
{



    QPalette palette;
    palette.setBrush(QPalette::Background,QColor(Qt::lightGray));
    this->setPalette(palette);
    this->setAutoFillBackground(true);
    this->setFixedSize(1000,1000);
    ui->setupUi(this);

    setAppList(apps,apps.size());
    this->placeAppIcons();

    this->udpId = 0;
    this->timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(this->timer,SIGNAL(timeout()),this,SLOT(execObCmdTimeout()));

    this->homeBack = new Home();
    this->homeBack->show();
    this->homeBack->move(300,0);
    connect(this->homeBack->home,SIGNAL(buttonClick(void*)),this,SLOT(homeButtonHomeMsgSlot(void*)));
    connect(this->homeBack->back,SIGNAL(buttonClick(void*)),this,SLOT(homeButtonBackMsgSlot(void*)));
    connect(this->homeBack->resize,SIGNAL(buttonClick(void*)),this,SLOT(homeButtonResizeMsgSlot(void*)));
    connect(this->homeBack->screen,SIGNAL(buttonClick(void*)),this,SLOT(homeButtonScreenMsgSlot(void*)));

    /*
    obCall = new ObCall(this,"127.0.0.1",3333);
    connect(this,SIGNAL(execObCmdSignal(APP_COM)),this->obCall,SLOT(call(APP_COM)));
    connect(this->obCall,SIGNAL(returnDate(APP_COM,QString,QString)),this,SLOT(execObAppCmdSlot(APP_COM,QString,QString)));

    */
    ui->label_winid->setText(QString("winid %1  %2  %3").arg(parent->winId()).arg(this->winId()).arg(this->homeBack->winId()));


    qDebug()<<parent->winId()<<"   "<<this->winId() <<"   "<<this->homeBack->winId();

    //this->setSelfLayer(parent->winId(),this->homeBack->winId());



}

AppManager::~AppManager()
{
    delete ui;

}
bool AppManager::placeAppIcons(void){
    APP *app;
    int i = 1;
    IconPushButton *button;


    qDebug()<<"apps size -> "<<this->apps.size();

    foreach(app,apps){
        app->max_run = i;
        button = new IconPushButton(this,app);
        connect(button,SIGNAL(buttonClick(void*)),this,SLOT(iconClick(void*)));

        button->setGeometry(120*i++,50,100,100);


    }
    foreach(app,apps){
        qDebug()<<"max_run"<<app->max_run <<"  cmd->"<<app->cmd;
    }

    return true;

}
bool AppManager::setAppList(QList<APP> apps ,int size)
{
    APP *tmp = NULL;
    if(size <= 0)
            return false;
    for(int i = 0;i<size;i++){
        tmp = new APP;
        *tmp = apps.at(i);

        tmp->max_run = 1;
        tmp->run_main.process = new QProcess();
        //tmp->run_extend.process = new QProcess();
        //tmp->run_extend.process->setProcessChannelMode(QProcess::MergedChannels);
        tmp->run_main.process->setProcessChannelMode(QProcess::MergedChannels);
        //connect(tmp->run_extend.process,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(appProcessChanged(QProcess::ProcessState)));
        connect(tmp->run_main.process,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(appProcessChanged(QProcess::ProcessState)));
        memset(&(tmp->run_main.info),0,sizeof(APP_COM));
        memset(&(tmp->run_extend.info),0,sizeof(APP_COM));
        this->apps.append(tmp);
    }
    return true;
}
bool AppManager::iconClick(void *ptr)
{
    QString msg ;
    APP *app = (APP *)ptr;
    msg.sprintf("from button ->%d",app->max_run);
    ui->label_status->setText(msg);

    //this->setSelfLayer((QWidget *(this->parent()))->winId(),this->homeBack->winId());


    QProcess::ProcessState now =  app->run_main.process->state();

    qDebug()<<"click"<<now;
    switch(now)
    {
    case QProcess::Starting :
        qDebug()<<"just wait ....";
        break;
    case QProcess::Running :
        showRunningApp(app);
        break;
    case QProcess::NotRunning:
        startAppFromButton(app);
        break;

    }
}
bool AppManager::setSelfLayer(int desktopWinid,int homeWinid)
{

    qDebug()<<"desktop id ->"<< desktopWinid <<"fuck ->"<<homeWinid;



    this->appCom.method = OB_SET_BOTTOM;
    this->appCom.winid = desktopWinid;
    execObCmd(&this->appCom);

    this->appCom.method = OB_SET_TOP;
    this->appCom.winid = homeWinid;
    execObCmd(&this->appCom);

}
bool AppManager::appProcessChanged(QProcess::ProcessState newState)
{
    QProcess *src = dynamic_cast<QProcess*>(sender());
    APP *app = NULL,*tmp = NULL;
   // qDebug()<<"process new state"<<newState<<" src->"<<src->pid();
    //qDebug()<<"process kill state"<<newState<<" src->"<<src->pid();
    foreach(tmp,this->apps)
    {
        if(tmp->run_main.process == src)
        {
            qDebug()<<"find the process"<<src<<"  "<<app->run_main.process;
            app = tmp;
            break;
        }
    }
    if(app == NULL)
    {
        qDebug()<<"can,t find process handle";
        return false;
    }
    switch(newState){
    case QProcess::NotRunning:
        qDebug()<<"process is not run.."<<src->pid();
        if(this->main.main  == app )
        {
            qDebug()<<"close main screen max app";
            this->main.main = NULL;
        }
        if(this->main.extend == app)
        {
            qDebug()<<"close extend screen max app";
            this->main.extend = NULL;
        }
        if(this->main.resize == app)
        {
            qDebug()<<"close extend screen max app";
            this->main.resize = NULL;
        }
        this->main.running.removeAll(app);
        memset(&(app->run_main.info),0,sizeof(APP_INFO));

        break;
    case QProcess::Running:
        qDebug()<<"process is running.."<<src->pid();
        //getAppInfoFromWinid(*(app->run_main.info));
        break;
    case QProcess::Starting:
        qDebug()<<"process is staring.."<<src->pid();
        break;
    default :
        break;
    }

    return true;
}
bool AppManager::showRunningApp(APP *app)
{
    app->run_main.info.max = true;
    app->run_main.info.min = false;
    app->run_main.info.method = OB_SET_MAX;
    this->execObCmd(&app->run_main.info);
}

bool AppManager::startAppFromButton(APP *app)
{
    qDebug()<<"start app "<<app->cmd<<app->run_main.process->state();
    app->run_main.process->start(app->cmd);
    qDebug()<<"create pid ->";
    qDebug()<<"create pid ->"<<app->run_main.process->pid();
    app->run_main.info.pid = app->run_main.process->pid();
    this->main.running.append(app);
    //add
    this->main.main = app;
    return true;
}


bool AppManager::homeButtonHomeMsgSlot(void* ptr)
{
    qDebug()<<"recevie home msg";
    ui->textBrowser->insertPlainText("recevie home msg\n");
    APP *app = NULL,*tmp= NULL;
    this->main.main = NULL;
    this->main.extend = NULL;
    foreach(tmp,this->main.running)
    {
        if(tmp == this->main.resize){
            qDebug()<<"current windows is resize window";
            continue;
        }
        else
        {
            tmp->run_main.info.min = true;
            tmp->run_main.info.method = OB_SET_MIN;
            this->execObCmd(&tmp->run_main.info);
        }
    }



    return true;

}

bool AppManager::homeButtonBackMsgSlot(void* ptr)
{
    qDebug()<<"recevie back msg";
     //ui->textBrowser->insertPlainText("recevie back msg\n");
     ui->textBrowser->append("recevie back msg");

     if(this->main.main != NULL)
     {
         this->main.main->run_main.process->close();
         this->main.main = NULL;
     }



}
bool AppManager::homeButtonResizeMsgSlot(void* ptr)
{
    qDebug()<<"recevie resize msg";
     //ui->textBrowser->insertPlainText("recevie resize msg\n");
     ui->textBrowser->append("recevie back msg");

     APP *app;
}
bool AppManager::homeButtonScreenMsgSlot(void* ptr)
{
    qDebug()<<"recevie screen msg";
     //ui->textBrowser->insertPlainText("recevie screen msg\n");
     ui->textBrowser->append("recevie back msg");




}
bool AppManager::execObCmd(APP_COM *com)
{
   qDebug()<<"send in data"<<com->id<<" method->"<<com->method;
    emit execObCmdSignal(*com);
}

bool AppManager::execObCmdAndWait(APP_COM *com,int timeoutMs)
{
    emit execObCmdSignal(*com);
    this->timeout = false;
    qDebug()<<"wait for result ->",timeoutMs;
    this->timer->start(timeoutMs);
    while(this->timer->isActive());
    if(!this->timeout)
    {
        *com = this->appCom;
    }
    else
    {
        return false;
    }




}
bool AppManager::execObAppsCmdSlot(QList<APP_COM>coms,QString state,QString error)
{
    if(this->timer->isActive())
        this->timer->stop();
}

bool AppManager::execObAppCmdSlot(APP_COM com,QString state,QString error)
{
    this->appCom = com;
    if(this->timer->isActive())
        this->timer->stop();
    else
    {

        qDebug()<<"receiver data ????"<<state<<error<<"  "<<com.id<<"  "<<com.method << com.winid;

    }

}
bool AppManager::execObCmdTimeout()
{
    qDebug()<<"ob cm dexec timeout";
    this->timeout = true;
}
bool AppManager::getAppInfoFromWinid(APP_COM *app)
{
    bool result = true;
    do{
        qDebug()<<"try to get msg from winid ->"<<app->pid;
        result = execObCmdAndWait(app,2000);
      } while(!result);
     ui->textBrowser->append(QString("winid %1 -- %2").arg(app->pid).arg(app->winid));
}
void AppManager::cleanProcess(void)
{
    APP *app;
    foreach(app,this->apps)
    {
        app->run_main.process->close();
        app->run_extend.process->close();
    }
}

