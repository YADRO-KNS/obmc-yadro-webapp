// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __VERSION_PROVIDER_H__
#define __VERSION_PROVIDER_H__

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

class Version final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* versionInterfaceName =
        "xyz.openbmc_project.Software.Version";
    static constexpr const char* namePropertyVersion = "Version";
    static constexpr const char* namePropertyPurpose = "Purpose";

    static constexpr const char* purposeBios =
        "xyz.openbmc_project.Software.Version.VersionPurpose.Host";
    static constexpr const char* purposeBmc =
        "xyz.openbmc_project.Software.Version.VersionPurpose.BMC";
  public:
    enum class VersionPurpose: uint8_t {
        BMC,
        BIOS,
        Unknown
    };

    static VersionPurpose getPurpose(const IEntity::InstancePtr& instance)
    {
        static std::map<std::string, VersionPurpose> purposes{
            {purposeBmc, VersionPurpose::BMC},
            {purposeBios, VersionPurpose::BIOS},
        };

        auto inputPurpose =
            instance->getField(supplement_providers::version::fieldPurpose)
                ->getStringValue();
        auto findPurposeIt = purposes.find(inputPurpose);
        if (findPurposeIt == purposes.end())
        {
            return VersionPurpose::Unknown;
        }

        return findPurposeIt->second;
    }

    Version() : dbus::FindObjectDBusQuery()
    {}
    ~Version() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                versionInterfaceName,
                {
                    {
                        namePropertyVersion,
                        supplement_providers::version::fieldVersion,
                    },
                    {
                        namePropertyPurpose,
                        supplement_providers::version::fieldPurpose,
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
            "/xyz/openbmc_project/software",
            {
                versionInterfaceName,
            },
            nextOneDepth,
            std::nullopt,
        };

        return criteria;
    }
};

} // namespace obmc
} // namespace query
} // namespace app

#endif // __VERSION_PROVIDER_H__
