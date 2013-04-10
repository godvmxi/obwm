#ifndef FILETYPE_H
#define FILETYPE_H
#include <QString>
#include <QRect>
#include <QList>
#include <QStack>
#include <QProcess>
#include <QIcon>

typedef enum{
    OB_SET_FULL = 3,
    OB_SET_MAX = 4,
    OB_SET_MIN = 5,
    OB_SET_BOTTOM = 6,
    OB_SET_TOP = 7,
    OB_SET_NORMAL = 8,
    OB_GET_APPS_LIST = 9,
    OB_GET_APP_STATE = 10,
    OB_EXIT = 11,
    OB_RESTART = 12,
    OB_REFRESH = 13,
    OB_RESIZE = 14,
    OB_MOVE = 15,
    OB_RESIZE_MOVE = 16,
    OB_RAISE = 21,
    OB_FOCUS = 22,
    OB_DECORATE = 23
}METHOD;
typedef struct{
    QIcon iconNormal;
    QIcon iconFocus;
    QIcon iconPressDown;
    QIcon iconRunning;
}ICONS;
typedef struct{
    int method;
    int id;
    int winid;
    int pid;
    QRect   rect;
    bool max;
    bool full;
    bool undecorate;
    bool min;
    int layer;
}APP_COM;

typedef struct{
    int sequence;
    QProcess *process;

    ICONS icons;
    QString name;
    QString cmd;
    QString para;

    APP_COM info;


}APP_INFO;

typedef struct{
    QString name;
    QString cmd;
    int max_run;
    APP_INFO run_main;
    APP_INFO run_extend;
}APP;



#endif // FILETYPE_H
