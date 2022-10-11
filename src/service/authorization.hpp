// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>
#include <phosphor-logging/log.hpp>
#include <service/pam_authenticate.hpp>
#include <service/session.hpp>

namespace app
{
namespace service
{
namespace authorization
{

using namespace phosphor::logging;
using namespace app::helpers::utils;

constexpr const char* xXSRFToken = "HTTP_X_XSRF_TOKEN";
constexpr const char* xAuthToken = "HTTP_X_AUTH_TOKEN";
constexpr const char* authBasic = "Basic ";
constexpr const char* authToken = "Token ";
constexpr const char* cookieSession = "SESSION";

static session::UserSessionPtr performTokenAuth(std::string_view auth_header)
{
    log<level::DEBUG>("[AuthMiddleware] Token authentication");

    std::string_view token = auth_header.substr(strlen(authToken));
    auto session =
        session::SessionStore::getInstance().loginSessionByToken(token);
    return session;
}

static session::UserSessionPtr
    performXtokenAuth(const app::core::RequestPtr& request)
{
    log<level::DEBUG>("[AuthMiddleware] X-Auth-Token authentication");

    try
    {
        std::string_view token = request->environment().others.at(xAuthToken);
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
        log<level::DEBUG>("The 'X-Auth-Token' header is not present");
    }
    return nullptr;
}

static session::UserSessionPtr
    performCookieAuth(const app::core::RequestPtr& request)
{
    log<level::DEBUG>("[AuthMiddleware] Cookie authentication");
    auto sessionValueIt = request->environment().cookies.find(cookieSession);
    if (sessionValueIt == request->environment().cookies.end())
    {
        log<level::DEBUG>(
            "[AuthMiddleware] The 'SESSION' Cookie is not present");
        return nullptr;
    }

    auto session = session::SessionStore::getInstance().loginSessionByToken(
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
            if (!constantTimeStringCompare(csrf, session->csrfToken))
            {
                return nullptr;
            }
        }
        catch (std::out_of_range&)
        {
            log<level::DEBUG>("The 'X-XSRF-TOKEN' header is not present");
            return nullptr;
        }
    }
#endif
    return session;
}

static session::UserSessionPtr
    performBasicAuth(const app::core::RequestPtr& request)
{
    log<level::DEBUG>("[AuthMiddleware] Basic authentication");

    std::string authData;
    const auto authHeader = request->environment().authorization;
    const auto param = authHeader.substr(strlen(authBasic));
    try
    {
        authData = base64Decode(param);
    }
    catch (...)
    {
        return nullptr;
    }
    std::size_t separator = authData.find(':');
    if (separator == std::string::npos)
    {
        return nullptr;
    }

    std::string user = authData.substr(0, separator);
    separator += 1;
    if (separator > authData.size())
    {
        return nullptr;
    }
    std::string pass = authData.substr(separator);

    log<level::DEBUG>("[AuthMiddleware] Authenticating...",
                      entry("USER=%s", user.c_str()),
                      entry("DEST_IP=%s", request->getClientIp().c_str()));

    int pamrc = pamAuthenticateUser(user, pass);
    bool isConfigureSelfOnly = pamrc == PAM_NEW_AUTHTOK_REQD;
    if ((pamrc != PAM_SUCCESS) && !isConfigureSelfOnly)
    {
        return nullptr;
    }

    // TODO(ed) generateUserSession is a little expensive for basic
    // auth, as it generates some random identifiers that will never be
    // used.  This should have a "fast" path for when user tokens aren't
    // needed.
    // This whole flow needs to be revisited anyway, as we can't be
    // calling directly into pam for every request
    auto session = session::SessionStore::getInstance().newBasicAuthSession(
        user, isConfigureSelfOnly, request->getClientIp());

    return session;
}

// checks if request can be forwarded without authentication
inline bool isOnWhitelist(const app::core::RequestPtr& req)
{
    // it's allowed to GET root node without authentication
    const auto url = req->getUriPath();
    const auto compare = [url](const auto& value) -> bool {
        return value == url;
    };
    bool isOnWhitelist = false;
    if (Fastcgipp::Http::RequestMethod::GET == req->environment().requestMethod)
    {
        static constexpr std::array whiteListGetUris{
            "/redfish/", "/redfish/v1/", "/redfish/v1/odata/"};
        isOnWhitelist = std::any_of(whiteListGetUris.begin(),
                                    whiteListGetUris.end(), compare);
    }

    // it's allowed to POST on session collection & login without
    // authentication
    if (Fastcgipp::Http::RequestMethod::POST ==
        req->environment().requestMethod)
    {
        static constexpr std::array whiteListPostUris{
            "/login/",
            "/redfish/v1/SessionService/Sessions/",
        };
        isOnWhitelist = std::any_of(whiteListPostUris.begin(),
                                    whiteListPostUris.end(), compare);
    }

    return isOnWhitelist;
}

static bool authenticate(const app::core::RequestPtr& request,
                         const app::core::ResponsePtr& response)
{
    if (isOnWhitelist(request))
    {
        return true;
    }

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
                request->setSession(performBasicAuth(request));
            }
        }
    }

    if (request->isSessionEmpty())
    {
        using Code = app::http::statuses::Code;
        using namespace app::http::headers;

        log<level::WARNING>("[AuthMiddleware] authorization failed");

        if (request->isBrowserRequest())
        {
            using namespace app::helpers::utils;

            static constexpr const char* loginPathNext = "/#/login?next=";

            response->setStatus(Code::TemporaryRedirect);
            response->setHeader(location, loginPathNext +
                                              urlEncode(request->getUriPath()));

            return false;
        }

        response->setStatus(Code::Unauthorized);
        // only send the WWW-authenticate header if this isn't a xhr
        // from the browser.  most scripts,
        if (request->environment().userAgent.empty())
        {
            response->setHeader(wwwAuthenticate, authBasic);
        }

        return false;
    }

    if (!request->isSessionEmpty())
    {
        session::ConfigFile::getConfig().commit();
    }
    return !request->isSessionEmpty();
}

} // namespace authorization
} // namespace service
} // namespace app
