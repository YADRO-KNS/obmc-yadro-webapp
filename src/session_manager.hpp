// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/helpers/utils.hpp>
#include <core/entity/proxy_query.hpp>
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

class SessionManager final :
    public Collection,
    public LazySource,
    public NamedEntity<SessionManager>
{
  public:
    enum class SessionType
    {
        hostConsole,
        ipmi,
        kvmip,
        managerConsole,
        redfish,
        virtualMedia,
        webui,
        nbd,
        unknown
    };
    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, RemoteIPAddr)
    ENTITY_DECL_FIELD(std::string, Username)
    ENTITY_DECL_FIELD_ENUM(SessionType, SessionType, unknown)
  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* sessionManagerService =
            "xyz.openbmc_project.SessionManager";
        static constexpr const char* sessionIface =
            "xyz.openbmc_project.Session.Item";

        class FormatType : public query::dbus::IFormatter
        {
            static inline const std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.Session.Item.Type." + val;
            }
          public:
            ~FormatType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, SessionType> types{
                    {dbusEnum("HostConsole"), SessionType::hostConsole},
                    {dbusEnum("IPMI"), SessionType::ipmi},
                    {dbusEnum("KVMIP"), SessionType::kvmip},
                    {dbusEnum("ManagerConsole"), SessionType::managerConsole},
                    {dbusEnum("Redfish"), SessionType::redfish},
                    {dbusEnum("VirtualMedia"), SessionType::virtualMedia},
                    {dbusEnum("WebUI"), SessionType::webui},
                    {dbusEnum("NBD"), SessionType::nbd},
                };

                return formatValueFromDict(types, property, value, SessionType::unknown);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                sessionIface,
                DBUS_QUERY_EP_FIELDS_ONLY("SessionID", fieldId),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldRemoteIPAddr),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldUsername),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldSessionType,
                                        DBUS_QUERY_EP_CSTR(FormatType))
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/session_manager/",
            DBUS_QUERY_CRIT_IFACES(sessionIface),
            noDepth,
            sessionManagerService
        )
        /* clang-format on */
    };

  public:
    SessionManager() : Collection(), query(std::make_shared<Query>())
    {
    }
    ~SessionManager() override = default;

  protected:
    ENTITY_DECL_QUERY(query)

  private:
    DBusQueryPtr query;
};

}
} // namespace obmc
} // namespace app
