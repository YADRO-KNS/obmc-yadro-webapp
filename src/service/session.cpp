// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <uuid/uuid.h>

#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <service/session.hpp>
#include <session_manager.hpp>

#include <fstream>
#include <memory>
#include <string_view>

namespace app
{
namespace service
{
namespace session
{

using ConfigFileStorage = std::pair<ConfigFilePath, ConfigFileName>;
using ConfigDict = std::map<configFileStorageType, ConfigFileStorage>;

const UserSessionPtr UserSession::fromJson(const nlohmann::json& j)
{
    const auto userSession = std::make_shared<UserSession>();
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
            log<level::ERR>("Got unexpected property reading persistent file",
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

void AuthConfigMethods::fromJson(const nlohmann::json& j)
{
    std::map<std::string, std::reference_wrapper<bool>> settersMap{
        {"XToken", std::ref(xtoken)},
        {"Cookie", std::ref(cookie)},
        {"SessionToken", std::ref(sessionToken)},
        {"BasicAuth", std::ref(basic)},
        {"TLS", std::ref(tls)},
    };

    for (const auto& element : j.items())
    {
        if (!element.value().is_boolean())
        {
            continue;
        }
        auto setterIt = settersMap.find(element.key());
        if (setterIt == settersMap.end())
        {
            continue;
        }
        setterIt->second.get() = element.value().get<bool>();
    }
}

const UserSessionPtr SessionStore::generateUserSession(
    const std::string_view username, PersistenceType persistence,
    bool isConfigureSelfOnly, const std::string_view clientId,
    const std::string_view clientIp, const configFileStorageType storageType)
{
    // TODO(ed) find a secure way to not generate session identifiers if
    // persistence is set to SINGLE_REQUEST
    static constexpr std::array<char, 62> alphanum = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
        'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
        'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    std::string sessionToken;
    sessionToken.resize(sessionTokenSize, '0');
    std::uniform_int_distribution<size_t> dist(0, alphanum.size() - 1);

    OpenSSLGenerator gen;

    for (char& sessionChar : sessionToken)
    {
        sessionChar = alphanum[dist(gen)];
        if (gen.error())
        {
            return nullptr;
        }
    }
    // Only need csrf tokens for cookie based auth, token doesn't matter
    std::string csrfToken;
    csrfToken.resize(sessionTokenSize, '0');
    for (char& csrfChar : csrfToken)
    {
        csrfChar = alphanum[dist(gen)];
        if (gen.error())
        {
            return nullptr;
        }
    }

    std::string uniqueId;
    uniqueId.resize(10, '0');
    for (char& uidChar : uniqueId)
    {
        uidChar = alphanum[dist(gen)];
        if (gen.error())
        {
            return nullptr;
        }
    }
    auto session = std::make_shared<UserSession>(
        UserSession{uniqueId, sessionToken, std::string(username), csrfToken,
                    std::string(clientId), std::string(clientIp),
                    std::chrono::steady_clock::now(), persistence, false,
                    isConfigureSelfOnly, storageType});
    auto it = storeAuthToken(sessionToken, session);
    return it->second;
}

SessionStore::AuthTokenDict::iterator
    SessionStore::storeAuthToken(const std::string& token,
                                 UserSessionPtr session)
{
    static const std::map<SessionManager::SessionType,
                          std::vector<configFileStorageType>>
        sessTypeFromStorageType{
            {
                SessionManager::SessionType::webui,
                {
                    configFileStorageType::configCurrentBmcSession,
                    configFileStorageType::configCurrentServiceSession,
                },
            },
            {
                SessionManager::SessionType::redfish,
                {
                    configFileStorageType::configLongTermStorage,
                },
            },
        };
    auto statusPair = authTokens.emplace(token, session);
    for (const auto& [sessionType, storeType] : sessTypeFromStorageType)
    {
        if (std::any_of(storeType.begin(), storeType.end(),
                        [session](auto&& type) {
                            return session->storageType == type;
                        }))
        {
            if (statusPair.second)
            {
                // if (SessionManager::create(
                //         session->username, session->clientIp,
                //         sessionType, session->obmcSessionId))
                // {
                //     BMCWEB_LOG_ERROR
                //         << "Fail to create new REDFISH|WEBUI session";
                // }
            }
            break;
        }
    }
    ConfigFile::getConfig().commit();
    return statusPair.first;
}

void SessionStore::removeSession(const UserSessionPtr& session)
{
    // if (!session->closeSession())
    // {
    //     BMCWEB_LOG_ERROR
    //         << "Can't close session that is managed by SessionManager";
    // }
    authTokens.erase(session->sessionToken);
    ConfigFile::getConfig().commit();
}

const UserSessionPtr
    SessionStore::newBasicAuthSession(const std::string_view username,
                                      bool isConfigureSelfOnly,
                                      const std::string_view clientIp)

{
    return this->generateUserSession(
        username, PersistenceType::SINGLE_REQUEST, isConfigureSelfOnly,
        clientIp, clientIp, configFileStorageType::configLongTermStorage);
}

const UserSessionPtr
    SessionStore::newStandardLoginSession(const std::string_view username,
                                          bool isConfigureSelfOnly,
                                          const std::string_view clientIp)

{
    return this->generateUserSession(
        username, PersistenceType::TIMEOUT, isConfigureSelfOnly, clientIp,
        clientIp, configFileStorageType::configCurrentBmcSession);
}

const UserSessionPtr
    SessionStore::loginSessionByToken(const std::string_view token)
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
    UserSessionPtr userSession = sessionIt->second;
    userSession->lastUpdated = std::chrono::steady_clock::now();
    return userSession;
}

const UserSessionPtr SessionStore::getSessionByUid(const std::string_view uid)
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

std::vector<const std::string*>
    SessionStore::getUniqueIds(bool getAll, const PersistenceType& type)
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

void SessionStore::updateAuthMethodsConfig(const AuthConfigMethods& config)
{
    bool isTLSchanged = (authMethodsConfig.tls != config.tls);
    authMethodsConfig = config;
    if (isTLSchanged)
    {
        // recreate socket connections with new settings
        // TODO(IK) send SIGHUP signal to the lighttpd service
        // The TLS now not supported, so skip the action.
    }
}

void SessionStore::updateSessionTimeout(
    std::chrono::seconds newTimeoutInSeconds)
{
    timeoutInSeconds = newTimeoutInSeconds;
}

AuthConfigMethods& SessionStore::getAuthMethodsConfig()
{
    return authMethodsConfig;
}

int64_t SessionStore::getTimeoutInSeconds() const
{
    return std::chrono::seconds(timeoutInSeconds).count();
}

SessionStore& SessionStore::getInstance()
{
    static SessionStore sessionStore;
    return sessionStore;
}

const SessionStore::AuthTokenDict& SessionStore::getAuthTokenDict() const
{
    return authTokens;
}

void SessionStore::restore(const UserSessionPtr& session)
{
    log<level::DEBUG>("Restored session",
                      entry("SESSION_ID=%s", session->uniqueId.c_str()));
    authTokens.emplace(session->sessionToken, session);
}

void SessionStore::applySessionTimeouts()
{
    auto timeNow = std::chrono::steady_clock::now();
    if (timeNow - lastTimeoutUpdate > std::chrono::seconds(1))
    {
        lastTimeoutUpdate = timeNow;
        auto authTokensIt = authTokens.begin();
        while (authTokensIt != authTokens.end())
        {
            if (timeNow - authTokensIt->second->lastUpdated >= timeoutInSeconds)
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
/**
 * Configuration file storage metadata depending on storage type.
 * @note The file sorage names origins from bmcweb implementation to save logic
 *       til the full transition from bmcweb to yaweb completed.
 */
static const ConfigDict configFilePathDict{
    /* Long-term configurations storing in the root home dir */
    /* todo(ed) should read this from a fixed location somewhere, not CWD */
    {
        configFileStorageType::configLongTermStorage,
        {"/home/root", "bmcweb_persistent_data.json"},
    },
    /* The configuration file for the current BMC OS Session will be stored
       in the tempfs  */
    {
        configFileStorageType::configCurrentBmcSession,
        {"/tmp/bmcweb_metadata", "sessions.json"},
    },
    /* The configuration for the current bmcweb service session will be
       stored in a 'heap' */
};

ConfigFile::ConfigFile()
{
    readData();
}

ConfigFile::~ConfigFile()
{
    // Make sure we aren't writing stale sessions
    SessionStore::getInstance().applySessionTimeouts();
}

void ConfigFile::readData()
{
    for (auto& [storageType, sotrageMetaData] : configFilePathDict)
    {
        readData(storageType, sotrageMetaData.first, sotrageMetaData.second);
    }
}

void ConfigFile::commit()
{
    for (auto& [storageType, sotrageMetaData] : configFilePathDict)
    {
        writeData(storageType, sotrageMetaData.first, sotrageMetaData.second);
    }
}

const std::string& ConfigFile::getSystemUUID()
{
    return systemUuid;
}

void ConfigFile::readData(const session::configFileStorageType storageType,
                          const ConfigFilePath& configPath,
                          const ConfigFileName& configFileName)
{
    std::ifstream persistentFile(configPath + "/" + configFileName);

    if (persistentFile.is_open())
    {
        // call with exceptions disabled
        auto data = nlohmann::json::parse(persistentFile, nullptr, false);
        if (data.is_discarded())
        {
            log<level::ERR>("Error parsing persistent data in json file");
        }
        else
        {
            for (const auto& item : data.items())
            {
                if (item.key() == keySystemUUID)
                {
                    const std::string* jSystemUuid =
                        item.value().get_ptr<const std::string*>();
                    if (jSystemUuid != nullptr)
                    {
                        systemUuid = *jSystemUuid;
                    }
                }
                else if (item.key() == keyAuthConfig)
                {
                    session::SessionStore::getInstance()
                        .getAuthMethodsConfig()
                        .fromJson(item.value());
                }
                else if (item.key() == keySessions)
                {
                    for (const auto& elem : item.value())
                    {
                        const auto session =
                            session::UserSession::fromJson(elem);

                        if (session == nullptr)
                        {
                            log<level::ERR>("Problem reading session "
                                            "from persistent store");
                            continue;
                        }
                        session->storageType = storageType;
                        SessionStore::getInstance().restore(session);
                    }
                }
                else if (item.key() == keyTimeout)
                {
                    const int64_t* jTimeout =
                        item.value().get_ptr<const int64_t*>();
                    if (jTimeout == nullptr)
                    {
                        log<level::DEBUG>(
                            "Problem reading session timeout value");
                        continue;
                    }
                    std::chrono::seconds sessionTimeoutInseconds(*jTimeout);
                    log<level::DEBUG>(
                        "Session timeout successfully restored",
                        entry("TIMEOUT=%ld",
                              static_cast<long int>(
                                  sessionTimeoutInseconds.count())));
                    session::SessionStore::getInstance().updateSessionTimeout(
                        sessionTimeoutInseconds);
                }
                else
                {
                    // Do nothing in the case of extra fields.  We may have
                    // cases where fields are added in the future, and we
                    // want to at least attempt to gracefully support
                    // downgrades in that case, even if we don't officially
                    // support it
                }
            }
        }
    }
    if (systemUuid.empty())
    {
        static constexpr auto uuidCharLen = 36U;
        uuid_t uuidBuffer;
        uuid_generate_time_safe(uuidBuffer);

        systemUuid.resize(
            uuidCharLen); // ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0"
        uuid_unparse_upper(uuidBuffer, systemUuid.data());
        commit();
    }
}

void ConfigFile::writeData(const configFileStorageType storageType,
                           const ConfigFilePath& configPath,
                           const ConfigFileName& configFileName)
{
    createConfigPath(configPath);
    const auto& persistenConfigFilename = configPath + "/" + configFileName;
    std::ofstream persistentFile(persistenConfigFilename);

    std::filesystem::permissions(persistenConfigFilename, configPermission);
    nlohmann::json data;
    const auto& handlersDict = configPopulatHandlersDict.at(storageType);
    for (auto& handler : handlersDict)
    {
        handler(std::ref(data));
    }

    persistentFile << data;
}

inline void ConfigFile::createConfigPath(const ConfigFilePath& configPath)
{
    if (std::filesystem::exists(configPath))
    {
        log<level::DEBUG>("The configuration file path already exists.");
        return;
    }

    std::filesystem::create_directories(configPath);
    std::filesystem::permissions(configPath, configPermission);
}

void ConfigFile::populateRevision(nlohmann::json& config)
{
    config[keyRevision] = jsonRevision;
}

void ConfigFile::populateSystemUUID(nlohmann::json& config)
{
    config[keySystemUUID] = systemUuid;
}

void ConfigFile::populateAuthConfig(nlohmann::json& config)
{
    const auto& authMethods =
        SessionStore::getInstance().getAuthMethodsConfig();
    nlohmann::json& authConfig = config[keyAuthConfig];
    authConfig =
        nlohmann::json::object({{"XToken", authMethods.xtoken},
                                {"Cookie", authMethods.cookie},
                                {"SessionToken", authMethods.sessionToken},
                                {"BasicAuth", authMethods.basic},
                                {"TLS", authMethods.tls}});
}

void ConfigFile::populateTimeout(nlohmann::json& config)
{
    config[keyTimeout] = SessionStore::getInstance().getTimeoutInSeconds();
}

void ConfigFile::populateSessions(nlohmann::json& config,
                                  const configFileStorageType storageType)
{
    nlohmann::json& sessions = config[keySessions];
    sessions = nlohmann::json::array();
    for (const auto& p : SessionStore::getInstance().getAuthTokenDict())
    {
        if (p.second->persistence != PersistenceType::SINGLE_REQUEST &&
            p.second->storageType == storageType)
        {
            sessions.push_back({
                {"unique_id", p.second->uniqueId},
                {"session_token", p.second->sessionToken},
                {"username", p.second->username},
                {"csrf_token", p.second->csrfToken},
                {"client_ip", p.second->clientIp},
            });
        }
    }
}

ConfigFile& ConfigFile::getConfig()
{
    static ConfigFile f;
    return f;
}

} // namespace session
} // namespace service
} // namespace app
