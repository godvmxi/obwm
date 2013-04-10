#ifndef DESKTOP_H
#define DESKTOP_H

#include <QMainWindow>

namespace Ui {
    class desktop;
}

class desktop : public QMainWindow
{
    Q_OBJECT

public:
    explicit desktop(QWidget *parent = 0);
    ~desktop();

private:
    Ui::desktop *ui;
};

#endif // DESKTOP_H
