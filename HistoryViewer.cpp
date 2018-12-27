/*
** Copyright 2017 ViVoka
**
** Made by leroy_v
** Mail <leroy_v@vivoka.com>
**
** vivoka.com
*/

#include "HistoryViewer.hpp"

// Project includes ------------------------------------------------------------
#include "DateTimeItem.hpp"

// Qt includes -----------------------------------------------------------------
#include <QKeyEvent>

namespace
{
    constexpr const auto maxHistorySize = 100;
}

HistoryViewer::HistoryViewer(QWidget * parent) :
    QWidget(parent)
{
    _requests.reserve(maxHistorySize);

    _ui.setupUi(this);
    _ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _ui.tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    QObject::connect(_ui.tableWidget, &QTableWidget::itemSelectionChanged,
                     this, &HistoryViewer::_itemSelectionChanged);

    // Clear history button
    QObject::connect(_ui.pbClear, &QPushButton::clicked,
                     this, &HistoryViewer::_onPbClearClicked);
    // Delete request button
    QObject::connect(_ui.pbDelete, &QPushButton::clicked,
                     this, &HistoryViewer::_onPbDeleteClicked);
    {
    });
    // Delete request button
    QObject::connect(_ui.pbDelete, &QPushButton::clicked, [this]
    {
    });
}

bool HistoryViewer::hasRequest(RequestPtr request) const
{
    return _requests.contains(request);
}

void HistoryViewer::updateRequest(RequestPtr request)
{
    const int index = _requests.indexOf(request);
    if (Q_UNLIKELY(index == -1))
        return ;

    _ui.tableWidget->item(index, 0)->setText(request->method.constData());
    _ui.tableWidget->item(index, 1)->setText(request->url().toString());
    _ui.tableWidget->item(index, 2)->setText(QString("%1 %2").arg(request->statusCode)
                                                       .arg(request->reasonPhrase));
    _ui.tableWidget->item(index, 3)->setText(request->date.toString(DateTimeItem::dateFormat));
    _ui.tableWidget->item(index, 4)->setText(_formatSize(request->responseContent.size()));
    _ui.tableWidget->item(index, 5)->setText(QString("%1 ms").arg(request->elapsedTime));

    _ui.tableWidget->viewport()->update();
}

void HistoryViewer::addRequest(RequestPtr request)
{
    if (_requests.size() >= maxHistorySize)
    {
        QObject::disconnect(_ui.tableWidget, &QTableWidget::itemSelectionChanged,
                            this, &HistoryViewer::_itemSelectionChanged);
        _ui.tableWidget->removeRow(_ui.tableWidget->rowCount() - 1);
        QObject::connect(_ui.tableWidget, &QTableWidget::itemSelectionChanged,
                            this, &HistoryViewer::_itemSelectionChanged);
        _requests.pop_back();
    }

    _requests.push_front(request);
    _addRequestToTable(request);

    _ui.tableWidget->selectRow(0);
}

void HistoryViewer::load(QDataStream & in)
{
    in >> _requests;
    for (const auto & request : _requests)
        _addRequestToTable(request);
}

void HistoryViewer::save(QDataStream & out) const
{
    out << _requests;
}

void HistoryViewer::keyPressEvent(QKeyEvent * event)
{
    if (!event->matches(QKeySequence::Delete))
        return ;

    _ui.pbDelete->click();
}

void HistoryViewer::_addRequestToTable(const RequestPtr request)
{
    const int row = _ui.tableWidget->rowCount();
    _ui.tableWidget->setRowCount(row + 1);

    _ui.tableWidget->setItem(row, 0, _createTableItem(request->method.constData()));
    _ui.tableWidget->setItem(row, 1, _createTableItem(request->url().toString()));
    _ui.tableWidget->setItem(row, 2, _createTableItem(QString("%1 %2").arg(request->statusCode)
                                                       .arg(request->reasonPhrase)));
    _ui.tableWidget->setItem(row, 3, _createTableItem(request->date.toString(DateTimeItem::dateFormat), true));
    _ui.tableWidget->setItem(row, 4, _createTableItem(_formatSize(request->responseContent.size())));
    _ui.tableWidget->setItem(row, 5, _createTableItem(QString("%1 ms").arg(request->elapsedTime)));

    _ui.pbClear->setEnabled(true);
    _ui.tableWidget->sortItems(3, Qt::DescendingOrder);
    _ui.tableWidget->viewport()->update();
}

QTableWidgetItem * HistoryViewer::_createTableItem(const QString & text, bool dateTime)
{
    auto item = dateTime ? new DateTimeItem(text) : new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    return item;
}

QString HistoryViewer::_formatSize(qint32 size)
{
    if (size < 1024)
        return QString("%1 B").arg(size);
    else if (size < 1024 * 1024)
        return QString("%1 kB").arg(size / 1024);
    else if (size < 1024 * 1024 * 1024)
        return QString("%1 MB").arg(size / (1024 * 1024));
    else
        return QString("%1 GB").arg(size / (1024 * 1024 * 1024));
}

void HistoryViewer::_itemSelectionChanged()
{
    const auto areItemSelected = !_ui.tableWidget->selectedItems().isEmpty();
    _ui.pbDelete->setEnabled(areItemSelected);

    if (!areItemSelected)
        return ;

    const auto row = _ui.tableWidget->currentRow();
    if (row < _requests.size())
        emit currentChanged(_requests.at(row));
}

void HistoryViewer::_onPbClearClicked()
{
    _ui.tableWidget->clearContents();
    _ui.tableWidget->setRowCount(0);
    _requests.clear();
    _ui.pbClear->setEnabled(false);
}

void HistoryViewer::_onPbDeleteClicked()
{
    const auto row = _ui.tableWidget->currentRow();
    _requests.removeAt(row);

    QObject::disconnect(_ui.tableWidget, &QTableWidget::itemSelectionChanged,
                        this, &HistoryViewer::_itemSelectionChanged);
    _ui.tableWidget->removeRow(row);
    QObject::connect(_ui.tableWidget, &QTableWidget::itemSelectionChanged,
                     this, &HistoryViewer::_itemSelectionChanged);

    if (_ui.tableWidget->rowCount() == 0)
        _ui.pbClear->setEnabled(false);

    _itemSelectionChanged();
}
