// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/request.hpp>
#include <core/response.hpp>
#include <core/helpers/utils.hpp>

#include <memory>
#include <string>

#include <phosphor-logging/log.hpp>

namespace app
{
namespace core
{

class IRouteHandler;
class Router;

using RouteHandlerPtr = std::shared_ptr<IRouteHandler>;
using RouteUni = std::unique_ptr<Router>;
using UriSegments = std::vector<std::string>;

using namespace phosphor::logging;

class IRouteHandler
{
  public:
    virtual const ResponsePtr run(const RequestPtr& request) = 0;
    virtual bool preHandlers(const RequestPtr& request) = 0;
    /**
     * @brief Get the URI of handling resource
     * 
     * @return const std::string& - URI
     */
    virtual ~IRouteHandler() = default;
};

class IDynamicRouteHandler
{
  public:
    virtual ~IDynamicRouteHandler() = default;
};

class Router
{
    using RoutePattern = std::size_t;
    using RouteHandlerBuilderFn =
        std::function<RouteHandlerPtr(const std::string&)>;
    using DynamicRoutePatternFn = std::function<bool(const UriSegments&)>;
    using DynamicRouteHandlerBuilderFn =
        std::function<RouteHandlerPtr(const RequestPtr&)>;
    using RouteMap = std::map<RoutePattern, RouteHandlerBuilderFn>;
    using DynamicRouteMap = std::vector<
        std::pair<DynamicRoutePatternFn, DynamicRouteHandlerBuilderFn>>;

  public:
    explicit Router(const RequestPtr& request);

    Router() = delete;
    Router(const Router&) = delete;
    Router(const Router&&) = delete;

    Router& operator=(const Router&) = delete;
    Router& operator=(const Router&&) = delete;

    virtual ~Router() = default;

    const ResponsePtr process();
    bool preHandler();

    template <class THandler, typename... TArg>
    static void registerUri(const std::string pattern, TArg... args)
    {
        static_assert(std::is_base_of_v<IRouteHandler, THandler>,
                      "Unexpected URI handler");
        const std::string uri = helpers::utils::toLower(pattern);
        auto hashPattern = std::hash<std::string>{}(uri);
        auto routerBuilder = [&](const std::string pattern) -> RouteHandlerPtr {
            return std::make_shared<THandler>(pattern, args...);
        };
        auto isRegistered =
            Router::routerHandlers.emplace(hashPattern, routerBuilder).second;
        if (!isRegistered)
        {
            throw std::runtime_error("The route URI '" + pattern +
                                     "' already registered");
        }
    }

    template <class THandler>
    static void registerDynamicUri(DynamicRoutePatternFn rule)
    {
        static_assert(std::is_base_of_v<IDynamicRouteHandler, THandler>,
                      "Unexpected dynamic URI handler");
        auto routerBuilder = [&](const RequestPtr& request) -> RouteHandlerPtr {
            return std::make_shared<THandler>(request);
        };
        auto isRegistered =
            Router::dynamicRouterHandlers.emplace_back(rule, routerBuilder).second;
        if (!isRegistered)
        {
            throw std::runtime_error(
                "The dynamic route URI already registered");
        }
    }

  protected:
    const RequestPtr& getRequest() const
    {
        return requestObject;
    }

    const RequestPtr& getRequest()
    {
        return requestObject;
    }

    void setGeneralHeaders(const ResponsePtr);
  private:
    RequestPtr requestObject;

    static RouteMap routerHandlers;
    static DynamicRouteMap dynamicRouterHandlers;

    RouteHandlerPtr handler;
};

} // namespace core

} // namespace app
