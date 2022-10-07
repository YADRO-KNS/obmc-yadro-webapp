// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <formatters.hpp>
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

class NetAdapter final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<NetAdapter>
{
  public:
    enum class State
    {
        enabled,
        absent
    };

    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD(std::string, Manufacturer)
    ENTITY_DECL_FIELD(std::string, Model)
    ENTITY_DECL_FIELD(std::string, MAC)
    ENTITY_DECL_FIELD_ENUM(State, State, absent)
  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* netAdpService = "com.yadro.NetworkAdapter";
        static constexpr const char* invNetAdpIface =
            "xyz.openbmc_project.Inventory.Item.NetworkInterface";
        static constexpr const char* stateOpStatusIface =
            "xyz.openbmc_project.State.Decorator.OperationalStatus";
        static constexpr const char* invItemIface =
            "xyz.openbmc_project.Inventory.Item";

        static constexpr const char* propName = "PrettyName";
        static constexpr const char* propMac = "MACAddress";

        class FormatState : public query::dbus::IFormatter
        {
          public:
            ~FormatState() override = default;

            const DbusVariantType format(const PropertyName&,
                                         const DbusVariantType& value) override
            {
                State state = State::absent;
                if (std::holds_alternative<bool>(value) &&
                    std::get<bool>(value))
                {
                    state = State::enabled;
                }
                return static_cast<int>(state);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(general::assets::assetInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldManufacturer),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldModel)),
            DBUS_QUERY_EP_IFACES(
                invItemIface, DBUS_QUERY_EP_FIELDS_ONLY(propName, fieldName),
                DBUS_QUERY_EP_SET_FORMATTERS("Present", fieldState,
                                             DBUS_QUERY_EP_CSTR(FormatState))),
            DBUS_QUERY_EP_IFACES(invNetAdpIface,
                                 DBUS_QUERY_EP_FIELDS_ONLY(propMac, fieldMAC)),
            DBUS_QUERY_EP_IFACES(
                stateOpStatusIface,
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "Functional", StatusProvider::fieldStatus,
                    DBUS_QUERY_EP_CSTR(StatusProvider::StatusByFunctional))))
      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/system/network/adapter/",
            DBUS_QUERY_CRIT_IFACES(invNetAdpIface), nextOneDepth, netAdpService)

        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            static const DefaultFieldsValueDict defaults{
                StatusRollup::defaultGetter<NetAdapter>(),
                StatusFromRollup::defaultGetter<NetAdapter>(),
            };
            return defaults;
        }
    };

  public:
    NetAdapter() : Collection(), query(std::make_shared<Query>())
    {}
    ~NetAdapter() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
