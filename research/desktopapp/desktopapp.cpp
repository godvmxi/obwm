#include "desktopapp.h"
#include "ui_desktopapp.h"
#include "iconpushbutton.h"


DesktopApp::DesktopApp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DesktopApp)
{

    QList<APP> apps;
    APP app;
    app.cmd = QString(tr("winid"));
    for(int i = 0;i<5;i++)
    {
        apps.append(app);
    }

    //appManager->show();
    //ui->formLayout->addWidget(appManager);
    //this->setLayout(ui->formLayout);



    ui->setupUi(this);
    this->move(0,0);
    this->appManager = new AppManager(this,apps);
    this->appManager->setGeometry(200,200,1000,600);


    //IconPushButton *test = new IconPushButton(this,NULL);
   // test->setGeometry(20,20,100,100);
    //connect(test,SIGNAL(buttonClick(void*)),this,SLOT(setSelfLayer()));
    ui->label_winid->setText(QString("winid-> %1").arg(this->winId()));

    obCall = new ObCall(this,"127.0.0.1",3333);
    connect(this->appManager,SIGNAL(execObCmdSignal(APP_COM)),this->obCall,SLOT(call(APP_COM)));
    connect(this->obCall,SIGNAL(returnDate(APP_COM,QString,QString)),this->appManager,SLOT(execObAppCmdSlot(APP_COM,QString,QString)));

    this->timer = new QTimer(this);
    this->timer->setSingleShot(true);
    connect(this->timer,SIGNAL(timeout()),this,SLOT(setSelfLayer()));
    this->timer->start(100);



}

DesktopApp::~DesktopApp()
{
    delete ui;
}
void DesktopApp::closeEvent(QCloseEvent *)
{
    qDebug()<<"close event;";
    this->appManager->homeBack->close();
    this->appManager->cleanProcess();
    this->appManager->close();
}
void DesktopApp::setSelfLayer(void)
{
    qDebug()<<"qtimer timerout";
    this->appManager->setSelfLayer(this->winId(),this->appManager->homeBack->winId());
}



void DesktopApp::on_pushButton_clicked()
{
    this->close();
}
