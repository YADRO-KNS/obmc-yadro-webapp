/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include <core/route/redfish/error_messages.hpp>
#include <core/route/redfish/response.hpp>

namespace app
{
namespace core
{
namespace redfish
{
namespace messages
{

/**
 * @internal
 * @brief Formats ResourceInUse message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json resourceInUse(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceInUse"},
        {"Message", "The change to the requested resource failed because "
                    "the resource is in use or in transition."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
        {"Resolution", "Remove the condition and resubmit the request if "
                       "the operation failed."}};
}

void resourceInUse(const RedfishContextPtr& ctx)
{
    ctx->getResponse()->setStatus(http::statuses::Code::ServiceUnavailable);
    ctx->getResponse()->addError(resourceInUse());
}

/**
 * @internal
 * @brief Formats MalformedJSON message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json malformedJSON(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.MalformedJSON"},
        {"Message", "The request body submitted was malformed JSON and "
                    "could not be parsed by the receiving service."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Ensure that the request body is valid JSON and "
                       "resubmit the request."}};
}

void malformedJSON(const RedfishContextPtr& context)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->addError(malformedJSON());
}

/**
 * @internal
 * @brief Formats ResourceMissingAtURI message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json resourceMissingAtURI(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceMissingAtURI"},
        {"Message", "The resource at the URI " + arg1 + " was not found."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Place a valid resource at the URI or correct the "
                       "URI and resubmit the request."}};
}

void resourceMissingAtURI(const RedfishContextPtr& context,
                          const std::string& arg1)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->addError(resourceMissingAtURI(arg1));
}

/**
 * @internal
 * @brief Formats ActionParameterValueFormatError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json actionParameterValueFormatError(
    const std::string& arg1, const std::string& arg2, const std::string& arg3)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ActionParameterValueFormatError"},
        {"Message",
         "The value " + arg1 + " for the parameter " + arg2 +
             " in the action " + arg3 +
             " is of a different format than the parameter can accept."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"MessageSeverity", "Warning"},
        {"Resolution",
         "Correct the value for the parameter in the request body and "
         "resubmit the request if the operation failed."}};
}

void actionParameterValueFormatError(const RedfishContextPtr& context,
                                     const std::string& arg1,
                                     const std::string& arg2,
                                     const std::string& arg3)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->addError(
        actionParameterValueFormatError(arg1, arg2, arg3));
}

/**
 * @internal
 * @brief Formats methodNotAllowed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json methodNotAllowed(const std::string& resource)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.MethodNotAllowed"},
        {"Message", "The requested method is not allowed for the resource '" +
                        resource + "'"},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Resubmit the request.  If the problem persists, "
                       "consider resetting the service."}};
}

void methodNotAllowed(const RedfishContextPtr& context)
{
    context->getResponse()->setStatus(http::statuses::Code::MethodNotAllowed);
    context->getResponse()->addError(
        methodNotAllowed(context->getRequest()->getUriPath()));
}

/**
 * @internal
 * @brief Formats InternalError message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json internalError(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.InternalError"},
        {"Message", "The request failed due to an internal service error.  "
                    "The service is still operational."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Resubmit the request.  If the problem persists, "
                       "consider resetting the service."}};
}

void internalError(const RedfishContextPtr& context)
{
    context->getResponse()->setStatus(
        http::statuses::Code::InternalServerError);
    context->getResponse()->addError(internalError());
}

void internalError(const RedfishContextPtr& context, const std::string& property)
{
    context->getResponse()->setStatus(
        http::statuses::Code::InternalServerError);
    context->getResponse()->propertyError(property, internalError());
}

/**
 * @internal
 * @brief Formats UnrecognizedRequestBody message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json unrecognizedRequestBody(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.UnrecognizedRequestBody"},
        {"Message", "The service detected a malformed request body that it "
                    "was unable to interpret."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
        {"Resolution", "Correct the request body and resubmit the request "
                       "if it failed."}};
}

void unrecognizedRequestBody(const RedfishContextPtr& context)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->addError(unrecognizedRequestBody());
}

/**
 * @internal
 * @brief Formats ResourceAtUriUnauthorized message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json resourceAtUriUnauthorized(const std::string& arg1,
                                                       const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceAtUriUnauthorized"},
        {"Message", "While accessing the resource at " + arg1 +
                        ", the service received an authorization error " +
                        arg2 + "."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Ensure that the appropriate access is provided for "
                       "the service in order for it to access the URI."}};
}

void resourceAtUriUnauthorized(const RedfishContextPtr& context,
                               const std::string& arg1, const std::string& arg2)
{
    context->getResponse()->setStatus(http::statuses::Code::Unauthorized);
    context->getResponse()->addError(resourceAtUriUnauthorized(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ActionParameterUnknown message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json actionParameterUnknown(const std::string& arg1,
                                                    const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ActionParameterUnknown"},
        {"Message", "The action " + arg1 +
                        " was submitted with the invalid parameter " + arg2 +
                        "."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
        {"Resolution", "Correct the invalid parameter and resubmit the "
                       "request if the operation failed."}};
}

void actionParameterUnknown(const RedfishContextPtr& context,
                            const std::string& arg1, const std::string& arg2)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->addError(actionParameterUnknown(arg1, arg2));
}

/**
 * @internal
 * @brief Formats ResourceCannotBeDeleted message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json resourceCannotBeDeleted(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceCannotBeDeleted"},
        {"Message", "The delete request failed because the resource "
                    "requested cannot be deleted."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Do not attempt to delete a non-deletable resource."}};
}

void resourceCannotBeDeleted(const RedfishContextPtr& context)
{
    context->getResponse()->setStatus(http::statuses::Code::Forbidden);
    context->getResponse()->addError(resourceCannotBeDeleted());
}

/**
 * @internal
 * @brief Formats PropertyDuplicate message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json propertyDuplicate(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyDuplicate"},
        {"Message", "The property " + arg1 + " was duplicated in the request."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Warning"},
        {"Resolution",
         "Remove the duplicate property from the request body and resubmit "
         "the request if the operation failed."}};
}

void propertyDuplicate(const RedfishContextPtr& context,
                       const std::string& arg1)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->propertyError(arg1, propertyDuplicate(arg1));
}

/**
 * @internal
 * @brief Formats ServiceTemporarilyUnavailable message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json
    serviceTemporarilyUnavailable(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ServiceTemporarilyUnavailable"},
        {"Message", "The service is temporarily unavailable.  Retry in " +
                        arg1 + " seconds."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Wait for the indicated retry duration and retry "
                       "the operation."}};
}

void serviceTemporarilyUnavailable(const RedfishContextPtr& context,
                                   const std::string& arg1)
{
    context->getResponse()->setHeader("Retry-After", arg1);
    context->getResponse()->setStatus(http::statuses::Code::ServiceUnavailable);
    context->getResponse()->addError(serviceTemporarilyUnavailable(arg1));
}

/**
 * @internal
 * @brief Formats ResourceAlreadyExists message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json resourceAlreadyExists(const std::string& arg1,
                                                   const std::string& arg2,
                                                   const std::string& arg3)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceAlreadyExists"},
        {"Message", "The requested resource of type " + arg1 +
                        " with the property " + arg2 + " with the value " +
                        arg3 + " already exists."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Do not repeat the create operation as the resource "
                       "has already been created."}};
}

void resourceAlreadyExists(const RedfishContextPtr& context,
                           const std::string& type, const std::string& property,
                           const std::string& value)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->propertyError(
        property, resourceAlreadyExists(type, property, value));
}

/**
 * @internal
 * @brief Formats AccountForSessionNoLongerExists message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json accountForSessionNoLongerExists(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.AccountForSessionNoLongerExists"},
        {"Message", "The account for the current session has been removed, "
                    "thus the current session has been removed as well."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "OK"},
        {"Resolution", "Attempt to connect with a valid account."}};
}

void accountForSessionNoLongerExists(const RedfishContextPtr& context)
{
    context->getResponse()->setStatus(http::statuses::Code::Forbidden);
    context->getResponse()->addError(accountForSessionNoLongerExists());
}

/**
 * @internal
 * @brief Formats CreateFailedMissingReqProperties message into JSON
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json
    createFailedMissingReqProperties(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.CreateFailedMissingReqProperties"},
        {"Message",
         "The create operation failed because the required property " + arg1 +
             " was missing from the request."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
        {"Resolution",
         "Correct the body to include the required property with a valid "
         "value and resubmit the request if the operation failed."}};
}

void createFailedMissingReqProperties(const RedfishContextPtr& context,
                                      const std::string& property)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->propertyError(
        property, createFailedMissingReqProperties(property));
}

/**
 * @internal
 * @brief Formats PropertyValueFormatError message into JSON for the specified
 * property
 *
 * See header file for more information
 * @endinternal
 */
static inline nlohmann::json propertyValueFormatError(const std::string& arg1,
                                                      const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyValueFormatError"},
        {"Message",
         "The value " + arg1 + " for the property " + arg2 +
             " is of a different format than the property can accept."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
        {"Resolution",
         "Correct the value for the property in the request body and "
         "resubmit the request if the operation failed."}};
}

void propertyValueFormatError(const RedfishContextPtr& context,
                              const std::string& value,
                              const std::string& property)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->propertyError(
        property, propertyValueFormatError(value, property));
}

/**
 * @internal
 * @brief Formats PropertyValueNotInList message into JSON for the specified
 * property
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json propertyValueNotInList(const std::string& arg1,
                                      const std::string& arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.PropertyValueNotInList"},
        {"Message", "The value " + arg1 + " for the property " + arg2 +
                        " is not in the list of acceptable values."},
        {"MessageArgs", {arg1, arg2}},
        {"MessageSeverity", "Warning"},
        {"Resolution", "Choose a value from the enumeration list that "
                       "the implementation "
                       "can support and resubmit the request if the "
                       "operation failed."}};
}

void propertyValueNotInList(const RedfishContextPtr& context,
                            const std::string& value, const std::string& property)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->propertyError(
        property, propertyValueFormatError(value, property));
}

/**
 * @internal
 * @brief Formats ResourceAtUriInUnknownFormat message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceAtUriInUnknownFormat(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceAtUriInUnknownFormat"},
        {"Message", "The resource at " + arg1 +
                        " is in a format not recognized by the service."},
        {"MessageArgs", {arg1}},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Place an image or resource or file that is "
                       "recognized by the service at the URI."}};
}

void resourceAtUriInUnknownFormat(const RedfishContextPtr& context,
                                  const std::string& uri)
{
    context->getResponse()->setStatus(http::statuses::Code::BadRequest);
    context->getResponse()->addError(resourceAtUriInUnknownFormat(uri));
}

/**
 * @internal
 * @brief Formats ServiceInUnknownState message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json serviceInUnknownState(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ServiceInUnknownState"},
        {"Message",
         "The operation failed because the service is in an unknown state "
         "and can no longer take incoming requests."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Restart the service and resubmit the request if "
                       "the operation failed."}};
}

void serviceInUnknownState(const RedfishContextPtr& context)
{
    context->getResponse()->setStatus(http::statuses::Code::ServiceUnavailable);
    context->getResponse()->addError(serviceInUnknownState());
}

/**
 * @internal
 * @brief Formats EventSubscriptionLimitExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json eventSubscriptionLimitExceeded(void)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.EventSubscriptionLimitExceeded"},
        {"Message",
         "The event subscription failed due to the number of simultaneous "
         "subscriptions exceeding the limit of the implementation."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Critical"},
        {"Resolution",
         "Reduce the number of other subscriptions before trying to "
         "establish the event subscription or increase the limit of "
         "simultaneous subscriptions (if supported)."}};
}

void eventSubscriptionLimitExceeded(const RedfishContextPtr& context)
{
    context->getResponse()->setStatus(http::statuses::Code::ServiceUnavailable);
    context->getResponse()->addError(eventSubscriptionLimitExceeded());
}

} // namespace messages
} // namespace redfish
} // namespace core
} // namespace app
