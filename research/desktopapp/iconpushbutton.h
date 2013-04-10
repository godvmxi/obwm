#ifndef ICONPUSHBUTTON_H
#define ICONPUSHBUTTON_H

#include <QLabel>
#include <QEvent>
#include <QMovie>


class IconPushButton : public QLabel
{
    Q_OBJECT
public:
    //explicit IconPushButton(QLabel *parent = 0,void * ptr);
    IconPushButton(QWidget *parent = 0,void *ptr=NULL);
    bool setIconText(QString text);

signals:
    void buttonClick(void * ptr);

public slots:
private :
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *ev);
    //void mouseMoveEvent(QMouseEvent *ev);


    void *ptr;
    QMovie *gif;

};

#endif // ICONPUSHBUTTON_H
