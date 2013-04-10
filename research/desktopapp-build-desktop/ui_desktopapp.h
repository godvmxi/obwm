/********************************************************************************
** Form generated from reading UI file 'desktopapp.ui'
**
** Created: 
**      by: Qt User Interface Compiler version 4.7.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DESKTOPAPP_H
#define UI_DESKTOPAPP_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCalendarWidget>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DesktopApp
{
public:
    QWidget *centralWidget;
    QLabel *label;
    QLabel *label_winid;
    QPushButton *pushButton;
    QCalendarWidget *calendarWidget;

    void setupUi(QMainWindow *DesktopApp)
    {
        if (DesktopApp->objectName().isEmpty())
            DesktopApp->setObjectName(QString::fromUtf8("DesktopApp"));
        DesktopApp->resize(927, 636);
        centralWidget = new QWidget(DesktopApp);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        label = new QLabel(centralWidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(300, 10, 151, 51));
        label_winid = new QLabel(centralWidget);
        label_winid->setObjectName(QString::fromUtf8("label_winid"));
        label_winid->setGeometry(QRect(470, 10, 421, 41));
        pushButton = new QPushButton(centralWidget);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(810, 10, 90, 28));
        calendarWidget = new QCalendarWidget(centralWidget);
        calendarWidget->setObjectName(QString::fromUtf8("calendarWidget"));
        calendarWidget->setGeometry(QRect(0, 0, 288, 177));
        DesktopApp->setCentralWidget(centralWidget);

        retranslateUi(DesktopApp);

        QMetaObject::connectSlotsByName(DesktopApp);
    } // setupUi

    void retranslateUi(QMainWindow *DesktopApp)
    {
        DesktopApp->setWindowTitle(QApplication::translate("DesktopApp", "DesktopApp", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("DesktopApp", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:16pt; color:#ff0000;\">Desktop APP</span></p></body></html>", 0, QApplication::UnicodeUTF8));
        label_winid->setText(QApplication::translate("DesktopApp", "WINID", 0, QApplication::UnicodeUTF8));
        pushButton->setText(QApplication::translate("DesktopApp", "Close", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        calendarWidget->setWhatsThis(QApplication::translate("DesktopApp", "States", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
    } // retranslateUi

};

namespace Ui {
    class DesktopApp: public Ui_DesktopApp {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DESKTOPAPP_H
