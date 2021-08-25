// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <logger/logger.hpp>
#include <core/exceptions.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>

#include <definitions.hpp>

namespace app
{
namespace query
{
namespace obmc
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;

using namespace std::placeholders;
using namespace app::entity::obmc::definitions;

class Settings final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* settingsServiceName =
        "xyz.openbmc_project.Settings";
    static constexpr const char* webuiConfigInterfaceName =
        "xyz.openbmc_project.WebUI.Configuration";
    static constexpr const char* namePropertyTitleTemplate = "TitleTemplate";

  public:
    Settings() : dbus::FindObjectDBusQuery()
    {}
    ~Settings() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                webuiConfigInterfaceName,
                {
                    {
                        namePropertyTitleTemplate,
                        settings::fieldTitleTemplate,
                    },
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/",
            {
                webuiConfigInterfaceName,
            },
            noDepth,
            settingsServiceName,
        };

        return criteria;
    }
};

} // namespace obmc
} // namespace query
} // namespace app

