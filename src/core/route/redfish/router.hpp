// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <core/application.hpp>
#include <core/exceptions.hpp>
#include <core/route/redfish/error_messages.hpp>
#include <core/route/redfish/redfish_root.hpp>
#include <core/route/redfish/redfish_metadata.hpp>
#include <core/route/redfish/response.hpp>
#include <core/router.hpp>
#include <phosphor-logging/log.hpp>

#include <type_traits>
#include <vector>

namespace app
{
namespace core
{
namespace redfish
{
namespace router
{
using namespace phosphor::logging;
namespace exceptions
{
using namespace ::app::core::exceptions;
class RedfishException : public ObmcAppException
{
  public:
    explicit RedfishException(const std::string& message) :
        ObmcAppException(message) _GLIBCXX_TXN_SAFE
    {}
    virtual ~RedfishException() = default;
};
class RedfishUriNotFound : public RedfishException
{
    explicit RedfishUriNotFound(const std::string& uri) :
        RedfishException(uri) _GLIBCXX_TXN_SAFE
    {}
    virtual ~RedfishUriNotFound() = default;
};
} // namespace exceptions

/**
 * @class RedfishRouter
 *
 * @brief Provides REDFISH resource router
 *
 */
class RedfishRouter : public IRouteHandler, public IDynamicRouteHandler
{
  public:
    explicit RedfishRouter(const RequestPtr& request) : request(request)
    {}

    RedfishRouter() = delete;
    ~RedfishRouter() override = default;

    bool preHandlers(const RequestPtr&) override
    {
        return true;
    }
    const ResponsePtr run(const RequestPtr& request) override
    {
        using namespace Fastcgipp::Http;
        static constexpr size_t firstSegmentIndex = 0;
        auto ctx = std::make_shared<RedfishContext>(request);
        ctx->getResponse()->setStatus(statuses::Code::BadRequest);
        ctx->getResponse()->setContentType(
            http::content_types::applicationJson);

        try
        {
            // Attempt to deduction of a relevant node
            auto node = RedfishRootNode::uriResolver(ctx, firstSegmentIndex);
            node->process();
        }
        catch (std::exception& e)
        {
            log<level::DEBUG>(
                "Error to handle REDFISH request",
                entry("REDFISH_URI=%s", request->getUriPath().c_str()),
                entry("ERROR=%s", e.what()));
            messages::internalError(ctx);
        }

        return ctx->getResponse();
    }

    static void registerRoute()
    {
        using namespace std::placeholders;
        Router::registerDynamicUri<RedfishRouter>(
            std::bind(RedfishRouter::searchRouteHandler, _1));
    }

  protected:
    /**
     * @brief Approximate an URI to verify if it matches to defined pattern
     *
     * @param uriSegments - The URI segments to be matched to defined pattern
     */
    static bool searchRouteHandler(const UriSegments& segments)
    {
        if (!segments.empty() && segments.at(0) == RedfishRootNode::segment)
        {
            return true;
        }

        return false;
    }

  private:
    RequestPtr request;
};
} // namespace router
} // namespace redfish
} // namespace core
} // namespace app
