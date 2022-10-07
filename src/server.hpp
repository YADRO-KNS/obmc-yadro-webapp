// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/helpers/utils.hpp>
#include <firmware.hpp>
#include <phosphor-logging/log.hpp>
#include <sensors.hpp>
#include <status_provider.hpp>

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
        ENTITY_DECL_FIELD(std::string, AssetTag)

        AssetTagQuery() : dbus::FindObjectDBusQuery()
        {}
        ~AssetTagQuery() override = default;

        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            assetTagInterfaceName, DBUS_QUERY_EP_FIELDS_ONLY2(fieldAssetTag)))

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
    ENTITY_DECL_FIELD(bool, Led)

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

class BootSettingsProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<BootSettingsProvider>,
    public CachedSource,
    public NamedEntity<BootSettingsProvider>
{
    static constexpr const char* settingsService =
        "xyz.openbmc_project.Settings";
    static constexpr const char* bootObject =
        "/xyz/openbmc_project/control/host0/auto_reboot";

  public:
    enum class RetryConfig
    {
        retryAttempts,
        disabled
    };

    ENTITY_DECL_FIELD_ENUM(RetryConfig, RetryConfig, disabled)

    class Query : public dbus::GetObjectDBusQuery
    {
        class FormatRetryConfig : public query::dbus::IFormatter
        {
          public:
            ~FormatRetryConfig() override = default;

            const DbusVariantType format(const PropertyName&,
                                         const DbusVariantType& value) override
            {
                if (holds_alternative<bool>(value))
                {
                    if (std::get<bool>(value))
                    {
                        return static_cast<int>(RetryConfig::retryAttempts);
                    }
                }
                return static_cast<int>(RetryConfig::disabled);
            }
        };

        static constexpr const char* rebootPolicyIface =
            "xyz.openbmc_project.Control.Boot.RebootPolicy";

      public:
        Query() : GetObjectDBusQuery(settingsService, bootObject)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            rebootPolicyIface, DBUS_QUERY_EP_SET_FORMATTERS(
                                   "AutoReboot", fieldRetryConfig,
                                   DBUS_QUERY_EP_CSTR(FormatRetryConfig))))

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                rebootPolicyIface,
            };
            return interfaces;
        }
        // clang-format: on
    };

    BootSettingsProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {}
    ~BootSettingsProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class HostWatchdogProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<HostWatchdogProvider>,
    public CachedSource,
    public NamedEntity<HostWatchdogProvider>
{
  public:
    enum class WatchdogAction
    {
        none,
        resetSystem,
        powerDown,
        powerCycle
    };
    ENTITY_DECL_FIELD_DEF(bool, Enabled, false)
    ENTITY_DECL_FIELD_ENUM(WatchdogAction, ExpireAction, none)
  private:
    class Query : public dbus::GetObjectDBusQuery
    {
        static constexpr const char* watchdogService =
            "xyz.openbmc_project.Watchdog";
        static constexpr const char* watchdogObject =
            "/xyz/openbmc_project/watchdog/host0";
        static constexpr const char* watchdogIface =
            "xyz.openbmc_project.State.Watchdog";
        class FormatExpireAction : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.State.Watchdog.Action." + val;
            }

          public:
            ~FormatExpireAction() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, WatchdogAction> actions{
                    {dbusEnum("None"), WatchdogAction::none},
                    {dbusEnum("HardReset"), WatchdogAction::resetSystem},
                    {dbusEnum("PowerOff"), WatchdogAction::powerDown},
                    {dbusEnum("PowerCycle"), WatchdogAction::powerCycle},
                };
                // clang-format: on
                return formatValueFromDict(actions, property, value,
                                           WatchdogAction::none);
            }
        };

      public:
        Query() : GetObjectDBusQuery(watchdogService, watchdogObject)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            watchdogIface, DBUS_QUERY_EP_FIELDS_ONLY2(fieldEnabled),
            DBUS_QUERY_EP_SET_FORMATTERS2(
                fieldExpireAction, DBUS_QUERY_EP_CSTR(FormatExpireAction))))

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                watchdogIface,
            };
            return interfaces;
        }
        // clang-format: on
    };

  public:
    HostWatchdogProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {}
    ~HostWatchdogProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class RebootRemainingProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<RebootRemainingProvider>,
    public CachedSource,
    public NamedEntity<RebootRemainingProvider>
{
  public:
    ENTITY_DECL_FIELD_DEF(uint32_t, AttemptsLeft, 0U)
  private:
    class Query : public dbus::GetObjectDBusQuery
    {
        static constexpr const char* rebootAttemptsIface =
            "xyz.openbmc_project.Control.Boot.RebootAttempts";
        static constexpr const char* hostService =
            "xyz.openbmc_project.State.Host";
        static constexpr const char* bootObject =
            "xyz/openbmc_project/state/host0";

      public:
        Query() : GetObjectDBusQuery(hostService, bootObject)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            rebootAttemptsIface, DBUS_QUERY_EP_FIELDS_ONLY2(fieldAttemptsLeft)))

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                rebootAttemptsIface,
            };
            return interfaces;
        }
        // clang-format: on
    };

  public:
    RebootRemainingProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {}
    ~RebootRemainingProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class PowerRestorePolicyProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<PowerRestorePolicyProvider>,
    public CachedSource,
    public NamedEntity<PowerRestorePolicyProvider>
{
  public:
    enum class RestorePolicy
    {
        alwaysOn,
        alwaysOff,
        lastState
    };
    ENTITY_DECL_FIELD_ENUM(RestorePolicy, PowerRestorePolicy, lastState)
  private:
    class Query : public dbus::GetObjectDBusQuery
    {
        static constexpr const char* settingsService =
            "xyz.openbmc_project.Settings";
        static constexpr const char* policyObject =
            "/xyz/openbmc_project/control/host0/power_restore_policy";
        static constexpr const char* restorePolicyIface =
            "xyz.openbmc_project.Control.Power.RestorePolicy";

        class FormatRestorePolicy : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Control.Power.RestorePolicy."
                       "Policy." +
                       val;
            }

          public:
            ~FormatRestorePolicy() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, RestorePolicy> policies{
                    {dbusEnum("AlwaysOn"), RestorePolicy::alwaysOn},
                    {dbusEnum("AlwaysOff"), RestorePolicy::alwaysOff},
                    {dbusEnum("Restore"), RestorePolicy::lastState},
                };
                // clang-format: on
                return formatValueFromDict(policies, property, value,
                                           RestorePolicy::lastState);
            }
        };

      public:
        Query() : GetObjectDBusQuery(settingsService, policyObject)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            restorePolicyIface, DBUS_QUERY_EP_SET_FORMATTERS2(
                                    fieldPowerRestorePolicy,
                                    DBUS_QUERY_EP_CSTR(FormatRestorePolicy))))

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                restorePolicyIface,
            };
            return interfaces;
        }
        // clang-format: on
    };

  public:
    PowerRestorePolicyProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {}
    ~PowerRestorePolicyProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class HostBootProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<HostBootProvider>,
    public CachedSource,
    public NamedEntity<HostBootProvider>
{
    static constexpr const char* settingsService =
        "xyz.openbmc_project.Settings";
    static constexpr const char* bootObject =
        "/xyz/openbmc_project/control/host0/boot";

  public:
    enum class Mode
    {
        diags,
        biosSetup,
        dbusOriginModeMax = biosSetup, ///< The biosSetup and both other origins
                                       ///< from `Mode` dbus interface
        hdd, ///< Rest modes origings from `Source` dbus interface
        cd,
        pxe,
        usb,
        none
    };
    enum class Type
    {
        legacy,
        uefi
    };

    ENTITY_DECL_FIELD(std::string, Source)
    ENTITY_DECL_FIELD_ENUM(Mode, Mode, none)
    ENTITY_DECL_FIELD_ENUM(Type, Type, legacy)

    class Query : public dbus::GetObjectDBusQuery
    {
      protected:
        class FormatMode : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Control.Boot.Mode.Modes." + val;
            }

          public:
            ~FormatMode() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, Mode> types{
                    {dbusEnum("Safe"), Mode::diags},
                    {dbusEnum("Setup"), Mode::biosSetup},
                    {dbusEnum("Regular"), Mode::none},
                };
                // clang-format: on
                return formatValueFromDict(types, property, value, Mode::none);
            }
        };
        class FormatSource
        {
            const InstancePtr instance;

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Control.Boot.Source.Sources." + val;
            }

          public:
            FormatSource(const InstancePtr instance) : instance(instance)
            {}
            ~FormatSource() = default;

            void operator()(const std::string& value)
            {
                // clang-format: off
                static const std::map<std::string, Mode> sources{
                    {dbusEnum("Disk"), Mode::hdd},
                    {dbusEnum("ExternalMedia"), Mode::cd},
                    {dbusEnum("Network"), Mode::pxe},
                    {dbusEnum("RemovableMedia"), Mode::usb},
                    {dbusEnum("Default"), Mode::none},
                };
                // clang-format: on
                Mode mode = getFieldMode(instance);
                auto searchIt = sources.find(value);
                if (searchIt != sources.end() && mode > Mode::dbusOriginModeMax)
                {
                    mode = searchIt->second;
                }
                setFieldMode(instance, mode);
            }
        };
        class FormatType : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Control.Boot.Type.Types." + val;
            }

          public:
            ~FormatType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, Type> types{
                    {dbusEnum("Legacy"), Type::legacy},
                    {dbusEnum("EFI"), Type::uefi},
                };
                // clang-format: on
                return formatValueFromDict(types, property, value,
                                           Type::legacy);
            }
        };

        static constexpr const char* modeIface =
            "xyz.openbmc_project.Control.Boot.Mode";
        static constexpr const char* sourceIface =
            "xyz.openbmc_project.Control.Boot.Source";
        static constexpr const char* typeIface =
            "xyz.openbmc_project.Control.Boot.Type";

      public:
        Query(const ObjectPath& path) :
            dbus::GetObjectDBusQuery(settingsService, path)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                modeIface,
                DBUS_QUERY_EP_SET_FORMATTERS("BootMode", fieldMode,
                                             DBUS_QUERY_EP_CSTR(FormatMode))),
            DBUS_QUERY_EP_IFACES(sourceIface,
                                 DBUS_QUERY_EP_FIELDS_ONLY("BootSource",
                                                           fieldSource)),
            DBUS_QUERY_EP_IFACES(
                typeIface,
                DBUS_QUERY_EP_SET_FORMATTERS("BootType", fieldType,
                                             DBUS_QUERY_EP_CSTR(FormatType))))

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                modeIface,
                sourceIface,
                typeIface,
            };
            return interfaces;
        }
        // clang-format: on
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            FormatSource formatSource(instance);
            formatSource(getFieldSource(instance));
        }
    };

    HostBootProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>(bootObject))
    {}
    ~HostBootProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class HostBootOnceProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<HostBootOnceProvider>,
    public CachedSource,
    public NamedEntity<HostBootOnceProvider>
{
    static constexpr const char* bootObject =
        "/xyz/openbmc_project/control/host0/boot/one_time";

  public:
    ENTITY_DECL_FIELD_DEF(bool, OneTime, false)
    ENTITY_DECL_FIELD_DEF(bool, ClearCmos, false)
  private:
    class Query final : public HostBootProvider::Query
    {
        static constexpr const char* clearCmosIface =
            "xyz.openbmc_project.Control.Boot.ClearCmos";
        static constexpr const char* enableIface =
            "xyz.openbmc_project.Object.Enable";

      public:
        Query() : HostBootProvider::Query(bootObject)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                modeIface,
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "BootMode", HostBootProvider::fieldMode,
                    DBUS_QUERY_EP_CSTR(HostBootProvider::Query::FormatMode))),
            DBUS_QUERY_EP_IFACES(
                typeIface,
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "BootType", HostBootProvider::fieldType,
                    DBUS_QUERY_EP_CSTR(HostBootProvider::Query::FormatType))),
            DBUS_QUERY_EP_IFACES(
                sourceIface,
                DBUS_QUERY_EP_FIELDS_ONLY("BootSource",
                                          HostBootProvider::fieldSource)),
            DBUS_QUERY_EP_IFACES(clearCmosIface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldClearCmos)),
            DBUS_QUERY_EP_IFACES(enableIface,
                                 DBUS_QUERY_EP_FIELDS_ONLY("Enabled",
                                                           fieldOneTime)), )

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                HostBootProvider::Query::modeIface,
                HostBootProvider::Query::sourceIface,
                HostBootProvider::Query::typeIface,
                clearCmosIface,
                enableIface,
            };
            return interfaces;
        }
        // clang-format: on
    };

  public:
    HostBootOnceProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {}
    ~HostBootOnceProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class Server final :
    public Entity,
    public CachedSource,
    public NamedEntity<Server>
{
    static constexpr const char* chassisInterface =
        "xyz.openbmc_project.Inventory.Item.Chassis";

  public:
    enum class State
    {
        enabled
    };

    enum class BootOverrideType
    {
        continuous,
        once,
        none
    };

    ENTITY_DECL_FIELD(std::string, ServiceName)
    ENTITY_DECL_FIELD(std::string, Datetime)
    ENTITY_DECL_FIELD(std::string, UUID)
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD(std::string, Type)
    ENTITY_DECL_FIELD(std::string, Model)
    ENTITY_DECL_FIELD(std::string, Manufacturer)
    ENTITY_DECL_FIELD(std::string, PartNumber)
    ENTITY_DECL_FIELD(std::string, SerialNumber)
    ENTITY_DECL_FIELD(std::string, BiosVersion)
    ENTITY_DECL_FIELD(std::string, BmcVersion)
    ENTITY_DECL_FIELD_ENUM(State, State, enabled)
    ENTITY_DECL_FIELD_ENUM(BootOverrideType, BootOverrideType, none)
  private:
    class ServerQuery final : public dbus::FindObjectDBusQuery
    {
      public:
        ServerQuery() : dbus::FindObjectDBusQuery()
        {}
        ~ServerQuery() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(chassisInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldName),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldType)),
            DBUS_QUERY_EP_IFACES(general::assets::assetInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldManufacturer),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldModel),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldPartNumber),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldSerialNumber)))
      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/system/chassis/",
            DBUS_QUERY_CRIT_IFACES(chassisInterface), nextOneDepth,
            std::nullopt)

        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            using namespace app::helpers::utils;

            static const DefaultFieldsValueDict defaultGetters{
                StatusRollup::defaultGetter<Server>(),
                {
                    IndicatorLedProvider::fieldLed,
                    [](const auto&) { return false; },
                },
                {
                    fieldDatetime,
                    [](const auto&) {
                        static constexpr const char* bmcDatetimeFormat =
                            "%FT%T%z";
                        return getFormattedCurrentDate(bmcDatetimeFormat);
                    },
                },
            };
            return defaultGetters;
        }
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            setFieldServiceName(instance, "system");
            setFieldState(instance, State::enabled);
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

    static void linkActiveVersions(const IEntity::InstancePtr& supplementer,
                                   const IEntity::InstancePtr& target)
    {
        const auto purpose = FirmwareProvider::getFieldPurpose(supplementer);
        const auto activation =
            FirmwareProvider::getFieldActivation(supplementer);
        if (activation == FirmwareProvider::Activations::active)
        {
            if (purpose == FirmwareProvider::Purpose::bmc)
            {
                setFieldBmcVersion(
                    target, FirmwareProvider::getFieldVersion(supplementer));
                return;
            }
            if (purpose == FirmwareProvider::Purpose::bios)
            {
                setFieldBiosVersion(
                    target, FirmwareProvider::getFieldVersion(supplementer));
            }
        }
    }

    static void linkHostBoot(const IEntity::InstancePtr& supplementer,
                             const IEntity::InstancePtr& target)
    {
        setFieldBootOverrideType(target, BootOverrideType::none);
        HostBootProvider::setFieldMode(target, HostBootProvider::Mode::none);
        HostBootProvider::resetFieldType(target);
        if (HostBootProvider::getFieldMode(supplementer) !=
            HostBootProvider::Mode::none)
        {
            target->supplementOrUpdate(supplementer);
            setFieldBootOverrideType(target, BootOverrideType::continuous);
        }
    }

    static void linkHostBootOnce(const IEntity::InstancePtr& supplementer,
                                 const IEntity::InstancePtr& target)
    {
        if (HostBootProvider::getFieldMode(supplementer) !=
                HostBootProvider::Mode::none &&
            HostBootOnceProvider::getFieldOneTime(supplementer))
        {
            target->supplementOrUpdate(supplementer);
            setFieldBootOverrideType(target, BootOverrideType::once);
        }
    }

    static void linkRebootAttempts(const IEntity::InstancePtr& supplementer,
                                   const IEntity::InstancePtr& target)
    {
        auto config = BootSettingsProvider::getFieldRetryConfig(target);
        RebootRemainingProvider::resetFieldAttemptsLeft(target);
        if (config == BootSettingsProvider::RetryConfig::retryAttempts)
        {
            target->supplementOrUpdate(supplementer);
        }
    }

  public:
    Server() : Entity(), query(std::make_shared<ServerQuery>())
    {
        this->createMember(fieldServiceName);
        this->createMember(fieldDatetime);
        this->createMember(fieldUUID);
        this->createMember(fieldState);
        this->createMember(fieldBootOverrideType);
        this->createMember(fieldBmcVersion);
        this->createMember(fieldBiosVersion);
    }
    ~Server() override = default;

    static const std::vector<std::string>& observedCauserList()
    {
        static const std::vector<std::string> causers{};
        return causers;
    }

  protected:
    /* clang-format off */
    ENTITY_DECL_PROVIDERS(
        ENTITY_PROVIDER_LINK_DEFAULT(AssetTagProvider),
        ENTITY_PROVIDER_LINK_DEFAULT(BootSettingsProvider),
        ENTITY_PROVIDER_LINK_DEFAULT(HostWatchdogProvider),
        ENTITY_PROVIDER_LINK_DEFAULT(PowerRestorePolicyProvider),
        ENTITY_PROVIDER_LINK(RebootRemainingProvider, linkRebootAttempts),
        ENTITY_PROVIDER_LINK(HostBootProvider, linkHostBoot),
        ENTITY_PROVIDER_LINK(HostBootOnceProvider, linkHostBootOnce),
        ENTITY_PROVIDER_LINK(IndicatorLedProvider,linkIndicatorLed),
        ENTITY_PROVIDER_LINK(FirmwareProvider, linkActiveVersions),
        ENTITY_PROVIDER_LINK_DEFAULT(StatusByCallbackManager<Server>),
    )
    ENTITY_DECL_RELATIONS(
        /* relation to bmc to object */
        ENTITY_DEF_RELATION(
            Firmware, Firmware::firmwareRelation<FirmwareProvider::Purpose::bios>()
        )
    )
    ENTITY_DECL_QUERY(query)
    /* clang-format on */
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
