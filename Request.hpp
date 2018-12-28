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
#include <QNetworkRequest>
#include <QList>
#include <QDateTime>
#include <QDataStream>
#include <QJsonObject>

// C++ standard library includes -----------------------------------------------
#include <memory>

struct Request : public QNetworkRequest
{
    using Headers = QList<QPair<QByteArray, QByteArray>>;

    QByteArray method;

    bool       hasContent;
    bool       contentIsFilename;
    QByteArray content;

    bool       hasReceiveResponse;
    quint32    statusCode;
    QString    reasonPhrase;
    QByteArray responseContent;
    Headers    responseHeaders;

    QDateTime  date;
    quint32    elapsedTime;

    qint32     displayFormat;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject & json);
};

using RequestPtr = std::shared_ptr<Request>;

QDataStream & operator<<(QDataStream & out, const Request & request);
QDataStream & operator>>(QDataStream & in, Request & request);
QDataStream & operator<<(QDataStream & out, const RequestPtr & request);
QDataStream & operator>>(QDataStream & in, RequestPtr & request);

Q_DECLARE_METATYPE(Request *);
