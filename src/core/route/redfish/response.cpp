// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#include <core/route/redfish/response.hpp>
#include <phosphor-logging/log.hpp>
#include <core/route/redfish/error_messages.hpp>

namespace app
{
namespace core
{
namespace redfish
{

using namespace phosphor::logging;

void RedfishResponse::addError(const nlohmann::json&& message)
{
    auto& error = this->payload["error"];

    // If this is the first error message, fill in the information from the
    // first error message to the top level struct
    if (!error.is_object())
    {
        auto messageIdIterator = message.find("MessageId");
        if (messageIdIterator == message.end())
        {
            log<level::CRIT>("Attempt to add error message without MessageId");
            return;
        }

        auto messageFieldIterator = message.find("Message");
        if (messageFieldIterator == message.end())
        {
            log<level::CRIT>("Attempt to add error message without Message");
            return;
        }
        error = {{"code", *messageIdIterator},
                 {"message", *messageFieldIterator}};
    }
    else
    {
        // More than 1 error occurred, so the message has to be generic
        error["code"] =
            std::string(messages::messageVersionPrefix) + "GeneralError";
        error["message"] = "A general error has occurred. See Resolution for "
                           "information on how to resolve the error.";
    }

    // This check could technically be done in in the default construction
    // branch above, but because we need the pointer to the extended info field
    // anyway, it's more efficient to do it here.
    auto& extendedInfo = error[messages::messageAnnotation];
    if (!extendedInfo.is_array())
    {
        extendedInfo = nlohmann::json::array();
    }

    extendedInfo.push_back(message);
}

void RedfishResponse::propertyError(const std::string& property,
                                    const nlohmann::json&& message)
{
    std::string extendedInfo(property + messages::messageAnnotation);

    if (!payload[extendedInfo].is_array())
    {
        // Force object to be an array
        payload[extendedInfo] = nlohmann::json::array();
    }

    // Object exists and it is an array so we can just push in the message
    payload[extendedInfo].push_back(message);
}

void RedfishResponse::add(const std::string& key, const nlohmann::json& json)
{
    log<level::DEBUG>(("Add payload: " + key + ", data: " + json.dump(2)).c_str());
    if (json.is_array())
    {
        if (payload[key].is_null() || !payload[key].is_array())
        {
            payload[key] = nlohmann::json::array_t({});
        }
        payload[key].insert(payload[key].end(), json.begin(), json.end());
        return;
    }

    payload[key] = json;
}

const nlohmann::json RedfishResponse::getJson() const
{
    return payload;
}

void RedfishResponse::flash()
{
    clear();
    push(payload.dump(2));
}

RedfishContext::~RedfishContext()
{
    response->flash();
}

const RequestPtr& RedfishContext::getRequest() const
{
    return request;
}

const RedfishResponsePtr& RedfishContext::getResponse() const
{
    return response;
}

const entity::IEntity::InstancePtr
    RedfishContext::ParameterCtx::getInstance() const
{
    return entityInstance;
}
const entity::EntityPtr
    RedfishContext::ParameterCtx::getEntity() const
{
    return sourceEntity;
}

} // namespace redfish
} // namespace core
} // namespace app
