// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/entity/proxy_query.hpp>
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
using namespace phosphor::logging;

class DimmSMBiosProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<DimmSMBiosProvider>,
    public CachedSource,
    public NamedEntity<DimmSMBiosProvider>
{
  public:
    enum class ECC
    {
        noECC,
        singleBitECC,
        multiBitECC,
        addressParity
    };

    enum class FormFactor
    {
        RDIMM,
        UDIMM,
        SO_DIMM,
        LRDIMM,
        mini_RDIMM,
        mini_UDIMM,
        SO_RDIMM_72b,
        SO_UDIMM_72b,
        SO_DIMM_16b,
        SO_DIMM_32b,
        die,
        unknown
    };

    enum class MemoryType
    {
        DRAM,
        intelOptane,
        unknown
    };
    enum class MemoryDeviceType
    {
        DDR,
        DDR2,
        DDR3,
        DDR4,
        DDR4E_SDRAM,
        LPDDR4_SDRAM,
        LPDDR3_SDRAM,
        DDR2_SDRAM_FB_DIMM,
        DDR2_SDRAM_FB_DIMM_PROBE,
        DDR_SGRAM,
        ROM,
        SDRAM,
        EDO,
        fastPageMode,
        pipelinedNibble,
        logical,
        HBM,
        HBM2,
        unknown
    };
    enum class State
    {
        enabled,
        absent
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Name)

    ENTITY_DECL_FIELD(std::string, BuildDate)
    ENTITY_DECL_FIELD(std::string, Manufacturer)
    ENTITY_DECL_FIELD(std::string, Model)
    ENTITY_DECL_FIELD(std::string, PartNumber)
    ENTITY_DECL_FIELD(std::string, SerialNumber)

    ENTITY_DECL_FIELD_DEF(uint64_t, AllocationAlignementInMiB, 0UL)
    ENTITY_DECL_FIELD_DEF(uint64_t, AllocationIncrementInMiB, 0UL)
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, AllowedMemoryModes, {})
    ENTITY_DECL_FIELD_DEF(std::vector<int>, AllowedSpeedsMHz, {})
    ENTITY_DECL_FIELD_DEF(uint16_t, CASLatencies, 0U)
    ENTITY_DECL_FIELD_DEF(uint32_t, CacheSizeInMiB, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, Channel, 0U)
    ENTITY_DECL_FIELD_ENUM(ECC, ECC, noECC)
    ENTITY_DECL_FIELD_ENUM(FormFactor, FormFactor, unknown)
    ENTITY_DECL_FIELD_DEF(bool, IsRankSpareEnabled, false)
    ENTITY_DECL_FIELD_DEF(bool, IsSpareDeviceInUse, false)
    ENTITY_DECL_FIELD_DEF(std::vector<int>, MaxAveragePowerLimitmW, {})
    ENTITY_DECL_FIELD_DEF(uint16_t, MaxMemorySpeedInMhz, 0U)
    ENTITY_DECL_FIELD_DEF(uint8_t, MemoryAttributes, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, MemoryConfiguredSpeedInMhz, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, MemoryController, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, MemoryDataWidth, 0U)
    ENTITY_DECL_FIELD(std::string, MemoryDeviceLocator)
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, MemoryMedia, {})
    ENTITY_DECL_FIELD_DEF(uint32_t, MemorySizeInMiB, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, MemoryTotalWidth, 0U)
    ENTITY_DECL_FIELD_ENUM(MemoryDeviceType, MemoryDeviceType, unknown)
    ENTITY_DECL_FIELD_ENUM(MemoryType, MemoryType, unknown)
    ENTITY_DECL_FIELD(std::string, MemoryTypeDetail)
    ENTITY_DECL_FIELD(std::string, ModuleManufacturerID)
    ENTITY_DECL_FIELD(std::string, ModuleProductID)
    ENTITY_DECL_FIELD_DEF(uint32_t, PmRegionMaxSizeMiB, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, PmRegionNumberLimit, 0U)
    ENTITY_DECL_FIELD_DEF(uint32_t, PmRegionSizeLimitMiB, 0U)
    ENTITY_DECL_FIELD_DEF(uint32_t, PmSizeInMiB, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, RevisionCode, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, Slot, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, Socket, 0U)
    ENTITY_DECL_FIELD_DEF(uint8_t, SpareDeviceCount, 0U)
    ENTITY_DECL_FIELD(std::string, SubsystemDeviceID)
    ENTITY_DECL_FIELD(std::string, SubsystemVendorID)
    ENTITY_DECL_FIELD_DEF(uint32_t, VolatileRegionMaxSizeMiB, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, VolatileRegionNumberLimit, 0U)
    ENTITY_DECL_FIELD_DEF(uint32_t, VolatileRegionSizeLimitMiB, 0U)
    ENTITY_DECL_FIELD_DEF(uint32_t, VolatileSizeInMiB, 0U)
    ENTITY_DECL_FIELD_ENUM(State, State, enabled)

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* smbiosService =
            "xyz.openbmc_project.Smbios.MDR_V2";
        static constexpr const char* dimmItemInterfaceName =
            "xyz.openbmc_project.Inventory.Item.Dimm";
        static constexpr const char* assetsIface =
            "xyz.openbmc_project.Inventory.Decorator.Asset";

        class FormatECC : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Inventory.Item.Dimm.Ecc." + val;
            }

          public:
            ~FormatECC() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, ECC> list{
                    {dbusEnum("NoECC"), ECC::noECC},
                    {dbusEnum("SingleBitECC"), ECC::singleBitECC},
                    {dbusEnum("MultiBitECC"), ECC::multiBitECC},
                    {dbusEnum("AddressParity"), ECC::addressParity},
                };

                return formatValueFromDict(list, property, value, ECC::noECC);
            }
        };

        class FormatFormFactor : public query::dbus::IFormatter
        {
            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Inventory.Item.Dimm.FormFactor." +
                       val;
            }
          public:
            ~FormatFormFactor() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, FormFactor> list{
                    {dbusEnum("RDIMM"), FormFactor::RDIMM},
                    {dbusEnum("UDIMM"), FormFactor::UDIMM},
                    {dbusEnum("SO_DIMM"), FormFactor::SO_DIMM},
                    {dbusEnum("LRDIMM"), FormFactor::LRDIMM},
                    {dbusEnum("Mini_RDIMM"), FormFactor::mini_RDIMM},
                    {dbusEnum("Mini_UDIMM"), FormFactor::mini_UDIMM},
                    {dbusEnum("SO_RDIMM_72b"), FormFactor::SO_RDIMM_72b},
                    {dbusEnum("SO_UDIMM_72b"), FormFactor::SO_UDIMM_72b},
                    {dbusEnum("SO_DIMM_16b"), FormFactor::SO_DIMM_16b},
                    {dbusEnum("SO_DIMM_32b"), FormFactor::SO_DIMM_32b},
                    {dbusEnum("Die"), FormFactor::die},
                };

                return formatValueFromDict(list, property, value,
                                           FormFactor::unknown);
            }
        };

        class FormatMemoryDeviceType : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType." +
                       val;
            }

          public:
            ~FormatMemoryDeviceType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, MemoryDeviceType> types{
                    {dbusEnum("DDR"), MemoryDeviceType::DDR},
                    {dbusEnum("DDR2"), MemoryDeviceType::DDR2},
                    {dbusEnum("DDR3"), MemoryDeviceType::DDR3},
                    {dbusEnum("DDR4"), MemoryDeviceType::DDR4},
                    {dbusEnum("DDR4E_SDRAM"), MemoryDeviceType::DDR4E_SDRAM},
                    {dbusEnum("LPDDR4_SDRAM"), MemoryDeviceType::LPDDR4_SDRAM},
                    {dbusEnum("LPDDR3_SDRAM"), MemoryDeviceType::LPDDR3_SDRAM},
                    {dbusEnum("DDR2_SDRAM_FB_DIMM"), MemoryDeviceType::DDR2_SDRAM_FB_DIMM},
                    {dbusEnum("DDR2_SDRAM_FB_DIMM_PROBE"), MemoryDeviceType::DDR2_SDRAM_FB_DIMM_PROBE},
                    {dbusEnum("DDR_SGRAM"), MemoryDeviceType::DDR_SGRAM},
                    {dbusEnum("ROM"), MemoryDeviceType::ROM},
                    {dbusEnum("SDRAM"), MemoryDeviceType::SDRAM},
                    {dbusEnum("EDO"), MemoryDeviceType::EDO},
                    {dbusEnum("FastPageMode"), MemoryDeviceType::fastPageMode},
                    {dbusEnum("PipelinedNibble"), MemoryDeviceType::pipelinedNibble},
                    {dbusEnum("Logical"), MemoryDeviceType::logical},
                    {dbusEnum("HBM"), MemoryDeviceType::HBM},
                    {dbusEnum("HBM2"), MemoryDeviceType::HBM2},
                };
                // clang-format: on
                return formatValueFromDict(types, property, value,
                                           MemoryDeviceType::unknown);
            }
        };

        class FormatKiBtoMiB : public query::dbus::IFormatter
        {
          public:
            ~FormatKiBtoMiB() override = default;

            const DbusVariantType format(const PropertyName&,
                                         const DbusVariantType& value) override
            {
                static constexpr uint8_t shiftToMiB = 10;
                if (std::holds_alternative<uint64_t>(value))
                {
                    return std::get<uint64_t>(value) >> shiftToMiB;
                }
                else if (std::holds_alternative<uint32_t>(value))
                {
                    return std::get<uint32_t>(value) >> shiftToMiB;
                }

                return 0;
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;
        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                dimmItemInterfaceName,
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "AllocationAlignementInKiB", fieldAllocationAlignementInMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "AllocationIncrementInKiB", fieldAllocationIncrementInMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "MemorySizeInKB", fieldMemorySizeInMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "PmRegionMaxSizeKiB", fieldPmRegionMaxSizeMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "PmRegionSizeLimitKiB", fieldPmRegionSizeLimitMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "PmSizeInKiB", fieldPmSizeInMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "VolatileRegionMaxSizeKiB", fieldVolatileRegionMaxSizeMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "VolatileRegionSizeLimitKiB", fieldVolatileRegionSizeLimitMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "VolatileSizeInKiB", fieldVolatileSizeInMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "CacheSizeInKB", fieldCacheSizeInMiB, 
                    DBUS_QUERY_EP_CSTR(FormatKiBtoMiB)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "ECC", fieldECC,
                    DBUS_QUERY_EP_CSTR(FormatECC)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "FormFactor", fieldFormFactor,
                    DBUS_QUERY_EP_CSTR(FormatFormFactor)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "MemoryType", fieldMemoryDeviceType,
                    DBUS_QUERY_EP_CSTR(FormatMemoryDeviceType)
                ),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldAllowedMemoryModes),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldAllowedSpeedsMHz),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldCASLatencies),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldChannel),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldIsRankSpareEnabled),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldIsSpareDeviceInUse),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMaxAveragePowerLimitmW),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMaxMemorySpeedInMhz),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMemoryAttributes),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMemoryConfiguredSpeedInMhz),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMemoryController),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMemoryDataWidth),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMemoryDeviceLocator),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMemoryMedia),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMemoryTotalWidth),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMemoryTypeDetail),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldModuleManufacturerID),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldModuleProductID),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPmRegionNumberLimit),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldRevisionCode),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSlot),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSocket),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSpareDeviceCount),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSubsystemDeviceID),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSubsystemVendorID),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldVolatileRegionNumberLimit),
            ),
            DBUS_QUERY_EP_IFACES(
                assetsIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldBuildDate),
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
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm/",
            DBUS_QUERY_CRIT_IFACES(dimmItemInterfaceName),
            noDepth,
            smbiosService
        )
        /* clang-format on */

        void setMemoryType(const DBusInstancePtr& instance) const
        {
            auto type = MemoryType::DRAM;
            if (getFieldMemoryDeviceType(instance) == MemoryDeviceType::logical)
            {
                type = MemoryType::intelOptane;
            }
            setFieldMemoryType(instance, type);
        }

        void setState(const DBusInstancePtr& instance) const
        {
            auto available = getFieldManufacturer(instance) != "NO DIMM";
            if (available) 
            {
                setFieldState(instance, State::enabled);
                return;
            }
            setFieldState(instance, State::absent);
        }

        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            using namespace app::helpers::utils;
            setFieldName(instance, getNameFromLastSegmentObjectPath(
                                       instance->getObjectPath()));
            setFieldId(instance, getNameFromLastSegmentObjectPath(
                                     instance->getObjectPath(), false));
            setFieldState(instance, State::enabled);
            setMemoryType(instance);
            setState(instance);
            // No way to determine the Memory health while a DIMM dbus object
            // hasn't valid associations between inventory and sensors.
            StatusProvider::setFieldStatus(instance, StatusProvider::Status::ok);
            StatusProvider::setFieldStatusRollup(instance, StatusProvider::Status::ok);
        }
    };

  public:
    DimmSMBiosProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {
        this->createMember(fieldName);
        this->createMember(fieldId);
        this->createMember(fieldState);
        this->createMember(fieldMemoryType);
        this->createMember(StatusProvider::fieldStatus);
        this->createMember(StatusProvider::fieldStatusRollup);
    }
    ~DimmSMBiosProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class Memory final :
    public Collection,
    public LazySource,
    public NamedEntity<Memory>
{
  public:
    using InstancePtr = IEntity::IEntityMember::InstancePtr;
    class SMBiosProxyQuery : public proxy::ProxyQuery
    {
      public:
        SMBiosProxyQuery(const EntitySupplementProviderPtr& provider) :
            proxy::ProxyQuery(provider)
        {}
        ~SMBiosProxyQuery() override = default;

        const IEntity::InstanceCollection process() override
        {
            getProvider()->populate();
            auto condition =
                Condition::buildEqual(DimmSMBiosProvider::fieldState,
                                      DimmSMBiosProvider::State::enabled);
            return getProvider()->getInstances({condition});
        }
    };
  public:
    Memory() :
        Collection(), query(std::make_shared<SMBiosProxyQuery>(
                          DimmSMBiosProvider::getSingleton()))
    {
    }
    ~Memory() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    std::shared_ptr<SMBiosProxyQuery> query;
};

class MemorySummary final :
    public Entity,
    public LazySource,
    public NamedEntity<MemorySummary>
{
  public:
    ENTITY_DECL_FIELD_DEF(uint32_t, TotalSize, 0U)
    ENTITY_DECL_FIELD_ENUM(DimmSMBiosProvider::State, State, enabled)
    ENTITY_DECL_FIELD_ENUM(StatusProvider::Status, Status, ok)
    ENTITY_DECL_FIELD_ENUM(StatusProvider::Status, StatusRollup, ok)

    class SMBiosProxyQuery : public proxy::ProxyQuery
    {
        static constexpr const char* instanceId = "MemroySummaryInstance";
      public:
        SMBiosProxyQuery(const EntitySupplementProviderPtr& provider) :
            proxy::ProxyQuery(provider)
        {}
        ~SMBiosProxyQuery() override = default;

        const IEntity::InstanceCollection process() override
        {
            getProvider()->populate();
            auto condition =
                Condition::buildEqual(DimmSMBiosProvider::fieldState,
                                      DimmSMBiosProvider::State::enabled);
            const auto memoryInstances = getProvider()->getInstances({condition});
            uint32_t totalCapacity = 0;
            StatusProvider::Status status = StatusProvider::Status::ok,
                                   statusRollup = StatusProvider::Status::ok;
            for (const auto& instance : memoryInstances)
            {
                status = StatusProvider::getHigherStatus(
                    status, StatusProvider::getFieldStatus(instance));
                statusRollup = StatusProvider::getHigherStatus(
                    statusRollup,
                    StatusProvider::getFieldStatusRollup(instance));
                totalCapacity +=
                    (DimmSMBiosProvider::getFieldMemorySizeInMiB(instance) >>
                     10);
            }
            const auto targetInstance =
                std::make_shared<Entity::StaticInstance>(instanceId);
            setFieldTotalSize(targetInstance, totalCapacity);
            StatusProvider::setFieldStatus(targetInstance, status);
            StatusProvider::setFieldStatusRollup(targetInstance, statusRollup);
            DimmSMBiosProvider::setFieldState(
                targetInstance, DimmSMBiosProvider::State::enabled);
            return {targetInstance};
        }

        const QueryFields getFields() const override
        {
            return {
                fieldTotalSize,
                DimmSMBiosProvider::fieldState,
                StatusProvider::fieldStatus,
                StatusProvider::fieldStatusRollup,
            };
        }
    };
  public:
    MemorySummary() :
        Entity(), query(std::make_shared<SMBiosProxyQuery>(
                          DimmSMBiosProvider::getSingleton()))
    {
    }
    ~MemorySummary() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    std::shared_ptr<SMBiosProxyQuery> query;
};

} // namespace entity
} // namespace obmc
} // namespace app
