// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <phosphor-logging/log.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::query::dbus;
using namespace phosphor::logging;

using StringFormatterDict = std::map<std::string, std::string>;

static const DbusVariantType
    formatStringValueFromDict(const StringFormatterDict& formatterDict,
                              const PropertyName& propertyName,
                              const DbusVariantType& value)
{
    auto formattedValue = std::visit(
        [propertyName,
         &formatterDict](auto&& property) -> const DbusVariantType {
            using TProperty = std::decay_t<decltype(property)>;

            if constexpr (std::is_same_v<TProperty, std::string>)
            {
                auto findTypeIt = formatterDict.find(property);
                if (findTypeIt == formatterDict.end())
                {
                    throw app::core::exceptions::InvalidType(propertyName);
                }

                return DbusVariantType(findTypeIt->second);
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
    static constexpr const char* chassisTypeOther = "Other";

  public:
    ~DBusEnumFormatter() override = default;

    const DbusVariantType format(const PropertyName& property,
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

} // namespace entity
} // namespace obmc
} // namespace app
