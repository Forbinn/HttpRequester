/*
** Copyright 2017 ViVoka
**
** Made by leroy_v
** Mail <leroy_v@vivoka.com>
**
** vivoka.com
*/

#pragma once

// Qt includes -----------------------------------------------------------------
#include <QWidget>

// Qt forward declarations -----------------------------------------------------
QT_BEGIN_NAMESPACE
class QProgressDialog;
QT_END_NAMESPACE

// Project forward declarations ------------------------------------------------
class RequestBuilder;
class ResponseViewer;
class HistoryViewer;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow();

    void restoreState();

private:
    void _saveOrLoadHistoryData(bool save);
    void _saveOrLoadWindow(bool save);

private:
    RequestBuilder *  _requestBuilder;
    ResponseViewer *  _responseViewer;
    HistoryViewer *   _historyViewer;

    QProgressDialog * _dialog;
};
