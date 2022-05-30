// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>
#include <common_fields.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::entity;
using namespace app::query;
using namespace app::helpers;
using namespace relations;

class StatusProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<StatusProvider>,
    public LazySource,
    public NamedEntity<StatusProvider>
{
  public:
    static constexpr const char* OK = "OK";
    static constexpr const char* warning = "Warning";
    static constexpr const char* critical = "Critical";

    static constexpr const char* fieldStatus = "Status";
    static constexpr const char* fieldObjectCauthPath = "Causer";

    StatusProvider() : EntitySupplementProvider()
    {
    }
    ~StatusProvider() override = default;

    static const std::string getHigherStatus(const std::string& left,
                                             const std::string& right)
    {
        constexpr const char* compStatusOK = "ok";
        constexpr const char* compStatusWarning = "warning";
        constexpr const char* compStatusCritical = "critical";

        static std::map<std::string, std::pair<uint8_t, std::string>>
            statusPriority{
                {compStatusOK, {0, OK}},
                {compStatusWarning, {1, warning}},
                {compStatusCritical, {2, critical}},
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
                "The one of comparing status fields has an unknown "
                "value. "
                "Left=" +
                left + ", Right=" + right);
        }
    }
};

} // namespace entity
} // namespace obmc
} // namespace app
