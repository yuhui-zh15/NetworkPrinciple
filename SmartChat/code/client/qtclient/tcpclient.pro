#-------------------------------------------------
#
# Project created by QtCreator 2015-08-29T13:48:44
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tcpclient
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    logindialog.cpp

HEADERS  += \
    mainwindow.h \
    logindialog.h

FORMS    += \
    mainwindow.ui \
    logindialog.ui

RESOURCES += icons.qrc
