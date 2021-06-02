
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/router.hpp>

#include <logger/logger.hpp>

#include <nlohmann/json.hpp>

#include <service/authorization.hpp>
#include <service/configuration.hpp>

namespace app
{
namespace core
{

Router::RouteMap Router::routerHandlers;

Router::Router(const RequestPtr& request) :
    requestObject(request),
    responseObject(std::make_unique<app::core::Response>())
{}

const ResponseUni& Router::process()
{
    // Current session sharing architecture between BMCWEB and WEBAPP processes,
    // in fact, works via filesystem synchronization. Hence, we need to re-read
    // the config file for each new connection.
    service::config::getConfig().readData();
    if (!app::service::authorization::authenticate(getRequest(), getResponse()))
    {
        BMC_LOG_INFO << "Unauthorized access from "
                 << getRequest()->environment().remoteAddress;
        return getResponse();
    }

    if (!this->handler) {
        auto builder = routerHandlers.find(getRequest()->environment().requestUri);
        if (builder == routerHandlers.end())
        {
            return getResponse();
        }
        this->handler = builder->second(builder->first);
    }

    handler->run(getRequest(), getResponse());

    return getResponse();
}

bool Router::preHandler()
{
    if (!this->handler)
    {
        auto builder =
            routerHandlers.find(getRequest()->environment().requestUri);
        if (builder == routerHandlers.end())
        {
            // Route no found. Immediate exit, but give the chanse to handle
            // that for the process()
            BMC_LOG_DEBUG << "Route no found. Pass throught";
            return true;
        }
        this->handler = builder->second(builder->first);
    }

    return handler->preHandlers(getRequest());
}

} // namespace core
} // namespace app
