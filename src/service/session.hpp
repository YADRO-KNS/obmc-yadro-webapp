// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <memory>

namespace app
{
namespace service
{
namespace session
{

using namespace phosphor::logging;

struct UserSession;

using UserSessionPtr = std::shared_ptr<UserSession>;

// entropy: 20 characters, 62 possibilities.  log2(62^20) = 119 bits of
// entropy.  OWASP recommends at least 64
// https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html#session-id-entropy
constexpr std::size_t sessionTokenSize = 20;

enum class PersistenceType
{
    TIMEOUT, // User session times out after a predetermined amount of time
    SINGLE_REQUEST // User times out once this request is completed.
};

/** Types of user-sessions storage */
enum class configFileStorageType : uint8_t
{
    /* The user-session storing for the long-term time, independent of the OS
       BMC sessions. */
    configLongTermStorage,
    /* The user-session storing only within the current BMC OS Session. */
    configCurrentBmcSession,
    /* The user-session storing only within the current bmcweb-service session.
     */
    configCurrentServiceSession,
};

struct UserSession
{
    std::string uniqueId;
    std::string sessionToken;
    std::string username;
    std::string csrfToken;
    std::string clientId;
    std::string clientIp;
    std::chrono::time_point<std::chrono::steady_clock> lastUpdated;
    PersistenceType persistence;
    bool cookieAuth = false;
    bool isConfigureSelfOnly = false;
    // by default keep sessions in the HEAP
    configFileStorageType storageType =
        configFileStorageType::configCurrentServiceSession;

    // There are two sources of truth for isConfigureSelfOnly:
    //  1. When pamAuthenticateUser() returns PAM_NEW_AUTHTOK_REQD.
    //  2. D-Bus User.Manager.GetUserInfo property UserPasswordExpired.
    // These should be in sync, but the underlying condition can change at any
    // time.  For example, a password can expire or be changed outside of
    // bmcweb.  The value stored here is updated at the start of each
    // operation and used as the truth within bmcweb.

    /**
     * @brief Fills object with data from UserSession's JSON representation
     *
     * This replaces nlohmann's from_json to ensure no-throw approach
     *
     * @param[in] j   JSON object from which data should be loaded
     *
     * @return a shared pointer if data has been loaded properly, nullptr
     * otherwise
     */
    static std::shared_ptr<UserSession> fromJson(const nlohmann::json& j)
    {
        std::shared_ptr<UserSession> userSession =
            std::make_shared<UserSession>();
        for (const auto& element : j.items())
        {
            const std::string* thisValue =
                element.value().get_ptr<const std::string*>();
            if (thisValue == nullptr)
            {
                log<level::ERR>("Error reading persistent store.",
                                entry("MSG=Property was not of type string"),
                                entry("PROPERTY=%s", element.key().c_str()));
                continue;
            }
            if (element.key() == "unique_id")
            {
                userSession->uniqueId = *thisValue;
            }
            else if (element.key() == "session_token")
            {
                userSession->sessionToken = *thisValue;
            }
            else if (element.key() == "csrf_token")
            {
                userSession->csrfToken = *thisValue;
            }
            else if (element.key() == "username")
            {
                userSession->username = *thisValue;
            }
            else if (element.key() == "client_ip")
            {
                userSession->clientIp = *thisValue;
            }

            else
            {
                log<level::ERR>(
                    "Got unexpected property reading persistent file",
                    entry("PROPERTY=%s", element.key().c_str()));
                continue;
            }
        }
        // If any of these fields are missing, we can't restore the session, as
        // we don't have enough information.  These 4 fields have been present
        // in every version of this file in bmcwebs history, so any file, even
        // on upgrade, should have these present
        if (userSession->uniqueId.empty() || userSession->username.empty() ||
            userSession->sessionToken.empty() || userSession->csrfToken.empty())
        {
            log<level::DEBUG>("Session missing required security information, "
                              "refusing to restore");
            return nullptr;
        }

        // For now, sessions that were persisted through a reboot get their idle
        // timer reset.  This could probably be overcome with a better
        // understanding of wall clock time and steady timer time, possibly
        // persisting values with wall clock time instead of steady timer, but
        // the tradeoffs of all the corner cases involved are non-trivial, so
        // this is done temporarily
        userSession->lastUpdated = std::chrono::steady_clock::now();
        userSession->persistence = PersistenceType::TIMEOUT;

        return userSession;
    }
};

struct AuthConfigMethods
{
    bool xtoken = true;
    bool cookie = true;
    bool sessionToken = true;
    bool basic = true;
    bool tls = false;

    void fromJson(const nlohmann::json& j)
    {
        for (const auto& element : j.items())
        {
            const bool* value = element.value().get_ptr<const bool*>();
            if (value == nullptr)
            {
                continue;
            }

            if (element.key() == "XToken")
            {
                xtoken = *value;
            }
            else if (element.key() == "Cookie")
            {
                cookie = *value;
            }
            else if (element.key() == "SessionToken")
            {
                sessionToken = *value;
            }
            else if (element.key() == "BasicAuth")
            {
                basic = *value;
            }
            else if (element.key() == "TLS")
            {
                tls = *value;
            }
        }
    }
};

class SessionStore
{
  public:
    std::shared_ptr<UserSession>
        loginSessionByToken(const std::string_view token)
    {
        applySessionTimeouts();
        if (token.size() != sessionTokenSize)
        {
            log<level::DEBUG>("Invalid token size");
            return nullptr;
        }
        auto sessionIt = authTokens.find(std::string(token));
        if (sessionIt == authTokens.end())
        {
            log<level::DEBUG>("Token not found");
            return nullptr;
        }
        std::shared_ptr<UserSession> userSession = sessionIt->second;
        userSession->lastUpdated = std::chrono::steady_clock::now();
        return userSession;
    }

    std::shared_ptr<UserSession> getSessionByUid(const std::string_view uid)
    {
        applySessionTimeouts();
        // TODO(Ed) this is inefficient
        auto sessionIt = authTokens.begin();
        while (sessionIt != authTokens.end())
        {
            if (sessionIt->second->uniqueId == uid)
            {
                return sessionIt->second;
            }
            sessionIt++;
        }
        return nullptr;
    }

    std::vector<const std::string*> getUniqueIds(
        bool getAll = true,
        const PersistenceType& type = PersistenceType::SINGLE_REQUEST)
    {
        applySessionTimeouts();

        std::vector<const std::string*> ret;
        ret.reserve(authTokens.size());
        for (auto& session : authTokens)
        {
            if (getAll || type == session.second->persistence)
            {
                ret.push_back(&session.second->uniqueId);
            }
        }
        return ret;
    }

    void updateAuthMethodsConfig(const AuthConfigMethods& config)
    {
        bool isTLSchanged = (authMethodsConfig.tls != config.tls);
        authMethodsConfig = config;
        if (isTLSchanged)
        {
            // recreate socket connections with new settings
            // TODO(IK) send SIGHUP signal to the lighttpd service
        }
    }

    void updateSessionTimeout(std::chrono::seconds newTimeoutInSeconds)
    {
        timeoutInSeconds = newTimeoutInSeconds;
    }

    AuthConfigMethods& getAuthMethodsConfig()
    {
        return authMethodsConfig;
    }

    int64_t getTimeoutInSeconds() const
    {
        return std::chrono::seconds(timeoutInSeconds).count();
    }

    static SessionStore& getInstance()
    {
        static SessionStore sessionStore;
        return sessionStore;
    }

    void applySessionTimeouts()
    {
        auto timeNow = std::chrono::steady_clock::now();
        if (timeNow - lastTimeoutUpdate > std::chrono::seconds(1))
        {
            lastTimeoutUpdate = timeNow;
            auto authTokensIt = authTokens.begin();
            while (authTokensIt != authTokens.end())
            {
                if (timeNow - authTokensIt->second->lastUpdated >=
                    timeoutInSeconds)
                {
                    authTokensIt = authTokens.erase(authTokensIt);
                }
                else
                {
                    authTokensIt++;
                }
            }
        }
    }

    SessionStore(const SessionStore&) = delete;
    SessionStore& operator=(const SessionStore&) = delete;

    std::unordered_map<std::string, std::shared_ptr<UserSession>,
                       std::hash<std::string>,
                       app::helpers::utils::ConstantTimeCompare>
        authTokens;

    std::chrono::time_point<std::chrono::steady_clock> lastTimeoutUpdate;
    std::chrono::seconds timeoutInSeconds;
    AuthConfigMethods authMethodsConfig;

  private:
    SessionStore() : timeoutInSeconds(3600)
    {}
};

} // namespace session
} // namespace service
} // namespace app
