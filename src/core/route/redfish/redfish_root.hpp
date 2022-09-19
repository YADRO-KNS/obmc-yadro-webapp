// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <core/route/redfish/node.hpp>
#include <core/router.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <redfish/generated/RedfishV1.hpp>

namespace app
{
namespace core
{
namespace redfish
{
using namespace phosphor::logging;
/**
 * @class RedfishRoot
 * @brief Any resource that a client discovers through hyperlinks that the
 *        service root or any service root-referenced service or resource
 * returns shall conform to the same protocol version that the service root
 * supports.
 * @link http://redfish.dmtf.org/schemas/v1/redfish-schema.v1_8_0
 */
class RedfishRootNode : public Node<RedfishRootNode, RedfishV1>
{
  public:
    static constexpr const char* segment = "redfish";

    explicit RedfishRootNode(const RequestPtr& request) :
        Node<RedfishRootNode, RedfishV1>(request)
    {}
    RedfishRootNode(const RedfishContextPtr ctx) :
        Node<RedfishRootNode, RedfishV1>(ctx)
    {}
    ~RedfishRootNode() override = default;

  protected:
    const FieldHandlers getFieldsGetters() const override
    {
        static const FieldHandlers getters{
            // Link to the REDFISH V1
            createAction<StringGetter>("v1", "/redfish/v1/"),
        };
        return getters;
    }

    void methodGet() const override
    {
        try
        {
            ctx->getResponse()->setStatus(http::statuses::Code::OK);
            Node::methodGet();
        }
        catch (const std::exception& e)
        {
            messages::internalError(ctx);
        }
    }
};
} // namespace redfish
} // namespace core
} // namespace app
