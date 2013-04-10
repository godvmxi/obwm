#include "home.h"
#include "ui_home.h"

Home::Home(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Home)
{
    ui->setupUi(this);
    home = new IconPushButton();
    back = new IconPushButton();
    resize = new IconPushButton();
    screen = new IconPushButton();
    //home->setFixedSize(50,50);
    this->setFixedSize(300,150);
    //home->setIconText("Home");
    //back->setIconText("Back");
    //resize->setIconText("Resize");

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addWidget(home);
    mainLayout->addWidget(back);
    mainLayout->addWidget(resize);
    mainLayout->addWidget(screen);

    homeLabel= new QLabel("Home");
    backLabel= new QLabel("Back");
    resizeLabel= new QLabel("Resize");
    screenLabel= new QLabel("Screen");
    QHBoxLayout *labelLayout = new QHBoxLayout();
    labelLayout->addWidget(homeLabel);
    labelLayout->addWidget(backLabel);
    labelLayout->addWidget(resizeLabel);
    labelLayout->addWidget(screenLabel);

    QVBoxLayout *main =  new QVBoxLayout();

    main->addLayout(mainLayout);
    main->addLayout(labelLayout);

    this->setLayout(main);
    QPalette palette;
    palette.setBrush(QPalette::Background,QColor(Qt::cyan));
    this->setPalette(palette);


}

Home::~Home()
{
    delete ui;
}
