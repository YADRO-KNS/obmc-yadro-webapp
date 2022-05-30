// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <phosphor-logging/log.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>

#include <common_fields.hpp>

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
    static constexpr const char* fieldName = "Name";
    static constexpr const char* fieldPresent = "Present";
    static constexpr const char* fieldFunctional = "Functional";
    static constexpr const char* fieldManufacturer = "Manufacturer";
    static constexpr const char* fieldModel = "Model";
    static constexpr const char* fieldMac = "MAC";

    static constexpr const char* fieldEnabled = "Enabled";
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

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                general::assets::assetInterface,
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::manufacturer),
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::model)
            ),
            DBUS_QUERY_EP_IFACES(
                invItemIface,
                DBUS_QUERY_EP_FIELDS_ONLY(propName, fieldName),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPresent)
            ),
            DBUS_QUERY_EP_IFACES(
                invNetAdpIface,
                DBUS_QUERY_EP_FIELDS_ONLY(propMac, fieldMac)
            ),
            DBUS_QUERY_EP_IFACES(
                stateOpStatusIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldFunctional)
            )
        )
      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/system/network/adapter/",
            DBUS_QUERY_CRIT_IFACES(invNetAdpIface),
            nextOneDepth,
            netAdpService
        )

        void setEnabled(const DBusInstancePtr& instance) const
        {
            // default enabled
            bool enabled = true;
            try
            {
                /* clang-format off */
                log<level::DEBUG>(
                    "Adjust 'Enabled' field",
                    entry("ENTITY=%s", 
                        instance->getField(fieldName)->getStringValue().c_str()),
                    entry("Functional_TYPE='%s'",
                        instance->getField(fieldFunctional)->getType().c_str()),
                    entry("Present_TYPE='%s'",
                        instance->getField(fieldPresent)->getType().c_str()));
                /* clang-format on */

                bool functional =
                    instance->getField(fieldFunctional)->getBoolValue();
                bool present = instance->getField(fieldPresent)->getBoolValue();

                enabled = (functional && present);
            }
            catch (const std::exception& e)
            {
                log<level::ERR>(
                    "Can't get Functional or Present field to calcualte",
                    entry("FIELD=%s", fieldEnabled),
                    entry("ERROR=%s", e.what()));
            }
            instance->supplementOrUpdate(fieldEnabled, enabled);
        }

        void supplementByStaticFields(const DBusInstancePtr& instance) const override
        {
            this->setEnabled(instance);
        }
    };
  public:
    NetAdapter() : Collection(), query(std::make_shared<Query>())
    {
        createMember(fieldEnabled);
    }
    ~NetAdapter() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
