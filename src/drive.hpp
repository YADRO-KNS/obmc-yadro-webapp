// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
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

class Drive final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<Drive>
{
  public:
    enum class Protocol
    {
        sas,
        sata,
        nvme,
        fc,
        unknown
    };

    enum class MediaType
    {
        hdd,
        ssd,
        unknown
    };

    enum class State
    {
        enabled,
        absent
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD(std::string, PrettyName)
    ENTITY_DECL_FIELD(std::string, Manufacturer)
    ENTITY_DECL_FIELD(std::string, Model)
    ENTITY_DECL_FIELD(std::string, SerialNumber)
    ENTITY_DECL_FIELD(std::string, PartNumber)
    ENTITY_DECL_FIELD_DEF(uint64_t, Capacity, 0UL)
    ENTITY_DECL_FIELD_ENUM(Protocol, Protocol, unknown)
    ENTITY_DECL_FIELD_ENUM(MediaType, MediaType, unknown)
    ENTITY_DECL_FIELD_DEF(bool, Updating, false)
    ENTITY_DECL_FIELD_ENUM(StatusProvider::Status, Status, ok)
    ENTITY_DECL_FIELD_ENUM(StatusProvider::Status, StatusRollup, ok)
    ENTITY_DECL_FIELD_ENUM(State, State, absent)

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* invDriveIface =
            "xyz.openbmc_project.Inventory.Item.Drive";
        static constexpr const char* driveStateIface =
            "xyz.openbmc_project.State.Drive";
        static constexpr const char* invItemIface =
            "xyz.openbmc_project.Inventory.Item";

        static constexpr const char* propPresent = "Present";
        static constexpr const char* propType = "Type";
        static constexpr const char* propRebuild = "Rebuilding";

        class FormatProtocol : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Inventory.Item.Drive."
                       "DriveProtocol." +
                       val;
            }

          public:
            ~FormatProtocol() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, Protocol> dict{
                    {dbusEnum("SAS"), Protocol::sas},
                    {dbusEnum("SATA"), Protocol::sata},
                    {dbusEnum("NVMe"), Protocol::nvme},
                    {dbusEnum("FC"), Protocol::fc},
                };
                // clang-format: on
                return formatValueFromDict(dict, property, value,
                                           Protocol::unknown);
            }
        };

        class FormatMediaType : public query::dbus::IFormatter
        {

            static std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Inventory.Item.Drive.DriveType." +
                       val;
            }

          public:
            ~FormatMediaType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, MediaType> dict{
                    {dbusEnum("HDD"), MediaType::hdd},
                    {dbusEnum("SSD"), MediaType::ssd},
                };
                // clang-format: on
                return formatValueFromDict(dict, property, value,
                                           MediaType::unknown);
            }
        };

        class FormatState : public query::dbus::IFormatter
        {
          public:
            ~FormatState() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                State state = State::absent;
                if (!std::holds_alternative<bool>(value))
                {
                    log<level::ERR>("Invalid type of dbus property to "
                                    "calculate Drive::State",
                                    entry("PROP=%s", property.c_str()));
                }
                else
                {
                    if (std::get<bool>(value))
                    {
                        state = State::enabled;
                    }
                }
                return static_cast<int>(state);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                general::assets::assetInterface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldManufacturer),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldModel),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSerialNumber),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPartNumber)),
            DBUS_QUERY_EP_IFACES(invDriveIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldCapacity),
                DBUS_QUERY_EP_SET_FORMATTERS2(
                    fieldProtocol,
                    DBUS_QUERY_EP_CSTR(FormatProtocol)),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    propType, fieldMediaType,
                    DBUS_QUERY_EP_CSTR(FormatMediaType))
            ),
            DBUS_QUERY_EP_IFACES(driveStateIface,
                DBUS_QUERY_EP_FIELDS_ONLY(propRebuild, fieldUpdating)
            ),
            DBUS_QUERY_EP_IFACES(invItemIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPrettyName),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    propPresent, fieldState,
                    DBUS_QUERY_EP_CSTR(FormatState))
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory",
            DBUS_QUERY_CRIT_IFACES(invDriveIface),
            noDepth, 
            std::nullopt
        )
        /* clang-format on */
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            const auto& objectPath = instance->getObjectPath();
            setFieldId(instance,
                       getNameFromLastSegmentObjectPath(objectPath, false));
            setFieldName(instance,
                       getNameFromLastSegmentObjectPath(objectPath));
        }
        const DefaultFieldsValueDict& getDefaultFieldsValue() const
        {
            static const DefaultFieldsValueDict defaultFields{
                StatusRollup::defaultGetter<Drive>(),
                StatusFromRollup::defaultGetter<Drive>(),
            };
            return defaultFields;
        }
    };

  public:
    Drive() : Collection(), query(std::make_shared<Query>())
    {
        this->createMember(fieldId);
        this->createMember(fieldName);
    }
    ~Drive() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class StorageControllers final :
    public entity::Collection,
    public LazySource,
    public NamedEntity<StorageControllers>
{
  public:
    ENTITY_DECL_FIELD(std::string, Id);
    ENTITY_DECL_FIELD(std::string, PrettyName);
    ENTITY_DECL_FIELD_ENUM(StatusProvider::Status, Status, ok)
    ENTITY_DECL_FIELD_ENUM(Drive::State, State, absent)
    private:
    class Query :
        public IQuery,
        public ISingleton<Query>
    {
        static constexpr const char* defaultStorageId =
            "PCH_Storage_Controller";
        static constexpr const char* defaultStoragePrettyName =
            "PCH Storage Controller";

      public:
        Query() = default;
        ~Query() override = default;
        /**
         * @brief Run the configured quiry.
         *
         * @return std::vector<TInstance> The list of retrieved data which
         *                                incapsulated at TInstance objects
         */
        const entity::IEntity::InstanceCollection process() override
        {
            const InstancePtr instance =
                std::make_shared<StaticInstance>(defaultStorageId);

            StorageControllers::setFieldId(instance, defaultStorageId);
            StorageControllers::setFieldPrettyName(instance, defaultStoragePrettyName);
            // PCH Storage Controller enabled all through a time.
            Drive::setFieldStatus(instance, StatusProvider::Status::ok);
            Drive::setFieldState(instance, Drive::State::enabled);
            return {instance};
        }

        const QueryFields getFields() const override
        {
            QueryFields fields{
                fieldId,
                fieldPrettyName,
                fieldState,
                fieldStatus,
            };

            return std::forward<QueryFields>(fields);
        }
    };

  public:
    StorageControllers() : Collection(), query(std::make_shared<Query>())
    {
        this->initialize();
    }
    ~StorageControllers() override = default;

  protected:
    // Current implementation based on PCH-only storage controller.
    // Thun, link each Drive instance to the PchControllerInstance
    ENTITY_DECL_RELATIONS(ENTITY_DEF_RELATION_DIRECT(Drive))
    ENTITY_DECL_QUERY(query)
  private:
    std::shared_ptr<Query> query;
};

} // namespace entity
} // namespace obmc
} // namespace app
