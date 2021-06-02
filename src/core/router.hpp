// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef BMC_ROUTER_HPP
#define BMC_ROUTER_HPP

#include <core/request.hpp>
#include <core/response.hpp>

#include <memory>
#include <string>

namespace app
{
namespace core
{

class IRouteHandler;
class Router;

using RouteHandlerPtr = std::shared_ptr<IRouteHandler>;
using RouteUni = std::unique_ptr<Router>;

class IRouteHandler
{
  public:
    virtual void run(const RequestPtr& request, ResponseUni& response) = 0;
    virtual bool preHandlers(const RequestPtr& request) = 0;

    virtual ~IRouteHandler() = default;
};

class Router
{
    using RoutePattern = std::string;
    using RouteHandlerBuilderFn = std::function<RouteHandlerPtr(const std::string&)>;
    using RouteMap = std::map<RoutePattern, RouteHandlerBuilderFn>;

  public:
    explicit Router(const RequestPtr& request);

    Router() = delete;
    Router(const Router&) = delete;
    Router(const Router&&) = delete;

    Router& operator=(const Router&) = delete;
    Router& operator=(const Router&&) = delete;

    virtual ~Router() = default;

    const ResponseUni& getResponse() const
    {
        return responseObject;
    }

    const ResponseUni& process();
    bool preHandler();

    template <class THandler, typename... TArg>
    static void registerUri(const std::string pattern, TArg... args)
    {
        static_assert(std::is_base_of_v<IRouteHandler, THandler>,
                      "Unexpected URI handler");
        Router::routerHandlers.emplace(
            pattern, [&](const std::string pattern) -> RouteHandlerPtr {
                return std::make_shared<THandler>(pattern, args...);
            });
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

    ResponseUni& getResponse()
    {
        return responseObject;
    }

  private:
    RequestPtr requestObject;
    ResponseUni responseObject;

    static RouteMap routerHandlers;

    RouteHandlerPtr handler;
};

} // namespace core

} // namespace app

#endif //! BMC_ROUTER_HPP
