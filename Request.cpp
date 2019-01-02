/*
** Copyright 2017 ViVoka
**
** Made by leroy_v
** Mail <leroy_v@vivoka.com>
**
** vivoka.com
*/

#include "Request.hpp"

// Project includes ------------------------------------------------------------
#include "Constants.hpp"

namespace
{
QJsonObject headerToJson(const Request::Headers & headers)
{
    QJsonObject json;
    for (const auto & header : headers)
        json.insert(header.first.constData(), header.second.toBase64().constData());
    return json;
}

QJsonObject headerToJson(const QNetworkRequest & request)
{
    QJsonObject json;
    const auto headerNameList = request.rawHeaderList();
    for (const auto & headerName : headerNameList)
        json.insert(headerName.constData(), request.rawHeader(headerName).toBase64().constData());
    return json;
}

Request::Headers headerFromJson(const QJsonObject & json)
{
    Request::Headers headers;
    for (auto itr = json.begin(); itr != json.end(); ++itr)
        headers.append({itr.key().toUtf8(),
                        QByteArray::fromBase64(itr.value().toString().toUtf8())});
    return headers;
}

void headerFromJson(const QJsonObject & json, QNetworkRequest & request)
{
    for (auto itr = json.begin(); itr != json.end(); ++itr)
        request.setRawHeader(itr.key().toUtf8(),
                             QByteArray::fromBase64(itr.value().toString().toUtf8()));
}

void from18Request(const QJsonObject & json, Request & request)
{
    const auto jsonRequest  = json.value(Keys::request).toObject();
    const auto jsonResponse = json.value(Keys::response).toObject();

    request.method            = jsonRequest.value(Keys::requestMethod).toString().toUtf8();
    request.contentIsFilename = jsonRequest.value(Keys::requestContentIsFilename).toBool();
    request.content           = QByteArray::fromBase64(jsonRequest.value(Keys::requestContent).toString().toUtf8());
    request.setUrl(jsonRequest.value(Keys::requestUrl).toString());
    headerFromJson(jsonRequest.value(Keys::requestHeaders).toObject(), request);

    request.statusCode      = static_cast<quint32>(jsonResponse.value(Keys::responseStatus).toInt());
    request.reasonPhrase    = jsonResponse.value(Keys::responseReason).toString();
    request.responseContent = QByteArray::fromBase64(jsonResponse.value(Keys::responseContent).toString().toUtf8());
    request.responseHeaders = headerFromJson(jsonResponse.value(Keys::responseHeaders).toObject());

    request.date          = QDateTime::fromString(json.value(Keys::requestDate).toString(), Constants::exportDateFormat);
    request.elapsedTime   = static_cast<quint32>(json.value(Keys::requestElaspedTime).toInt());
    request.displayFormat = json.value(Keys::responseDisplayFormat).toInt();
}
} // !namespace

QJsonObject Request::toJson() const
{
    const QJsonObject jsonRequest{
        { Keys::requestMethod,            method.constData()             },
        { Keys::requestUrl,               url().toString()               },
        { Keys::requestContentIsFilename, contentIsFilename              },
        { Keys::requestContent,           content.toBase64().constData() },
        { Keys::requestHeaders,           headerToJson(*this)            }
    };
    const QJsonObject jsonResponse{
        { Keys::responseStatus,  static_cast<qint32>(statusCode)        },
        { Keys::responseReason,  reasonPhrase                           },
        { Keys::responseContent, responseContent.toBase64().constData() },
        { Keys::responseHeaders, headerToJson(responseHeaders)          }
    };

    return QJsonObject{
        { Keys::request,               jsonRequest                                        },
        { Keys::response,              jsonResponse                                       },
        { Keys::requestDate,           date.toUTC().toString(Constants::exportDateFormat) },
        { Keys::requestElaspedTime,    static_cast<qint32>(elapsedTime)                   },
        { Keys::responseDisplayFormat, displayFormat                                      },
        { Keys::exportVersion,         Constants::applicationVersion                      }
    };
}

void Request::fromJson(const QJsonObject & json)
{
    const auto version = json.value(Keys::exportVersion).toString();
    if (version == "1.8")
        from18Request(json, *this);
}

bool Request::isNull() const
{
    return method.isEmpty() ||
           url().isEmpty();
}

QDataStream & operator<<(QDataStream & out, const Request & request)
{
    out << request.url();
    out << request.method;

    Request::Headers headers;
    headers.reserve(request.rawHeaderList().size());
    for (const auto & header : request.rawHeaderList())
        headers.append(qMakePair(header, request.rawHeader(header)));
    out << headers;

    out << request.hasContent;
    out << request.contentIsFilename;
    out << request.content;

    out << request.hasReceiveResponse;
    out << request.statusCode;
    out << request.reasonPhrase;
    out << request.responseContent;
    out << request.responseHeaders;

    out << request.date;
    out << request.elapsedTime;

    out << request.displayFormat;

    return out;
}

QDataStream & operator>>(QDataStream & in, Request & request)
{
    QUrl url;
    in >> url;
    request.setUrl(url);

    in >> request.method;

    Request::Headers headers;
    in >> headers;
    for (const auto & p : headers)
        request.setRawHeader(p.first, p.second);

    in >> request.hasContent;
    in >> request.contentIsFilename;
    in >> request.content;

    in >> request.hasReceiveResponse;
    in >> request.statusCode;
    in >> request.reasonPhrase;
    in >> request.responseContent;
    in >> request.responseHeaders;

    in >> request.date;
    in >> request.elapsedTime;

    in >> request.displayFormat;

    return in;
}

QDataStream & operator<<(QDataStream & out, const RequestPtr & request)
{
    out << *request;
    return out;
}

QDataStream & operator>>(QDataStream & in, RequestPtr & request)
{
    if (request == nullptr)
        request = std::make_shared<Request>();
    in >> *request;
    return in;
}
