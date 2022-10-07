// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef BMC_HEADERS_HPP
#define BMC_HEADERS_HPP

#include <string>
#include <iostream>

namespace app
{
namespace http
{
namespace headers
{
constexpr const char* http = "HTTP/1.1";
constexpr const char* contentType = "Content-Type";
constexpr const char* contentLength = "Content-Length";
constexpr const char* date = "Date";
constexpr const char* location = "Location";
constexpr const char* wwwAuthenticate = "WWW-Authenticate";
} // namespace headers

namespace statuses
{

enum class Code: uint16_t
{
    /*####### 1xx - Informational #######*/
    /* Indicates an interim response for communicating connection status
     * or request progress prior to completing the requested action and
     * sending a final response.
     */
    Continue = 100, // Indicates that the initial part of a request has been
                    // received and has not yet been rejected by the server.
    SwitchingProtocols =
        101, // Indicates that the server understands and is willing to comply
             // with the client's request, via the Upgrade header field, for a
             // change in the application protocol being used on this
             // connection.
    Processing = 102, // Is an interim response used to inform the client that
                      // the server has accepted the complete request, but has
                      // not yet completed it.
    EarlyHints = 103, // Indicates to the client that the server is likely to
                      // send a final response with the header fields included
                      // in the informational response.

    /*####### 2xx - Successful #######*/
    /* Indicates that the client's request was successfully received,
     * understood, and accepted.
     */
    OK = 200,       // Indicates that the request has succeeded.
    Created = 201,  // Indicates that the request has been fulfilled and has
                    // resulted in one or more new resources being created.
    Accepted = 202, // Indicates that the request has been accepted for
                    // processing, but the processing has not been completed.
    NonAuthoritativeInformation =
        203, // Indicates that the request was successful but the enclosed
             // payload has been modified from that of the origin server's 200
             // (OK) response by a transforming proxy.
    NoContent = 204, // Indicates that the server has successfully fulfilled
                     // the request and that there is no additional content to
                     // send in the response payload body.
    ResetContent =
        205, // Indicates that the server has fulfilled the request and
             // desires that the user agent reset the \"document view\", which
             // caused the request to be sent, to its original state as
             // received from the origin server.
    PartialContent =
        206, // Indicates that the server is successfully fulfilling a range
             // request for the target resource by transferring one or more
             // parts of the selected representation that correspond to the
             // satisfiable ranges found in the requests's Range header field.
    MultiStatus = 207, // Provides status for multiple independent operations.
    AlreadyReported =
        208, // Used inside a DAV:propstat response element to avoid
             // enumerating the internal members of multiple bindings to the
             // same collection repeatedly. [RFC 5842]
    IMUsed =
        226, // The server has fulfilled a GET request for the resource, and
             // the response is a representation of the result of one or more
             // instance-manipulations applied to the current instance.

    /*####### 3xx - Redirection #######*/
    /* Indicates that further action needs to be taken by the user agent
     * in order to fulfill the request.
     */
    MultipleChoices =
        300, // Indicates that the target resource has more than one
             // representation, each with its own more specific identifier,
             // and information about the alternatives is being provided so
             // that the user (or user agent) can select a preferred
             // representation by redirecting its request to one or more of
             // those identifiers.
    MovedPermanently =
        301, // Indicates that the target resource has been assigned a new
             // permanent URI and any future references to this resource ought
             // to use one of the enclosed URIs.
    Found = 302,    // Indicates that the target resource resides temporarily
                    // under a different URI.
    SeeOther = 303, // Indicates that the server is redirecting the user agent
                    // to a different resource, as indicated by a URI in the
                    // Location header field, that is intended to provide an
                    // indirect response to the original request.
    NotModified =
        304, // Indicates that a conditional GET request has been received and
             // would have resulted in a 200 (OK) response if it were not for
             // the fact that the condition has evaluated to false.
    UseProxy = 305, // \deprecated \parblock Due to security concerns
                    // regarding in-band configuration of a proxy.
                    // \endparblock The requested resource MUST be accessed
                    // through the proxy given by the Location field.
    TemporaryRedirect =
        307, // Indicates that the target resource resides temporarily under a
             // different URI and the user agent MUST NOT change the request
             // method if it performs an automatic redirection to that URI.
    PermanentRedirect =
        308, // The target resource has been assigned a new permanent URI and
             // any future references to this resource outght to use one of
             // the enclosed URIs. [...] This status code is similar to 301
             // Moved Permanently (Section 7.3.2 of rfc7231), except that it
             // does not allow rewriting the request method from POST to GET.

    /*####### 4xx - Client Error #######*/
    /* Indicates that the client seems to have erred.
     */
    BadRequest = 400, // Indicates that the server cannot or will not process
                      // the request because the received syntax is invalid,
                      // nonsensical, or exceeds some limitation on what the
                      // server is willing to process.
    Unauthorized = 401, // Indicates that the request has not been applied
                        // because it lacks valid authentication credentials
                        // for the target resource.
    PaymentRequired = 402, // *Reserved*
    Forbidden = 403, // Indicates that the server understood the request but
                     // refuses to authorize it.
    NotFound = 404, // Indicates that the origin server did not find a current
                    // representation for the target resource or is not
                    // willing to disclose that one exists.
    MethodNotAllowed = 405, // Indicates that the method specified in the
                            // request-line is known by the origin server but
                            // not supported by the target resource.
    NotAcceptable =
        406, // Indicates that the target resource does not have a current
             // representation that would be acceptable to the user agent,
             // according to the proactive negotiation header fields received
             // in the request, and the server is unwilling to supply a
             // default representation.
    ProxyAuthenticationRequired =
        407, // Is similar to 401 (Unauthorized), but indicates that the
             // client needs to authenticate itself in order to use a proxy.
    RequestTimeout =
        408, // Indicates that the server did not receive a complete request
             // message within the time that it was prepared to wait.
    Conflict = 409, // Indicates that the request could not be completed due
                    // to a conflict with the current state of the resource.
    Gone = 410, // Indicates that access to the target resource is no longer
                // available at the origin server and that this condition is
                // likely to be permanent.
    LengthRequired = 411, // Indicates that the server refuses to accept the
                          // request without a defined Content-Length.
    PreconditionFailed =
        412, // Indicates that one or more preconditions given in the request
             // header fields evaluated to false when tested on the server.
    PayloadTooLarge = 413, // Indicates that the server is refusing to process
                           // a request because the request payload is larger
                           // than the server is willing or able to process.
    URITooLong = 414, // Indicates that the server is refusing to service the
                      // request because the request-target is longer than the
                      // server is willing to interpret.
    UnsupportedMediaType =
        415, // Indicates that the origin server is refusing to service the
             // request because the payload is in a format not supported by
             // the target resource for this method.
    RangeNotSatisfiable =
        416, // Indicates that none of the ranges in the request's Range
             // header field overlap the current extent of the selected
             // resource or that the set of ranges requested has been rejected
             // due to invalid ranges or an excessive request of small or
             // overlapping ranges.
    ExpectationFailed = 417, // Indicates that the expectation given in the
                             // request's Expect header field could not be met
                             // by at least one of the inbound servers.
    ImATeapot = 418, // Any attempt to brew coffee with a teapot should result
                     // in the error code 418 I'm a teapot.
    UnprocessableEntity =
        422, // Means the server understands the content type of the request
             // entity (hence a 415(Unsupported Media Type) status code is
             // inappropriate), and the syntax of the request entity is
             // correct (thus a 400 (Bad Request) status code is
             // inappropriate) but was unable to process the contained
             // instructions.
    Locked = 423, // Means the source or destination resource of a method is
                  // locked.
    FailedDependency =
        424, // Means that the method could not be performed on the resource
             // because the requested action depended on another action and
             // that action failed.
    UpgradeRequired =
        426, // Indicates that the server refuses to perform the request using
             // the current protocol but might be willing to do so after the
             // client upgrades to a different protocol.
    PreconditionRequired = 428, // Indicates that the origin server requires
                                // the request to be conditional.
    TooManyRequests =
        429, // Indicates that the user has sent too many requests in a given
             // amount of time (\"rate limiting\").
    RequestHeaderFieldsTooLarge =
        431, // Indicates that the server is unwilling to process the request
             // because its header fields are too large.
    UnavailableForLegalReasons =
        451, // This status code indicates that the server is denying access
             // to the resource in response to a legal demand.

    /*####### 5xx - Server Error #######*/
    /* Indicates that the server is aware that it has erred
     * or is incapable of performing the requested method.
     */
    InternalServerError =
        500, // Indicates that the server encountered an unexpected condition
             // that prevented it from fulfilling the request.
    NotImplemented = 501, // Indicates that the server does not support the
                          // functionality required to fulfill the request.
    BadGateway =
        502, // Indicates that the server, while acting as a gateway or proxy,
             // received an invalid response from an inbound server it
             // accessed while attempting to fulfill the request.
    ServiceUnavailable =
        503, // Indicates that the server is currently unable to handle the
             // request due to a temporary overload or scheduled maintenance,
             // which will likely be alleviated after some delay.
    GatewayTimeout =
        504, // Indicates that the server, while acting as a gateway or proxy,
             // did not receive a timely response from an upstream server it
             // needed to access in order to complete the request.
    HTTPVersionNotSupported =
        505, // Indicates that the server does not support, or refuses to
             // support, the protocol version that was used in the request
             // message.
    VariantAlsoNegotiates =
        506, // Indicates that the server has an internal configuration error:
             // the chosen variant resource is configured to engage in
             // transparent content negotiation itself, and is therefore not a
             // proper end point in the negotiation process.
    InsufficientStorage =
        507, // Means the method could not be performed on the resource
             // because the server is unable to store the representation
             // needed to successfully complete the request.
    LoopDetected =
        508, // Indicates that the server terminated an operation because it
             // encountered an infinite loop while processing a request with
             // "Depth: infinity". [RFC 5842]
    NotExtended = 510, // The policy for accessing the resource has not been
                       // met in the request. [RFC 2774]
    NetworkAuthenticationRequired =
        511, // Indicates that the client needs to authenticate to gain
             // network access.
};

/**
 * @brief Converts a Code to its corresponding integer value.
 * @param code The code to be converted.
 * @return The numeric value of @p code.
 * @since 1.2.0
 */
inline int toInt(Code code)
{
    return static_cast<int>(code);
}

inline bool isInformational(int code)
{
    return (code >= 100 && code < 200);
} // @returns @c true if the given \p code is an informational code.
inline bool isSuccessful(int code)
{
    return (code >= 200 && code < 300);
} // @returns @c true if the given \p code is a successful code.
inline bool isRedirection(int code)
{
    return (code >= 300 && code < 400);
} // @returns @c true if the given \p code is a redirectional code.
inline bool isClientError(int code)
{
    return (code >= 400 && code < 500);
} // @returns @c true if the given \p code is a client error code.
inline bool isServerError(int code)
{
    return (code >= 500 && code < 600);
} // @returns @c true if the given \p code is a server error code.
inline bool isError(int code)
{
    return (code >= 400);
} // @returns @c true if the given \p code is any type of error code.

inline bool isInformational(Code code)
{
    return isInformational(static_cast<int>(code));
} // @overload
inline bool isSuccessful(Code code)
{
    return isSuccessful(static_cast<int>(code));
} // @overload
inline bool isRedirection(Code code)
{
    return isRedirection(static_cast<int>(code));
} // @overload
inline bool isClientError(Code code)
{
    return isClientError(static_cast<int>(code));
} // @overload
inline bool isServerError(Code code)
{
    return isServerError(static_cast<int>(code));
} // @overload
inline bool isError(Code code)
{
    return isError(static_cast<int>(code));
} // @overload


static const std::map<Code, std::string> reasonPhraseDict {
    {Code::Continue, "Continue"},
    {Code::SwitchingProtocols, "Switching Protocols"},
    {Code::Processing, "Processing"},
    {Code::EarlyHints, "Early Hints"},

    //####### 2xx - Successful #######
    {Code::OK, "OK"},
    {Code::Created, "Created"},
    {Code::Accepted, "Accepted"},
    {Code::NonAuthoritativeInformation, "Non-Authoritative Information"},
    {Code::NoContent, "No Content"},
    {Code::ResetContent, "Reset Content"},
    {Code::PartialContent, "Partial Content"},
    {Code::MultiStatus, "Multi-Status"},
    {Code::AlreadyReported, "Already Reported"},
    {Code::IMUsed, "IM Used"},

    //####### 3xx - Redirection #######
    {Code::MultipleChoices, "Multiple Choices"},
    {Code::MovedPermanently, "Moved Permanently"},
    {Code::Found, "Found"},
    {Code::SeeOther, "See Other"},
    {Code::NotModified, "Not Modified"},
    {Code::UseProxy, "Use Proxy"},
    {Code::TemporaryRedirect, "Temporary Redirect"},
    {Code::PermanentRedirect, "Permanent Redirect"},

    //####### 4xx - Client Error #######
    {Code::BadRequest, "Bad Request"},
    {Code::Unauthorized, "Unauthorized"},
    {Code::PaymentRequired, "Payment Required"},
    {Code::Forbidden, "Forbidden"},
    {Code::NotFound, "Not Found"},
    {Code::MethodNotAllowed, "Method Not Allowed"},
    {Code::NotAcceptable, "Not Acceptable"},
    {Code::ProxyAuthenticationRequired, "Proxy Authentication Required"},
    {Code::RequestTimeout, "Request Timeout"},
    {Code::Conflict, "Conflict"},
    {Code::Gone, "Gone"},
    {Code::LengthRequired, "Length Required"},
    {Code::PreconditionFailed, "Precondition Failed"},
    {Code::PayloadTooLarge, "Payload Too Large"},
    {Code::URITooLong, "URI Too Long"},
    {Code::UnsupportedMediaType, "Unsupported Media Type"},
    {Code::RangeNotSatisfiable, "Range Not Satisfiable"},
    {Code::ExpectationFailed, "Expectation Failed"},
    {Code::ImATeapot, "I'm a teapot"},
    {Code::UnprocessableEntity, "Unprocessable Entity"},
    {Code::Locked, "Locked"},
    {Code::FailedDependency, "Failed Dependency"},
    {Code::UpgradeRequired, "Upgrade Required"},
    {Code::PreconditionRequired, "Precondition Required"},
    {Code::TooManyRequests, "Too Many Requests"},
    {Code::RequestHeaderFieldsTooLarge, "Request Header Fields Too Large"},
    {Code::UnavailableForLegalReasons, "Unavailable For Legal Reasons"},

    //####### 5xx - Server Error #######
    {Code::InternalServerError, "Internal Server Error"},
    {Code::NotImplemented, "Not Implemented"},
    {Code::BadGateway, "Bad Gateway"},
    {Code::ServiceUnavailable, "Service Unavailable"},
    {Code::GatewayTimeout, "Gateway Time-out"},
    {Code::HTTPVersionNotSupported, "HTTP Version Not Supported"},
    {Code::VariantAlsoNegotiates, "Variant Also Negotiates"},
    {Code::InsufficientStorage, "Insufficient Storage"},
    {Code::LoopDetected, "Loop Detected"},
    {Code::NotExtended, "Not Extended"},
    {Code::NetworkAuthenticationRequired, "Network Authentication Required"},
};

/**
 * @param code An HttpStatus::Code.
 * @return The standard HTTP reason phrase for the given code or an empty
 * std::string() if no standard phrase for the given code is known.
 */
inline const std::string reasonPhrase(Code code)
{
    auto phraseIt = reasonPhraseDict.find(code);
    if (phraseIt == reasonPhraseDict.end())
    {
        return std::string();
    }

    return phraseIt->second;
}

inline const std::string reasonPhrase(uint16_t code)
{
    return reasonPhrase(static_cast<Code>(code));
}

} // namespace statuses

namespace content_types
{
constexpr const char* applicationJson = "application/json; charset=UTF-8";
constexpr const char* textPlain = "text/plain; charset=UTF-8";
}

inline const std::string header(const std::string& name, const std::string& value)
{
    return std::move(name + ": " + value);
}

inline const std::string headerStatus(statuses::Code code)
{
    return std::string(headers::http) + " " +
           std::to_string(static_cast<int>(code)) + " " +
           statuses::reasonPhrase(code);
}

} // namespace http

} // namespace app

#endif //! BMC_HEADERS_HPP
