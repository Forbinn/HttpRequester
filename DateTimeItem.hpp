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
#include <QTableWidgetItem>
#include <QDateTime>

class DateTimeItem : public QTableWidgetItem
{
public:
    static constexpr const auto dateFormat = "dd MMM yyyy - HH:mm:ss";

public:
    using QTableWidgetItem::QTableWidgetItem;

    bool operator<(const QTableWidgetItem & other) const
    { return QDateTime::fromString(text(),       dateFormat) <
             QDateTime::fromString(other.text(), dateFormat); }
};

