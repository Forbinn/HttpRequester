/*
** Copyright 2018 ViVoka
**
** Made by Vincent Leroy
** Mail <vl@vivoka.com>
**
** vivoka.com
*/

namespace Keys
{
    constexpr const auto request                  = "request";
    constexpr const auto requestMethod            = "method";
    constexpr const auto requestUrl               = "url";
    constexpr const auto requestContentIsFilename = "content_is_filename";
    constexpr const auto requestContent           = "content";
    constexpr const auto requestHeaders           = "headers";
    constexpr const auto requestDate              = "date";
    constexpr const auto requestElaspedTime       = "elapsed_time";

    constexpr const auto response                 = "response";
    constexpr const auto responseStatus           = "status_code";
    constexpr const auto responseReason           = "reason";
    constexpr const auto responseContent          = "content";
    constexpr const auto responseHeaders          = "headers";
    constexpr const auto responseDisplayFormat    = "display_format";

    constexpr const auto exportVersion            = "version";
} // !namespace Keys

namespace Constants
{
    constexpr const auto maxHistorySize     = 100;
    constexpr const auto applicationVersion = "1.8";
    constexpr const auto exportDateFormat   = "dd-MM-yyyyTHH:mm:ss.zzz";
} // !namespace Constants
