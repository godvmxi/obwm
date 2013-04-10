#include "iconpushbutton.h"

IconPushButton::IconPushButton(QWidget *parent,void *ptr) :
    QLabel(parent)
{

    this->gif = new QMovie("/home/sunbird/dev/desktopapp/images/flower.gif");
    this->setMovie(this->gif);
    this->gif->start();
    this->ptr = ptr;

    this->setWindowFlags(Qt::FramelessWindowHint);
    QPalette palette;
    palette.setBrush(QPalette::Background,QColor(0,0xff,0,0));
    this->setPalette(palette);
    //this->setMouseTracking(true);


}

void IconPushButton::mouseReleaseEvent(QMouseEvent *e)
{
    this->gif->stop();
    this->gif->setFileName("/home/sunbird/dev/desktopapp/images/flower.gif");
    this->gif->start();
    emit(buttonClick(this->ptr));

}
void IconPushButton::mousePressEvent(QMouseEvent *ev)
{
    this->gif->stop();
    this->gif->setFileName("/home/sunbird/dev/desktopapp/images/heart.gif");
    this->gif->start();
}
bool IconPushButton::setIconText(QString text)
{
    //this->setText(text);
}

/*
void IconPushButton::mouseMoveEvent(QMouseEvent *ev)
{
    this->gif->stop();
    this->gif->setFileName("/home/sunbird/dev/desktopapp/images/love.gif");
    this->gif->start();
}
*/
