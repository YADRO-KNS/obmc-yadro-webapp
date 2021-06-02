// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/application.hpp>
#include <core/connection.hpp>
#include <core/route/handlers/graphql_handler.hpp>

#include <csignal>

namespace app
{
namespace core
{

void Application::configure()
{
    BMC_LOG_DEBUG << "configure";

    registerAllRoutes();

    this->initEntityMap();
    this->initBrokers();

    // register handlers
    std::signal(SIGINT, &Application::handleSignals);
}

void Application::start()
{
    /* First we make a Fastcgipp::Manager object, with our request handling
     * class as a template parameter.
     */
    Fastcgipp::Manager<Connection> app;
    /* Now just call the object handler function. It will sleep quietly when
     * there are no requests and efficiently manage them when there are many.
     */
    app.setupSignals();
    app.listen();
    app.start();
    app.join();
}

void Application::terminate()
{
    dbusBrokerManager.terminate();
}

void Application::initBrokers()
{
    dbusBrokerManager.start();
    BMC_LOG_DEBUG << "Start DBus broker";
}

void Application::handleSignals(int signal)
{
    BMC_LOG_DEBUG << "Catch signal: " << strsignal(signal);
    if (SIGINT == signal)
    {
        BMC_LOG_DEBUG << "SIGINT handle";

        application.terminate();
    }
}

} // namespace core
} // namespace app
