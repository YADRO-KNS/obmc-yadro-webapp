// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/helpers/utils.hpp>
#include <firmware.hpp>
#include <formatters.hpp>
#include <server.hpp>
#include <status_provider.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::entity;
using namespace app::query;

enum class EntityManagerType
{
    bmc,
    unkown
};

class BmcManager final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<BmcManager>
{
  public:
    enum class Transition
    {
        reboot,
        none
    };

    enum class BMCState
    {
        ready,
        notReady,
        updateInProgress
    };

    enum class PowerState
    {
        on,
        poweringOn,
        poweringOff,
    };

    enum class GraphicalConnect
    {
        kvmip
    };

    enum class SerialConnect
    {
        ipmi,
        ssh
    };

    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD(std::string, Model)
    ENTITY_DECL_FIELD(std::string, Datetime)
    ENTITY_DECL_FIELD_DEF(uint64_t, LastRebootTime, 0UL)
    ENTITY_DECL_FIELD_ENUM(EntityManagerType, ManagerType, bmc)
    ENTITY_DECL_FIELD_ENUM(PowerState, PowerState, poweringOn)
    ENTITY_DECL_FIELD(std::string, LastRebootTimeFmt)
    ENTITY_DECL_FIELD_ENUM(BMCState, CurrentBMCState, notReady)
    ENTITY_DECL_FIELD_ENUM(Transition, RequestedBMCTransition, none)
    ENTITY_DECL_FIELD_DEF(std::vector<int>, GCTypeSupported, {})
    ENTITY_DECL_FIELD_DEF(std::vector<int>, SCTypeSupported, {})
    ENTITY_DECL_FIELD_DEF(uint8_t, GCCount, 4)
    ENTITY_DECL_FIELD_DEF(uint8_t, SCCount, 15)
    ENTITY_DECL_FIELD(std::string, UUID)

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* bmcManagerSN =
            "xyz.openbmc_project.State.BMC";
        static constexpr const char* bmcManagerIface =
            "xyz.openbmc_project.State.BMC";

        static std::string castToDbusTransitionEnum(const std::string& val)
        {
            return "xyz.openbmc_project.State.BMC.Transition." + val;
        }
        static std::string castToDbusBMCStateEnum(const std::string& val)
        {
            return "xyz.openbmc_project.State.BMC.BMCState." + val;
        }

        class FormatTransition : public query::dbus::IFormatter
        {
          public:
            ~FormatTransition() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, Transition> types{
                    {castToDbusTransitionEnum("Ready"), Transition::reboot},
                    {castToDbusTransitionEnum("NotReady"), Transition::none},
                };

                return formatValueFromDict(types, property, value,
                                           Transition::none);
            }
        };

        class FormatBMCState : public query::dbus::IFormatter
        {
          public:
            ~FormatBMCState() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, BMCState> types{
                    {castToDbusBMCStateEnum("Ready"), BMCState::ready},
                    {castToDbusBMCStateEnum("NotReady"), BMCState::notReady},
                    {castToDbusBMCStateEnum("UpdateInProgress"),
                     BMCState::updateInProgress},
                };

                return formatValueFromDict(types, property, value,
                                           BMCState::notReady);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                bmcManagerIface,
                  DBUS_QUERY_EP_SET_FORMATTERS2(
                    fieldCurrentBMCState, DBUS_QUERY_EP_CSTR(FormatBMCState)),
                  DBUS_QUERY_EP_SET_FORMATTERS2(
                    fieldRequestedBMCTransition, DBUS_QUERY_EP_CSTR(FormatTransition)),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldLastRebootTime),
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/state/",
            DBUS_QUERY_CRIT_IFACES(bmcManagerIface),
            nextOneDepth,
            bmcManagerSN
        )
        /* clang-format on */

        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            using namespace app::helpers::utils;

            this->setName(instance);
            this->setLastRebootTimeFmt(instance);
            this->setPowerState(instance);
            this->setGCTypeSupported(instance);
            this->setSCTypeSupported(instance);
            setFieldManagerType(instance, EntityManagerType::bmc);
            setFieldModel(instance, "OpenBMC");
            setFieldUUID(instance, getUuid());
        }

        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            using namespace app::helpers::utils;

            static const DefaultFieldsValueDict defaults{
                StatusRollup::defaultGetter<BmcManager>(),
                {
                    fieldDatetime,
                    [](const auto&) {
                        return getFormattedCurrentDate("%FT%T%z");
                    },
                },
            };
            return defaults;
        }

      private:
        void setName(const DBusInstancePtr& instance) const
        {
#ifdef ENITTY_NAMES_FROM_DBUS
            using namespace app::helpers::utils;
            const auto& objectPath = instance->getObjectPath();
            const auto name = getNameFromLastSegmentObjectPath(objectPath);
#else
            static constexpr const char* managerDefaultName = "bmc";
            setFieldName(instance, managerDefaultName);
#endif
        }

        void setPowerState(const DBusInstancePtr& instance) const
        {
            using PowerStatesCalc =
                std::map<PowerState, std::pair<BMCState, Transition>>;
            static const PowerStatesCalc powerStatesCalc{
                {
                    PowerState::poweringOn,
                    {BMCState::notReady, Transition::none},

                },
                {
                    PowerState::on,
                    {BMCState::ready, Transition::none},

                },
                {
                    PowerState::poweringOff,
                    {BMCState::ready, Transition::reboot},

                },
            };

            auto state = getFieldCurrentBMCState(instance);
            auto transition = getFieldRequestedBMCTransition(instance);

            for (auto [psc, checkPair] : powerStatesCalc)
            {
                if (checkPair.first == state && checkPair.second == transition)
                {
                    setFieldPowerState(instance, psc);
                    return;
                }
            }
            setFieldPowerState(instance, PowerState::poweringOn);
        }

        void setLastRebootTimeFmt(const DBusInstancePtr& instance) const
        {
            FormatTimeMsToSec formatter;
            auto datetime = formatter.format(fieldLastRebootTimeFmt,
                                             getFieldLastRebootTime(instance));
            setFieldLastRebootTimeFmt(instance,
                                      std::get<std::string>(datetime));
        }

        void setGCTypeSupported(const DBusInstancePtr& instance) const
        {
            static const std::vector<int> types{
                static_cast<int>(GraphicalConnect::kvmip),
            };
            setFieldGCTypeSupported(instance, types);
            setFieldGCCount(instance, 4);
        }

        void setSCTypeSupported(const DBusInstancePtr& instance) const
        {
            static const std::vector<int> types{
                static_cast<int>(SerialConnect::ipmi),
                static_cast<int>(SerialConnect::ssh),
            };
            setFieldSCTypeSupported(instance, types);
            setFieldSCCount(instance, 15);
        }
    };

  public:
    BmcManager() : Entity(), query(std::make_shared<Query>())
    {
        this->createMember(fieldName);
        this->createMember(fieldDatetime);
        this->createMember(fieldModel);
        this->createMember(fieldManagerType);
        this->createMember(fieldPowerState);
        this->createMember(fieldGCTypeSupported);
        this->createMember(fieldSCTypeSupported);
        this->createMember(fieldGCCount);
        this->createMember(fieldSCCount);
        this->createMember(fieldLastRebootTimeFmt);
        this->createMember(fieldUUID);
    }
    ~BmcManager() override = default;

    static const std::vector<std::string>& observedCauserList()
    {
        static const std::vector<std::string> causers{
            "/xyz/openbmc_project/state/bmc0",
        };
        return causers;
    }

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_PROVIDERS(
        ENTITY_PROVIDER_LINK_DEFAULT(StatusByCallbackManager<BmcManager>), )
    ENTITY_DECL_RELATIONS(ENTITY_DEF_RELATION(
        Firmware, Firmware::firmwareRelation<FirmwareProvider::Purpose::bmc>()))
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
