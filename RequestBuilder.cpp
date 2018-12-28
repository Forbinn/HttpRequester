/*
** Copyright 2017 ViVoka
**
** Made by leroy_v
** Mail <leroy_v@vivoka.com>
**
** vivoka.com
*/

#include "RequestBuilder.hpp"

// Qt includes -----------------------------------------------------------------
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QBuffer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QSysInfo>
#include <QUrlQuery>
#include <QTimer>
#include <QCompleter>
#include <QFileSystemModel>
#include <QStringListModel>
#include <QList>
#include <QJsonDocument>
#include <QSet>

// C++ standard library includes -----------------------------------------------
#include <memory>

RequestBuilder::RequestBuilder(QWidget * parent) :
    QWidget(parent),
    _networkManager(new QNetworkAccessManager(this)),
    _currentRequest(nullptr),
    _urlCompletionModel(new QStringListModel())
{
    _ui.setupUi(this);

    // Add completer to content type line edit
    auto contentTypeCompleter = new QCompleter({"application/json",
                                                "application/x-www-form-urlencoded",
                                                "application/xml",
                                                "application/zip",
                                                "text/plain",
                                                "text/html"},
                                               _ui.leContentType);
    contentTypeCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    _ui.leContentType->setCompleter(contentTypeCompleter);

    // Add completer to file path line edit
    auto filePathCompleterModel = new QFileSystemModel(_ui.leFilePath);
    filePathCompleterModel->setRootPath(QDir::rootPath());
    _ui.leFilePath->setCompleter(new QCompleter(filePathCompleterModel, _ui.leFilePath));

    // Add completer to the URL line edit with history for completion
    _urlCompletionModel->setParent(_ui.leUrl);
    _ui.leUrl->setCompleter(new QCompleter(_urlCompletionModel, _ui.leUrl));

    // Radio button content + file
    QObject::connect(_ui.rbFile, &QRadioButton::toggled, [this](bool checked)
    {
        _ui.leFilePath->setEnabled(checked);
        _ui.pbBrowseFile->setEnabled(checked);
        _ui.pteContent->setEnabled(!checked);
    });
    // Browse file button
    QObject::connect(_ui.pbBrowseFile, &QPushButton::clicked, [this]
    {
        static auto directoryPath = _ui.leFilePath->text().isEmpty() ? QDir::homePath()
                                         : QFileInfo(_ui.leFilePath->text()).absoluteFilePath();
        const auto filename = QFileDialog::getOpenFileName(this, "Choose file", directoryPath);
        if (filename.isEmpty())
            return ;
        directoryPath = QFileInfo(filename).absoluteFilePath();
        _ui.leFilePath->setText(filename);
    });

    // Push button GET, POST, PUT, Submit
    QObject::connect(_ui.pbGet, &QPushButton::clicked,
                     [this]{ _submitRequest(_ui.pbGet->text()); });
    QObject::connect(_ui.pbPost, &QPushButton::clicked,
                     [this]{ _submitRequest(_ui.pbPost->text()); });
    QObject::connect(_ui.pbPut,  &QPushButton::clicked,
                     [this]{ _submitRequest(_ui.pbPut->text()); });
    QObject::connect(_ui.pbSubmit, &QPushButton::clicked, [this]
    { _submitRequest(_ui.cbMethod->currentText()); });

    // Header view add button
    QObject::connect(_ui.leNameHeaders, &QLineEdit::textChanged, [this](const QString & text)
    { _ui.pbAddHeaders->setEnabled(!text.isEmpty() && !_ui.leValueHeaders->text().isEmpty()); });
    QObject::connect(_ui.leValueHeaders, &QLineEdit::textChanged, [this](const QString & text)
    { _ui.pbAddHeaders->setEnabled(!text.isEmpty() && !_ui.leNameHeaders->text().isEmpty()); });
    QObject::connect(_ui.pbAddHeaders, &QPushButton::clicked, [this]
    {
        if (!_addEntryToTable(_ui.tableHeaders, _ui.leNameHeaders->text(), _ui.leValueHeaders->text()))
            return ;
        _ui.leNameHeaders->clear();
        _ui.leValueHeaders->clear();
    });
    // Header view delete button
    QObject::connect(_ui.tableHeaders, &QTableWidget::itemSelectionChanged, [this]
    { _ui.pbDeleteHeaders->setEnabled(!_ui.tableHeaders->selectedItems().isEmpty()); });
    QObject::connect(_ui.pbDeleteHeaders, &QPushButton::clicked, [this]
    {
        _removeRowOfSelectedItemsInTable(_ui.tableHeaders);

        if (_ui.tableHeaders->rowCount() == 0)
            _ui.pbDeleteHeaders->setEnabled(false);
    });

    // Content options buttons
    QObject::connect(_ui.pbContentBase64, &QPushButton::clicked, [this]
    {
        const auto currentData = _ui.pteContent->toPlainText().toUtf8();
        _ui.pteContent->setPlainText(currentData.toBase64().constData());
    });
    QObject::connect(_ui.pbContentParameter, &QPushButton::clicked, [this]
    {
        _ui.leContentType->setText("application/x-www-form-urlencoded");
        const auto url = QUrl::fromUserInput(_ui.leUrl->text());
        const auto urlQuery = QUrlQuery(url);
        _ui.pteContent->setPlainText(urlQuery.toString());
        _ui.leUrl->setText(url.toString(QUrl::RemoveQuery));
    });
    QObject::connect(_ui.pbFormatJson, &QPushButton::clicked, [this]
    { // This button is enable only if the JSON is valid
        const auto jsonDocument = QJsonDocument::fromJson(_ui.pteContent->toPlainText().toUtf8());
        QObject::disconnect(_ui.pteContent, &QPlainTextEdit::textChanged, this, &RequestBuilder::_requestContentChanged);
        _ui.pteContent->setPlainText(jsonDocument.toJson(QJsonDocument::Indented));
        QObject::connect(_ui.pteContent, &QPlainTextEdit::textChanged, this, &RequestBuilder::_requestContentChanged);
    });
    QObject::connect(_ui.pteContent, &QPlainTextEdit::textChanged, this, &RequestBuilder::_requestContentChanged);

    // Url input changed
    QObject::connect(_ui.leUrl, &QLineEdit::textChanged, this, &RequestBuilder::_urlChanged);

    // Parameter view add button
    QObject::connect(_ui.leNameParameters, &QLineEdit::textChanged, [this](const QString & text)
    { _ui.pbAddParameters->setEnabled(!text.isEmpty()); });
    QObject::connect(_ui.pbAddParameters, &QPushButton::clicked, [this]
    {
        if (!_addEntryToTable(_ui.tableParameters, _ui.leNameParameters->text(), _ui.leValueParameters->text()))
            return ;
        _ui.leNameParameters->clear();
        _ui.leValueParameters->clear();
    });
    // Parameter view delete button
    QObject::connect(_ui.tableParameters, &QTableWidget::itemSelectionChanged, [this]
    { _ui.pbDeleteParameters->setEnabled(!_ui.tableParameters->selectedItems().isEmpty()); });
    QObject::connect(_ui.pbDeleteParameters, &QPushButton::clicked, [this]
    {
        _removeRowOfSelectedItemsInTable(_ui.tableParameters);
        auto url = QUrl::fromUserInput(_ui.leUrl->text());
        QUrlQuery query;
        for (int i = 0; i < _ui.tableParameters->rowCount(); ++i)
            query.addQueryItem(_ui.tableParameters->item(i, 0)->text(),
                               _ui.tableParameters->item(i, 1)->text());

        url.setQuery(query);
        _ui.leUrl->setText(url.toString());

        if (_ui.tableParameters->rowCount() == 0)
            _ui.pbDeleteParameters->setEnabled(false);
    });
    // Parameter view item changed
    QObject::connect(_ui.tableParameters, &QTableWidget::itemChanged,
                     this, &RequestBuilder::_parameterItemChanged);

    _installEventFiler(_ui.tableHeaders, &RequestBuilder::_filterTableHeadersEvent);
    _installEventFiler(_ui.tableParameters, &RequestBuilder::_filterTableParametersEvent);
    _installEventFiler(_ui.leUrl, &RequestBuilder::_filterLeUrlEvent);
    _installEventFiler(_ui.leContentType, &RequestBuilder::_filterLeContentTypeEvent);
}

void RequestBuilder::setRequestForCompletion(const QVector<RequestPtr> & requests)
{
    QSet<QString> urls;
    for (const auto & request : requests)
        urls.insert(request->url().toString());

    _urlCompletionModel->setStringList(QStringList::fromSet(urls));
}

void RequestBuilder::displayRequest(RequestPtr request)
{
    _ui.leUrl->setText(request->url().toString());
    _ui.cbMethod->setCurrentText(request->method);
    _ui.leContentType->setText(request->header(QNetworkRequest::ContentTypeHeader).toString());

    _ui.pteContent->clear();
    _ui.leFilePath->clear();
    if (request->hasContent)
    {
        if (request->contentIsFilename)
        {
            _ui.rbFile->setChecked(true);
            _ui.leFilePath->setText(request->content);
        }
        else
        {
            _ui.rbContent->setChecked(true);
            _ui.pteContent->setPlainText(request->content);
        }
    }

    _ui.tableHeaders->clearContents();
    _ui.tableHeaders->setRowCount(0);
    for (const auto & header : request->rawHeaderList())
        _addEntryToTable(_ui.tableHeaders, header, request->rawHeader(header));
}

bool RequestBuilder::eventFilter(QObject * watched, QEvent * event)
{
    return (this->*_eventFilters.value(watched))(event);
}

bool RequestBuilder::_filterTableHeadersEvent(QEvent * event)
{
    if (event->type() != QEvent::KeyPress)
        return QObject::eventFilter(_ui.tableHeaders, event);

    const auto * keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->matches(QKeySequence::Delete))
    {
        _ui.pbDeleteHeaders->click();
        return true;
    }

    return QObject::eventFilter(_ui.tableHeaders, event);
}

bool RequestBuilder::_filterTableParametersEvent(QEvent * event)
{
    if (event->type() != QEvent::KeyPress)
        return QObject::eventFilter(_ui.tableParameters, event);

    const auto * keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->matches(QKeySequence::Delete))
    {
        _ui.pbDeleteParameters->click();
        return true;
    }

    return QObject::eventFilter(_ui.tableParameters, event);
}

bool RequestBuilder::_filterLeUrlEvent(QEvent * event)
{
    if (event->type() != QEvent::KeyPress)
        return QObject::eventFilter(_ui.leUrl, event);

    const auto * keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        _submitRequest(_ui.cbMethod->currentText());
    else if (keyEvent->key() == Qt::Key_Down)
        _ui.leUrl->completer()->complete();
    else
        return QObject::eventFilter(_ui.leUrl, event);

    return true;
}

bool RequestBuilder::_filterLeContentTypeEvent(QEvent * event)
{
    if (event->type() != QEvent::KeyPress)
        return QObject::eventFilter(_ui.leContentType, event);

    const auto * keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_Down)
    {
        _ui.leContentType->completer()->complete();
        return true;
    }

    return QObject::eventFilter(_ui.leContentType, event);
}

void RequestBuilder::_submitRequest(QString method)
{
    _currentRequest = std::make_shared<Request>();
    _ui.cbMethod->setCurrentText(method);

    method.remove('&');
    QString errorString;
    const auto url = [this]
    {
        auto url = QUrl::fromUserInput(_ui.leUrl->text());
        if (url.scheme().isEmpty() && url.port() == -1)
            url.setScheme("http");
        return url;
    }();

    if (!_isUrlValid(url, errorString))
    {
        QMessageBox::critical(this, "Invalid URL",
                              QString("The URL you have entered is invalid: %1")
                              .arg(errorString));
        return ;
    }

    std::unique_ptr<QIODevice> device = nullptr;
    if (method != "GET" && method != "DELETE" && method != "OPTIONS")
    {
        _currentRequest->hasContent = true;
        if (_ui.rbContent->isChecked())
        {
            auto buffer = new QBuffer;
            device.reset(buffer);
            buffer->setData(_ui.pteContent->toPlainText().toUtf8());
            _currentRequest->content = buffer->data();
            _currentRequest->contentIsFilename = false;
        }
        else
        {
            auto file = new QFile(_ui.leFilePath->text());
            device.reset(file);
            if (!file->open(QIODevice::ReadOnly))
            {
                QMessageBox::critical(this, "Unable to open file",
                                      QString("Failed to open file '%1': %2")
                                      .arg(_ui.leFilePath->text())
                                      .arg(file->errorString()));
                return ;
            }

            _currentRequest->contentIsFilename = true;
            _currentRequest->content = file->fileName().toUtf8();
        }
    }
    else
    {
        _ui.leFilePath->clear();
        _ui.pteContent->clear();
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, _generateDefaultUserAgent());
    for (int i = 0; i < _ui.tableHeaders->rowCount(); ++i)
        request.setRawHeader(_ui.tableHeaders->item(i, 0)->text().toUtf8(),
                             _ui.tableHeaders->item(i, 1)->text().toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, _ui.leContentType->text());

    _currentRequest->swap(request);
    _currentRequest->method = method.toLatin1();
    _currentRequest->date = QDateTime::currentDateTime();
    _currentRequest->displayFormat = -1;

    auto currentCompletionList = _urlCompletionModel->stringList().toSet();
    currentCompletionList.insert(url.toString());
    _urlCompletionModel->setStringList(QStringList::fromSet(currentCompletionList));

    auto internalDevice = device.release();
    auto reply = _networkManager->sendCustomRequest(*_currentRequest, method.toUtf8(), internalDevice);
    if (internalDevice != nullptr)
        QObject::connect(reply, &QNetworkReply::finished, internalDevice, &QObject::deleteLater);

    emit requestSubmitted(reply);
    _currentRequest.reset();
}

void RequestBuilder::_urlChanged(const QString & rawUrl)
{
    static QUrlQuery oldQuery;

    const auto url = QUrl::fromUserInput(rawUrl);
    const auto query = QUrlQuery(url);
    if (query == oldQuery)
        return ;
    oldQuery = query;

    const auto queryItems = query.queryItems();

    QObject::disconnect(_ui.tableParameters, &QTableWidget::itemChanged,
                        this, &RequestBuilder::_parameterItemChanged);

    // There is probably more effiecient way of filling this table
    // but this is the simplest and safer
    _ui.tableParameters->clearContents();
    _ui.tableParameters->setRowCount(queryItems.size());

    for (auto i = 0; i < queryItems.size(); ++i)
    {
        const auto & name  = queryItems.at(i).first;
        const auto & value = queryItems.at(i).second;

        _ui.tableParameters->setItem(i, 0, _createTableItem(name));
        _ui.tableParameters->setItem(i, 1, _createTableItem(value));
    }

    QObject::connect(_ui.tableParameters, &QTableWidget::itemChanged,
                     this, &RequestBuilder::_parameterItemChanged);
}

void RequestBuilder::_parameterItemChanged(QTableWidgetItem * item)
{
    if (_ui.tableParameters->item(item->row(), 1) == nullptr)
        return ;

    QUrlQuery query;
    for (int i = 0; i < _ui.tableParameters->rowCount(); ++i)
    {
        const auto name  = _ui.tableParameters->item(i, 0)->text();
        const auto value = _ui.tableParameters->item(i, 1)->text();
        query.addQueryItem(name, value);
    }

    QObject::disconnect(_ui.leUrl, &QLineEdit::textChanged, this, &RequestBuilder::_urlChanged);

    auto url = QUrl::fromUserInput(_ui.leUrl->text());
    url.setQuery(query);
    _ui.leUrl->setText(url.toString());

    QObject::connect(_ui.leUrl, &QLineEdit::textChanged, this, &RequestBuilder::_urlChanged);
}

void RequestBuilder::_requestContentChanged()
{
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(_ui.pteContent->toPlainText().toUtf8(), &error);
    _ui.pbFormatJson->setEnabled(!doc.isNull());
    if (error.error != QJsonParseError::NoError)
        _ui.pbFormatJson->setToolTip(error.errorString());
    else
        _ui.pbFormatJson->setToolTip({});
}

void RequestBuilder::_installEventFiler(QObject * obj, EventFilter filterFunc)
{
    _eventFilters.insert(obj, filterFunc);
    obj->installEventFilter(this);
}

bool RequestBuilder::_addEntryToTable(QTableWidget * table,
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

QTableWidgetItem * RequestBuilder::_createTableItem(const QString & text)
{
    auto item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    return item;
}

void RequestBuilder::_removeRowOfSelectedItemsInTable(QTableWidget * table)
{
    while (!table->selectedRanges().isEmpty())
    {
        const auto range = table->selectedRanges().first();
        for (int i = 0; i < range.rowCount(); ++i)
            table->removeRow(range.topRow());
    }
}

bool RequestBuilder::_isUrlValid(const QUrl & url, QString & errorString)
{
    if (!url.isValid())
        errorString = url.errorString();
    else if (url.scheme().isEmpty() && url.port() == -1)
        errorString = "No scheme nor port precised";
    else if (url.host().isEmpty())
        errorString = "Host is empty";
    else
        return true;

    return false;
}

QString RequestBuilder::_generateDefaultUserAgent()
{
    return QString("%1/%2 (%3 %4) %5")
            .arg(QApplication::applicationName())
            .arg(QApplication::applicationVersion())
            .arg(QSysInfo::kernelType())
            .arg(QSysInfo::currentCpuArchitecture())
            .arg(QApplication::organizationName());
}
