#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QString titile = "Home<-->" +QString::number(this->winId());
    this->setWindowTitle(titile);
}

MainWindow::~MainWindow()
{
    delete ui;
}
