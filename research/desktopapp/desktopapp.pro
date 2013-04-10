#-------------------------------------------------
#
# Project created by QtCreator 2013-04-08T11:32:38
#
#------------------------------------------------
QT       += core gui xml network


TARGET = desktopapp
TEMPLATE = app


SOURCES += main.cpp\
        desktopapp.cpp \
    appmanager.cpp \
    iconpushbutton.cpp \
    home.cpp \
    obcall.cpp

HEADERS  += desktopapp.h \
    appmanager.h \
    filetype.h \
    iconpushbutton.h \
    home.h \
    obcall.h

FORMS    += desktopapp.ui \
    appmanager.ui \
    home.ui
