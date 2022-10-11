// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <session_manager.hpp>

#include <fstream>
#include <memory>

namespace app
{
namespace service
{
namespace session
{

using namespace phosphor::logging;
using namespace obmc::entity;

struct UserSession;
using UserSessionPtr = std::shared_ptr<UserSession>;

// entropy: 20 characters, 62 possibilities.  log2(62^20) = 119 bits of
// entropy.  OWASP recommends at least 64
// https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html#session-id-entropy
static constexpr std::size_t sessionTokenSize = 20;

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
    std::string obmcSessionId = "";

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
    static const UserSessionPtr fromJson(const nlohmann::json& j);
};

struct AuthConfigMethods
{
    bool xtoken = true;
    bool cookie = true;
    bool sessionToken = true;
    bool basic = true;
    bool tls = false;

    void fromJson(const nlohmann::json& j);
};

class SessionStore
{
    const UserSessionPtr generateUserSession(
        const std::string_view username,
        PersistenceType persistence = PersistenceType::TIMEOUT,
        bool isConfigureSelfOnly = false, const std::string_view clientId = "",
        const std::string_view clientIp = "",
        const configFileStorageType storageType =
            configFileStorageType::configLongTermStorage);

  public:
    using AuthTokenDict =
        std::unordered_map<std::string, UserSessionPtr, std::hash<std::string>,
                           app::helpers::utils::ConstantTimeCompare>;

    const UserSessionPtr newBasicAuthSession(const std::string_view username,
                                             bool isConfigureSelfOnly,
                                             const std::string_view clientIp);
    const UserSessionPtr
        newStandardLoginSession(const std::string_view username,
                                bool isConfigureSelfOnly,
                                const std::string_view clientIp);
    const UserSessionPtr loginSessionByToken(const std::string_view token);
    const UserSessionPtr getSessionByUid(const std::string_view uid);
    std::vector<const std::string*> getUniqueIds(
        bool getAll = true,
        const PersistenceType& type = PersistenceType::SINGLE_REQUEST);
    void updateAuthMethodsConfig(const AuthConfigMethods& config);
    void removeSession(const UserSessionPtr& session);
    void updateSessionTimeout(std::chrono::seconds newTimeoutInSeconds);
    AuthConfigMethods& getAuthMethodsConfig();
    int64_t getTimeoutInSeconds() const;
    static SessionStore& getInstance();
    void applySessionTimeouts();
    const AuthTokenDict& getAuthTokenDict() const;
    void restore(const UserSessionPtr& session);

    SessionStore(const SessionStore&) = delete;
    SessionStore& operator=(const SessionStore&) = delete;

    std::chrono::time_point<std::chrono::steady_clock> lastTimeoutUpdate;
    std::chrono::seconds timeoutInSeconds;
    AuthConfigMethods authMethodsConfig;

    AuthTokenDict::iterator storeAuthToken(const std::string& token,
                                           const UserSessionPtr session);

  private:
    SessionStore() : timeoutInSeconds(3600)
    {}
    AuthTokenDict authTokens;
};

using ConfigFilePath = std::string;
using ConfigFileName = std::string;

class ConfigFile
{
    using TPopulate = std::function<void(nlohmann::json&)>;

    static constexpr const char* keyRevision = "revision";
    static constexpr const char* keyAuthConfig = "auth_config";
    static constexpr const char* keySystemUUID = "system_uuid";
    static constexpr const char* keyTimeout = "timeout";
    static constexpr const char* keySessions = "sessions";

  public:
    ConfigFile();
    ~ConfigFile();
    void readData();
    void commit();
    const std::string& getSystemUUID();

    static ConfigFile& getConfig();

  private:
    void readData(const session::configFileStorageType storageType,
                  const ConfigFilePath& configPath,
                  const ConfigFileName& configFileName);
    void writeData(const configFileStorageType storageType,
                   const ConfigFilePath& configPath,
                   const ConfigFileName& configFileName);
    inline void createConfigPath(const ConfigFilePath& configPath);
    void populateRevision(nlohmann::json& config);
    void populateSystemUUID(nlohmann::json& config);
    void populateAuthConfig(nlohmann::json& config);
    void populateTimeout(nlohmann::json& config);
    void populateSessions(nlohmann::json& config,
                          const configFileStorageType storageType);
    /** Configuration fields definition depending on storage type. */
    const std::map<configFileStorageType, std::vector<TPopulate>>
        configPopulatHandlersDict{
            {
                configFileStorageType::configLongTermStorage,
                {
                    std::bind(&ConfigFile::populateRevision, this,
                              std::placeholders::_1),
                    std::bind(&ConfigFile::populateAuthConfig, this,
                              std::placeholders::_1),
                    std::bind(&ConfigFile::populateSystemUUID, this,
                              std::placeholders::_1),
                    std::bind(&ConfigFile::populateTimeout, this,
                              std::placeholders::_1),
                    std::bind(&ConfigFile::populateSessions, this,
                              std::placeholders::_1,
                              configFileStorageType::configLongTermStorage),
                },
            },
            {
                configFileStorageType::configCurrentBmcSession,
                {
                    std::bind(&ConfigFile::populateRevision, this,
                              std::placeholders::_1),
                    std::bind(&ConfigFile::populateSessions, this,
                              std::placeholders::_1,
                              configFileStorageType::configCurrentBmcSession),
                },
            },
        };

    std::string systemUuid{""};
    uint64_t jsonRevision = 1;
    // set the permission of the file to 640
    std::filesystem::perms configPermission =
        std::filesystem::perms::owner_read |
        std::filesystem::perms::owner_write |
        std::filesystem::perms::group_read;
};

} // namespace session
} // namespace service
} // namespace app
