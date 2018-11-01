QT += core gui widgets network

TARGET = HttpRequester
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11 \
                  -Wextra

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
    DateTimeItem.hpp

FORMS += \
    RequestBuilder.ui \
    ResponseViewer.ui \
    HistoryViewer.ui \
    MainWindow.ui
