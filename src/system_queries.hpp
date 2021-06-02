// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __SYSTEM_QUERIES_H__
#define __SYSTEM_QUERIES_H__

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/compares.hpp>
#include <definitions.hpp>
#include <logger/logger.hpp>
#include <status_provider.hpp>
#include <version_provider.hpp>

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

using namespace std::placeholders;

using VariantType = dbus::DbusVariantType;
using FieldSet = std::vector<std::string>;

namespace general
{
namespace assets
{

constexpr const char* assetInterface =
    "xyz.openbmc_project.Inventory.Decorator.Asset";

constexpr const char* propertyManufacturer = "Manufacturer";
constexpr const char* propertyModel = "Model";
constexpr const char* propertyPartNumber = "PartNumber";
constexpr const char* propertySerialNumber = "SerialNumber";
} // namespace assets
} // namespace general

class AssetTag final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* assetTagInterfaceName =
        "xyz.openbmc_project.Inventory.Decorator.AssetTag";
    static constexpr const char* systemInterfaceName =
        "xyz.openbmc_project.Inventory.Item.System";

    static constexpr const char* namePropertyAssetTag = "AssetTag";

  public:
    AssetTag() : dbus::FindObjectDBusQuery()
    {}
    ~AssetTag() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                assetTagInterfaceName,
                {
                    {
                        namePropertyAssetTag,
                        supplement_providers::assetTag::fieldTag,
                    },
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
            {
                assetTagInterfaceName,
                systemInterfaceName,
            },
            noDepth,
            std::nullopt,
        };

        return criteria;
    }
};

class IndicatorLed final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* ledAssertedInterfaceName =
        "xyz.openbmc_project.Led.Group";
    static constexpr const char* ledService =
        "xyz.openbmc_project.LED.GroupManager";

    static constexpr const char* namePropertyAssertedLed = "Asserted";

  public:
    IndicatorLed() : dbus::FindObjectDBusQuery()
    {}
    ~IndicatorLed() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                ledAssertedInterfaceName,
                {
                    {
                        namePropertyAssertedLed,
                        supplement_providers::indicatorLed::fieldLed,
                    },
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/led/groups",
            {
                ledAssertedInterfaceName,
            },
            nextOneDepth,
            ledService,
        };

        return criteria;
    }
};

class Server final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* chassisInterface =
        "xyz.openbmc_project.Inventory.Item.Chassis";
    static constexpr const char* namePropertyName = "Name";
    static constexpr const char* namePropertyType = "Type";

  public:
    Server() : dbus::FindObjectDBusQuery()
    {}
    ~Server() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                chassisInterface,
                {
                    {
                        namePropertyName,
                        app::entity::obmc::definitions::fieldName,
                    },
                    {
                        namePropertyType,
                        app::entity::obmc::definitions::fieldType,
                    },
                },
            },
            {
                general::assets::assetInterface,
                {
                    {
                        general::assets::propertyManufacturer,
                        app::entity::obmc::definitions::fieldManufacturer,
                    },
                    {
                        general::assets::propertyModel,
                        app::entity::obmc::definitions::fieldModel,
                    },
                    {
                        general::assets::propertySerialNumber,
                        app::entity::obmc::definitions::fieldSerialNumber,
                    },
                    {
                        general::assets::propertyPartNumber,
                        app::entity::obmc::definitions::fieldPartNumber,
                    },
                },
            },
        };

        return dictionary;
    }

    static void linkStatus(const IEntity::InstancePtr& supplementer,
                                 const IEntity::InstancePtr& target)
    {
        using namespace app::entity::obmc;
        using namespace definitions::supplement_providers;
        using namespace status;

        try
        {
            auto candidate =
                supplementer->getField(fieldStatus)->getStringValue();
            auto current = target->getField(fieldStatus)->getStringValue();

            target->getField(status::fieldStatus)
                ->setValue(Status::getHigherStatus(current, candidate));
        }
        catch (std::bad_variant_access& ex)
        {
            BMC_LOG_ERROR << "Can't supplement the 'Status' field of 'Server' "
                         "Entity. Reason: "
                      << ex.what();
        }
    }

    static void linkIndicatorLed(const IEntity::InstancePtr& supplementer,
                                 const IEntity::InstancePtr& target)
    {
        static constexpr std::string_view indicatorObjectPath =
            "/xyz/openbmc_project/led/groups/enclosure_identify";
        if (supplementer->getField(metaObjectPath)->getStringValue() ==
            indicatorObjectPath)
        {
            auto asserted =
                supplementer
                    ->getField(supplement_providers::indicatorLed::fieldLed)
                    ->getValue();
            target->supplementOrUpdate(
                supplement_providers::indicatorLed::fieldLed, asserted);
        }
    }

    static void linkVersions(const IEntity::InstancePtr& Supplementer,
                             const IEntity::InstancePtr& target)
    {
        using namespace app::entity::obmc;
        using namespace definitions::supplement_providers::version;

        static std::map<Version::VersionPurpose, MemberName>
            purposeToMemberName{
                {Version::VersionPurpose::BMC,
                 definitions::version::fieldVersionBmc},
                {Version::VersionPurpose::BIOS,
                 definitions::version::fieldVersionBios},
            };
        try
        {
            auto purpose = Version::getPurpose(Supplementer);
            if (Version::VersionPurpose::Unknown == purpose)
            {
                BMC_LOG_ERROR << "Accepted unknown version purpose. Skipping";
                return;
            }

            auto findMemberNameIt = purposeToMemberName.find(purpose);
            if (findMemberNameIt == purposeToMemberName.end())
            {
                throw app::core::exceptions::InvalidType("Version Purpose");
            }

            auto versionValue =
                Supplementer->getField(fieldVersion)->getValue();
            target->supplementOrUpdate(findMemberNameIt->second, versionValue);
        }
        catch (std::bad_variant_access& ex)
        {
            BMC_LOG_ERROR << "Can't supplement the 'Version' field of 'Server' "
                         "Entity. Reason: "
                      << ex.what();
        }
    }

    const DefaultFieldsValueDict& getDefaultFieldsValue() const
    {
        using namespace app::entity::obmc::definitions;
        using namespace supplement_providers;
        using namespace app::helpers::utils;

        static constexpr const char* bmcDatetimeFormat = "%FT%T%z";
        static const DefaultFieldsValueDict defaultStatusOk{
            {status::fieldStatus, std::string(Status::statusOK)},
            {indicatorLed::fieldLed, false},
            {fieldDatetime, getFormattedCurrentDate(bmcDatetimeFormat)},
        };
        return defaultStatusOk;
    }
  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/inventory/system/chassis/",
            {
                chassisInterface,
            },
            nextOneDepth,
            std::nullopt,
        };

        return criteria;
    }
};

} // namespace obmc
} // namespace query
} // namespace app

#endif // __SYSTEM_QUERIES_H__
