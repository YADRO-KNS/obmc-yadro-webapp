// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <logger/logger.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>

#include <service/session.hpp>
#include <service/configuration.hpp>

#include <map>
#include <string>
#include <functional>
#include <filesystem>
#include <fstream>

namespace app
{
namespace service
{
namespace config
{

using ConfigFilePath = std::string;
using ConfigFileName = std::string;

/** Configuration file storage metadata depending on storage type. */
static const std::map<session::configFileStorageType,
                      std::pair<ConfigFilePath, ConfigFileName>>
    configFilePathDict{
        {
            session::configFileStorageType::configCurrentBmcSession,
            {"/tmp/bmcweb_metadata", "sessions.json"},
        },
    };

class ConfigFile;

class ConfigFile
{
    using TPopulate = std::function<void(nlohmann::json&)>;

    static constexpr const char* keyRevision = "revision";
    static constexpr const char* keyAuthConfig = "auth_config";
    static constexpr const char* keySystemUUID = "system_uuid";
    static constexpr const char* keyTimeout = "timeout";
    static constexpr const char* keySessions = "sessions";

  public:
    ConfigFile()
    {
        readData();
    }

    ~ConfigFile()
    {
        // Make sure we aren't writing stale sessions
        session::SessionStore::getInstance().applySessionTimeouts();
    }

    void readData()
    {
        for (auto& [storageType, sotrageMetaData] : configFilePathDict)
        {
            readData(storageType, sotrageMetaData.first,
                     sotrageMetaData.second);
        }
    }

    const std::string& getSystemUUID()
    {
        return systemUuid;
    }

  private:
    void readData(const session::configFileStorageType storageType,
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
                BMC_LOG_ERROR
                    << "Error parsing persistent data in json file.";
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
                            std::shared_ptr<session::UserSession> newSession =
                                session::UserSession::fromJson(elem);

                            if (newSession == nullptr)
                            {
                                BMC_LOG_ERROR << "Problem reading session "
                                                    "from persistent store";
                                continue;
                            }
                            newSession->storageType = storageType;
                            BMC_LOG_DEBUG
                                << "Restored session: " << newSession->csrfToken
                                << " " << newSession->uniqueId << " "
                                << newSession->sessionToken;
                            session::SessionStore::getInstance().authTokens.emplace(
                                newSession->sessionToken, newSession);
                        }
                    }
                    else if (item.key() == keyTimeout)
                    {
                        const int64_t* jTimeout =
                            item.value().get_ptr<int64_t*>();
                        if (jTimeout == nullptr)
                        {
                            BMC_LOG_DEBUG
                                << "Problem reading session timeout value";
                            continue;
                        }
                        std::chrono::seconds sessionTimeoutInseconds(*jTimeout);
                        BMC_LOG_DEBUG << "Restored Session Timeout: "
                                         << sessionTimeoutInseconds.count();
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
    }

    std::string systemUuid{""};
    uint64_t jsonRevision = 1;
};

inline ConfigFile& getConfig()
{
    static ConfigFile f;
    return f;
}

} // namespace config
} // namespace service
} // namespace app
