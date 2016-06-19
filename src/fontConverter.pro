#-------------------------------------------------
#
# Project created by QtCreator 2013-10-12T21:08:58
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = fontConverter
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    glcd.cpp \
    glcdscene.cpp \
    converter.cpp

HEADERS  += mainwindow.h \
    glcd.h \
    glcdscene.h \
    converter.h

FORMS    += mainwindow.ui
