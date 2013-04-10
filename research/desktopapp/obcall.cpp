#include "obcall.h"
#include <QtXml/QDomDocument>
#include <QDebug>


ObCall::ObCall(QObject *parent,QString ipv4,int port):
    QObject(parent)
{
    this->ipv4 = ipv4;
    this->port = port;
    initSocket();
}

void ObCall::initSocket()
{

    udp = new QUdpSocket(this);
    udp->bind();
    connect(udp,SIGNAL(readyRead()),this,SLOT(receiveData()));
}

void ObCall::call(APP_COM appinfo)
{
    qDebug()<<"get data"<<appinfo.id<<" method->"<<appinfo.method;
    QString xmlStr = genXML(appinfo);
    udp->writeDatagram(qPrintable(xmlStr),QHostAddress(this->ipv4),this->port);
    //qDebug()<<xmlStr;
}

QString ObCall::genXML(APP_COM appinfo)
{
    QDomDocument doc;
    QDomText text;
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    QDomElement method = doc.createElement("method");
    root.appendChild(method);
    text = doc.createTextNode(QString::number(appinfo.method));
    method.appendChild(text);

    QDomElement id = doc.createElement("id");
    root.appendChild(id);
    text = doc.createTextNode(QString::number(appinfo.id));
    id.appendChild(text);

    QDomElement app = doc.createElement("app");
    root.appendChild(app);

        QDomElement winid = doc.createElement("winid");
        app.appendChild(winid);
        text = doc.createTextNode(QString::number(appinfo.winid));
        winid.appendChild(text);

        QDomElement pid = doc.createElement("pid");
        app.appendChild(pid);
        text = doc.createTextNode(QString::number(appinfo.pid));
        pid.appendChild(text);

    QDomElement data = doc.createElement("data");
    root.appendChild(data);

        QDomElement xpos = doc.createElement("x");
        data.appendChild(xpos);
        text = doc.createTextNode(QString::number(appinfo.rect.x()));
        xpos.appendChild(text);

        QDomElement ypos = doc.createElement("y");
        data.appendChild(ypos);
        text = doc.createTextNode(QString::number(appinfo.rect.y()));
        ypos.appendChild(text);

        QDomElement width = doc.createElement("width");
        data.appendChild(width);
        text = doc.createTextNode(QString::number(appinfo.rect.width()));
        width.appendChild(text);

        QDomElement height = doc.createElement("height");
        data.appendChild(height);
        text = doc.createTextNode(QString::number(appinfo.rect.height()));
        height.appendChild(text);

    return doc.toString();

}

void ObCall::receiveData()
{
    while(udp->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udp->pendingDatagramSize());
        udp->readDatagram(datagram.data(),datagram.size());
        //qDebug()<<datagram.data();
        returnAppInfo = xmlToAppcom(datagram);
//        qDebug()<<"xml ack data ->"<<this->returnAppInfo
        qDebug()<<returnAppInfo.id<<returnAppInfo.pid<<returnAppInfo.winid;
        emit returnDate(returnAppInfo,state,error);
    }
}

APP_COM ObCall::xmlToAppcom(QByteArray xml)
{
    //APP_COM app;
    QDomDocument doc("xml");
    doc.setContent(xml);
    QDomElement xmlRoot = doc.documentElement();
    QDomNode node =xmlRoot.firstChild();
    while(!node.isNull())
    {
        QDomElement e = node.toElement();
        if(e.tagName()==QString("id"))
        {
            app.id = e.text().toInt();
        }
        if(e.tagName()==QString("state"))
        {
            state = e.text();
        }
        if(e.tagName()==QString("data"))
        {
            QDomNode dataNode = e.firstChild();
            app.layer = 0;
            while(!dataNode.isNull())
            {
                QDomElement dataElement = dataNode.toElement();
                if(dataElement.tagName()==QString("error"))
                {
                    error = dataElement.text();
                }
                if(dataElement.tagName()==QString("app"))
                {
                    QDomNode appNode = dataElement.firstChild();
                    while(!appNode.isNull())
                    {
                        QDomElement appElement = appNode.toElement();
                        if(appElement.tagName()==QString("pid"))
                        {
                            app.pid = appElement.text().toInt();
                        }
                        if(appElement.tagName()==QString("winid"))
                        {
                            app.winid = appElement.text().toInt();
                        }
                        if(appElement.tagName()==QString("x"))
                        {
                            app.rect.setX(appElement.text().toInt());
                        }
                        if(appElement.tagName()==QString("y"))
                        {
                            app.rect.setY(appElement.text().toInt());
                        }
                        if(appElement.tagName()==QString("width"))
                        {
                            app.rect.setWidth(appElement.text().toInt());
                        }
                        if(appElement.tagName()==QString("height"))
                        {
                            app.rect.setHeight(appElement.text().toInt());
                        }
                        if(appElement.tagName()==QString("full"))
                        {
                            app.full = appElement.text().toInt();
                        }
                        if(appElement.tagName()==QString("undecorated"))
                        {
                            app.undecorate = appElement.text().toInt();
                        }
                        if(appElement.tagName()==QString("horz"))
                        {
                            app.max = appElement.text().toInt();
                        }
                        if(appElement.tagName()==QString("below") && appElement.text()=="1")
                        {
                            app.layer = -1;
                        }
                        if(appElement.tagName()==QString("above") && appElement.text()=="1")
                        {
                            app.layer = 1;
                        }

                        appNode = appNode.nextSibling();
                    }
                }
                dataNode = dataNode.nextSibling();
            }
        }
        node = node.nextSibling();
    }

    return app;
}
