/*
** Copyright 2017 ViVoka
**
** Made by leroy_v
** Mail <leroy_v@vivoka.com>
**
** vivoka.com
*/

#include "MainWindow.hpp"

// Qt includes -----------------------------------------------------------------
#include <QApplication>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QShortcut>
#include <QProgressDialog>

MainWindow::MainWindow()
{
    _ui.setupUi(this);

    QObject::connect(_ui.requestBuilder, &RequestBuilder::requestSubmitted, [this](QNetworkReply * reply)
    {
        _ui.responseViewer->setRequest(_ui.requestBuilder->request());
        _ui.responseViewer->handleReply(reply);
        if (!_ui.historyViewer->hasRequest(_ui.requestBuilder->request()))
            _ui.historyViewer->addRequest(_ui.requestBuilder->request());
        else
            _ui.historyViewer->updateRequest(_ui.requestBuilder->request());

        _dialog = new QProgressDialog(this);
        _dialog->setModal(true);
        _dialog->setWindowModality(Qt::WindowModal);
        _dialog->setWindowTitle("Request submitted");
        _dialog->setLabelText("Waiting response...");
        _dialog->setMinimumDuration(0);
        _dialog->open();
        QObject::connect(reply, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal)
        {
            _dialog->setMaximum(bytesTotal);
            _dialog->setValue(bytesReceived);
        });

        QObject::connect(_dialog, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
    });

    QObject::connect(_ui.responseViewer, &ResponseViewer::replyReceived, [this]
    {
        _dialog->close();
        _dialog->deleteLater();
        _ui.historyViewer->updateRequest(_ui.responseViewer->request());
    });

    QObject::connect(_ui.historyViewer, &HistoryViewer::currentChanged, [this](RequestPtr request)
    {
        _ui.responseViewer->setRequest(request);
        _ui.requestBuilder->displayRequest(request);
    });

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [this]
    {
        _saveOrLoadHistoryData(true);
        _saveOrLoadWindow(true);
    });

    auto quitShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
    QObject::connect(quitShortcut, &QShortcut::activated, qApp, &QApplication::quit);
}

void MainWindow::restoreState()
{
    _saveOrLoadHistoryData(false);
    _saveOrLoadWindow(false);

    _ui.requestBuilder->setRequestForCompletion(_ui.historyViewer->request());
}

void MainWindow::_saveOrLoadHistoryData(bool save)
{
    static const auto filename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/request_ui.history";
    if (save)
    {
        const auto absolutePath = QFileInfo(filename).absolutePath();
        if (!QDir::root().mkpath(absolutePath))
        {
            qWarning("Failed to create directory at '%s'", qPrintable(absolutePath));
            return ;
        }
    }

    QFile file(filename);
    if (save)
    {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            qWarning("Unable to open/create file '%s': %s", qPrintable(filename), qPrintable(file.errorString()));
            return ;
        }
    }
    else if (!file.open(QIODevice::ReadOnly))
        return ;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setVersion(QDataStream::Qt_5_6);

    if (save)
        _ui.historyViewer->save(stream);
    else
        _ui.historyViewer->load(stream);
}

void MainWindow::_saveOrLoadWindow(bool save)
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;

    if (save)
    {
        settings.beginGroup("MainWindow");
        settings.setValue("geometry", saveGeometry());
        settings.endGroup();
    }
    else
    {
        settings.beginGroup("MainWindow");
        if (settings.contains("geometry"))
        {
            restoreGeometry(settings.value("geometry").toByteArray());
            show();
        }
        else
            showMaximized();
        settings.endGroup();
    }
}
