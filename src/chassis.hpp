// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <phosphor-logging/log.hpp>
#include <status_provider.hpp>
#include <formatters.hpp>

namespace app
{
namespace obmc
{
namespace entity
{
using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;
using namespace std::placeholders;
using namespace general::assets;

class Chassis final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<Chassis>
{
    static constexpr const char* fieldType = "Type";

  private:
    class ChassisQuery final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* yadroInterface = "com.yadro.Platform";

      private:
        class FormatType : public query::dbus::IFormatter
        {
            static constexpr const char* chassisTypeOther = "Other";

          public:
            ~FormatType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const StringFormatterDict types{
                    {"23", "Rack Mount"},
                };

                return formatStringValueFromDict(types, property, value);
            }
        };

      public:
        ChassisQuery() : dbus::FindObjectDBusQuery()
        {}
        ~ChassisQuery() = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                yadroInterface,
                DBUS_QUERY_EP_SET_FORMATTERS("ChassisType", fieldType,
                                             DBUS_QUERY_EP_CSTR(FormatType)),
                DBUS_QUERY_EP_FIELDS_ONLY("ChassisPartNumber", partNumber)),
            DBUS_QUERY_EP_IFACES(
                general::assets::assetInterface,
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::manufacturer)))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/inventory/system/board/",
            DBUS_QUERY_CRIT_IFACES(yadroInterface), nextOneDepth, std::nullopt)
    };

  public:
    Chassis() : Entity(), query(std::make_shared<ChassisQuery>())
    {}
    ~Chassis() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
