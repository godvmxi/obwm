#include "home.h"
#include "ui_home.h"

Home::Home(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Home)
{
    ui->setupUi(this);
    home = new IconPushButton();
    back = new IconPushButton();
    this->setFixedSize(300,200);
    home->setIconText("Home");
    back->setIconText("Back");
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addWidget(home);
    mainLayout->addWidget(back);
    this->setLayout(mainLayout);

    QPalette palette;
    palette.setBrush(QPalette::Background,QColor(Qt::cyan));
    this->setPalette(palette);


}

Home::~Home()
{
    delete ui;
}