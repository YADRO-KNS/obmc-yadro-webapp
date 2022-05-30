// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <phosphor-logging/log.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>

#include <common_fields.hpp>
#include <formatters.hpp>

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
    static constexpr const char* fieldName = "Name";
    static constexpr const char* fieldEnabled = "Enabled";
    static constexpr const char* fieldCap = "Capacity";
    static constexpr const char* fieldProto = "Protocol";
    static constexpr const char* fieldType = "MediaType";
    static constexpr const char* fieldUpdating = "Updating";

    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* invDriveIface =
            "xyz.openbmc_project.Inventory.Item.Drive";
        static constexpr const char* driveStateIface =
            "xyz.openbmc_project.State.Drive";
        static constexpr const char* invItemIface =
            "xyz.openbmc_project.Inventory.Item";

        static constexpr const char* propName = "PrettyName";
        static constexpr const char* propPresent = "Present";
        static constexpr const char* propType = "Type";
        static constexpr const char* propRebuild = "Rebuilding";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                general::assets::assetInterface,
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::manufacturer),
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::model),
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::serialNumber),
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::partNumber)),
            DBUS_QUERY_EP_IFACES(invDriveIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldCap),
                DBUS_QUERY_EP_SET_FORMATTERS2(
                    fieldProto,
                    DBUS_QUERY_EP_CSTR(DBusEnumFormatter)),
                DBUS_QUERY_EP_SET_FORMATTERS(
                    propType, fieldType,
                    DBUS_QUERY_EP_CSTR(DBusEnumFormatter))),
            DBUS_QUERY_EP_IFACES(driveStateIface,
                DBUS_QUERY_EP_FIELDS_ONLY(propRebuild,fieldUpdating)),
            DBUS_QUERY_EP_IFACES(invItemIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldName),
                DBUS_QUERY_EP_FIELDS_ONLY(propPresent, fieldEnabled)))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory",
            DBUS_QUERY_CRIT_IFACES(invDriveIface),
            noDepth, 
            std::nullopt
        )
        /* clang-format on */

        const DefaultFieldsValueDict& getDefaultFieldsValue() const
        {
            static const DefaultFieldsValueDict defaultFields{
                {fieldUpdating, []() { return false; }},
            };
            return defaultFields;
        }
    };

  public:
    Drive() : Collection(), query(std::make_shared<Query>())
    {}
    ~Drive() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
