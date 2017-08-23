/*
** Copyright 2017 ViVoka
**
** Made by leroy_v
** Mail <leroy_v@vivoka.com>
**
** vivoka.com
*/

#include "ResponseViewer.hpp"

// Project includes ------------------------------------------------------------
#include "ui_ResponseViewer.h"
#include "QJsonModel.hpp"

// Qt includes -----------------------------------------------------------------
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QFileDialog>
#include <QMessageBox>

ResponseViewer::ResponseViewer(QWidget * parent) :
    QTabWidget(parent),
    _ui(new Ui::ResponseViewer),
    _jsonModel(new QJsonModel),
    _currentRequest(nullptr)
{
    _ui->setupUi(this);
    _ui->treeResponse->setModel(_jsonModel);
    _ui->treeResponse->setHeaderHidden(true);
    _ui->treeResponse->setUniformRowHeights(true);
    _ui->pbCollapse->hide();
    _ui->pbExpand->hide();

    QObject::connect(_jsonModel, &QJsonModel::dataChanged, _ui->treeResponse->viewport(), [this]
    { _ui->treeResponse->resizeColumnToContents(0); });

    QObject::connect(_ui->cbFormat, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int format)
    {
        if (_currentRequest == nullptr)
            return ;

        _currentRequest->displayFormat = format;
        _displayResponseData(_currentRequest->responseContent);
    });

    QObject::connect(_ui->stackedWidget, &QStackedWidget::currentChanged, [this](int index)
    {
        const bool visible = index == 1;
        _ui->pbCollapse->setVisible(visible);
        _ui->pbExpand->setVisible(visible);
        if (visible)
        {
            _ui->treeResponse->expandAll();
            _ui->treeResponse->resizeColumnToContents(0);
        }
    });

    QObject::connect(_ui->pbCollapse, &QPushButton::clicked,
                     _ui->treeResponse, &QTreeView::collapseAll);
    QObject::connect(_ui->pbExpand, &QPushButton::clicked,
                     _ui->treeResponse, &QTreeView::expandAll);

    QObject::connect(_ui->pbSave, &QPushButton::clicked, [this]
    {
        static auto directoryPath = QDir::homePath();
        const auto filename = QFileDialog::getSaveFileName(this, "Save response content to file",
                                                           directoryPath);
        if (filename.isEmpty())
            return ;
        directoryPath = QFileInfo(filename).absoluteFilePath();
        QString errorString;
        if (!saveResponseContentToFile(filename, &errorString))
            QMessageBox::critical(this, "Save failed", errorString);
    });

    _ui->lUrl->installEventFilter(this);
}

ResponseViewer::~ResponseViewer()
{
    delete _ui;
}

void ResponseViewer::setRequest(RequestPtr request)
{
    _currentRequest = request;
    _updateGui();
}

void ResponseViewer::handleReply(QNetworkReply * reply)
{
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    // TODO: timeout?
    QObject::connect(reply, &QNetworkReply::finished, [reply, elapsedTimer, this]
    {
        _currentRequest->hasReceiveResponse = true;
        _currentRequest->statusCode         = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt();
        _currentRequest->reasonPhrase       = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        _currentRequest->responseContent    = reply->readAll();
        _currentRequest->responseHeaders    = reply->rawHeaderPairs();
        _currentRequest->elapsedTime        = elapsedTimer.elapsed();

        if (reply->error() != QNetworkReply::NoError && reply->error() < QNetworkReply::ProxyConnectionRefusedError)
            _currentRequest->reasonPhrase = reply->errorString();

        _updateGui();
        emit replyReceived();
    });
}

bool ResponseViewer::saveResponseContentToFile(const QString & filename,
                                               QString * errorString) const
{
    QString dummy;
    QString & errString = errorString == nullptr ? dummy : *errorString;

    if (_currentRequest == nullptr)
    {
        errString = "No response has been set yet";
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        errString = file.errorString();
        return false;
    }

    const auto content = _currentRequest->responseContent;
    switch (_ui->cbFormat->currentIndex())
    {
        case 1: // Indented
        case 2: // Tree
        {
            const auto jsonDocument = QJsonDocument::fromJson(content);
            if (jsonDocument.isNull())
                file.write(content);
            else
                file.write(jsonDocument.toJson(QJsonDocument::Indented));
        }
            break;
        default:
            file.write(content);
            break;
    }

    return true;
}

bool ResponseViewer::eventFilter(QObject * watched, QEvent * event)
{
    if (watched != _ui->lUrl || event->type() != QEvent::Resize)
        return QObject::eventFilter(watched, event);

    if (_currentRequest == nullptr)
        return QObject::eventFilter(watched, event);

    const auto resize = static_cast<QResizeEvent *>(event);
    if (resize->oldSize().width() == resize->size().width())
        return QObject::eventFilter(watched, event);

    const auto metrics = QFontMetrics(_ui->lUrl->font());
    const auto text = metrics.elidedText(QString("%1 on %2")
                                         .arg(_currentRequest->method.constData())
                                         .arg(_currentRequest->url().toString()),
                                         Qt::ElideMiddle, _ui->lUrl->width());
    _ui->lUrl->setText(text);

    return QObject::eventFilter(watched, event);
}

void ResponseViewer::_updateGui()
{
    const auto metrics = QFontMetrics(_ui->lUrl->font());
    const auto text = metrics.elidedText(QString("%1 on %2")
                                         .arg(_currentRequest->method.constData())
                                         .arg(_currentRequest->url().toString()),
                                         Qt::ElideMiddle, _ui->lUrl->width());
    _ui->lUrl->setText(text);
    _ui->lStatus->setText(QString("%1 %2").arg(_currentRequest->statusCode)
                                          .arg(_currentRequest->reasonPhrase));

    _displayResponseData(_currentRequest->responseContent);

    _ui->tableHeaders->clearContents();
    _ui->tableHeaders->setRowCount(0);
    for (const auto & p : _currentRequest->responseHeaders)
        _addEntryToTable(_ui->tableHeaders, p.first, p.second);
    _ui->tableHeaders->resizeColumnToContents(0);

    if (_currentRequest->displayFormat == -1)
        _currentRequest->displayFormat = _ui->cbFormat->currentIndex();
    else
        _ui->cbFormat->setCurrentIndex(_currentRequest->displayFormat);
}

void ResponseViewer::_displayResponseData(const QByteArray & data)
{
    _ui->stackedWidget->setCurrentIndex(0);
    if (data.isEmpty())
        _ui->pteResponse->clear();
    else
    {
        switch (_ui->cbFormat->currentIndex())
        {
            case 0: // Raw
                _ui->pteResponse->setPlainText(data);
                break;
            case 1: // Indented
            {
                const auto json = QJsonDocument::fromJson(data);
                _ui->pteResponse->setPlainText(json.isNull() ? data : json.toJson(QJsonDocument::Indented));
            }
                break;
            case 2: // Tree
                if (!_jsonModel->loadJson(data))
                    _ui->pteResponse->setPlainText(data);
                else
                    _ui->stackedWidget->setCurrentIndex(1);
                break;
            default:
                break;

        }
    }
}

bool ResponseViewer::_addEntryToTable(QTableWidget * table,
                                      const QString & name,
                                      const QString & value)
{
    auto itemName  = _createTableItem(name);
    auto itemValue = _createTableItem(value);

    const auto row = table->rowCount();
    table->setRowCount(row + 1);
    table->setItem(row, 0, itemName);
    table->setItem(row, 1, itemValue);
    return true;
}

QTableWidgetItem * ResponseViewer::_createTableItem(const QString & text)
{
    auto item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    return item;
}
