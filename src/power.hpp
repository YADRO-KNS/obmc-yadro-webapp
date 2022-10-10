// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <phosphor-logging/log.hpp>
#include <status_provider.hpp>

namespace app
{
namespace obmc
{
namespace entity
{
using namespace app::entity;
using namespace app::query;
using namespace phosphor::logging;

class HostPower final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<HostPower>
{
    static constexpr const char* metaStatus = "__meta__raw_dbus_status";

    static constexpr const char* stateOn = "On";
    static constexpr const char* stateOff = "Off";

    static constexpr const char* statusEnabled = "Enabled";
    static constexpr const char* statusQuiesced = "Quiesced";
    static constexpr const char* statusInTest = "InTest";
    static constexpr const char* statusDisabled = "Disabled";

  public:
    static constexpr const char* fieldState = "PowerState";
    static constexpr const char* fieldStatus = "Status";

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* hostServiceName =
            "xyz.openbmc_project.State.Host";
        static constexpr const char* hostInterfaceName =
            "xyz.openbmc_project.State.Host";
        static constexpr const char* propHostState = "CurrentHostState";

        static constexpr const char* stateRunning =
            "xyz.openbmc_project.State.Host.HostState.Running";
        static constexpr const char* stateQuiesced =
            "xyz.openbmc_project.State.Host.HostState.Quiesced";
        static constexpr const char* stateDiagnosticMode =
            "xyz.openbmc_project.State.Host.HostState.DiagnosticMode";
        static constexpr const char* stateDefaultOff =
            "xyz.openbmc_project.State.Host.HostState.Off";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                hostInterfaceName,
                DBUS_QUERY_EP_FIELDS_ONLY(propHostState, metaStatus),
            )
        )

        void supplementByStaticFields(const DBusInstancePtr& instance) const override
        {
            this->setStatusPower(instance);
        }

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
                "/xyz/openbmc_project/state",
            DBUS_QUERY_CRIT_IFACES(hostInterfaceName),
            nextOneDepth,
            hostServiceName
        )
        /* clang-format on */

        void setStatusPower(const IEntity::InstancePtr& instance) const
        {
            using TStates =
                std::map<std::string, std::pair<std::string, std::string>>;

            static const TStates hostStates = {
                {stateRunning, {stateOn, statusEnabled}},
                {stateQuiesced, {stateOn, statusQuiesced}},
                {stateDiagnosticMode, {stateOn, statusInTest}},
                {stateDefaultOff, {stateOff, statusDisabled}},
            };

            std::pair<std::string, std::string> result;

            auto rawHostStatus =
                instance->getField(metaStatus)->getStringValue();

            instance->supplementOrUpdate(fieldState, std::string(stateOff));

            log<level::DEBUG>("Host power state changed",
                              entry("NEW_STATE=%s", rawHostStatus.c_str()));

            auto findTypeIt = hostStates.find(rawHostStatus);
            if (findTypeIt == hostStates.end())
            {
                auto defaultIt = hostStates.at(stateDefaultOff);
                instance->supplementOrUpdate(fieldState, defaultIt.first);
                instance->supplementOrUpdate(fieldStatus, defaultIt.second);
                return;
            }

            instance->supplementOrUpdate(fieldState, findTypeIt->second.first);
            instance->supplementOrUpdate(fieldStatus,
                                         findTypeIt->second.second);
        }
    };

  public:
    HostPower() : Entity(), query(std::make_shared<Query>())
    {
        this->createMember(fieldState);
        this->createMember(fieldStatus);
    }
    ~HostPower() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
