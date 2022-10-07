// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/entity/proxy_query.hpp>
#include <core/helpers/utils.hpp>
#include <formatters.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::query;
using namespace app::query::dbus;
using namespace app::query::proxy;
using namespace app::service::session;

class AccountSettingsProvider final :
    public EntitySupplementProvider,
    public ISingleton<AccountSettingsProvider>,
    public CachedSource,
    public NamedEntity<AccountSettingsProvider>
{
  public:
    ENTITY_DECL_FIELD_DEF(uint32_t, AccountLockoutDuration, 0U)
    ENTITY_DECL_FIELD_DEF(uint16_t, AccountLockoutThreshold, 0U)
    ENTITY_DECL_FIELD_DEF(uint8_t, MinPasswordLength, 0U)
    ENTITY_DECL_FIELD_DEF(uint8_t, MaxPasswordLength, 0U)
    ENTITY_DECL_FIELD_DEF(uint8_t, RememberOldPasswordTimes, 0U)
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, AllGroups, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, AllPrivileges, {})
    ENTITY_DECL_FIELD_DEF(bool, BasicAuth, false)
    ENTITY_DECL_FIELD_DEF(bool, Cookie, false)
    ENTITY_DECL_FIELD_DEF(bool, SessionToken, false)
    ENTITY_DECL_FIELD_DEF(bool, TLS, false)
    ENTITY_DECL_FIELD_DEF(bool, XToken, false)
  private:
    class Query final : public GetObjectDBusQuery
    {
        static constexpr const char* service =
            "xyz.openbmc_project.User.Manager";
        static constexpr const char* object = "/xyz/openbmc_project/user";
        static constexpr const char* policyIface =
            "xyz.openbmc_project.User.AccountPolicy";
        static constexpr const char* managerIface =
            "xyz.openbmc_project.User.Manager";

      public:
        Query() : GetObjectDBusQuery(service, object)
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                policyIface,
                DBUS_QUERY_EP_FIELDS_ONLY("AccountUnlockTimeout",
                                        fieldAccountLockoutDuration),
                DBUS_QUERY_EP_FIELDS_ONLY("MaxLoginAttemptBeforeLockout",
                                        fieldAccountLockoutThreshold),
                DBUS_QUERY_EP_FIELDS_ONLY("MinPasswordLength",
                                        fieldMinPasswordLength),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldRememberOldPasswordTimes),
            ),
            DBUS_QUERY_EP_IFACES(
                managerIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldAllGroups),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldAllPrivileges),
            )
        )

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                policyIface,
                managerIface,
            };
            return interfaces;
        }
        /* clang-format on */
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            setFieldMaxPasswordLength(instance, 20U);
        }
    };

  public:
    AccountSettingsProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {
        this->createMember(fieldMaxPasswordLength);
        this->createMember(fieldBasicAuth);
        this->createMember(fieldCookie);
        this->createMember(fieldSessionToken);
        this->createMember(fieldTLS);
        this->createMember(fieldXToken);
    }
    ~AccountSettingsProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class AccountSettings final :
    public Entity,
    public LazySource,
    public NamedEntity<AccountSettings>
{
  public:
    AccountSettings() :
        Entity(), query(std::make_shared<ProxyQuery>(
                      AccountSettingsProvider::getSingleton()))
    {}
    ~AccountSettings() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    ProxyQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
