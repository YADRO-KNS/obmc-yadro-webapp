
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/router.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <service/authorization.hpp>

#include <iterator>
#include <sstream>

namespace app
{
namespace core
{

using namespace phosphor::logging;

Router::RouteMap Router::routerHandlers;
Router::DynamicRouteMap Router::dynamicRouterHandlers;

Router::Router(const RequestPtr& request) : requestObject(request)
{}

const ResponsePtr Router::process()
{
    // Current session sharing architecture between BMCWEB and WEBAPP processes,
    // in fact, works via filesystem synchronization. Hence, we need to re-read
    // the config file for each new connection.
    service::session::ConfigFile::getConfig().readData();
    const auto authResponse = std::make_shared<app::core::Response>();
    if (!app::service::authorization::authenticate(getRequest(), authResponse))
    {
        log<level::INFO>(
            "Unauthorized access registried",
            entry("REMOTE=%s", getRequest()->getClientIp().c_str()));
        return authResponse;
    }

    if (!this->handler)
    {
        setGeneralHeaders(authResponse);
        authResponse->setStatus(http::statuses::Code::NotFound);
        return authResponse;
    }

    auto response = handler->run(getRequest());
    setGeneralHeaders(response);
    log<level::DEBUG>(response->getHead().c_str());
    return response;
}

bool Router::preHandler()
{
    if (this->handler)
    {
        handler->preHandlers(getRequest());
    }

    // Static route pattern
    // Lookup for static routes at first due in fact that is simple comparing
    // O(log N) of std::size_t types.
    const std::string uri = helpers::utils::toLower(getRequest()->getUriPath());
    auto hashPattern = std::hash<std::string>{}(uri);
    auto builder = routerHandlers.find(hashPattern);
    if (builder != routerHandlers.end())
    {
        this->handler = builder->second(uri);
        return handler->preHandlers(getRequest());
    }

    // Dynamic route pattern
    // This is complex operation to find relevant route handler, match callback
    // will be called of each IDynamicRouteHandler until it will be found.
    for (auto& [condition, builder] : dynamicRouterHandlers)
    {
        if (condition(getRequest()->environment().pathInfo))
        {
            this->handler = builder(getRequest());
            return this->handler->preHandlers(getRequest());
        }
    }

    // Route no found. Immediate exit, but give a chanse to handle that for the
    // process()
    log<level::DEBUG>("Route no found. Pass throught",
                      entry("PATH=%s", uri.c_str()));
    return true;
}

void Router::setGeneralHeaders(const ResponsePtr response)
{
    using namespace app::http;
    constexpr const char* headerDateFormat = "%a, %d %b %Y %H:%M:%S GMT";
    response->setHeader(headers::contentLength,
                        std::to_string(response->totalSize()));
    response->setHeader(
        headers::date,
        app::helpers::utils::getFormattedCurrentDate(headerDateFormat));
}

} // namespace core
} // namespace app
