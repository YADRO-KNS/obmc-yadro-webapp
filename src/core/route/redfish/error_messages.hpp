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
#pragma once

#include <nlohmann/json.hpp>
#include <core/route/redfish/response.hpp>

namespace app
{
namespace core
{
namespace redfish
{
namespace messages
{

constexpr const char* messageVersionPrefix = "Base.1.8.1.";
constexpr const char* messageAnnotation = "@Message.ExtendedInfo";

/**
 * @brief Formats ResourceInUse message into JSON
 * Message body: "The change to the requested resource failed because the
 * resource is in use or in transition."
 *
 *
 * @returns Message ResourceInUse formatted to JSON */
void resourceInUse(const RedfishContextPtr& res);

/**
 * @brief Formats MalformedJSON message into JSON
 * Message body: "The request body submitted was malformed JSON and could not be
 * parsed by the receiving service."
 *
 *
 * @returns Message MalformedJSON formatted to JSON */
void malformedJSON(const RedfishContextPtr& context);

/**
 * @brief Formats ResourceMissingAtURI message into JSON
 * Message body: "The resource at the URI <arg1> was not found."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ResourceMissingAtURI formatted to JSON */
void resourceMissingAtURI(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats ActionParameterValueFormatError message into JSON
 * Message body: "The value <arg1> for the parameter <arg2> in the action <arg3>
 * is of a different format than the parameter can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 * @param[in] arg3 Parameter of message that will replace %3 in its body.
 */
void actionParameterValueFormatError(const RedfishContextPtr& context,
                                     const std::string& arg1,
                                     const std::string& arg2,
                                     const std::string& arg3);

/**
 * @brief Formats InternalError message into JSON
 * Message body: "The request failed due to an internal service error.  The
 * service is still operational."
 */
void internalError(const RedfishContextPtr& context);

/**
 * @brief Formats InternalError message into JSON
 * Message body for specified property: "The request failed due to an internal
 * service error.  The service is still operational."
 * @param[in] property The name of the property to be attached into message
 */
void internalError(const RedfishContextPtr& context, const std::string& property);

/**
 * @brief Formats MethodNotAllowed message into JSON
 * Message body: "The requested method is not allowed for the resource '<uri>'"
 */
void methodNotAllowed(const RedfishContextPtr& context);
/**
 * @brief Formats UnrecognizedRequestBody message into JSON
 * Message body: "The service detected a malformed request body that it was
 * unable to interpret."
 *
 *
 * @returns Message UnrecognizedRequestBody formatted to JSON */
void unrecognizedRequestBody(const RedfishContextPtr& context);

/**
 * @brief Formats ResourceAtUriUnauthorized message into JSON
 * Message body: "While accessing the resource at <arg1>, the service received
 * an authorization error <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ResourceAtUriUnauthorized formatted to JSON */
void resourceAtUriUnauthorized(const RedfishContextPtr& context, const std::string& arg1,
                               const std::string& arg2);

/**
 * @brief Formats ActionParameterUnknown message into JSON
 * Message body: "The action <arg1> was submitted with the invalid parameter
 * <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ActionParameterUnknown formatted to JSON */
void actionParameterUnknown(const RedfishContextPtr& context, const std::string& arg1,
                            const std::string& arg2);

/**
 * @brief Formats ResourceCannotBeDeleted message into JSON
 * Message body: "The delete request failed because the resource requested
 * cannot be deleted."
 *
 *
 * @returns Message ResourceCannotBeDeleted formatted to JSON */
void resourceCannotBeDeleted(const RedfishContextPtr& context);

/**
 * @brief Formats PropertyDuplicate message into JSON
 * Message body: "The property <arg1> was duplicated in the request."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message PropertyDuplicate formatted to JSON */
void propertyDuplicate(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats ServiceTemporarilyUnavailable message into JSON
 * Message body: "The service is temporarily unavailable.  Retry in <arg1>
 * seconds."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ServiceTemporarilyUnavailable formatted to JSON */
void serviceTemporarilyUnavailable(const RedfishContextPtr& context,
                                   const std::string& arg1);

/**
 * @brief Formats ResourceAlreadyExists message into JSON
 * Message body: "The requested resource of type <arg1> with the property <arg2>
 * with the value <arg3> already exists."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 * @param[in] arg3 Parameter of message that will replace %3 in its body.
 *
 * @returns Message ResourceAlreadyExists formatted to JSON */
void resourceAlreadyExists(const RedfishContextPtr& context, const std::string& arg1,
                           const std::string& arg2, const std::string& arg3);

/**
 * @brief Formats AccountForSessionNoLongerExists message into JSON
 * Message body: "The account for the current session has been removed, thus the
 * current session has been removed as well."
 *
 *
 * @returns Message AccountForSessionNoLongerExists formatted to JSON */
void accountForSessionNoLongerExists(const RedfishContextPtr& context);

/**
 * @brief Formats CreateFailedMissingReqProperties message into JSON
 * Message body: "The create operation failed because the required property
 * <arg1> was missing from the request."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message CreateFailedMissingReqProperties formatted to JSON */
void createFailedMissingReqProperties(const RedfishContextPtr& context,
                                      const std::string& arg1);

/**
 * @brief Formats PropertyValueFormatError message into JSON
 * Message body: "The value <arg1> for the property <arg2> is of a different
 * format than the property can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueFormatError formatted to JSON */
void propertyValueFormatError(const RedfishContextPtr& context, const std::string& arg1,
                              const std::string& arg2);

/**
 * @brief Formats PropertyValueNotInList message into JSON
 * Message body: "The value <arg1> for the property <arg2> is not in the list of
 * acceptable values."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueNotInList formatted to JSON */
void propertyValueNotInList(const RedfishContextPtr& context, const std::string& arg1,
                            const std::string& arg2);

/**
 * @brief Formats ResourceAtUriInUnknownFormat message into JSON
 * Message body: "The resource at <arg1> is in a format not recognized by the
 * service."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ResourceAtUriInUnknownFormat formatted to JSON */
void resourceAtUriInUnknownFormat(const RedfishContextPtr& context,
                                  const std::string& arg1);

/**
 * @brief Formats ServiceInUnknownState message into JSON
 * Message body: "The operation failed because the service is in an unknown
 * state and can no longer take incoming requests."
 *
 *
 * @returns Message ServiceInUnknownState formatted to JSON */
void serviceInUnknownState(const RedfishContextPtr& context);

/**
 * @brief Formats EventSubscriptionLimitExceeded message into JSON
 * Message body: "The event subscription failed due to the number of
 * simultaneous subscriptions exceeding the limit of the implementation."
 *
 *
 * @returns Message EventSubscriptionLimitExceeded formatted to JSON */
void eventSubscriptionLimitExceeded(const RedfishContextPtr& context);

/**
 * @brief Formats ActionParameterMissing message into JSON
 * Message body: "The action <arg1> requires the parameter <arg2> to be present
 * in the request body."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ActionParameterMissing formatted to JSON */
void actionParameterMissing(const RedfishContextPtr& context, const std::string& arg1,
                            const std::string& arg2);

/**
 * @brief Formats StringValueTooLong message into JSON
 * Message body: "The string <arg1> exceeds the length limit <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message StringValueTooLong formatted to JSON */
void stringValueTooLong(const RedfishContextPtr& context, const std::string& arg1,
                        const int& arg2);

/**
 * @brief Formats SessionTerminated message into JSON
 * Message body: "The session was successfully terminated."
 *
 *
 * @returns Message SessionTerminated formatted to JSON */
void sessionTerminated(const RedfishContextPtr& context);

/**
 * @brief Formats SubscriptionTerminated message into JSON
 * Message body: "The event subscription has been terminated."
 *
 *
 * @returns Message SubscriptionTerminated formatted to JSON */
void subscriptionTerminated(const RedfishContextPtr& context);

/**
 * @brief Formats ResourceTypeIncompatible message into JSON
 * Message body: "The @odata.type of the request body <arg1> is incompatible
 * with the @odata.type of the resource which is <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ResourceTypeIncompatible formatted to JSON */
void resourceTypeIncompatible(const RedfishContextPtr& context, const std::string& arg1,
                              const std::string& arg2);

/**
 * @brief Formats ResetRequired message into JSON
 * Message body: "In order to complete the operation, a component reset is
 * required with the Reset action URI '<arg1>' and ResetType '<arg2>'."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ResetRequired formatted to JSON */
void resetRequired(const RedfishContextPtr& context, const std::string& arg1,
                   const std::string& arg2);

/**
 * @brief Formats ChassisPowerStateOnRequired message into JSON
 * Message body: "The Chassis with Id '<arg1>' requires to be powered on to
 * perform this request."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ChassisPowerStateOnRequired formatted to JSON */
void chassisPowerStateOnRequired(const RedfishContextPtr& context,
                                 const std::string& arg1);

/**
 * @brief Formats ChassisPowerStateOffRequired message into JSON
 * Message body: "The Chassis with Id '<arg1>' requires to be powered off to
 * perform this request."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ChassisPowerStateOffRequired formatted to JSON */
void chassisPowerStateOffRequired(const RedfishContextPtr& context,
                                  const std::string& arg1);

/**
 * @brief Formats PropertyValueConflict message into JSON
 * Message body: "The property '<arg1>' could not be written because its value
 * would conflict with the value of the '<arg2>' property."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueConflict formatted to JSON */
void propertyValueConflict(const RedfishContextPtr& context, const std::string& arg1,
                           const std::string& arg2);

/**
 * @brief Formats PropertyValueIncorrect message into JSON
 * Message body: "The property '<arg1>' with the requested value of '<arg2>'
 * could not be written because the value does not meet the constraints of the
 * implementation."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueIncorrect formatted to JSON */
void propertyValueIncorrect(const RedfishContextPtr& context, const std::string& arg1,
                            const std::string& arg2);

/**
 * @brief Formats ResourceCreationConflict message into JSON
 * Message body: "The resource could not be created.  The service has a resource
 * at URI '<arg1>' that conflicts with the creation request."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ResourceCreationConflict formatted to JSON */
void resourceCreationConflict(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats MaximumErrorsExceeded message into JSON
 * Message body: "Too many errors have occurred to report them all."
 *
 *
 * @returns Message MaximumErrorsExceeded formatted to JSON */
void maximumErrorsExceeded(const RedfishContextPtr& context);

/**
 * @brief Formats PreconditionFailed message into JSON
 * Message body: "The ETag supplied did not match the ETag required to change
 * this resource."
 *
 *
 * @returns Message PreconditionFailed formatted to JSON */
void preconditionFailed(const RedfishContextPtr& context);

/**
 * @brief Formats PreconditionRequired message into JSON
 * Message body: "A precondition header or annotation is required to change this
 * resource."
 *
 *
 * @returns Message PreconditionRequired formatted to JSON */
void preconditionRequired(const RedfishContextPtr& context);

/**
 * @brief Formats OperationFailed message into JSON
 * Message body: "An error occurred internal to the service as part of the
 * overall request.  Partial results may have been returned."
 *
 *
 * @returns Message OperationFailed formatted to JSON */
void operationFailed(const RedfishContextPtr& context);

/**
 * @brief Formats OperationTimeout message into JSON
 * Message body: "A timeout internal to the service occured as part of the
 * request.  Partial results may have been returned."
 *
 *
 * @returns Message OperationTimeout formatted to JSON */
void operationTimeout(const RedfishContextPtr& context);

/**
 * @brief Formats PropertyValueTypeError message into JSON
 * Message body: "The value <arg1> for the property <arg2> is of a different
 * type than the property can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueTypeError formatted to JSON */
void propertyValueTypeError(const RedfishContextPtr& context, const std::string& arg1,
                            const std::string& arg2);

/**
 * @brief Formats ResourceNotFound message into JSON
 * Message body: "The requested resource of type <arg1> named <arg2> was not
 * found."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ResourceNotFound formatted to JSON */
void resourceNotFound(const RedfishContextPtr& context, const std::string& arg1,
                      const std::string& arg2);

/**
 * @brief Formats UriNotFound message into JSON
 * Message body: "The requested resource on the URL <uri> was not found."
 *
 * @returns Message UriNotFound formatted to JSON */
void uriNotFound(const RedfishContextPtr& context);

/**
 * @brief Formats CouldNotEstablishConnection message into JSON
 * Message body: "The service failed to establish a Connection with the URI
 * <arg1>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message CouldNotEstablishConnection formatted to JSON */
void couldNotEstablishConnection(const RedfishContextPtr& context,
                                 const std::string& arg1);

/**
 * @brief Formats PropertyNotWritable message into JSON
 * Message body: "The property <arg1> is a read only property and cannot be
 * assigned a value."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message PropertyNotWritable formatted to JSON */
void propertyNotWritable(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats QueryParameterValueTypeError message into JSON
 * Message body: "The value <arg1> for the query parameter <arg2> is of a
 * different type than the parameter can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message QueryParameterValueTypeError formatted to JSON */
void queryParameterValueTypeError(const RedfishContextPtr& context,
                                  const std::string& arg1,
                                  const std::string& arg2);

/**
 * @brief Formats ServiceShuttingDown message into JSON
 * Message body: "The operation failed because the service is shutting down and
 * can no longer take incoming requests."
 *
 *
 * @returns Message ServiceShuttingDown formatted to JSON */
void serviceShuttingDown(const RedfishContextPtr& context);

/**
 * @brief Formats ActionParameterDuplicate message into JSON
 * Message body: "The action <arg1> was submitted with more than one value for
 * the parameter <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ActionParameterDuplicate formatted to JSON */
void actionParameterDuplicate(const RedfishContextPtr& context, const std::string& arg1,
                              const std::string& arg2);

/**
 * @brief Formats ActionParameterNotSupported message into JSON
 * Message body: "The parameter <arg1> for the action <arg2> is not supported on
 * the target resource."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message ActionParameterNotSupported formatted to JSON */
void actionParameterNotSupported(const RedfishContextPtr& context,
                                 const std::string& arg1,
                                 const std::string& arg2);

/**
 * @brief Formats SourceDoesNotSupportProtocol message into JSON
 * Message body: "The other end of the Connection at <arg1> does not support the
 * specified protocol <arg2>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message SourceDoesNotSupportProtocol formatted to JSON */
void sourceDoesNotSupportProtocol(const RedfishContextPtr& context,
                                  const std::string& arg1,
                                  const std::string& arg2);

/**
 * @brief Formats AccountRemoved message into JSON
 * Message body: "The account was successfully removed."
 *
 *
 * @returns Message AccountRemoved formatted to JSON */
void accountRemoved(const RedfishContextPtr& context);

/**
 * @brief Formats AccessDenied message into JSON
 * Message body: "While attempting to establish a Connection to <arg1>, the
 * service denied access."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message AccessDenied formatted to JSON */
void accessDenied(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats QueryNotSupported message into JSON
 * Message body: "Querying is not supported by the implementation."
 *
 *
 * @returns Message QueryNotSupported formatted to JSON */
void queryNotSupported(const RedfishContextPtr& context);

/**
 * @brief Formats CreateLimitReachedForResource message into JSON
 * Message body: "The create operation failed because the resource has reached
 * the limit of possible resources."
 *
 *
 * @returns Message CreateLimitReachedForResource formatted to JSON */
void createLimitReachedForResource(const RedfishContextPtr& context);

/**
 * @brief Formats GeneralError message into JSON
 * Message body: "A general error has occurred. See ExtendedInfo for more
 * information."
 *
 *
 * @returns Message GeneralError formatted to JSON
 */
void generalError(const RedfishContextPtr& context);

/**
 * @brief Formats Success message into JSON
 * Message body: "Successfully Completed Request"
 *
 *
 * @returns Message Success formatted to JSON
 */
void success(const RedfishContextPtr& context);

/**
 * @brief Formats Created message into JSON
 * Message body: "The resource has been created successfully"
 *
 *
 * @returns Message Created formatted to JSON
 */
void created(const RedfishContextPtr& context);

/**
 * @brief Formats NoOperation message into JSON
 * Message body: "The request body submitted contain no data to act upon and
 * no changes to the resource took place."
 *
 *
 * @returns Message NoOperation formatted to JSON
 */
void noOperation(const RedfishContextPtr& context);

/**
 * @brief Formats PropertyUnknown message into JSON
 * Message body: "The property <arg1> is not in the list of valid properties for
 * the resource."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message PropertyUnknown formatted to JSON
 */
void propertyUnknown(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats NoValidSession message into JSON
 * Message body: "There is no valid session established with the
 * implementation."
 *
 *
 * @returns Message NoValidSession formatted to JSON
 */
void noValidSession(const RedfishContextPtr& context);

/**
 * @brief Formats InvalidObject message into JSON
 * Message body: "The object at <arg1> is invalid."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message InvalidObject formatted to JSON
 */
void invalidObject(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats ResourceInStandby message into JSON
 * Message body: "The request could not be performed because the resource is in
 * standby."
 *
 *
 * @returns Message ResourceInStandby formatted to JSON
 */
void resourceInStandby(const RedfishContextPtr& context);

/**
 * @brief Formats ActionParameterValueTypeError message into JSON
 * Message body: "The value <arg1> for the parameter <arg2> in the action <arg3>
 * is of a different type than the parameter can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 * @param[in] arg3 Parameter of message that will replace %3 in its body.
 *
 * @returns Message ActionParameterValueTypeError formatted to JSON
 */
void actionParameterValueTypeError(const RedfishContextPtr& context,
                                   const std::string& arg1,
                                   const std::string& arg2,
                                   const std::string& arg3);

/**
 * @brief Formats SessionLimitExceeded message into JSON
 * Message body: "The session establishment failed due to the number of
 * simultaneous sessions exceeding the limit of the implementation."
 *
 *
 * @returns Message SessionLimitExceeded formatted to JSON
 */
void sessionLimitExceeded(const RedfishContextPtr& context);

/**
 * @brief Formats ActionNotSupported message into JSON
 * Message body: "The action <arg1> is not supported by the resource."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ActionNotSupported formatted to JSON
 */
void actionNotSupported(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats InvalidIndex message into JSON
 * Message body: "The index <arg1> is not a valid offset into the array."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message InvalidIndex formatted to JSON
 */
void invalidIndex(const RedfishContextPtr& context, const int& arg1);

/**
 * @brief Formats EmptyJSON message into JSON
 * Message body: "The request body submitted contained an empty JSON object and
 * the service is unable to process it."
 *
 *
 * @returns Message EmptyJSON formatted to JSON
 */
void emptyJSON(const RedfishContextPtr& context);

/**
 * @brief Formats QueryNotSupportedOnResource message into JSON
 * Message body: "Querying is not supported on the requested resource."
 *
 *
 * @returns Message QueryNotSupportedOnResource formatted to JSON
 */
void queryNotSupportedOnResource(const RedfishContextPtr& context);

/**
 * @brief Formats QueryNotSupportedOnOperation message into JSON
 * Message body: "Querying is not supported with the requested operation."
 *
 *
 * @returns Message QueryNotSupportedOnOperation formatted to JSON
 */
void queryNotSupportedOnOperation(const RedfishContextPtr& context);

/**
 * @brief Formats QueryCombinationInvalid message into JSON
 * Message body: "Two or more query parameters in the request cannot be used
 * together."
 *
 *
 * @returns Message QueryCombinationInvalid formatted to JSON
 */
void queryCombinationInvalid(const RedfishContextPtr& context);

/**
 * @brief Formats InsufficientPrivilege message into JSON
 * Message body: "There are insufficient privileges for the account or
 * credentials associated with the current session to perform the requested
 * operation."
 *
 *
 * @returns Message InsufficientPrivilege formatted to JSON
 */
void insufficientPrivilege(const RedfishContextPtr& context);

/**
 * @brief Formats PropertyValueModified message into JSON
 * Message body: "The property <arg1> was assigned the value <arg2> due to
 * modification by the service."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message PropertyValueModified formatted to JSON
 */
void propertyValueModified(const RedfishContextPtr& context, const std::string& arg1,
                           const std::string& arg2);

/**
 * @brief Formats AccountNotModified message into JSON
 * Message body: "The account modification request failed."
 *
 *
 * @returns Message AccountNotModified formatted to JSON
 */
void accountNotModified(const RedfishContextPtr& context);

/**
 * @brief Formats QueryParameterValueFormatError message into JSON
 * Message body: "The value <arg1> for the parameter <arg2> is of a different
 * format than the parameter can accept."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message QueryParameterValueFormatError formatted to JSON
 */
void queryParameterValueFormatError(const RedfishContextPtr& context,
                                    const std::string& arg1,
                                    const std::string& arg2);

/**
 * @brief Formats PropertyMissing message into JSON
 * Message body: "The property <arg1> is a required property and must be
 * included in the request."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message PropertyMissing formatted to JSON */
void propertyMissing(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats ResourceExhaustion message into JSON
 * Message body: "The resource <arg1> was unable to satisfy the request due to
 * unavailability of resources."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message ResourceExhaustion formatted to JSON */
void resourceExhaustion(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats AccountModified message into JSON
 * Message body: "The account was successfully modified."
 *
 *
 * @returns Message AccountModified formatted to JSON */
void accountModified(const RedfishContextPtr& context);

/**
 * @brief Formats QueryParameterOutOfRange message into JSON
 * Message body: "The value <arg1> for the query parameter <arg2> is out of
 * range <arg3>."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 * @param[in] arg3 Parameter of message that will replace %3 in its body.
 *
 * @returns Message QueryParameterOutOfRange formatted to JSON */
void queryParameterOutOfRange(const RedfishContextPtr& context, const std::string& arg1,
                              const std::string& arg2, const std::string& arg3);

/**
 * @brief Formats PasswordChangeRequired message into JSON
 * Message body: The password provided for this account must be changed
 * before access is granted.  PATCH the 'Password' property for this
 * account located at the target URI '%1' to complete this process.
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 *
 * @returns Message PasswordChangeRequired formatted to JSON */
void passwordChangeRequired(const RedfishContextPtr& context, const std::string& arg1);

/**
 * @brief Formats InvalidUpload message into JSON
 * Message body: Invalid file uploaded to %1: %2.*
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message InvalidUpload formatted to JSON */
void invalidUpload(const RedfishContextPtr& context, const std::string& arg1,
                   const std::string& arg2);

/**
 * @brief Formats MutualExclusiveProperties message into JSON
 * Message body: "The properties <arg1> and <arg2> are mutually exclusive."
 *
 * @param[in] arg1 Parameter of message that will replace %1 in its body.
 * @param[in] arg2 Parameter of message that will replace %2 in its body.
 *
 * @returns Message MutualExclusiveProperties formatted to JSON */
void mutualExclusiveProperties(const RedfishContextPtr& context, const std::string& arg1,
                               const std::string& arg2);

} // namespace messages
} // namespace redfish
} // namespace core
} // namespace app
