#ifndef OBCALL_H
#define OBCALL_H

#include "filetype.h"
#include <QString>
#include <QtNetwork/QUdpSocket>

class ObCall: public QObject
{
    Q_OBJECT

public:
    explicit ObCall(QObject *parent = 0,QString ipv4="127.0.0.1",int port = 3333);
    APP_COM returnAppInfo;


private:

    QUdpSocket *udp;
    QString genXML(APP_COM com);
    void initSocket();
    APP_COM xmlToAppcom(QByteArray xml);

    QString state;
    QString error;
    QString ipv4;
    int port;


public slots:
    void receiveData();
    void call(APP_COM appinfo);



signals:
    void returnDate(APP_COM,QString,QString);

};

#endif // OBCALL_H
