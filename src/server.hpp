// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <phosphor-logging/log.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/helpers/utils.hpp>

#include <common_fields.hpp>
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
using namespace std::placeholders;



class AssetTagProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<AssetTagProvider>,
    public CachedSource,
    public NamedEntity<AssetTagProvider>
{
    class AssetTagQuery final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* assetTagInterfaceName =
            "xyz.openbmc_project.Inventory.Decorator.AssetTag";
        static constexpr const char* systemInterfaceName =
            "xyz.openbmc_project.Inventory.Item.System";

      public:
        AssetTagQuery() : dbus::FindObjectDBusQuery()
        {}
        ~AssetTagQuery() override = default;

        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            assetTagInterfaceName, DBUS_QUERY_EP_FIELDS_ONLY2("AssetTag")))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory",
            DBUS_QUERY_CRIT_IFACES(assetTagInterfaceName, systemInterfaceName),
            noDepth, std::nullopt)
    };

  public:
    AssetTagProvider() :
        EntitySupplementProvider(), query(std::make_shared<AssetTagQuery>())
    {}
    ~AssetTagProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)

  private:
    DBusQueryPtr query;
};

class IndicatorLedProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<IndicatorLedProvider>,
    public CachedSource,
    public NamedEntity<IndicatorLedProvider>
{
  public:
    static constexpr const char* fieldLed = "Led";

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* ledAssertedInterfaceName =
            "xyz.openbmc_project.Led.Group";
        static constexpr const char* ledService =
            "xyz.openbmc_project.LED.GroupManager";
        static constexpr const char* assertedLed = "Asserted";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            ledAssertedInterfaceName,
            DBUS_QUERY_EP_FIELDS_ONLY(assertedLed, fieldLed)))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/led/groups",
            DBUS_QUERY_CRIT_IFACES(ledAssertedInterfaceName), nextOneDepth,
            ledService)
    };

  public:
    IndicatorLedProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {}
    ~IndicatorLedProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class VersionProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<VersionProvider>,
    public CachedSource,
    public NamedEntity<VersionProvider>
{
    static constexpr const char* purposeBios =
        "xyz.openbmc_project.Software.Version.VersionPurpose.Host";
    static constexpr const char* purposeBmc =
        "xyz.openbmc_project.Software.Version.VersionPurpose.BMC";

  public:
    static constexpr const char* fieldVersion = "Version";
    static constexpr const char* fieldPurpose = "Purpose";

    static constexpr const char* fieldVersionBios = "BiosVersion";
    static constexpr const char* fieldVersionBmc = "BmcVersion";

    enum class VersionPurpose : uint8_t
    {
        BMC,
        BIOS,
        Unknown
    };

  private:
    class VersionQuery final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* versionInterfaceName =
            "xyz.openbmc_project.Software.Version";

      public:
        VersionQuery() : dbus::FindObjectDBusQuery()
        {}
        ~VersionQuery() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            versionInterfaceName, DBUS_QUERY_EP_FIELDS_ONLY2(fieldVersion),
            DBUS_QUERY_EP_FIELDS_ONLY2(fieldPurpose)))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/software",
            DBUS_QUERY_CRIT_IFACES(versionInterfaceName), nextOneDepth,
            std::nullopt)
        // clang-format: on
    };

  public:
    VersionProvider() :
        EntitySupplementProvider(), query(std::make_shared<VersionQuery>())
    {
        this->createMember(fieldVersionBmc);
        this->createMember(fieldVersionBios);
    }
    ~VersionProvider() override = default;

    static VersionPurpose getPurpose(const IEntity::InstancePtr& instance)
    {
        static std::map<std::string, VersionPurpose> purposes{
            {purposeBmc, VersionPurpose::BMC},
            {purposeBios, VersionPurpose::BIOS},
        };

        auto inputPurpose = instance->getField(fieldPurpose)->getStringValue();
        auto findPurposeIt = purposes.find(inputPurpose);
        if (findPurposeIt == purposes.end())
        {
            return VersionPurpose::Unknown;
        }

        return findPurposeIt->second;
    }

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class Server final : public Entity, public CachedSource, NamedEntity<Server>
{
    static constexpr const char* chassisInterface =
        "xyz.openbmc_project.Inventory.Item.Chassis";
    static constexpr const char* fieldDatetime = "Datetime";

    class ServerQuery final : public dbus::FindObjectDBusQuery
    {
      public:
        ServerQuery() : dbus::FindObjectDBusQuery()
        {}
        ~ServerQuery() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(chassisInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2("Name"),
                                 DBUS_QUERY_EP_FIELDS_ONLY2("Type")),
            DBUS_QUERY_EP_IFACES(
                general::assets::assetInterface,
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::manufacturer),
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::model),
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::partNumber),
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::serialNumber)))
      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/system/chassis/",
            DBUS_QUERY_CRIT_IFACES(chassisInterface), nextOneDepth,
            std::nullopt)

        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            using namespace app::helpers::utils;

            static const DefaultFieldsValueDict defaultStatusOk{
                {
                    StatusProvider::fieldStatus,
                    []() { return std::string(StatusProvider::OK); },
                },
                {
                    IndicatorLedProvider::fieldLed,
                    []() { return false; },
                },
                {
                    fieldDatetime,
                    []() {
                        static constexpr const char* bmcDatetimeFormat =
                            "%FT%T%z";
                        return getFormattedCurrentDate(bmcDatetimeFormat);
                    },
                },
            };
            return defaultStatusOk;
        }

        void setStatus(const DBusInstancePtr& instance) const
        {
            const auto sensors =
                getEntityManager().getEntity<Sensors>()->getInstances();
            // initialize to OK
            instance->supplementOrUpdate(StatusProvider::fieldStatus,
                                         std::string(StatusProvider::OK));
            for (const auto sensor : sensors)
            {
                try
                {
                    const auto& currentStatus =
                        instance->getField(StatusProvider::fieldStatus)
                            ->getStringValue();
                    if (currentStatus == StatusProvider::critical)
                    {
                        break;
                    }
                    const auto& sensorStatus =
                        sensor->getField(StatusProvider::fieldStatus)
                            ->getStringValue();

                    const std::string status = StatusProvider::getHigherStatus(
                        currentStatus, sensorStatus);
                    instance->supplementOrUpdate(StatusProvider::fieldStatus,
                                                 status);
                }
                catch (std::exception& ex)
                {
                    log<level::ERR>(
                        "Fail to calculate server status",
                        entry("DESCRIPTION=Can't access to the server field"),
                        entry("FIELD=%s", StatusProvider::fieldStatus),
                        entry("ERROR=%s", ex.what()));
                }
            }
        }

        void supplementByStaticFields(const DBusInstancePtr& instance) const override
        {
            this->setStatus(instance);
        }
    };

    static void linkIndicatorLed(const IEntity::InstancePtr& supplementer,
                                 const IEntity::InstancePtr& target)
    {
        static constexpr std::array indicatorObjectsPath{
            "/xyz/openbmc_project/led/groups/enclosure_identify",
            "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        };
        for (auto& indicatorObjectPath : indicatorObjectsPath)
        {
            if (supplementer->getField(metaObjectPath)->getStringValue() ==
                std::string(indicatorObjectPath))
            {
                try
                {
                    // Only the `true` value will be set because `false` is
                    // the default.
                    if (supplementer->getField(IndicatorLedProvider::fieldLed)
                            ->getBoolValue())
                    {
                        target->supplementOrUpdate(
                            IndicatorLedProvider::fieldLed, true);
                    }
                }
                catch (std::out_of_range&)
                {
                    // Error on retrieving the bool value of Indicator LED.
                }
            }
        }
    }

    static void linkVersions(const IEntity::InstancePtr& suppl,
                             const IEntity::InstancePtr& target)
    {
        static std::map<VersionProvider::VersionPurpose, MemberName>
            purposeToMemberName{
                {VersionProvider::VersionPurpose::BMC,
                 VersionProvider::fieldVersionBmc},
                {VersionProvider::VersionPurpose::BIOS,
                 VersionProvider::fieldVersionBios},
            };
        static const std::vector<std::string> availableVersionObjects{
            "bmc active",
            "bios active",
        };

        try
        {
            auto availableName =
                helpers::utils::getNameFromLastSegmentObjectPath(
                    suppl->getField(metaObjectPath)->getStringValue());

            auto versionObjectValid = std::any_of(
                availableVersionObjects.begin(), availableVersionObjects.end(),
                [availableName](
                    const std::string& availableVersionObject) -> bool {
                    return availableName == availableVersionObject;
                });
            if (!versionObjectValid)
            {
                return;
            }

            auto purpose = VersionProvider::getPurpose(suppl);
            if (VersionProvider::VersionPurpose::Unknown == purpose)
            {
                log<level::ERR>("Fail to acquire firmware version",
                                entry("DESCRIPTION=Accepted unknown version "
                                      "purpose. Skipping"));
                return;
            }

            auto findMemberNameIt = purposeToMemberName.find(purpose);
            if (findMemberNameIt == purposeToMemberName.end())
            {
                throw app::core::exceptions::InvalidType("Version Purpose");
            }

            auto versionValue =
                suppl->getField(VersionProvider::fieldVersion)->getValue();
            target->supplementOrUpdate(findMemberNameIt->second, versionValue);
        }
        catch (std::bad_variant_access& ex)
        {
            log<level::ERR>("Fail to get firmware version",
                            entry("DESCRIPTION=Can't supplement the 'Version' "
                                  "field of 'Server'"),
                            entry("ERROR=%s", ex.what()));
        }
    }

  public:
    Server() : Entity(), query(std::make_shared<ServerQuery>())
    {
        // manual initialize versions', status' fields because their comes from
        // calucalted provider linker rule.
        this->createMember(fieldDatetime);
        this->createMember(StatusProvider::fieldStatus);
    }
    ~Server() override = default;

  protected:
    /* clang-format off */
    ENTITY_DECL_PROVIDERS(
        ENTITY_PROVIDER_LINK_DEFAULT(AssetTagProvider),
        ENTITY_PROVIDER_LINK(IndicatorLedProvider,linkIndicatorLed),
        ENTITY_PROVIDER_LINK(VersionProvider, linkVersions)
    )
    ENTITY_DECL_QUERY(query)
    /* clang-format on */
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
