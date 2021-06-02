// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __RELATION_PROVIDER_H__
#define __RELATION_PROVIDER_H__

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/compares.hpp>
#include <definitions.hpp>
#include <logger/logger.hpp>

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

class Relation final : public dbus::IntrospectServiceDBusQuery
{
    static constexpr const char* serviceName =
        "xyz.openbmc_project.ObjectMapper";
    static constexpr const char* interfaceName =
        "xyz.openbmc_project.Association";
    static constexpr const char* namePropertyEndpoint = "endpoints";

  public:
    Relation() : dbus::IntrospectServiceDBusQuery(serviceName)
    {}
    ~Relation() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                interfaceName,
                {{
                    namePropertyEndpoint,
                    app::entity::obmc::definitions::supplement_providers::
                        relations::fieldEndpoint,
                }},
            },
        };

        return dictionary;
    }
};

namespace general_relations
{
} // namespace general_relations

} // namespace obmc
} // namespace query
} // namespace app

#endif // __RELATION_PROVIDER_H__
