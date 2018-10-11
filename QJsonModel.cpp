/*
** Copyright 2017 ViVoka
**
** Made by leroy_v
** Mail <leroy_v@vivoka.com>
**
** vivoka.com
*/

#include "QJsonModel.hpp"

// Qt includes -----------------------------------------------------------------
#include <QFile>
#include <QMap>
#include <QFont>

QJsonTreeItem::QJsonTreeItem(QJsonTreeItem * parent)
{
    _parent = parent;
}

QJsonTreeItem::~QJsonTreeItem()
{
    qDeleteAll(_childs);
}

void QJsonTreeItem::appendChild(QJsonTreeItem * item)
{
    _childs.append(item);
}

QJsonTreeItem * QJsonTreeItem::child(int row)
{
    return _childs.value(row);
}

QJsonTreeItem * QJsonTreeItem::parent()
{
    return _parent;
}

void QJsonTreeItem::setKey(const QString & key)
{
    _key = key;
}

void QJsonTreeItem::setValue(const QString & value)
{
    _value = value;
}

void QJsonTreeItem::setType(const QJsonValue::Type & type)
{
    _type = type;
}

QJsonTreeItem * QJsonTreeItem::load(const QJsonValue & value, QJsonTreeItem * parent)
{
    auto rootItem = new QJsonTreeItem(parent);
    rootItem->setKey("root");

    if (value.isObject())
    {
        // Get all QJsonValue childs
        for (const auto & key : value.toObject().keys())
        {
            const auto v = value.toObject().value(key);
            auto child = load(v,rootItem);
            child->setKey(key);
            child->setType(v.type());
            rootItem->appendChild(child);
        }
    }
    else if (value.isArray())
    {
        // Get all QJsonValue childs
        int index = 0;
        for (const auto & v : value.toArray())
        {
            auto child = load(v,rootItem);
            child->setKey(QString("[%1]").arg(index));
            child->setType(v.type());
            rootItem->appendChild(child);
            ++index;
        }
    }
    else
    {
        rootItem->setValue(value.toVariant().toString());
        rootItem->setType(value.type());
    }

    return rootItem;
}

QString QJsonTreeItem::jsonTypeToString(QJsonValue::Type type)
{
    static const QMap<QJsonValue::Type, QString> map = {
        {QJsonValue::Null,   "null"},
        {QJsonValue::Bool,   "bool"},
        {QJsonValue::Double, "double"},
        {QJsonValue::String, "string"},
        {QJsonValue::Array,  "array"},
        {QJsonValue::Object, "object"}
    };

    return map.value(type, "undefined");
}

//=========================================================================

QJsonModel::QJsonModel(QObject * parent) :
    QAbstractItemModel(parent)
{
    _rootItem = new QJsonTreeItem;
}

bool QJsonModel::load(const QString & fileName)
{
    QFile file(fileName);
    bool success = false;
    if (file.open(QIODevice::ReadOnly))
    {
        success = load(&file);
        file.close();
    }

    return success;
}

bool QJsonModel::load(QIODevice * device)
{
    return loadJson(device->readAll());
}

bool QJsonModel::loadJson(const QByteArray & json)
{
    _document = QJsonDocument::fromJson(json);

    if (!_document.isNull())
    {
        beginResetModel();
        if (_document.isArray())
            _rootItem = QJsonTreeItem::load(QJsonValue(_document.array()));
        else
            _rootItem = QJsonTreeItem::load(QJsonValue(_document.object()));
        endResetModel();
        return true;
    }

    return false;
}

QVariant QJsonModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto item = static_cast<const QJsonTreeItem *>(index.internalPointer());

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (index.column() == 0)
            return QString("%1").arg(item->key());
        else if (index.column() == 1)
            return QString("%1").arg(item->value());
    }
    else if (role == Qt::ToolTipRole)
        return QJsonTreeItem::jsonTypeToString(item->type());

    return QVariant();
}

QVariant QJsonModel::headerData(int, Qt::Orientation, int) const
{
    return {};
}

QModelIndex QJsonModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QJsonTreeItem * parentItem;

    if (!parent.isValid())
        parentItem = _rootItem;
    else
        parentItem = static_cast<QJsonTreeItem *>(parent.internalPointer());

    auto childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex QJsonModel::parent(const QModelIndex & index) const
{
    if (!index.isValid())
        return QModelIndex();

    auto childItem = static_cast<QJsonTreeItem *>(index.internalPointer());
    auto parentItem = childItem->parent();

    if (parentItem == _rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

bool QJsonModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return false;

    auto item = static_cast<QJsonTreeItem *>(index.internalPointer());

    if (index.column() == 0)
        item->setKey(value.toString());
    else if (index.column() == 1)
        item->setValue(value.toString());
    else
        return false;

    emit dataChanged(index, index, {role});
    return true;
}

int QJsonModel::rowCount(const QModelIndex & parent) const
{
    QJsonTreeItem * parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = _rootItem;
    else
        parentItem = static_cast<QJsonTreeItem *>(parent.internalPointer());

    return parentItem->childCount();
}

int QJsonModel::columnCount(const QModelIndex &) const
{
    return 2;
}

Qt::ItemFlags QJsonModel::flags(const QModelIndex & index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
}
