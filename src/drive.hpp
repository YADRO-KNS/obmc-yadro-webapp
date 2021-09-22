// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <system_queries.hpp>

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

class Drive final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* invDriveIface =
        "xyz.openbmc_project.Inventory.Item.Drive";
    static constexpr const char* driveStateIface =
        "xyz.openbmc_project.State.Drive";
    static constexpr const char* invItemIface =
        "xyz.openbmc_project.Inventory.Item";

    static constexpr const char* propName = "PrettyName";
    static constexpr const char* propPresent = "Present";
    static constexpr const char* propCapacity = "Capacity";
    static constexpr const char* propInterfaces = "Interfaces";
    static constexpr const char* propType = "Type";
    static constexpr const char* propRebuild = "Rebuilding";

    static constexpr const char* fieldName = "Name";
    static constexpr const char* fieldEnabled = "Enabled";
    static constexpr const char* fieldCap = "Capacity";
    static constexpr const char* fieldProto = "Protocol";
    static constexpr const char* fieldType = "MediaType";
    static constexpr const char* fieldUpdating = "Updating";

  public:
    static const std::vector<std::string> fields;
    static constexpr const char* entityName = "Drive";

    Drive() : dbus::FindObjectDBusQuery()
    {}
    ~Drive() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                general::assets::assetInterface,
                {
                    {general::assets::propertyManufacturer, fieldManufacturer},
                    {general::assets::propertyModel, fieldModel},
                    {general::assets::propertySerialNumber, fieldSerialNumber},
                    {general::assets::propertyPartNumber, fieldPartNumber},
                },
            },
            {
                invDriveIface,
                {
                    {propCapacity, fieldCap},
                    {propInterfaces, fieldProto},
                    {propType, fieldType},
                },
            },
            {
                driveStateIface,
                {
                    {propRebuild, fieldUpdating},
                },
            },
            {
                invItemIface,
                {
                    {propName, fieldName},
                    {propPresent, fieldEnabled},
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/inventory",
            {invDriveIface},
            noDepth,
            std::nullopt,
        };

        return criteria;
    }

    const FieldsFormattingMap& getFormatters() const override
    {
        static const FieldsFormattingMap formatters{
            {propInterfaces, {Drive::formatProtocols}},
            {propType, {Drive::formatTypes}},
        };

        return formatters;
    }

    static const DbusVariantType formatProtocols(const PropertyName&,
                                                 const DbusVariantType& value,
                                                 DBusInstancePtr)
    {
        try
        {
            std::vector<std::string> result;
            const auto& protocols = std::get<std::vector<std::string>>(value);
            for (const auto& proto : protocols)
            {
                auto lastSegmenPos = proto.find_last_of('.');
                if (lastSegmenPos == std::string::npos)
                {
                    continue;
                }
                result.push_back(proto.substr(lastSegmenPos + 1));
                return DbusVariantType(result);
            }
        }
        catch (const std::exception& e)
        {
            BMC_LOG_ERROR << "Failure formatting Drive::protocol: " << e.what();
        }
        return std::string(Entity::EntityMember::fieldValueNotAvailable);
    }

    static const DbusVariantType formatTypes(const PropertyName&,
                                             const DbusVariantType& value,
                                             DBusInstancePtr)
    {
        try
        {
            const auto& mediaTypeDirty = std::get<std::string>(value);
            auto lastSegmenPos = mediaTypeDirty.find_last_of('.');
            if (lastSegmenPos == std::string::npos)
            {
                throw std::invalid_argument("Invalid meida type format");
            }
            auto mediaType = mediaTypeDirty.substr(lastSegmenPos + 1);
            if (mediaType == "Unknown")
            {
                return std::string(
                    Entity::EntityMember::fieldValueNotAvailable);
            }

            return DbusVariantType(mediaType);
        }
        catch (const std::exception& e)
        {
            BMC_LOG_ERROR << "Failure formatting Drive::mediaType: "
                          << e.what();
        }
        return std::string(Entity::EntityMember::fieldValueNotAvailable);
    }

    const DefaultFieldsValueDict& getDefaultFieldsValue() const
    {
        static const DefaultFieldsValueDict defaultFields{
            {fieldUpdating, []() { return false; }},
        };
        return defaultFields;
    }
};

const std::vector<std::string> Drive::fields{
    fieldName,       fieldManufacturer, fieldModel, fieldSerialNumber,
    fieldPartNumber, fieldEnabled,      fieldCap,   fieldProto,
    fieldType,       fieldUpdating,
};

} // namespace obmc
} // namespace query
} // namespace app
