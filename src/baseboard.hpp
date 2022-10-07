// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <sensors.hpp>
#include <status_provider.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::query;

class Baseboard final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<Baseboard>
{
  public:
    enum class Type
    {
        systemBoard,
        backplane,
        board,
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD_ENUM(Type, Type, board)
    ENTITY_DECL_FIELD(std::string, Manufacturer)
    ENTITY_DECL_FIELD(std::string, Model)
    ENTITY_DECL_FIELD(std::string, PartNumber)
    ENTITY_DECL_FIELD(std::string, SerialNumber)
  private:
    class BaseboardQuery final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* boardInterface =
            "xyz.openbmc_project.Inventory.Item.Board";

      public:
        BaseboardQuery() : dbus::FindObjectDBusQuery()
        {}
        ~BaseboardQuery() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(boardInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldName)),
            DBUS_QUERY_EP_IFACES(general::assets::assetInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldManufacturer),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldModel),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldPartNumber),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldSerialNumber)))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/system/board/",
            DBUS_QUERY_CRIT_IFACES(boardInterface), nextOneDepth, std::nullopt)

        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            static const DefaultFieldsValueDict defaults{
                StatusRollup::defaultGetter<Baseboard>(),
                StatusFromRollup::defaultGetter<Baseboard>(),
            };
            return defaults;
        }

        void setType(const DBusInstancePtr& instance) const
        {
            static const std::map<std::string, Type> typeByModelTmpls{
                {"Motherboard", Type::systemBoard},
                {"Riser", Type::board},
                {"Backplane", Type::backplane},
            };

            auto model = getFieldModel(instance);
            for (const auto [tmplWord, enumVal] : typeByModelTmpls)
            {
                if (model.find(tmplWord) != std::string::npos)
                {
                    setFieldType(instance, enumVal);
                    return;
                }
            }
            setFieldType(instance, Type::board);
        }

        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            const auto& objectPath = instance->getObjectPath();
            setFieldId(instance,
                       getNameFromLastSegmentObjectPath(objectPath, false));
            this->setType(instance);
        }
    };

  public:
    Baseboard() : Entity(), query(std::make_shared<BaseboardQuery>())
    {
        this->createMember(fieldId);
        this->createMember(fieldType);
    }
    ~Baseboard() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATIONS(ENTITY_DEF_RELATION(Sensors,
                                              Sensors::inventoryRelation()))
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
