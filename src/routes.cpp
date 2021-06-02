
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/application.hpp>
#include <core/route/handlers/graphql_handler.hpp>

namespace app
{
namespace core
{

void Application::registerAllRoutes()
{
    Router::registerUri<route::handlers::GraphqlRouter>("/api/graphql");
}

} // namespace core
} // namespace app
