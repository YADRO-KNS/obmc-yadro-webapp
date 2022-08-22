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

      public:
        Query() : GetObjectDBusQuery(chassisStateSvc, chassisObject)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            chassisIface, DBUS_QUERY_EP_SET_FORMATTERS2(
                              fieldLastStateChangeTime,
                              DBUS_QUERY_EP_CSTR(FormatTimeMsToSec))))
      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                chassisIface,
            };
            return interfaces;
        }
        // clang-format: on
    };
  public:
    ChassisStateProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {}
    ~ChassisStateProvider() override = default;

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

    ENTITY_DECL_FIELD_DEF(std::string, Name, "MainChassis")
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
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            instance->supplementOrUpdate(fieldName, std::string("MainChassis"));
        }
    };

  public:
    Chassis() : Entity(), query(std::make_shared<ChassisQuery>())
    {
        createMember(fieldName);
    }
    ~Chassis() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_PROVIDERS(
      ENTITY_PROVIDER_LINK_DEFAULT(ChassisStateProvider)
    )
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
