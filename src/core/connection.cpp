
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/connection.hpp>
#include <phosphor-logging/log.hpp>

namespace app
{
namespace core
{

using namespace phosphor::logging;

Connection::Connection() :
    Fastcgipp::Request<char>(maxBodySizeByte), totalBytesRecived(0), request(),
    router()
{}

void Connection::inHandler(int postSize)
{
    log<level::DEBUG>("Accpeted new request", entry("POST_SIZE=%d", postSize));
    if (!request)
    {
        request = std::make_shared<app::core::Request>(this->environment());
    }
}

bool Connection::inProcessor()
{
    log<level::DEBUG>(
        "Request: process new request",
        entry("CONTENT_TYPE=%s", environment().contentType.c_str()));
    if (!request)
    {
        log<level::DEBUG>("Request: not initialized");
        inHandler(0);
    }

    if (environment().contentType.empty())
    {
        log<level::DEBUG>("Request: empty content-type");
    }

    if (!router)
    {
        router = std::make_unique<app::core::Router>(this->request);
    }

    // Processing the 'preHandlers' in the 'inProcessor', because after that
    // procedure the Post Data Buffer will be cleared.
    return request->validate() && router->preHandler();
}

bool Connection::response()
{
    using namespace Fastcgipp::Http;

    if (!router)
    {
        log<level::ERR>("Response: router not initialized");
        return false;
    }

    out << *(router->process());
    out.flush();

    return true;
}

} // namespace core

} // namespace app
