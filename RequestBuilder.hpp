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
#include "ui_RequestBuilder.h"
#include "Request.hpp"

// Qt forward declarations -----------------------------------------------------
QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QTableWidget;
class QTableWidgetItem;
class QStringListModel;
class QEvent;
QT_END_NAMESPACE

class RequestBuilder : public QWidget
{
    Q_OBJECT

public:
    explicit RequestBuilder(QWidget * parent = nullptr);

    RequestPtr request() const { return _currentRequest; }

    void setRequestForCompletion(const QList<RequestPtr> & requests);

public slots:
    void displayRequest(RequestPtr request);

protected:
    bool eventFilter(QObject * watched, QEvent * event) override;

private:
    void _submitRequest(const QString & method);
    void _urlChanged(const QString & rawUrl);
    void _parameterItemChanged(QTableWidgetItem * item);
    void _requestContentChanged();

private:
    static bool _addEntryToTable(QTableWidget * table,
                                 const QString & name,
                                 const QString & value);
    static QTableWidgetItem * _createTableItem(const QString & text = {});

    static void _removeRowOfSelectedItemsInTable(QTableWidget * table);

    static bool _isUrlValid(const QUrl & url, QString & errorString);
    static QString _generateDefaultUserAgent();

signals:
    void requestSubmitted(QNetworkReply * reply);

private:
    Ui::RequestBuilder      _ui;
    QNetworkAccessManager * _networkManager;

    RequestPtr              _currentRequest;
    QStringListModel *      _urlCompletionModel;
};
