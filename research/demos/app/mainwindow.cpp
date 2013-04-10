#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QDebug>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label->setText(QString::number(this->winId()));
}

MainWindow::~MainWindow()
{
    delete ui;
}
