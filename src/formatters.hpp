// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <phosphor-logging/log.hpp>
#include <core/helpers/utils.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::query::dbus;
using namespace phosphor::logging;
using namespace app::helpers::utils;

using StringFormatterDict = std::map<std::string, std::string>;

template<typename TRawValue, typename TResolvedValue>
static const DbusVariantType
    formatValueFromDict(const std::map<TRawValue, TResolvedValue>& formatterDict,
                              const PropertyName& propertyName,
                              const DbusVariantType& value,
                              const TResolvedValue defaultValue)
{
    auto formattedValue = std::visit(
        [propertyName,
         &formatterDict, defaultValue](auto&& property) -> const DbusVariantType {
            using TProperty = std::decay_t<decltype(property)>;

            if constexpr (std::is_same_v<TProperty, TRawValue>)
            {
                auto findTypeIt = formatterDict.find(property);
                auto value = defaultValue;
                if (findTypeIt != formatterDict.end())
                {
                    value = findTypeIt->second;
                }

                if constexpr (std::is_enum_v<TResolvedValue>)
                {
                    return DbusVariantType(static_cast<int>(value));
                }
                else
                {
                    return DbusVariantType(value);
                }
            }

            throw app::core::exceptions::InvalidType(propertyName);
        },
        value);
    return formattedValue;
}

/**
 * @class DBusEnumFormatter
 * @brief Format DBus enum (string representation via full namespace) to the
 *        string that providing last segment of full namepsace enum
 *
 */
class DBusEnumFormatter : public query::dbus::IFormatter
{
  public:
    ~DBusEnumFormatter() override = default;

    const DbusVariantType format(const PropertyName&,
                                 const DbusVariantType& value) override
    {
        try
        {
            const auto& protocol = std::get<std::string>(value);
            auto lastSegmenPos = protocol.find_last_of('.');
            if (lastSegmenPos == std::string::npos)
            {
                throw std::invalid_argument("Invalid DBus enum format");
            }
            return DbusVariantType(protocol.substr(lastSegmenPos + 1));
        }
        catch (const std::exception& e)
        {
            log<level::ERR>("Failure formatting DBus value",
                            entry("DESC=Enum cast"),
                            entry("ERROR=%s", e.what()));
        }
        return std::string(Entity::EntityMember::fieldValueNotAvailable);
    }
};
template<typename TNumber = uint64_t>
class StrHexToNumberFormatter : public query::dbus::IFormatter
{
  public:
    ~StrHexToNumberFormatter() override = default;

    const DbusVariantType format(const PropertyName& property,
                                 const DbusVariantType& value) override
    {
        if (!std::holds_alternative<std::string>(value))
        {
            log<level::ERR>("Invalid DbusVariantType to casting hex to number",
                            entry("PROPERTY=%S", property.c_str()));
            return TNumber(0);
        }
        const auto strValue = std::get<std::string>(value);
        try {
            std::cerr << "HexToNumber: " << strValue << std::endl;
            const auto numValue = std::stoul(strValue, nullptr, 16);
            return static_cast<TNumber>(numValue);
        }
        catch (std::logic_error& ex)
        {
            log<level::ERR>("Error while casting hex to number",
                            entry("PROPERTY=%S", property.c_str()),
                            entry("ERROR=%s", ex.what()));
        }
        return TNumber(0);
    }
};

class TrimFormatter : public query::dbus::IFormatter
{
  public:
    ~TrimFormatter() override = default;

    const DbusVariantType format(const PropertyName& property,
                                 const DbusVariantType& value) override
    {
        if (!std::holds_alternative<std::string>(value))
        {
            throw std::invalid_argument("The property '" + property + "' type must be literal");
        }

        const auto strValue = std::get<std::string>(value);
        return trim(strValue);
    }
};

class FormatTime : public query::dbus::IFormatter
{
  public:
    ~FormatTime() override = default;

    const DbusVariantType format(const PropertyName& property,
                                 const DbusVariantType& value) override
    {
        if (!std::holds_alternative<uint64_t>(value))
        {
            log<level::DEBUG>("Can't cast non uint64_t type to GTM datetime",
                              entry("FIELD=%s", property.c_str()));
            return std::string("0000-00-00T00:00:00Z00:00");
        }
        time_t lastResetTimeStamp =
            static_cast<time_t>(std::get<uint64_t>(value));
        return getFormattedDate("%FT%T%z", lastResetTimeStamp);
    }
};

class FormatTimeMsToSec : public FormatTime
{
  public:
    ~FormatTimeMsToSec() override = default;

    const DbusVariantType format(const PropertyName& property,
                                 const DbusVariantType& value) override
    {
        if (!std::holds_alternative<uint64_t>(value))
        {
            log<level::DEBUG>("Can't cast non uint64_t type to GTM datetime",
                              entry("FIELD=%s", property.c_str()));
            return std::string("0000-00-00T00:00:00Z00:00");
        }
        uint64_t valueSec = std::get<uint64_t>(value) / 1000;
        return FormatTime::format(property, valueSec);
    }
};

} // namespace entity
} // namespace obmc
} // namespace app
