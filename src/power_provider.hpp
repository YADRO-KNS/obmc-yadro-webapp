// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __POWER_PROVIDER_H__
#define __POWER_PROVIDER_H__

#include <logger/logger.hpp>
#include <core/exceptions.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>

#include <definitions.hpp>

namespace app
{
namespace query
{
namespace obmc
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;

using namespace app::entity::obmc::definitions;

class HostPower final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* hostServiceName =
        "xyz.openbmc_project.State.Host";
    static constexpr const char* hostInterfaceName =
        "xyz.openbmc_project.State.Host";
    static constexpr const char* namePropertyHostState = "CurrentHostState";

    static constexpr const char* stateRunning =
        "xyz.openbmc_project.State.Host.HostState.Running";
    static constexpr const char* stateQuiesced =
        "xyz.openbmc_project.State.Host.HostState.Quiesced";
    static constexpr const char* stateDiagnosticMode =
        "xyz.openbmc_project.State.Host.HostState.DiagnosticMode";
    static constexpr const char* stateDefaultOff =
        "xyz.openbmc_project.State.Host.HostState.Off";

    static constexpr const char* stateOn = "On";
    static constexpr const char* stateOff = "Off";

    static constexpr const char* statusEnabled = "Enabled";
    static constexpr const char* statusQuiesced = "Quiesced";
    static constexpr const char* statusInTest = "InTest";
    static constexpr const char* statusDisabled = "Disabled";

  public:
    void supplementByStaticFields(DBusInstancePtr& instance) const override
    {
        this->setStatusPower(instance);
    }

    void setStatusPower(const IEntity::InstancePtr& instance) const
    {
        std::pair<std::string, std::string> result;

        auto rawHostStatus =
            instance->getField(power::metaStatus)->getStringValue();

        instance->supplementOrUpdate(power::fieldState, std::string(stateOff));

        BMC_LOG_DEBUG << "SET POWER: " << rawHostStatus;

        auto findTypeIt = hostStateDict.find(rawHostStatus);
        if (findTypeIt == hostStateDict.end())
        {
            auto defaultIt = hostStateDict.at(stateDefaultOff);
            instance->supplementOrUpdate(power::fieldState, defaultIt.first);
            instance->supplementOrUpdate(power::fieldStatus, defaultIt.second);
            return;
        }

        instance->supplementOrUpdate(power::fieldState,
                                     findTypeIt->second.first);
        instance->supplementOrUpdate(power::fieldStatus,
                                     findTypeIt->second.second);
    }

    HostPower() : dbus::FindObjectDBusQuery()
    {}
    ~HostPower() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                hostInterfaceName,
                {
                    {
                        namePropertyHostState,
                        power::metaStatus,
                    }
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/state",
            {
                hostInterfaceName,
            },
            nextOneDepth,
            hostServiceName,
        };

        return criteria;
    }

    static const std::map<std::string, std::pair<std::string, std::string>> hostStateDict;
};

const std::map<std::string, std::pair<std::string, std::string>>
    HostPower::hostStateDict = {
        {
            stateRunning,
            {
                stateOn,
                statusEnabled,
            },
        },
        {
            stateQuiesced,
            {
                stateOn,
                statusQuiesced,
            },
        },
        {
            stateDiagnosticMode,
            {
                stateOn,
                statusInTest,
            },
        },
        {
            stateDefaultOff,
            {
                stateOff,
                statusDisabled,
            },
        },
};
} // namespace obmc
} // namespace query
} // namespace app

#endif // __POWER_PROVIDER_H__
