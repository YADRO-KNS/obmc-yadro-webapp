// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/entity/proxy_query.hpp>
#include <core/helpers/utils.hpp>
#include <formatters.hpp>
#include <phosphor-logging/log.hpp>
#include <status_provider.hpp>

namespace app
{
namespace obmc
{
namespace entity
{
using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;
using namespace app::query::proxy;
using namespace std::placeholders;
using namespace app::helpers::utils;

class FirmwareProvider final :
    public EntitySupplementProvider,
    public ISingleton<FirmwareProvider>,
    public CachedSource,
    public NamedEntity<FirmwareProvider>
{
  public:
    enum class Activations
    {
        notReady,
        invalid,
        ready,
        activating,
        activatingAsStandbySpare,
        active,
        failed,
        standbySpare,
        staged,
        staging
    };

    enum class Purpose
    {
        bmc,
        bios,
        unknown
    };

    enum class RequestedActivations
    {
        none,
        active,
        standbySpare
    };

    enum class State
    {
        enabled,
        updating,
        standbySpare
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, SoftwareId)
    ENTITY_DECL_FIELD(std::string, Version)
    ENTITY_DECL_FIELD_ENUM(Activations, Activation, notReady)
    ENTITY_DECL_FIELD_ENUM(RequestedActivations, RequestedActivation, none)
    ENTITY_DECL_FIELD_ENUM(Purpose, Purpose, unknown)
    ENTITY_DECL_FIELD_ENUM(State, State, enabled)
    ENTITY_DECL_FIELD_ENUM(StatusProvider::Status, Status, ok)
    ENTITY_DECL_FIELD_DEF(bool, Updateable, false)
  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* versionIface =
            "xyz.openbmc_project.Software.Version";
        static constexpr const char* activationIface =
            "xyz.openbmc_project.Software.Activation";

        class FormatPurpose : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Software.Version.VersionPurpose." +
                       val;
            }

          public:
            ~FormatPurpose() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, Purpose> dict{
                    {dbusEnum("BMC"), Purpose::bmc},
                    {dbusEnum("Host"), Purpose::bios},
                };
                // clang-format: on
                return formatValueFromDict(dict, property, value,
                                           Purpose::unknown);
            }
        };
        class FormatActivations : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Software.Activation.Activations." +
                       val;
            }

          public:
            ~FormatActivations() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, Activations> dict{
                    {dbusEnum("NotReady"), Activations::notReady},
                    {dbusEnum("Invalid"), Activations::invalid},
                    {dbusEnum("Ready"), Activations::ready},
                    {dbusEnum("Activating"), Activations::activating},
                    {dbusEnum("ActivatingAsStandbySpare"),
                     Activations::activatingAsStandbySpare},
                    {dbusEnum("Active"), Activations::active},
                    {dbusEnum("Failed"), Activations::failed},
                    {dbusEnum("StandbySpare"), Activations::standbySpare},
                    {dbusEnum("Staged"), Activations::staged},
                    {dbusEnum("Staging"), Activations::staging},
                };
                // clang-format: on
                return formatValueFromDict(dict, property, value,
                                           Activations::notReady);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(versionIface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldVersion),
                                 DBUS_QUERY_EP_SET_FORMATTERS2(
                                     fieldPurpose,
                                     DBUS_QUERY_EP_CSTR(FormatPurpose))),
            DBUS_QUERY_EP_IFACES(activationIface,
                                 DBUS_QUERY_EP_SET_FORMATTERS2(
                                     fieldActivation,
                                     DBUS_QUERY_EP_CSTR(FormatActivations))))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA("/xyz/openbmc_project/software",
                                    DBUS_QUERY_CRIT_IFACES(activationIface),
                                    nextOneDepth, std::nullopt)
        // clang-format: on

        void setId(const DBusInstancePtr& instance) const
        {
            const auto& objectPath = instance->getObjectPath();
            setFieldId(instance,
                       getNameFromLastSegmentObjectPath(objectPath, false));
            setFieldSoftwareId(instance,
                               getNameFromLastSegmentObjectPath(objectPath));
        }
        void setUpdateable(const DBusInstancePtr instance) const
        {
            setFieldUpdateable(instance, false);
            if (getFieldActivation(instance) == Activations::active)
            {
                setFieldUpdateable(instance, true);
            }
        }
        void setStatus(const DBusInstancePtr instance) const
        {
            auto status = StatusProvider::Status::ok;
            auto activation = getFieldActivation(instance);
            if (activation != Activations::active &&
                activation != Activations::activating &&
                activation != Activations::ready)
            {
                status = StatusProvider::Status::warning;
            }
            setFieldStatus(instance, status);
        }
        void setState(const DBusInstancePtr instance) const
        {
            auto state = State::enabled;
            auto activation = getFieldActivation(instance);
            if (activation == Activations::activating)
            {
                state = State::updating;
            }
            else if (activation == Activations::standbySpare)
            {
                state = State::standbySpare;
            }
            setFieldState(instance, state);
        }
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            this->setId(instance);
            this->setUpdateable(instance);
            this->setStatus(instance);
            this->setState(instance);
        }
    };

  public:
    FirmwareProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {
        this->createMember(fieldId);
        this->createMember(fieldSoftwareId);
        this->createMember(fieldUpdateable);
        this->createMember(fieldState);
        this->createMember(fieldStatus);
    }
    ~FirmwareProvider() override = default;

    static const ConditionPtr firmwareByPurpose(Purpose value)
    {
        return Condition::buildEqual(fieldPurpose, value);
    }
    static const ConditionPtr firmwareByActivations(Activations value)
    {
        return Condition::buildEqual(fieldActivation, value);
    }

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class Firmware final :
    public Collection,
    public LazySource,
    public NamedEntity<Firmware>
{
  public:
    Firmware() :
        Collection(),
        query(std::make_shared<ProxyQuery>(FirmwareProvider::getSingleton()))
    {}
    ~Firmware() override = default;

    template <FirmwareProvider::Purpose requredPurpose>
    static const IRelation::RelationRulesList& firmwareRelation()
    {
        static const IRelation::RelationRulesList relations{
            {IRelation::dummyField, FirmwareProvider::fieldPurpose,
             [](const IEntityMember::InstancePtr& instance,
                const IEntityMember::IInstance::FieldType&) -> bool {
                 auto value = instance->getValue();
                 if (!std::holds_alternative<int>(value))
                 {
                     log<level::WARNING>(
                         "FirmwareRelation: invalid type of field 'Purpose'",
                         entry("TYPE=%s", instance->getType().c_str()));
                     return false;
                 }

                 auto firmwarePurpose = static_cast<FirmwareProvider::Purpose>(
                     std::get<int>(value));
                 return requredPurpose == firmwarePurpose;
             }},
        };
        return relations;
    }

    template <FirmwareProvider::Purpose requredPurpose>
    static const IRelation::RelationRulesList& inventoryRelation()
    {
        static const IRelation::RelationRulesList relations{
            {FirmwareProvider::fieldPurpose, IRelation::dummyField,
             [](const IEntityMember::InstancePtr&,
                const IEntityMember::IInstance::FieldType& fwPrpsVal) -> bool {
                 if (!std::holds_alternative<int>(fwPrpsVal))
                 {
                     log<level::WARNING>(
                         "FirmwareInventoryRelation: invalid type of field "
                         "'Purpose'");
                     return false;
                 }
                 auto firmwarePurpose = static_cast<FirmwareProvider::Purpose>(
                     std::get<int>(fwPrpsVal));
                 return requredPurpose == firmwarePurpose;
             }},
        };
        return relations;
    }

    static const ConditionPtr activeFirmware()
    {
        return FirmwareProvider::firmwareByActivations(
            FirmwareProvider::Activations::active);
    }
    static const ConditionPtr biosFirmware()
    {
        return FirmwareProvider::firmwareByPurpose(
            FirmwareProvider::Purpose::bios);
    }
    static const ConditionPtr bmcFirmware()
    {
        return FirmwareProvider::firmwareByPurpose(
            FirmwareProvider::Purpose::bmc);
    }

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATIONS(
        ENTITY_DEF_RELATION2(
            BmcManager,
            Firmware::inventoryRelation<FirmwareProvider::Purpose::bmc>()),
        ENTITY_DEF_RELATION2(
            Server,
            Firmware::inventoryRelation<FirmwareProvider::Purpose::bios>()))

  private:
    ProxyQueryPtr query;
};

class FirmwareManagment final :
    public Entity,
    public CachedSource,
    public NamedEntity<FirmwareManagment>
{
  public:
    enum class ApplyTime
    {
        onReset,
        immediate,
        unknown
    };
    ENTITY_DECL_FIELD_ENUM(ApplyTime, ApplyTime, unknown)
    ENTITY_DECL_FIELD_DEF(uint64_t, MaxImageSize, 0U)
  private:
    class Query : public dbus::GetObjectDBusQuery
    {
        static constexpr const char* settingsService =
            "xyz.openbmc_project.Settings";
        static constexpr const char* applyTimeObject =
            "/xyz/openbmc_project/software/apply_time";
        static constexpr const char* applyTimeIface =
            "xyz.openbmc_project.Software.ApplyTime";

        class FormatApplyTime : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Software.ApplyTime."
                       "RequestedApplyTimes." +
                       val;
            }

          public:
            ~FormatApplyTime() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, ApplyTime> dict{
                    {dbusEnum("OnReset"), ApplyTime::onReset},
                    {dbusEnum("Immediate"), ApplyTime::immediate},
                };
                // clang-format: on
                return formatValueFromDict(dict, property, value,
                                           ApplyTime::unknown);
            }
        };

      public:
        Query() : GetObjectDBusQuery(settingsService, applyTimeObject)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            applyTimeIface,
            DBUS_QUERY_EP_SET_FORMATTERS("RequestedApplyTime", fieldApplyTime,
                                         DBUS_QUERY_EP_CSTR(FormatApplyTime))))

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                applyTimeIface,
            };
            return interfaces;
        }
        // clang-format: on

        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            constexpr size_t mibToBytesShift = 0x14U;
            setFieldMaxImageSize(instance,
                                 HTTP_REQ_BODY_LIMIT_MB << mibToBytesShift);
        }
    };

  public:
    FirmwareManagment() : Entity(), query(std::make_shared<Query>())
    {
        this->createMember(fieldMaxImageSize);
    }
    ~FirmwareManagment() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
