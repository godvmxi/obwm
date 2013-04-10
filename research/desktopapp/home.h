#ifndef HOME_H
#define HOME_H

#include <QWidget>
#include "iconpushbutton.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "home.h"

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

private:
    Ui::Home *ui;

};

#endif // HOME_H
