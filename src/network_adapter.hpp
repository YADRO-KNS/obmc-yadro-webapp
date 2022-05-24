// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <decorator_asset.hpp>
#include <logger/logger.hpp>

namespace app
{
namespace query
{
namespace obmc
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;

using namespace std::placeholders;
using namespace app::entity::obmc::definitions;

class NetworkAdapter final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* netAdpService = "com.yadro.NetworkAdapter";
    static constexpr const char* invNetAdpIface =
        "xyz.openbmc_project.Inventory.Item.NetworkInterface";
    static constexpr const char* stateOpStatusIface =
        "xyz.openbmc_project.State.Decorator.OperationalStatus";
    static constexpr const char* invItemIface =
        "xyz.openbmc_project.Inventory.Item";

    static constexpr const char* propName = "PrettyName";
    static constexpr const char* propPresent = "Present";
    static constexpr const char* propFunctional = "Functional";
    static constexpr const char* propMac = "MACAddress";

    static constexpr const char* fieldName = "Name";
    static constexpr const char* fieldPresent = "Present";
    static constexpr const char* fieldFunctional = "Functional";
    static constexpr const char* fieldEnabled = "Enabled";
    static constexpr const char* fieldManufacturer = "Manufacturer";
    static constexpr const char* fieldModel = "Model";
    static constexpr const char* fieldMac = "MAC";

  public:
    static const std::vector<std::string> fields;
    static constexpr const char* entityName = "NetworkAdapters";

    NetworkAdapter() : dbus::FindObjectDBusQuery()
    {}
    ~NetworkAdapter() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                general::assets::assetInterface,
                {
                    {general::assets::propertyManufacturer, fieldManufacturer},
                    {general::assets::propertyModel, fieldModel},
                },
            },
            {
                invItemIface,
                {
                    {propName, fieldName},
                    {propPresent, fieldPresent},
                },
            },
            {
                invNetAdpIface,
                {
                    {propMac, fieldMac},
                },
            },
            {
                stateOpStatusIface,
                {
                    {propFunctional, fieldFunctional},
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/inventory/system/network/adapter/",
            {invNetAdpIface},
            nextOneDepth,
            netAdpService,
        };

        return criteria;
    }

    void setEnabled(DBusInstancePtr& instance) const
    {
        // default enabled
        bool enabled = true;
        try
        {
            bool functional =
                instance->getField(fieldFunctional)->getBoolValue();
            bool present = instance->getField(fieldPresent)->getBoolValue();

            enabled = (functional && present);
        }
        catch (const std::exception& e)
        {
            // Some instance might haven't functional or present fields, because
            // separate interfaces might incoming by separate instance-objects.
            // So keep pass throught such cases.
        }
        instance->supplementOrUpdate(fieldEnabled, enabled);
    }

    void supplementByStaticFields(DBusInstancePtr& instance) const override
    {
        this->setEnabled(instance);
    }
};

const std::vector<std::string> NetworkAdapter::fields{
    fieldName, fieldManufacturer, fieldModel,   fieldEnabled,
    fieldMac,  fieldFunctional,   fieldPresent,
};

} // namespace obmc
} // namespace query
} // namespace app
