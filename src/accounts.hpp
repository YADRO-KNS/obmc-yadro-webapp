// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/entity/proxy_query.hpp>
#include <core/helpers/utils.hpp>
#include <formatters.hpp>
#include <roles.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;
using namespace app::helpers::utils;

class Accounts final :
    public Collection,
    public CachedSource,
    public NamedEntity<Accounts>
{
  public:
    enum class AccountType
    {
        redfish,
        snpm,
        hostConsole,
        managerConsole,
        ipmi,
        kvmip,
        virtualMedia,
        webui,
        unknown
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD_DEF(bool, RemoteUser, false)
    ENTITY_DECL_FIELD_DEF(bool, UserEnabled, false)
    ENTITY_DECL_FIELD_DEF(bool, UserLockedForFailedAttempt, false)
    ENTITY_DECL_FIELD_DEF(bool, UserPasswordExpired, false)
    ENTITY_DECL_FIELD(std::string, UserPrivilege)
    ENTITY_DECL_FIELD_DEF(std::vector<int>, UserGroups, {})

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* userManagerSvc =
            "xyz.openbmc_project.User.Manager";
        static constexpr const char* userAttrsIface =
            "xyz.openbmc_project.User.Attributes";
        class FormatType : public query::dbus::IFormatter
        {
          public:
            ~FormatType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, AccountType> types{
                    {"redfish", AccountType::redfish},
                    {"ipmi", AccountType::ipmi},
                    {"ssh", AccountType::managerConsole},
                    {"web", AccountType::webui},
                    {"kvm", AccountType::kvmip},
                };
                std::vector<int> result;
                if (!std::holds_alternative<std::vector<std::string>>(value))
                {
                    return result;
                }
                const auto& groups = std::get<std::vector<std::string>>(value);
                for (const auto& group : groups)
                {
                    auto enumVal = formatValueFromDict(types, property, group,
                                                       AccountType::unknown);
                    if (std::get<int>(enumVal) !=
                        static_cast<int>(AccountType::unknown))
                    {
                        result.emplace_back(std::get<int>(enumVal));
                    }
                }
                return result;
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                userAttrsIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldRemoteUser),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldUserEnabled),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldUserLockedForFailedAttempt),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldUserPasswordExpired),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldUserPrivilege),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldUserGroups,
                                        DBUS_QUERY_EP_CSTR(FormatType)),
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/user",
            DBUS_QUERY_CRIT_IFACES(userAttrsIface),
            nextOneDepth,
            userManagerSvc
        )
        /* clang-format on */
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            this->setName(instance);
        }
        void setName(const DBusInstancePtr& instance) const
        {
            const auto& objectPath = instance->getObjectPath();
            setFieldId(instance,
                       getNameFromLastSegmentObjectPath(objectPath, false));
            setFieldName(instance,
                         getNameFromLastSegmentObjectPath(objectPath));
        }
    };

  public:
    Accounts() : Collection(), query(std::make_shared<Query>())
    {
        this->createMember(fieldId);
        this->createMember(fieldName);
    }
    ~Accounts() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATIONS(
        ENTITY_DEF_RELATION(Roles, Roles::roleDestRelation(fieldUserPrivilege)))
  private:
    DBusQueryPtr query;
};

class DomainGroups final :
    public Collection,
    public CachedSource,
    public NamedEntity<DomainGroups>
{
  public:
    enum class Type
    {
        activeDirectory,
        openLdap,
        unknown
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, GroupName)
    ENTITY_DECL_FIELD(std::string, PrivilegeId)
    ENTITY_DECL_FIELD_ENUM(Type, Type, unknown)

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* privConfigIface =
            "xyz.openbmc_project.User.PrivilegeMapperEntry";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                privConfigIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldGroupName),
                DBUS_QUERY_EP_FIELDS_ONLY("Privilege", fieldPrivilegeId),
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/user/",
            DBUS_QUERY_CRIT_IFACES(privConfigIface),
            noDepth,
            std::nullopt
        )
        /* clang-format on */
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            this->setId(instance);
            this->setType(instance);
        }
        void setId(const DBusInstancePtr& instance) const
        {
            const auto& objectPath = instance->getObjectPath();
            setFieldId(instance, getNameFromLastSegmentObjectPath(objectPath));
        }
        void setType(const DBusInstancePtr& instance) const
        {
            static const std::map<std::string, Type> types{
                {"/openldap/", Type::openLdap},
                {"/active_directory/", Type::activeDirectory},
            };
            const auto& objectPath = instance->getObjectPath();
            for (const auto& [segment, type] : types)
            {
                if (objectPath.find(segment) != std::string::npos)
                {
                    setFieldType(instance, type);
                    return;
                }
            }
        }
    };

  public:
    DomainGroups() : Collection(), query(std::make_shared<Query>())
    {
        this->createMember(fieldId);
        this->createMember(fieldType);
    }
    ~DomainGroups() override = default;

    static const ConditionPtr adGroups()
    {
        return Condition::buildEqual(fieldType, Type::activeDirectory);
    }

    static const ConditionPtr ldapGroups()
    {
        return Condition::buildEqual(fieldType, Type::openLdap);
    }

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATIONS(
        ENTITY_DEF_RELATION(Roles, Roles::roleDestRelation(fieldPrivilegeId)))
  private:
    DBusQueryPtr query;
};

class DomainAccounts final :
    public Collection,
    // Some properties of '.Ldap.Config' service does not emits change-signals.
    // The reason is not determined.
    // Bad workaround by caching for 2 seconds...
    public ShortTimeCachedSource<2>,
    public NamedEntity<DomainAccounts>
{
  public:
    enum class State
    {
        enabled,
        standbyOffline
    };
    enum class Type
    {
        activeDirectory,
        openLdap,
        unknown
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, LDAPServerURI, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, LDAPBaseDN, {})
    ENTITY_DECL_FIELD(std::string, LDAPBindDN)
    ENTITY_DECL_FIELD(std::string, LDAPSearchScope)
    ENTITY_DECL_FIELD(std::string, GroupNameAttribute)
    ENTITY_DECL_FIELD(std::string, UserNameAttribute)
    ENTITY_DECL_FIELD_DEF(bool, Enabled, false)
    ENTITY_DECL_FIELD_ENUM(State, State, standbyOffline)
    ENTITY_DECL_FIELD_ENUM(Type, Type, unknown)

  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* ldapConfigIface =
            "xyz.openbmc_project.User.Ldap.Config";
        static constexpr const char* enableIface =
            "xyz.openbmc_project.Object.Enable";
        class FormatState : public query::dbus::IFormatter
        {
          public:
            ~FormatState() override = default;

            const DbusVariantType format(const PropertyName&,
                                         const DbusVariantType& value) override
            {
                if (std::holds_alternative<bool>(value) &&
                    std::get<bool>(value))
                {
                    return static_cast<int>(State::enabled);
                }
                return static_cast<int>(State::standbyOffline);
            }
        };
        class FormatType : public query::dbus::IFormatter
        {
            static inline const std::string dbusEnum(const std::string& val)
            {
                return "xyz.openbmc_project.User.Ldap.Config.Type." + val;
            }

          public:
            ~FormatType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, Type> types{
                    {dbusEnum("OpenLdap"), Type::openLdap},
                    {dbusEnum("ActiveDirectory"), Type::activeDirectory},
                };

                return formatValueFromDict(types, property, value,
                                           Type::unknown);
            }
        };
        class FormatList : public query::dbus::IFormatter
        {
          public:
            ~FormatList() override = default;

            const DbusVariantType format(const PropertyName&,
                                         const DbusVariantType& value) override
            {
                std::vector<std::string> list;
                if (std::holds_alternative<std::string>(value))
                {
                    const auto& item = std::get<std::string>(value);
                    if (!item.empty())
                    {
                        list.emplace_back(item);
                    }
                }
                return list;
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                ldapConfigIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldLDAPBindDN),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldLDAPSearchScope),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldGroupNameAttribute),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldUserNameAttribute),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldLDAPBaseDN,
                                        DBUS_QUERY_EP_CSTR(FormatList)),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldLDAPServerURI,
                                        DBUS_QUERY_EP_CSTR(FormatList)),
                DBUS_QUERY_EP_SET_FORMATTERS("LDAPType", fieldType,
                                        DBUS_QUERY_EP_CSTR(FormatType))
            ),
            DBUS_QUERY_EP_IFACES(
                enableIface,
                DBUS_QUERY_EP_SET_FORMATTERS("Enabled", fieldState,
                                         DBUS_QUERY_EP_CSTR(FormatState))
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/user/ldap/",
            DBUS_QUERY_CRIT_IFACES(ldapConfigIface),
            nextOneDepth,
            std::nullopt
        )
        /* clang-format on */
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            this->setName(instance);
            this->setEnabled(instance);
        }
        void setName(const DBusInstancePtr& instance) const
        {
            const auto& objectPath = instance->getObjectPath();
            setFieldId(instance,
                       getNameFromLastSegmentObjectPath(objectPath, false));
            setFieldName(instance,
                         getNameFromLastSegmentObjectPath(objectPath));
        }
        void setEnabled(const DBusInstancePtr& instance) const
        {
            setFieldEnabled(instance,
                            getFieldState(instance) == State::enabled);
        }
    };

    static const IRelation::RelationRulesList& groupsRelation()
    {
        static const IRelation::RelationRulesList relations{
            {
                fieldType,
                DomainGroups::fieldType,
                [](const auto& groupTypeInstance, const auto& accType) -> bool {
                    if (!std::holds_alternative<int>(accType) ||
                        !std::holds_alternative<int>(
                            groupTypeInstance->getValue()))
                    {
                        log<level::WARNING>(
                            "Account groups relation: invalid type of field "
                            "index fields(DomainGroups::fieldType, fieldType)");
                        return false;
                    }
                    return groupTypeInstance->getIntValue() ==
                           std::get<int>(accType);
                },
            },
        };
        return relations;
    }

  public:
    DomainAccounts() : Collection(), query(std::make_shared<Query>())
    {
        this->createMember(fieldId);
        this->createMember(fieldName);
        this->createMember(fieldEnabled);
    }
    ~DomainAccounts() override = default;

    static const ConditionPtr adAccounts()
    {
        return Condition::buildEqual(fieldType, Type::activeDirectory);
    }

    static const ConditionPtr openLdapAccounts()
    {
        return Condition::buildEqual(fieldType, Type::openLdap);
    }

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATIONS(ENTITY_DEF_RELATION(DomainGroups, groupsRelation()))
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
