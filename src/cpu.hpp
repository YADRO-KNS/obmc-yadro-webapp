// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <formatters.hpp>
#include <phosphor-logging/log.hpp>
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
using namespace phosphor::logging;

class CpuSMBiosProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<CpuSMBiosProvider>,
    public CachedSource,
    public NamedEntity<CpuSMBiosProvider>
{
  public:
    enum class ProcessorType
    {
        cpu,
        unknown
    };
    enum class InstructionSet
    {
        x86_64,
        powerISA,
        unknown
    };
    enum class Arch
    {
        x86,
        power,
        unknown
    };

    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD_ENUM(ProcessorType, ProcessorType, unknown)
    ENTITY_DECL_FIELD_ENUM(InstructionSet, InstructionSet, unknown)
    ENTITY_DECL_FIELD_ENUM(Arch, Arch, unknown)

    ENTITY_DECL_FIELD(std::string, Characteristics)
    ENTITY_DECL_FIELD(std::string, Family)
    ENTITY_DECL_FIELD(std::string, Manufacturer)
    ENTITY_DECL_FIELD(std::string, Socket)
    ENTITY_DECL_FIELD(std::string, Model)

    ENTITY_DECL_FIELD_DEF(uint16_t, CoreCount, 0)
    ENTITY_DECL_FIELD_DEF(uint32_t, IdentityReg, 0)
    ENTITY_DECL_FIELD_DEF(uint16_t, MaxSpeed, 0)
    ENTITY_DECL_FIELD_DEF(uint16_t, ThreadCount, 0)

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* cpuService =
            "xyz.openbmc_project.Smbios.MDR_V2";
        static constexpr const char* cpuItemInterfaceName =
            "xyz.openbmc_project.Inventory.Item.Cpu";

        class FormatType : public query::dbus::IFormatter
        {
          public:
            ~FormatType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, ProcessorType> types{
                    {"Central Processor", ProcessorType::cpu},
                };

                return formatValueFromDict(types, property, value,
                                           ProcessorType::unknown);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                cpuItemInterfaceName,
                DBUS_QUERY_EP_FIELDS_ONLY("ProcessorCharacteristics", fieldCharacteristics),
                DBUS_QUERY_EP_FIELDS_ONLY("ProcessorCoreCount", fieldCoreCount),
                DBUS_QUERY_EP_FIELDS_ONLY("ProcessorFamily", fieldFamily),
                DBUS_QUERY_EP_FIELDS_ONLY("ProcessorId", fieldIdentityReg),
                DBUS_QUERY_EP_FIELDS_ONLY("ProcessorManufacturer", fieldManufacturer),
                DBUS_QUERY_EP_FIELDS_ONLY("ProcessorMaxSpeed", fieldMaxSpeed),
                DBUS_QUERY_EP_FIELDS_ONLY("ProcessorSocket", fieldSocket),
                DBUS_QUERY_EP_FIELDS_ONLY("ProcessorThreadCount", fieldThreadCount),
                DBUS_QUERY_EP_FIELDS_ONLY("ProcessorVersion", fieldModel),
                DBUS_QUERY_EP_SET_FORMATTERS("ProcessorType", fieldProcessorType,
                                            DBUS_QUERY_EP_CSTR(FormatType)),
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/",
            DBUS_QUERY_CRIT_IFACES(cpuItemInterfaceName),
            noDepth,
            cpuService
        )
        /* clang-format on */
        void setISetAndArch(const DBusInstancePtr& instance) const
        {
            setFieldInstructionSet(instance, InstructionSet::unknown);
            setFieldArch(instance, Arch::unknown);
            try
            {
                using DetectingDict =
                    std::map<std::string, std::pair<InstructionSet, Arch>>;
                static const DetectingDict dictToDetect{
                    {"Intel", {InstructionSet::x86_64, Arch::x86}},
                    {"IBM", {InstructionSet::powerISA, Arch::power}},
                };
                const auto manufacturer =
                    instance->getField(fieldManufacturer)->getStringValue();
                for (const auto [slug, payload] : dictToDetect)
                {
                    if (manufacturer.find(slug) != std::string::npos)
                    {
                        setFieldInstructionSet(instance, payload.first);
                        setFieldArch(instance, payload.second);
                        return;
                    }
                }
            }
            catch (const std::exception& e)
            {
                log<level::ERR>("Fail to obtain InstructionSet field for CPU",
                                entry("ERROR=%s", e.what()));
            }
        }

        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            using namespace app::helpers::utils;
            setFieldName(instance, getNameFromLastSegmentObjectPath(
                                       instance->getObjectPath()));
            this->setISetAndArch(instance);
        }
    };

  public:
    CpuSMBiosProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {
        this->createMember(fieldName);
        this->createMember(fieldInstructionSet);
        this->createMember(fieldArch);
    }
    ~CpuSMBiosProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class Processor final :
    public Collection,
    public CachedSource,
    public NamedEntity<Processor>
{
  public:
    enum class State
    {
        enabled,
        absent
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, PrettyName)
    ENTITY_DECL_FIELD_ENUM(State, State, absent)

    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* cpuInventoryIface =
            "xyz.openbmc_project.Inventory.Item";
        static constexpr const char* cpuSensorsService =
            "xyz.openbmc_project.CPUSensor";

        class FormatState : public query::dbus::IFormatter
        {
          public:
            ~FormatState() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<bool, State> states{
                    {false, State::absent},
                    {true, State::enabled},
                };

                return formatValueFromDict(states, property, value,
                                           State::absent);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                cpuInventoryIface, 
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPrettyName),
                DBUS_QUERY_EP_SET_FORMATTERS("Present", fieldState, 
                    DBUS_QUERY_EP_CSTR(FormatState)
                )
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/",
            DBUS_QUERY_CRIT_IFACES(cpuInventoryIface),
            nextOneDepth, 
            cpuSensorsService
        )
        /* clang-format on */     
        
        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            static const DefaultFieldsValueDict defaults{
                StatusRollup::defaultGetter<Processor>(),
                StatusFromRollup::defaultGetter<Processor>(),
            };
            return defaults;
        }
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            setFieldId(instance, getFieldPrettyName(instance));
        }
    };

    static void linkSMBios(const IEntity::InstancePtr& supplementer,
                           const IEntity::InstancePtr& target)
    {
        try
        {
            const auto suplfieldName =
                CpuSMBiosProvider::getFieldName(supplementer);
            const auto targetName = getFieldPrettyName(target);
            if (suplfieldName == targetName)
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

  public:
    Processor() : Collection(), query(std::make_shared<Query>())
    {
        this->createMember(fieldId);
    }
    ~Processor() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
    /* clang-format off */
    ENTITY_DECL_PROVIDERS(
        ENTITY_PROVIDER_LINK(CpuSMBiosProvider, linkSMBios),
    )
    ENTITY_DECL_RELATIONS(
        ENTITY_DEF_RELATION(Sensors, Sensors::inventoryRelation())
    )
    /* clang-format on */
  private:
    DBusQueryPtr query;
};

class ProcessorSummary final :
    public Entity,
    public LazySource,
    public NamedEntity<ProcessorSummary>
{
  public:
    ENTITY_DECL_FIELD_DEF(uint32_t, Count, 0U)
    ENTITY_DECL_FIELD_ENUM(Processor::State, State, absent)
    ENTITY_DECL_FIELD_ENUM(StatusProvider::Status, Status, ok)
    ENTITY_DECL_FIELD_ENUM(StatusProvider::Status, StatusRollup, ok)

    class Query : public IQuery
    {
        static constexpr const char* instanceId = "ProcessorSummaryInstance";
      public:
        Query() = default;
        ~Query() override = default;

        const IEntity::InstanceCollection process() override
        {
            const auto processorEntity = Processor::getEntity();
            const auto memoryInstances = processorEntity->getInstances();
            auto count = memoryInstances.size();
            StatusProvider::Status status = StatusProvider::Status::ok,
                                   statusRollup = StatusProvider::Status::ok;
            for (const auto& instance : memoryInstances)
            {
                status = StatusProvider::getHigherStatus(
                    status, StatusProvider::getFieldStatus(instance));
                statusRollup = StatusProvider::getHigherStatus(
                    statusRollup,
                    StatusProvider::getFieldStatusRollup(instance));
            }
            const auto targetInstance =
                std::make_shared<Entity::StaticInstance>(instanceId);
            setFieldCount(targetInstance, count);
            StatusProvider::setFieldStatus(targetInstance, status);
            StatusProvider::setFieldStatusRollup(targetInstance, statusRollup);
            Processor::setFieldState(
                targetInstance, Processor::State::enabled);
            return {targetInstance};
        }

        const QueryFields getFields() const override
        {
            return {
                fieldCount,
                Processor::fieldState,
                StatusProvider::fieldStatus,
                StatusProvider::fieldStatusRollup,
            };
        }
    };
  public:
    ProcessorSummary() :
        Entity(), query(std::make_shared<Query>())
    {
    }
    ~ProcessorSummary() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    std::shared_ptr<Query> query;
};

} // namespace entity
} // namespace obmc
} // namespace app
