// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <formatters.hpp>
#include <pcie.hpp>
#include <phosphor-logging/log.hpp>
#include <status_provider.hpp>
#include <sensors.hpp>

namespace app
{
namespace obmc
{
namespace entity
{
using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;
using namespace std::placeholders;
using namespace general::assets;

class ChassisStateProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<ChassisStateProvider>,
    public CachedSource,
    public NamedEntity<ChassisStateProvider>
{
  public:
    enum class State
    {
        enabled,
        standbyOffline
    };

    enum class Power
    {
        on,
        off
    };

    ENTITY_DECL_FIELD_ENUM(State, State, standbyOffline)
    ENTITY_DECL_FIELD_ENUM(Power, PowerState, off)
    ENTITY_DECL_FIELD(std::string, LastStateChangeTime)

  private:
    class Query : public dbus::GetObjectDBusQuery
    {
        static constexpr const char* chassisStateSvc =
            "xyz.openbmc_project.State.Chassis";
        static constexpr const char* chassisObject =
            "/xyz/openbmc_project/state/chassis0";
        static constexpr const char* chassisIface =
            "xyz.openbmc_project.State.Chassis";

        class FormatPower : public query::dbus::IFormatter
        {
            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.State.Chassis.PowerState." + val;
            }

          public:
            ~FormatPower() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, Power> states{
                    {dbusEnum("On"), Power::on},
                };

                return formatValueFromDict(states, property, value, Power::off);
            }
        };

      public:
        Query() : GetObjectDBusQuery(chassisStateSvc, chassisObject)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            chassisIface,
            DBUS_QUERY_EP_SET_FORMATTERS2(
                fieldLastStateChangeTime,
                DBUS_QUERY_EP_CSTR(FormatTimeMsToSec)),
            DBUS_QUERY_EP_SET_FORMATTERS("CurrentPowerState", fieldPowerState,
                                         DBUS_QUERY_EP_CSTR(FormatPower))))
      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                chassisIface,
            };
            return interfaces;
        }
        // clang-format: on
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            if (getFieldPowerState(instance) == Power::on)
            {
                setFieldState(instance, State::enabled);
                return;
            }
            setFieldState(instance, State::standbyOffline);
        }
    };

  public:
    ChassisStateProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {
        this->createMember(fieldState);
    }
    ~ChassisStateProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class IntrusionSensor final :
    public Entity,
    public CachedSource,
    public NamedEntity<IntrusionSensor>
{
  public:
    enum class Status
    {
        normal,
        hardwareIntrusion,
        tamperingDetected
    };
    ENTITY_DECL_FIELD_ENUM(Status, Status, normal)

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* iface =
            "xyz.openbmc_project.Chassis.Intrusion";
        class FormatStatus : public query::dbus::IFormatter
        {
          public:
            ~FormatStatus() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, Status> statusList{
                    {"Normal", Status::normal},
                    {"HardwareIntrusion", Status::hardwareIntrusion},
                    {"TamperingDetected", Status::tamperingDetected},
                };

                return formatValueFromDict(statusList, property, value, Status::normal);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
            iface,
            DBUS_QUERY_EP_SET_FORMATTERS2(fieldStatus,
                                         DBUS_QUERY_EP_CSTR(FormatStatus))
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/Intrusion",
            DBUS_QUERY_CRIT_IFACES(iface),
            nextOneDepth,
            std::nullopt
        )
    };

  public:
    IntrusionSensor() :
        Entity(), query(std::make_shared<Query>())
    {}
    ~IntrusionSensor() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class Chassis final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<Chassis>
{
  public:
    enum class ChassisType
    {
        RackMount,
        Other
    };

    ENTITY_DECL_FIELD_ENUM(ChassisType, Type, Other)
    ENTITY_DECL_FIELD(std::string, PartNumber)
    ENTITY_DECL_FIELD(std::string, Manufacturer)

  private:
    class ChassisQuery final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* yadroInterface = "com.yadro.Platform";

      private:
        class FormatType : public query::dbus::IFormatter
        {
          public:
            ~FormatType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, ChassisType> types{
                    {"23", ChassisType::RackMount},
                };

                return formatValueFromDict(types, property, value,
                                           ChassisType::Other);
            }
        };

      public:
        ChassisQuery() : dbus::FindObjectDBusQuery()
        {}
        ~ChassisQuery() = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                yadroInterface,
                DBUS_QUERY_EP_SET_FORMATTERS("ChassisType", fieldType,
                                             DBUS_QUERY_EP_CSTR(FormatType)),
                DBUS_QUERY_EP_FIELDS_ONLY("ChassisPartNumber",
                                          fieldPartNumber)),
            DBUS_QUERY_EP_IFACES(general::assets::assetInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldManufacturer)))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/system/board/",
            DBUS_QUERY_CRIT_IFACES(yadroInterface), nextOneDepth, std::nullopt)

        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            static const DefaultFieldsValueDict defaults{
                StatusRollup::defaultGetter<Chassis>(),
                StatusFromRollup::defaultGetter<Chassis>(),
            };
            return defaults;
        }
    };

  public:
    Chassis() : Entity(), query(std::make_shared<ChassisQuery>())
    {
    }
    ~Chassis() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_PROVIDERS(ENTITY_PROVIDER_LINK_DEFAULT(ChassisStateProvider))
    ENTITY_DECL_RELATIONS(ENTITY_DEF_RELATION_DIRECT(Sensors))
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
