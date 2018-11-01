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

// Project includes ------------------------------------------------------------
#include "ui_MainWindow.h"

// Qt forward declarations -----------------------------------------------------
class QProgressDialog;

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
    Ui::MainWindow _ui;

    QProgressDialog * _dialog;
};
