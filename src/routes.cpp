
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/application.hpp>
#include <core/route/handlers/graphql_handler.hpp>
#include <core/route/redfish/node.hpp>
#include <core/route/redfish/router.hpp>

namespace app
{
namespace core
{

void Application::registerAllRoutes()
{
    route::handlers::VisitorFactory::registerGqlVisitors();

    // GraphQL
    Router::registerUri<route::handlers::GraphqlRouter>("/api/graphql/");

    // Redfish
    redfish::router::RedfishRouter::registerRoute();
}

} // namespace core
} // namespace app
