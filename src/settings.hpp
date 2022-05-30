// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;

class Settings final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<Settings>
{
  public:
    ENTITY_DECL_FIELD(std::string, TitleTemplate);

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* settingsServiceName =
            "xyz.openbmc_project.Settings";
        static constexpr const char* webuiConfigInterfaceName =
            "xyz.openbmc_project.WebUI.Configuration";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                webuiConfigInterfaceName,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldTitleTemplate)
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/",
            DBUS_QUERY_CRIT_IFACES(webuiConfigInterfaceName),
            noDepth,
            settingsServiceName
        )
        /* clang-format on */
    };

  public:
    Settings() : Entity(), query(std::make_shared<Query>())
    {}
    ~Settings() override = default;

  protected:
    ENTITY_DECL_QUERY(query)

  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
