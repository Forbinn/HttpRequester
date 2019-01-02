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
#include "Constants.hpp"
#include "DateTimeItem.hpp"

// Qt includes -----------------------------------------------------------------
#include <QKeyEvent>

HistoryViewer::HistoryViewer(QWidget * parent) :
    QWidget(parent)
{
    _requests.reserve(Constants::maxHistorySize);

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
}

bool HistoryViewer::hasRequest(RequestPtr request) const
{
    return _requests.contains(request);
}

void HistoryViewer::updateRequest(RequestPtr request)
{
    const auto row = _getRowForRequest(request);
    _fillTableRow(row, request);

    _ui.tableWidget->viewport()->update();
}

void HistoryViewer::addRequest(RequestPtr request)
{
    if (_requests.size() >= Constants::maxHistorySize)
    {
        QObject::disconnect(_ui.tableWidget, &QTableWidget::itemSelectionChanged,
                            this, &HistoryViewer::_itemSelectionChanged);
        const auto rowToRemove = _ui.tableWidget->rowCount() - 1;
        const auto requestIdx  = _getRequestIdxForItem(_ui.tableWidget->item(rowToRemove, 0));
        _ui.tableWidget->removeRow(rowToRemove);
        _requests.remove(requestIdx);
        QObject::connect(_ui.tableWidget, &QTableWidget::itemSelectionChanged,
                            this, &HistoryViewer::_itemSelectionChanged);
    }

    _addRequestToTable(request);
    _requests.push_back(request);

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
    _fillTableRow(row, request);

    _ui.tableWidget->item(row, 0)->setData(Qt::UserRole, QVariant::fromValue(request.get()));

    _ui.pbClear->setEnabled(true);
    _ui.tableWidget->sortItems(3, Qt::DescendingOrder);
    _ui.tableWidget->viewport()->update();
}

void HistoryViewer::_fillTableRow(int row, const RequestPtr request)
{
    _ui.tableWidget->setItem(row, 0, _createTableItem(request->method.constData()));
    _ui.tableWidget->setItem(row, 1, _createTableItem(request->url().toString()));
    _ui.tableWidget->setItem(row, 2, _createTableItem(QString("%1 %2").arg(request->statusCode)
                                                       .arg(request->reasonPhrase)));
    _ui.tableWidget->setItem(row, 3, _createTableItem(request->date.toString(DateTimeItem::dateFormat), true));
    _ui.tableWidget->setItem(row, 4, _createTableItem(_formatSize(request->responseContent.size())));
    _ui.tableWidget->setItem(row, 5, _createTableItem(QString("%1 ms").arg(request->elapsedTime)));
}

int HistoryViewer::_getRequestIdxForItem(const QTableWidgetItem * item) const
{
    const auto request = _getRequestForItem(item);
    for (int i = 0; i < _requests.size(); ++i)
        if (_requests.at(i).get() == request)
            return i;
    return -1;
}

Request * HistoryViewer::_getRequestForItem(const QTableWidgetItem * item) const
{
    const auto firstItem = item->column() == 0 ? item : _ui.tableWidget->item(item->row(), 0);
    return qvariant_cast<Request *>(firstItem->data(Qt::UserRole));
}

int HistoryViewer::_getRowForRequest(const RequestPtr request) const
{
    for (int row = 0; row < _ui.tableWidget->rowCount(); ++row)
    {
        const auto item = _ui.tableWidget->item(row, 0);
        if (qvariant_cast<Request *>(item->data(Qt::UserRole)) == request.get())
            return row;
    }

    return -1;
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

    const auto idx = _getRequestIdxForItem(_ui.tableWidget->currentItem());
    emit currentChanged(_requests.at(idx));
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
    auto selectedItems = _ui.tableWidget->selectedItems();
    std::sort(selectedItems.begin(), selectedItems.end(),
              [](const QTableWidgetItem * v1, const QTableWidgetItem * v2)
    { return v1->row() < v2->row(); });

    selectedItems.erase(std::unique(selectedItems.begin(), selectedItems.end(),
                                    [](const QTableWidgetItem * v1, const QTableWidgetItem * v2)
    { return v1->row() == v2->row(); }), selectedItems.end());

    for (const auto & item : selectedItems)
    {
        const auto idx = _getRequestIdxForItem(item);
        _requests.removeAt(idx);

        QObject::disconnect(_ui.tableWidget, &QTableWidget::itemSelectionChanged,
                            this, &HistoryViewer::_itemSelectionChanged);
        _ui.tableWidget->removeRow(item->row());
        QObject::connect(_ui.tableWidget, &QTableWidget::itemSelectionChanged,
                         this, &HistoryViewer::_itemSelectionChanged);
    }

    if (_ui.tableWidget->rowCount() == 0)
        _ui.pbClear->setEnabled(false);

    _itemSelectionChanged();
}
