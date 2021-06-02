
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
}

} // namespace core
} // namespace app
