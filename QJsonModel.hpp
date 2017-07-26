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
#include <QAbstractItemModel>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QIcon>

// Qt forward declarations -----------------------------------------------------
QT_BEGIN_NAMESPACE
class QJsonModel;
class QJsonItem;
QT_END_NAMESPACE

class QJsonTreeItem
{
public:
    QJsonTreeItem(QJsonTreeItem * parent = nullptr);
    ~QJsonTreeItem();

    void appendChild(QJsonTreeItem * item);
    QJsonTreeItem * child(int row);
    QJsonTreeItem * parent();

    void setKey(const QString & key);
    void setValue(const QString & value);
    void setType(const QJsonValue::Type & type);

    int childCount() const        { return _childs.count(); }
    int row() const               { return _parent ? _parent->_childs.indexOf(const_cast<QJsonTreeItem *>(this)) : 0; }
    QString key() const           { return _key; }
    QString value() const         { return _value; }
    QJsonValue::Type type() const { return _type; }

public:
    static QJsonTreeItem * load(const QJsonValue & value,
                                QJsonTreeItem * parent = nullptr);
    static QString jsonTypeToString(QJsonValue::Type type);

private:
    QString                _key;
    QString                _value;
    QJsonValue::Type       _type;
    QList<QJsonTreeItem *> _childs;
    QJsonTreeItem *        _parent;
};

//---------------------------------------------------

class QJsonModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit QJsonModel(QObject * parent = nullptr);
    bool load(const QString & fileName);
    bool load(QIODevice * device);
    bool loadJson(const QByteArray & json);

    QVariant data(const QModelIndex & index, int role) const override;
    QVariant headerData(int, Qt::Orientation, int) const override;
    QModelIndex index(int row, int column,const QModelIndex & parent = {}) const override;
    QModelIndex parent(const QModelIndex & index) const override;

    bool setData(const QModelIndex & index, const QVariant & value, int role) override;

    int rowCount(const QModelIndex & parent = {}) const override;
    int columnCount(const QModelIndex & = {}) const override;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

private:
    QJsonTreeItem * _rootItem;
    QJsonDocument   _document;

};
