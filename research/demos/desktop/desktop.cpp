#include "desktop.h"
#include "ui_desktop.h"
#include <QString>
#include <QDebug>

desktop::desktop(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::desktop)
{
    ui->setupUi(this);
    QString titile = this->windowTitle() + "  <-->" +QString::number(this->winId());
    qDebug()<<titile;
    this->setWindowTitle(titile);
    ui->label_state->setText(QString::number(this->winId()));
}

desktop::~desktop()
{
    delete ui;
}
