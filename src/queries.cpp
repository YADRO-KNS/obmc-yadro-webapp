
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/application.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <definitions.hpp>
#include <network_provider.hpp>
#include <power_provider.hpp>
#include <relation_provider.hpp>
#include <service/configuration.hpp>
#include <status_provider.hpp>
#include <system_queries.hpp>
#include <version_provider.hpp>
#include <pcie_provider.hpp>
#include <settings.hpp>
#include <drive.hpp>
#include <pid_control.hpp>
#include <network_adapter.hpp>

namespace app
{
namespace core
{

void Application::initEntityMap()
{
    using namespace app::entity;
    using namespace app::entity::obmc;
    using namespace app::query;
    using namespace app::query::obmc;
    using namespace definitions::supplement_providers;

    using namespace std::literals;

    /* Define GLOBAL STATUS supplement provider */
    entityManager.buildSupplementProvider(status::providerStatus)
        ->addMembers({
            status::fieldStatus,
            status::fieldObjectCauthPath,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Status>()
        .complete();

    /* Define GLOBAL VERSION supplement provider */
    entityManager
        .buildSupplementProvider(
            definitions::supplement_providers::version::providerVersion)
        ->addMembers({
            definitions::supplement_providers::version::fieldPurpose,
            definitions::supplement_providers::version::fieldVersion,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Version>()
        .complete();

    /* Define SYSTEM ASSET TAG supplement provider */
    entityManager
        .buildSupplementProvider(
            definitions::supplement_providers::assetTag::providerAssetTag)
        ->addMembers({
            definitions::supplement_providers::assetTag::fieldTag,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<AssetTag>()
        .complete();

    /* Define SYSTEM INDICATOR LED supplement provider */
    entityManager
        .buildSupplementProvider(definitions::supplement_providers::
                                     indicatorLed::providerIndicatorLed)
        ->addMembers({
            definitions::supplement_providers::indicatorLed::fieldLed,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<IndicatorLed>()
        .complete();

    /* Define PCIe supplement provider */
    entityManager
        .buildSupplementProvider(
            supplement_providers::pcie::providerPCIe)
        ->addMembers(PCIeProvider::getPCIeMemberNameList())
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<PCIeProvider>()
        .complete();

    /* Define PCIe Functions entity */
    entityManager
        .buildEntity<PCIeFunctionEntity>(
            definitions::pcie::functions::entityFunction)
        // Each PCIe provider dynamic field is a PCIe Function member.
        ->addMembers(PCIeProvider::pcieFunctionsDynamicFields)
        .addMembers({
            supplement_providers::pcie::metaSBD,
            definitions::pcie::functions::fieldFunctionId,
        });

    /* Define PCIeDevice entity */
    entityManager.buildEntity<PCIeDeviceEntity>(definitions::pcie::entityPCIe)
        ->addMembers({
            supplement_providers::pcie::metaSBD,
            supplement_providers::pcie::fieldSocket,
            supplement_providers::pcie::fieldBus,
            supplement_providers::pcie::fieldAddress,
            supplement_providers::pcie::fieldDevice,
            supplement_providers::pcie::fieldDeviceType,
            supplement_providers::pcie::fieldManufacturer,
            supplement_providers::pcie::fieldSubsystem,
            supplement_providers::status::fieldStatus,
        })
        .addRelations(definitions::pcie::functions::entityFunction,
                      PCIeDeviceEntity::realtionToFunctions());

    /* Define Settings entity */
    entityManager.buildEntity(definitions::entitySettings)
        ->addMembers({settings::fieldTitleTemplate})
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Settings>()
        .complete();

    /* Define SERVER entity */
    entityManager.buildEntity(definitions::entityServer)
        ->addMembers({
            definitions::fieldName,
            definitions::fieldType,
            definitions::fieldManufacturer,
            definitions::fieldModel,
            definitions::fieldPartNumber,
            definitions::fieldSerialNumber,
            definitions::version::fieldVersionBios,
            definitions::version::fieldVersionBmc,
            definitions::fieldDatetime,
        })
        .linkSupplementProvider(
            definitions::supplement_providers::version::providerVersion,
            std::bind(&Server::linkVersions, _1, _2))
        .linkSupplementProvider(indicatorLed::providerIndicatorLed,
                                std::bind(&Server::linkIndicatorLed, _1, _2))
        .linkSupplementProvider(assetTag::providerAssetTag)
        .linkSupplementProvider(status::providerStatus,
                                std::bind(&Server::linkStatus, _1, _2))
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Server>()
        .complete();

    /* Define CHASSIS entity */
    entityManager.buildEntity(definitions::entityChassis)
        ->addMembers({
            definitions::fieldType,
            definitions::fieldPartNumber,
            definitions::fieldManufacturer,
        })
        .linkSupplementProvider(assetTag::providerAssetTag)
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Chassis>()
        .complete();

    /* Define HOST POWER entity */
    entityManager.buildEntity(definitions::power::entityHostPower)
        ->addMembers({
            definitions::power::fieldState,
            definitions::power::fieldStatus,
            definitions::power::metaStatus,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<HostPower>()
        .complete();

    /* Define BASEBOARD entity */
    entityManager.buildEntity(definitions::entityBaseboard)
        ->addMembers({
            definitions::fieldName,
            definitions::fieldType,
            definitions::fieldManufacturer,
            definitions::fieldModel,
            definitions::fieldPartNumber,
            definitions::fieldSerialNumber,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Baseboard>()
        .complete();

    /* Define NETWORK SYSTEM CONFIGURATION entity */
    entityManager.buildEntity(NetworkSysConfiguration::entityName)
        ->addMembers(NetworkSysConfiguration::fields)
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<NetworkSysConfiguration>()
        .complete();

    /* Define NETWORK DHCP CONFIGURATION entity */
    entityManager.buildEntity(NetworkDHCPConfiguration::entityName)
        ->addMembers(NetworkDHCPConfiguration::fields)
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<NetworkDHCPConfiguration>()
        .complete();

    /* Define NETWORK IP entity */
    entityManager.buildEntity(NetworkIPInterface::entityName)
        ->addMembers(NetworkIPInterface::fields)
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<NetworkIPInterface>()
        .complete();

    /* Define NETWORK ETHERNET entity */
    entityManager.buildEntity(NetworkEthInterface::entityName)
        ->addMembers(NetworkEthInterface::fields)
        .addRelations(NetworkIPInterface::entityName,
                      NetworkEthInterface::relationToIp())
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<NetworkEthInterface>()
        .complete();

    /* Define PID entity */
    entityManager.buildEntity(PID::entityName)
        ->addMembers(PID::fields)
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<PID>()
        .complete();

    /* Define PID ZONE entity */
    entityManager.buildEntity(PIDZone::entityName)
        ->addMembers({
            PIDZone::fieldName,
            PIDZone::fieldMinOutputPercent,
            PIDZone::fieldFailsafePercent,
        })
        .addRelations(PID::entityName, PIDZone::relationToPID())
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<PIDZone>()
        .complete();

    /* Define DRIVE entity */
    entityManager.buildEntity(Drive::entityName)
        ->addMembers(Drive::fields)
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Drive>()
        .complete();

    /* Define NETWORK ADAPTER entity */
    entityManager.buildCollection(NetworkAdapter::entityName)
        ->addMembers(NetworkAdapter::fields)
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<NetworkAdapter>()
        .complete();

    /* Define SENSORS entity */
    entityManager.buildEntity(definitions::entitySensors)
        ->addMembers({
            definitions::sensors::fieldName,
            definitions::sensors::fieldValue,
            definitions::sensors::fieldUnit,
            definitions::sensors::fieldLowWarning,
            definitions::sensors::fieldLowCritical,
            definitions::sensors::fieldHighWarning,
            definitions::sensors::fieldHightCritical,
            definitions::sensors::fieldAvailable,
            definitions::sensors::fieldFunctional,
        })
        .linkSupplementProvider(
            status::providerStatus,
            std::bind(&Sensors::linkStatus, _1, _2))
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Sensors>()
        .complete();
}

} // namespace core
} // namespace app
