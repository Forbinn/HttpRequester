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
#include <QVector>

// Project includes ------------------------------------------------------------
#include "ui_HistoryViewer.h"
#include "Request.hpp"

// Qt forward declarations -----------------------------------------------------
QT_BEGIN_NAMESPACE
class QTableWidgetItem;
class QKeyEvent;
QT_END_NAMESPACE

class HistoryViewer : public QWidget
{
    Q_OBJECT

public:
    HistoryViewer(QWidget * parent = nullptr);

    bool hasRequest(RequestPtr request) const;
    void updateRequest(RequestPtr request);
    void addRequest(RequestPtr request);
    const QVector<RequestPtr> & request() const { return _requests; }

public slots:
    void load(QDataStream & in);
    void save(QDataStream & out) const;

protected:
    void keyPressEvent(QKeyEvent * event) override;

private:
    void _addRequestToTable(const RequestPtr request);

private:
    static QTableWidgetItem * _createTableItem(const QString & text = {}, bool dateTime = false);
    static QString _formatSize(qint32 size);

private slots:
    void _itemSelectionChanged();

signals:
    void currentChanged(RequestPtr request);

private:
    Ui::HistoryViewer _ui;

    QVector<RequestPtr> _requests;
};
