/********************************************************************************
** Form generated from reading UI file 'appmanager.ui'
**
** Created: 
**      by: Qt User Interface Compiler version 4.7.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_APPMANAGER_H
#define UI_APPMANAGER_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QTextBrowser>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AppManager
{
public:
    QLabel *label_status;
    QLabel *label_winid;
    QTextBrowser *textBrowser;

    void setupUi(QWidget *AppManager)
    {
        if (AppManager->objectName().isEmpty())
            AppManager->setObjectName(QString::fromUtf8("AppManager"));
        AppManager->resize(393, 546);
        AppManager->setMinimumSize(QSize(50, 50));
        label_status = new QLabel(AppManager);
        label_status->setObjectName(QString::fromUtf8("label_status"));
        label_status->setGeometry(QRect(40, 270, 301, 41));
        label_winid = new QLabel(AppManager);
        label_winid->setObjectName(QString::fromUtf8("label_winid"));
        label_winid->setGeometry(QRect(30, 20, 271, 41));
        textBrowser = new QTextBrowser(AppManager);
        textBrowser->setObjectName(QString::fromUtf8("textBrowser"));
        textBrowser->setGeometry(QRect(10, 230, 341, 251));

        retranslateUi(AppManager);

        QMetaObject::connectSlotsByName(AppManager);
    } // setupUi

    void retranslateUi(QWidget *AppManager)
    {
        AppManager->setWindowTitle(QApplication::translate("AppManager", "Form", 0, QApplication::UnicodeUTF8));
        label_status->setText(QString());
        label_winid->setText(QApplication::translate("AppManager", "WINID", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class AppManager: public Ui_AppManager {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_APPMANAGER_H
