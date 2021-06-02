// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __SERVICE_AUTHORIZATION_H__
#define __SERVICE_AUTHORIZATION_H__

#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>
#include <logger/logger.hpp>
#include <service/session.hpp>

namespace app
{
namespace service
{
namespace authorization
{

constexpr const char* xXSRFToken = "HTTP_X_XSRF_TOKEN";
constexpr const char* xAuthToken = "HTTP_X_Auth_Token";
constexpr const char* authBasic = "Basic ";
constexpr const char* authToken = "Token ";
constexpr const char* cookieSession = "SESSION";

static session::UserSessionPtr performTokenAuth(std::string_view auth_header)
{
    BMC_LOG_DEBUG << "[AuthMiddleware] Token authentication";

    std::string_view token = auth_header.substr(strlen(authToken));
    auto session =
        session::SessionStore::getInstance().loginSessionByToken(token);
    return session;
}

static session::UserSessionPtr
    performXtokenAuth(const app::core::RequestPtr& request)
{
    BMC_LOG_DEBUG << "[AuthMiddleware] X-Auth-Token authentication";

    try
    {
        std::string_view token =
            request->environment().others.at(xAuthToken);
        if (token.empty())
        {
            return nullptr;
        }
        auto session =
            session::SessionStore::getInstance().loginSessionByToken(token);
        return session;
    }
    catch (std::out_of_range&)
    {
        BMC_LOG_INFO << "The 'X-Auth-Token' header is not present";
    }
    return nullptr;
}

static session::UserSessionPtr
    performCookieAuth(const app::core::RequestPtr& request)
{
    BMC_LOG_DEBUG << "[AuthMiddleware] Cookie authentication";
    auto sessionValueIt = request->environment().cookies.find(cookieSession);
    if (sessionValueIt == request->environment().cookies.end())
    {
        BMC_LOG_INFO << "The 'SESSION' Cookie is not present";
        return nullptr;
    }

    std::shared_ptr<session::UserSession> session =
        session::SessionStore::getInstance().loginSessionByToken(
            sessionValueIt->second);
    if (session == nullptr)
    {
        return nullptr;
    }

#ifndef BMC_INSECURE_DISABLE_CSRF_PREVENTION
    // RFC7231 defines methods that need csrf protection
    if (request->environment().requestMethod !=
        Fastcgipp::Http::RequestMethod::GET)
    {
        try
        {
            for (auto& other: request->environment().others)
            {
                BMC_LOG_DEBUG << "Other header " << other.first << "="
                          << other.second;
            }
            std::string_view csrf =
                request->environment().others.at(xXSRFToken);
            // Make sure both tokens are filled
            if (csrf.empty() || session->csrfToken.empty())
            {
                return nullptr;
            }

            if (csrf.size() != session::sessionTokenSize)
            {
                return nullptr;
            }
            // Reject if csrf token not available
            if (!app::helpers::utils::constantTimeStringCompare(
                    csrf, session->csrfToken))
            {
                return nullptr;
            }
        }
        catch (std::out_of_range&)
        {
            BMC_LOG_ERROR << "The 'X-XSRF-TOKEN' header is not present";
            return nullptr;
        }
    }
#endif
    return session;
}

static bool authenticate(const app::core::RequestPtr& request,
                         app::core::ResponseUni& response)
{
    const session::AuthConfigMethods& authMethodsConfig =
        session::SessionStore::getInstance().getAuthMethodsConfig();

    if (request->isSessionEmpty() && authMethodsConfig.xtoken)
    {
        request->setSession(performXtokenAuth(request));
    }
    if (request->isSessionEmpty() && authMethodsConfig.cookie)
    {
        request->setSession(performCookieAuth(request));
    }
    if (request->isSessionEmpty())
    {
        auto authHeader = request->environment().authorization;
        if (!authHeader.empty())
        {
            // Reject any kind of auth other than basic or token
            if (authHeader.starts_with(authToken) &&
                authMethodsConfig.sessionToken)
            {
                request->setSession(performTokenAuth(authHeader));
            }
            else if (authHeader.starts_with(authBasic) &&
                     authMethodsConfig.basic)
            {
                throw app::core::exceptions::NotImplemented(
                    "Basic athorization not supported by BMC WEBAPP");
            }
        }
    }

    if (!request->isSessionEmpty() && !request->getSession()->isPermitted())
    {
        BMC_LOG_WARNING << "[AuthMiddleware] authorization temporary restricted";
        throw app::core::exceptions::ObmcAppException(
            "authorization temporary restricted");
        return false;
    }

    if (request->isSessionEmpty())
    {
        BMC_LOG_WARNING << "[AuthMiddleware] authorization failed";

        response->setStatus(app::http::statuses::Code::Unauthorized);
        // only send the WWW-authenticate header if this isn't a xhr
        // from the browser.  most scripts,
        if (request->environment().userAgent.empty())
        {
            response->setHeader(app::http::headers::wwwAuthenticate, authBasic);
        }

        return false;
    }

    return !request->isSessionEmpty();
}

} // namespace authorization
} // namespace service
} // namespace app
#endif // __SERVICE_AUTHORIZATION_H__
