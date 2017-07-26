/*
** Copyright 2017 ViVoka
**
** Made by leroy_v
** Mail <leroy_v@vivoka.com>
**
** vivoka.com
*/

#include "Request.hpp"

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
