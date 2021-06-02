
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
    entityManager.buildEntity(definitions::network::config::entityNetConf)
        ->addMembers({
            definitions::network::config::fieldHostName,
            definitions::network::config::fieldDefaultIPv4Gateway,
            definitions::network::config::fieldDefaultIPv6Gateway,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<NetworkSysConfiguration>()
        .complete();

    /* Define NETWORK DHCP CONFIGURATION entity */
    entityManager.buildEntity(definitions::network::config::entityDHCPConf)
        ->addMembers({
            definitions::network::config::fieldDNSEnabled,
            definitions::network::config::fieldHostNameEnabled,
            definitions::network::config::fieldNTPEnabled,
            definitions::network::config::fieldSendHostNameEnabled,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<NetworkDHCPConfiguration>()
        .complete();

    /* Define NETWORK IP entity */
    entityManager.buildEntity(definitions::network::ip::entityIpIface)
        ->addMembers({
            definitions::network::ip::fieldAddress,
            definitions::network::ip::fieldGateway,
            definitions::network::ip::fieldMask,
            definitions::network::ip::fieldOrigin,
            definitions::network::ip::fieldType,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<NetworkIPInterface>()
        .complete();

    /* Define NETWORK ETHERNET entity */
    entityManager.buildEntity(definitions::network::ethernet::entityEthIface)
        ->addMembers({
            definitions::network::ethernet::fieldAutoNeg,
            definitions::network::ethernet::fieldDHCPEnabled,
            definitions::network::ethernet::fieldDomainName,
            definitions::network::ethernet::fieldIPv6AcceptRA,
            definitions::network::ethernet::fieldInterfaceName,
            definitions::network::ethernet::fieldLinkLocalAutoConf,
            definitions::network::ethernet::fieldLinkUp,
            definitions::network::ethernet::fieldNICEnabled,
            definitions::network::ethernet::fieldNTPServers,
            definitions::network::ethernet::fieldMACAddress,
            definitions::network::ethernet::fieldStaticNameServers,
        })
        .addRelations(definitions::network::ip::entityIpIface,
                      NetworkEthInterface::relationToIp())
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<NetworkEthInterface>()
        .complete();
}

} // namespace core
} // namespace app
