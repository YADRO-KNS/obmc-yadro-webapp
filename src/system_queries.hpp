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
            BMC_LOG_ERROR
                << "Can't supplement the 'Status' field of 'Server' Entity:"
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
            BMC_LOG_ERROR
                << "Can't supplement the 'Version' field of 'Server': "
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

class Chassis final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* yadroInterface = "com.yadro.Platform";

    static constexpr const char* namePropertyType = "ChassisType";
    static constexpr const char* namePropertyPartNumber = "ChassisPartNumber";

    static constexpr const char* chassisTypeOther = "Other";
    static const std::map<std::string, std::string> chassisTypesNames;

  public:
    Chassis() : dbus::FindObjectDBusQuery()
    {}
    ~Chassis() = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                yadroInterface,
                {
                    {
                        namePropertyType,
                        app::entity::obmc::definitions::fieldType,
                    },
                    {
                        namePropertyPartNumber,
                        app::entity::obmc::definitions::fieldPartNumber,
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
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/inventory/system/board/",
            {
                yadroInterface,
            },
            nextOneDepth,
            std::nullopt,
        };

        return criteria;
    }

    static const DbusVariantType formatChassisType(const PropertyName&,
                                                   const DbusVariantType& value,
                                                   DBusInstancePtr)
    {
        auto formattedValue = std::visit(
            [](auto&& chassisType) -> const DbusVariantType {
                using TChassisType = std::decay_t<decltype(chassisType)>;

                if constexpr (std::is_same_v<TChassisType, std::string>)
                {
                    auto findTypeIt = chassisTypesNames.find(chassisType);
                    if (findTypeIt == chassisTypesNames.end())
                    {
                        return DbusVariantType(std::string(chassisTypeOther));
                    }

                    return DbusVariantType(findTypeIt->second);
                }

                throw std::invalid_argument(
                    "Invalid value type of ChassisType property");
            },
            value);
        return formattedValue;
    }

    const FieldsFormattingMap& getFormatters() const override
    {
        static const FieldsFormattingMap formatters{
            {
                namePropertyType,
                {
                    Chassis::formatChassisType,
                },
            },
        };

        return formatters;
    }
};

class Baseboard final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* boardInterface =
        "xyz.openbmc_project.Inventory.Item.Board";
    static constexpr const char* namePropertyName = "Name";
    static constexpr const char* namePropertyType = "Type";

  public:
    Baseboard() : dbus::FindObjectDBusQuery()
    {}
    ~Baseboard() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                boardInterface,
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

    static std::vector<MemberName>
        relationsSensorsLinkRule(const IEntity::InstancePtr& supplementer,
                                 const IEntity::InstancePtr&)
    {
        using namespace app::entity::obmc::definitions;
        using namespace app::entity::obmc::definitions::supplement_providers;

        std::visit(
            [](auto&& relations) {
                using TProperty = std::decay_t<decltype(relations)>;

                if constexpr (std::is_same_v<std::string, TProperty>)
                {
                    for (auto& relation : relations)
                    {
                        BMC_LOG_DEBUG << "Relation: " << relation;
                    }
                }
            },
            supplementer->getField(relations::fieldEndpoint)->getValue());

        return {
            metaRelation,
        };
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/inventory/system/board/",
            {
                boardInterface,
            },
            nextOneDepth,
            std::nullopt,
        };

        return criteria;
    }
};

class Sensors final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* sensorThresholdWarningInterface =
        "xyz.openbmc_project.Sensor.Threshold.Warning";
    static constexpr const char* sensorThresholdCriticalInterface =
        "xyz.openbmc_project.Sensor.Threshold.Critical";
    static constexpr const char* sensorValueInterface =
        "xyz.openbmc_project.Sensor.Value";
    static constexpr const char* sensorAvailabilityInterface =
        "xyz.openbmc_project.State.Decorator.Availability";
    static constexpr const char* sensorOperationalStatusInterface =
        "xyz.openbmc_project.State.Decorator.OperationalStatus";


    static constexpr const char* namePropertyCriticalLow = "CriticalLow";
    static constexpr const char* namePropertyCriticalHigh = "CriticalHigh";
    static constexpr const char* namePropertyWarningLow = "WarningLow";
    static constexpr const char* namePropertyWarningHigh = "WarningHigh";
    static constexpr const char* namePropertyValue = "Value";
    static constexpr const char* namePropertyUnit = "Unit";

    static constexpr const char* defaultUnitLabel = "Unit";

    static constexpr const char* nameAvailable = "Available";
    static constexpr const char* nameFunctional = "Functional";

    static const std::map<std::string, std::string> sensorUnitsMap;

  public:
    Sensors() : dbus::FindObjectDBusQuery()
    {}
    ~Sensors() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                sensorValueInterface,
                {
                    {
                        namePropertyValue,
                        app::entity::obmc::definitions::sensors::fieldValue,
                    },
                    {
                        namePropertyUnit,
                        app::entity::obmc::definitions::sensors::fieldUnit,
                    },
                },
            },
            {
                sensorThresholdWarningInterface,
                {
                    {
                        namePropertyWarningLow,
                        app::entity::obmc::definitions::sensors::
                            fieldLowWarning,
                    },
                    {
                        namePropertyWarningHigh,
                        app::entity::obmc::definitions::sensors::
                            fieldHighWarning,
                    },
                },
            },
            {
                sensorThresholdCriticalInterface,
                {
                    {
                        namePropertyCriticalHigh,
                        app::entity::obmc::definitions::sensors::
                            fieldHightCritical,
                    },
                    {
                        namePropertyCriticalLow,
                        app::entity::obmc::definitions::sensors::
                            fieldLowCritical,
                    },
                },
            },
            {
                sensorAvailabilityInterface,
                {{
                    nameAvailable,
                    app::entity::obmc::definitions::sensors::fieldAvailable,
                }},
            },
            {
                sensorOperationalStatusInterface,
                {
                    {
                        nameFunctional,
                        app::entity::obmc::definitions::sensors::
                            fieldFunctional,
                    },
                },
            },
        };

        return dictionary;
    }

    static void linkStatus(const IEntity::InstancePtr& Supplementer,
                           const IEntity::InstancePtr& target)
    {
        using namespace app::entity::obmc;
        using namespace definitions::supplement_providers;
        using namespace status;

        try
        {
            auto& targetObjectPath =
                target->getField(metaObjectPath)->getStringValue();
            auto& causer =
                Supplementer->getField(fieldObjectCauthPath)->getStringValue();

            auto candidate =
                Supplementer->getField(fieldStatus)->getStringValue();
            auto current = target->getField(fieldStatus)->getStringValue();

            if (targetObjectPath != causer)
            {
                return;
            }

            BMC_LOG_DEBUG << "Supplementer object=" << targetObjectPath
                      << " field Status=" << candidate
                      << ". Current Value=" << current;

            target->getField(status::fieldStatus)
                ->setValue(Status::getHigherStatus(current, candidate));
        }
        catch (std::bad_variant_access& ex)
        {
            BMC_LOG_ERROR
                << "Can't supplement the 'Status' field of 'Sensor' Entity: "
                << ex.what();
        }
    }

    void supplementByStaticFields(DBusInstancePtr& instance) const override
    {
        this->setSensorName(instance);
    }

    const DefaultFieldsValueDict& getDefaultFieldsValue() const
    {
        using namespace app::entity::obmc::definitions::supplement_providers;
        static const DefaultFieldsValueDict defaultStatusOk{
            {status::fieldStatus, std::string(Status::statusOK)},
        };
        return defaultStatusOk;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/sensors",
            {
                sensorValueInterface,
            },
            noDepth,
            std::nullopt,
        };

        return criteria;
    }

    void setSensorName(DBusInstancePtr& instance) const
    {
        auto& objectPath = instance->getObjectPath();
        instance->supplementOrUpdate(
            app::entity::obmc::definitions::sensors::fieldName,
            app::helpers::utils::getNameFromLastSegmentObjectPath(objectPath));
    }

    static const DbusVariantType formatSensorUnit(const PropertyName&,
                                                  const DbusVariantType& value,
                                                  DBusInstancePtr)
    {
        auto formattedValue = std::visit(
            [](auto&& chassisType) -> const DbusVariantType {
                using TChassisType = std::decay_t<decltype(chassisType)>;

                if constexpr (std::is_same_v<TChassisType, std::string>)
                {
                    auto findTypeIt = sensorUnitsMap.find(chassisType);
                    if (findTypeIt == sensorUnitsMap.end())
                    {
                        return DbusVariantType(std::string(defaultUnitLabel));
                    }

                    return DbusVariantType(findTypeIt->second);
                }

                throw std::invalid_argument(
                    "Invalid value type of Sensor Unit property");
            },
            value);
        return formattedValue;
    }

    const FieldsFormattingMap& getFormatters() const override
    {
        static const FieldsFormattingMap formatters{
            {
                namePropertyUnit,
                {
                    Sensors::formatSensorUnit,
                },
            },
        };

        return formatters;
    }
};

} // namespace obmc
} // namespace query
} // namespace app

#endif // __SYSTEM_QUERIES_H__
