/*
** Copyright 2017 ViVoka
**
** Made by leroy_v
** Mail <leroy_v@vivoka.com>
**
** vivoka.com
*/

#include "MainWindow.hpp"

// Project includes ------------------------------------------------------------
#include "RequestBuilder.hpp"
#include "ResponseViewer.hpp"
#include "HistoryViewer.hpp"

// Qt includes -----------------------------------------------------------------
#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QProgressDialog>
#include <QSettings>
#include <QShortcut>

MainWindow::MainWindow() :
    _requestBuilder(new RequestBuilder(this)),
    _responseViewer(new ResponseViewer(this)),
    _historyViewer(new HistoryViewer(this))
{
    auto hSplitter = new QSplitter(Qt::Horizontal, this);
    hSplitter->setChildrenCollapsible(false);
    hSplitter->addWidget(_requestBuilder);
    hSplitter->addWidget(_responseViewer);

    auto vSplitter = new QSplitter(Qt::Vertical, this);
    vSplitter->setChildrenCollapsible(false);
    vSplitter->addWidget(hSplitter);
    vSplitter->addWidget(_historyViewer);

    setLayout(new QVBoxLayout);
    layout()->addWidget(vSplitter);

    QObject::connect(_requestBuilder, &RequestBuilder::requestSubmitted, [this](QNetworkReply * reply)
    {
        _responseViewer->setRequest(_requestBuilder->request());
        _responseViewer->handleReply(reply);
        if (!_historyViewer->hasRequest(_requestBuilder->request()))
            _historyViewer->addRequest(_requestBuilder->request());
        else
            _historyViewer->updateRequest(_requestBuilder->request());

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

    QObject::connect(_responseViewer, &ResponseViewer::replyReceived, [this]
    {
        _dialog->close();
        _dialog->deleteLater();
        _historyViewer->updateRequest(_responseViewer->request());
    });

    QObject::connect(_historyViewer, &HistoryViewer::currentChanged, [this](RequestPtr request)
    {
        _responseViewer->setRequest(request);
        _requestBuilder->displayRequest(request);
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

    _requestBuilder->setRequestForCompletion(_historyViewer->request());
}

void MainWindow::_saveOrLoadHistoryData(bool save)
{
    static const auto filename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/request_history";
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
        _historyViewer->save(stream);
    else
        _historyViewer->load(stream);
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
