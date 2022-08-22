// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <phosphor-logging/log.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
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
using namespace phosphor::logging;

class Fan final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<Fan>
{
  public:
    enum class State
    {
        enabled
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD(std::string, PrettyName)
    ENTITY_DECL_FIELD(std::string, Type)
    ENTITY_DECL_FIELD(std::string, Connector)

    ENTITY_DECL_FIELD(std::string, Manufacturer)
    ENTITY_DECL_FIELD(std::string, Model)
    ENTITY_DECL_FIELD(std::string, PartNumber)
    ENTITY_DECL_FIELD(std::string, SerialNumber)
    
    ENTITY_DECL_FIELD_ENUM(State, State, enabled)

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* fanInventoryIface =
            "xyz.openbmc_project.Inventory.Item.Fan";
        static constexpr const char* inventoryItemIface =
            "xyz.openbmc_project.Inventory.Item";
        static constexpr const char* assetsIface =
            "xyz.openbmc_project.Inventory.Decorator.Asset";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                fanInventoryIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldName),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldType),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldConnector),
            ),
            DBUS_QUERY_EP_IFACES(
                inventoryItemIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPrettyName),
            ),
            DBUS_QUERY_EP_IFACES(
                assetsIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldManufacturer),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldModel),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPartNumber),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSerialNumber),
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
                "/xyz/openbmc_project/inventory/system/fan/",
            DBUS_QUERY_CRIT_IFACES(fanInventoryIface),
            nextOneDepth,
            std::nullopt
        )
        /* clang-format on */

        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            static const DefaultFieldsValueDict defaults{
                StatusRollup::defaultGetter<Fan>(),
                StatusFromRollup::defaultGetter<Fan>(),
            };
            return defaults;
        }

        void supplementByStaticFields(const DBusInstancePtr& instance) const override
        {
            // We haven't presence GPIO to determine whether Fan present or not
            setFieldState(instance, State::enabled);
            setFieldId(instance, getFieldName(instance));
        }
    };
  public:
    Fan() : Collection(), query(std::make_shared<Query>())
    {
      this->createMember(fieldId);
      this->createMember(fieldState);
    }
    ~Fan() override = default;

    static const ConditionPtr readingRPM()
    {
        return Sensors::sensorsByType(Sensors::Unit::rotational);
    }

    static const ConditionPtr readingPWM()
    {
        return Sensors::sensorsByType(Sensors::Unit::percent);
    }

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATIONS(ENTITY_DEF_RELATION(Sensors,
                                              Sensors::inventoryRelation()))
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
