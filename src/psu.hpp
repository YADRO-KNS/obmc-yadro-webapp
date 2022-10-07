// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <phosphor-logging/log.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <status_provider.hpp>
#include <formatters.hpp>
#include <sensors.hpp>

namespace app
{
namespace obmc
{
namespace entity
{
using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;
using namespace phosphor::logging;
using namespace app::helpers::utils;

class PMBusProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<PMBusProvider>,
    public CachedSource,
    public NamedEntity<PMBusProvider>
{
  public:
    ENTITY_DECL_FIELD(std::string, CrpsMfrFWRevision)
    ENTITY_DECL_FIELD(std::string, PmbusMfrDate)
    ENTITY_DECL_FIELD(std::string, PmbusMfrID)
    ENTITY_DECL_FIELD(std::string, PmbusMfrLocation)
    ENTITY_DECL_FIELD(std::string, PmbusMfrModel)
    ENTITY_DECL_FIELD(std::string, PmbusMfrRevision)
    ENTITY_DECL_FIELD(std::string, PmbusMfrSerial)
    ENTITY_DECL_FIELD(std::string, PmbusRevision)
    ENTITY_DECL_FIELD(std::string, Id)

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* assocDefIface =
            "xyz.openbmc_project.Association.Definitions";
        static constexpr const char* psuSensorService =
            "xyz.openbmc_project.PSUSensor";
        static constexpr const char* psuYadroIface =
            "com.yadro.Inventory.PSU";

        class FormatId : public query::dbus::IFormatter
        {
          public:
            ~FormatId() override = default;

            const DbusVariantType format(const PropertyName&,
                                         const DbusVariantType& value) override
            {
                if (std::holds_alternative<DBusAssociationsType>(value))
                {
                    const auto associations =
                        std::get<DBusAssociationsType>(value);
                    for (const auto& association : associations)
                    {
                        const auto purpose = std::get<1>(association);
                        if (purpose == "status")
                        {
                            const auto psuObject = std::get<2>(association);
                            return getNameFromLastSegmentObjectPath(psuObject, false);
                        }
                    }
                }
                return std::string(EntityMember::fieldValueNotAvailable);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                assocDefIface,
                DBUS_QUERY_EP_SET_FORMATTERS("Associations", fieldId,
                    DBUS_QUERY_EP_CSTR(FormatId)
                )
            ),
            DBUS_QUERY_EP_IFACES(
                psuYadroIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldCrpsMfrFWRevision),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPmbusMfrDate),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPmbusMfrID),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPmbusMfrLocation),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPmbusMfrModel),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPmbusMfrRevision),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPmbusMfrSerial),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPmbusRevision),
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/State/Decorator/",
            DBUS_QUERY_CRIT_IFACES(psuYadroIface, assocDefIface),
            noDepth,
            psuSensorService
        )
        /* clang-format on */

        // void supplementByStaticFields(const DBusInstancePtr&) const override
        // {
        //     // We haven't presence GPIO to determine whether PSU enabled or
        //     // absent.
        //     // using namespace app::helpers::utils;
        //     // const auto
        //     // setFieldId(instance, psuId);
        // }
    };

  public:
    PMBusProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {
    }
    ~PMBusProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class PSU final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<PSU>
{
  public:
    enum class State
    {
        enabled
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD(std::string, PrettyName)
    ENTITY_DECL_FIELD(std::string, Type)
    ENTITY_DECL_FIELD(std::string, Connector)

    ENTITY_DECL_FIELD(std::string, Manufacturer)
    ENTITY_DECL_FIELD(std::string, Model)
    ENTITY_DECL_FIELD(std::string, PartNumber)
    ENTITY_DECL_FIELD(std::string, SerialNumber)

    ENTITY_DECL_FIELD_ENUM(State, State, enabled)
  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* psuInventoryIface =
            "xyz.openbmc_project.Inventory.Item.PowerSupply";
        static constexpr const char* inventoryItemIface =
            "xyz.openbmc_project.Inventory.Item";
        static constexpr const char* assetsIface =
            "xyz.openbmc_project.Inventory.Decorator.Asset";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                psuInventoryIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldName),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldType),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldConnector),
            ),
             DBUS_QUERY_EP_IFACES(
                inventoryItemIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPrettyName),
            ),
             DBUS_QUERY_EP_IFACES(
                assetsIface,
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldManufacturer,
                    DBUS_QUERY_EP_CSTR(TrimFormatter)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldModel,
                    DBUS_QUERY_EP_CSTR(TrimFormatter)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldPartNumber,
                    DBUS_QUERY_EP_CSTR(TrimFormatter)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldSerialNumber,
                    DBUS_QUERY_EP_CSTR(TrimFormatter)
                ),
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
              "/xyz/openbmc_project/inventory/system/powersupply/",
            DBUS_QUERY_CRIT_IFACES(psuInventoryIface),
            nextOneDepth,
            std::nullopt
        )
        /* clang-format on */
        void supplementByStaticFields(const DBusInstancePtr& instance) const override
        {
            // We haven't presence GPIO to determine whether PSU enabled or
            // absent.
            using namespace app::helpers::utils;
            setFieldId(instance, getNameFromLastSegmentObjectPath(
                                       instance->getObjectPath(), false));
            setFieldState(instance, State::enabled);
        }

        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            static const DefaultFieldsValueDict defaults{
                StatusRollup::defaultGetter<PSU>(),
                StatusFromRollup::defaultGetter<PSU>(),
            };
            return defaults;
        }
    };

    static void linkPSUPMBus(const IEntity::InstancePtr& supplementer,
                                 const IEntity::InstancePtr& target)
    {
        try
        {
            const auto suplPsuId = PMBusProvider::getFieldId(supplementer);
            const auto targetPsuId = getFieldId(target);
            if (suplPsuId == targetPsuId)
            {
                target->supplementOrUpdate(supplementer);
            }
        }
        catch (const std::exception& ex)
        {
            log<level::WARNING>(
                "Fail to supplement Processor instnace by SMBios provider",
                entry("ERROR=%s", ex.what()));
        }
    }

    static const ConditionPtr powerSensor(const InstancePtr& instance,
                                          const char* powerDirection)
    {
        const auto sensorId =
            getFieldPrettyName(instance) + "_" + powerDirection;
        return Condition::buildEqual(Sensors::fieldId, sensorId);
    }

  public:
    PSU() : Collection(), query(std::make_shared<Query>())
    {
      this->createMember(fieldId);
      this->createMember(fieldState);
    }
    ~PSU() override = default;

    static const ConditionPtr inputPowerSensor(const InstancePtr& instance)
    {
        return PSU::powerSensor(instance, "Input_Power");
    }
    static const ConditionPtr outputPowerSensor(const InstancePtr& instance)
    {
        return PSU::powerSensor(instance, "Output_Power");
    }
  protected:
    ENTITY_DECL_QUERY(query)
    /* clang-format off */
    ENTITY_DECL_PROVIDERS(
        ENTITY_PROVIDER_LINK(PMBusProvider, linkPSUPMBus),
    )
    ENTITY_DECL_RELATIONS(
        ENTITY_DEF_RELATION(Sensors, Sensors::inventoryRelation())
    )
    /* clang-format on */
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
