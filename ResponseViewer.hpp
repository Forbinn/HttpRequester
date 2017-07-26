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
#include <QTabWidget>

// Project includes ------------------------------------------------------------
#include "Request.hpp"

// Qt forward declarations -----------------------------------------------------
QT_BEGIN_NAMESPACE
class QNetworkReply;
class QTableWidget;
class QTableWidgetItem;
class QEvent;
QT_END_NAMESPACE

// Project forward declarations ------------------------------------------------
class QJsonModel;
namespace Ui
{
class ResponseViewer;
}

class ResponseViewer : public QTabWidget
{
    Q_OBJECT

public:
    explicit ResponseViewer(QWidget * parent = nullptr);
    ~ResponseViewer();

    RequestPtr request() const { return _currentRequest; }

public slots:
    void setRequest(RequestPtr request);
    void handleReply(QNetworkReply * reply);

protected:
    bool eventFilter(QObject * watched, QEvent * event) override;

private:
    void _updateGui();
    void _displayResponseData(const QByteArray & data);

private:
    static bool _addEntryToTable(QTableWidget * table,
                                 const QString & name,
                                 const QString & value);
    static QTableWidgetItem * _createTableItem(const QString & text = {});

signals:
    void replyReceived();

private:
    Ui::ResponseViewer * _ui;
    QJsonModel         * _jsonModel;

    RequestPtr           _currentRequest;
};
