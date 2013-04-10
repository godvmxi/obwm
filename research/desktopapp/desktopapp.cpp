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
    this->appManager = new AppManager(this,apps);
    this->appManager->setGeometry(200,200,1000,600);


    IconPushButton *test = new IconPushButton(this,NULL);
    test->setGeometry(20,20,100,100);
    ui->label_winid->setText(QString("winid-> %1").arg(this->winId()));
            /*
    obCall = new ObCall(this,"10.140.28.32",3333);
    connect(this->appManager,SIGNAL(execObCmdSignal(APP_COM)),this->obCall,SLOT(call(APP_COM)));
    connect(this->obCall,SIGNAL(returnDate(APP_COM,QString,QString)),this->appManager,SLOT(execObAppCmdSlot(APP_COM,QString,QString)));
*/

}

DesktopApp::~DesktopApp()
{
    delete ui;
}
void DesktopApp::closeEvent(QCloseEvent *)
{
    this->appManager->homeBack->close();
    this->appManager->close();
}
