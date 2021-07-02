// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __STATUS_PROVIDER_H__
#define __STATUS_PROVIDER_H__

#include <logger/logger.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>

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
using namespace app::helpers;

class Status final : public dbus::GetObjectDBusQuery
{
    static constexpr const char* callbackService =
        "xyz.openbmc_project.CallbackManager";
    static constexpr const char* statusSensorObjectPath =
        "/xyz/openbmc_project/sensors";
    static constexpr const char* definitionInterface =
        "xyz.openbmc_project.Association.Definitions";
    static constexpr const char* namePropertyAssociations = "Associations";

  public:
    static constexpr const char* statusOK = "OK";
    static constexpr const char* statusWarning = "Warning";
    static constexpr const char* statusCritical = "Critical";

    Status() : dbus::GetObjectDBusQuery(callbackService, statusSensorObjectPath)
    {}
    ~Status() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        using namespace app::entity::obmc::definitions::supplement_providers;

        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                definitionInterface,
                {
                    {
                        namePropertyAssociations,
                        relations::fieldAssociations,
                    },
                },
            },
            {
                relations::fieldAssociations,
                {
                    {
                        relations::fieldSource,
                        status::fieldStatus,
                    },
                    {
                        relations::fieldEndpoint,
                        status::fieldObjectCauthPath,
                    },
                },
            },
        };

        return dictionary;
    }

    static const std::string getHigherStatus(const std::string& left,
                                              const std::string& right)
    {
        constexpr const char* compStatusOK = "ok";
        constexpr const char* compStatusWarning = "warning";
        constexpr const char* compStatusCritical = "critical";

        static std::map<std::string, std::pair<uint8_t, std::string>>
            statusPriority{
                {compStatusOK, {0, statusOK}},
                {compStatusWarning, {1, statusWarning}},
                {compStatusCritical, {2, statusCritical}},
            };

        try
        {
            auto leftCompPair = statusPriority.at(utils::toLower(left));
            auto rightCompPair = statusPriority.at(utils::toLower(right));

            return (leftCompPair.first >= rightCompPair.first)
                       ? leftCompPair.second
                       : rightCompPair.second;
        }
        catch (std::out_of_range&)
        {
            throw app::core::exceptions::ObmcAppException(
                "The one of comparing status fields has an unknown value. "
                "Left=" + left + ", Right=" + right);
        }
    }
  protected:
    const DefaultFieldsValueDict& getDefaultFieldsValue() const
    {
        // Since the status object instances specify the attention statuses,
        // then have OK value as default if no one instance is found.
        using namespace app::entity::obmc::definitions::supplement_providers;
        static const DefaultFieldsValueDict defaultStatusOk{
            {
                status::fieldStatus,
                []() { return std::string(Status::statusOK); },
            },
        };
        return defaultStatusOk;
    }


    const InterfaceList& searchInterfaces() const override
    {
        static const InterfaceList interfaces{
            definitionInterface
        };

        return interfaces;
    }
};

} // namespace obmc
} // namespace query
} // namespace app

#endif // __STATUS_PROVIDER_H__
