// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>

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
    class BaseboardQuery final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* boardInterface =
            "xyz.openbmc_project.Inventory.Item.Board";

      public:
        ENTITY_DECL_FIELD(std::string, Name)
        ENTITY_DECL_FIELD(std::string, Type)
        ENTITY_DECL_FIELD(std::string, Manufacturer)
        ENTITY_DECL_FIELD(std::string, Model)
        ENTITY_DECL_FIELD(std::string, PartNumber)
        ENTITY_DECL_FIELD(std::string, SerialNumber)

        BaseboardQuery() : dbus::FindObjectDBusQuery()
        {}
        ~BaseboardQuery() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(boardInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldName),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldType)),
            DBUS_QUERY_EP_IFACES(general::assets::assetInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldManufacturer),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldModel),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldPartNumber),
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldSerialNumber)))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/system/board/",
            DBUS_QUERY_CRIT_IFACES(boardInterface), nextOneDepth, std::nullopt)
    };

  public:
    Baseboard() : Entity(), query(std::make_shared<BaseboardQuery>())
    {}
    ~Baseboard() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
