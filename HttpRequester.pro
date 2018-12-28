QT += core gui widgets network

TARGET = HttpRequester
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp \
    MainWindow.cpp \
    RequestBuilder.cpp \
    ResponseViewer.cpp \
    HistoryViewer.cpp \
    Request.cpp \
    QJsonModel.cpp

HEADERS += \
    MainWindow.hpp \
    RequestBuilder.hpp \
    ResponseViewer.hpp \
    HistoryViewer.hpp \
    Request.hpp \
    QJsonModel.hpp \
    DateTimeItem.hpp \
    Constants.hpp

FORMS += \
    RequestBuilder.ui \
    ResponseViewer.ui \
    HistoryViewer.ui \
    MainWindow.ui
