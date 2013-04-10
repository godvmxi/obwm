#ifndef HOME_H
#define HOME_H

#include <QWidget>
#include "iconpushbutton.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "home.h"
#include <QLabel>

namespace Ui {
    class Home;
}

class Home : public QWidget
{
    Q_OBJECT

public:
    explicit Home(QWidget *parent = 0);
    ~Home();
    IconPushButton *home;
    IconPushButton *back;
    IconPushButton *resize;
    IconPushButton *screen;
    QLabel *homeLabel;
    QLabel *backLabel;
    QLabel *resizeLabel;
    QLabel *screenLabel;

private:
    Ui::Home *ui;

};

#endif // HOME_H
