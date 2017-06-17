#-------------------------------------------------
#
# Project created by QtCreator 2017-03-23T21:26:19
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qcloud
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += c++11

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
#INCLUDEPATH += "D:/Program Files/boost_1_63_0"
#LIBS += -L "D:/Program Files/boost_1_63_0/stage/lib"
#LIBS += -lboost_serialization-mgw53-mt-1_63

SOURCES += \
    jsoncpp/jsoncpp.cpp \
    md5/md5.cc \
    main.cpp \
    login_dialog.cpp \
    signup_dialog.cpp \
    util.cpp \
    mainwindow.cpp \
    App.cpp \
    tcpsocket.cpp \
    Task.cpp \
    global.cpp \
    Model.cpp \
    updatePwd_dialog.cpp

HEADERS  += \
    jsoncpp/json/json.h \
    login_dialog.h \
    signup_dialog.h \
    util.h \
    mainwindow.h \
    App.h \
    protocol.h \
    tcpsocket.h \
    Task.h \
    global.h \
    Model.h \
    ProgressBarDelegate.h \
    updatePwd_dialog.h

FORMS    += \
    login_dialog.ui \
    signup_dialog.ui \
    mainwindow.ui \
    updatePwd_dialog.ui
