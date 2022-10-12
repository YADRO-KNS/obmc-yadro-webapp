// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <account_settings.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/helpers/utils.hpp>

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

class Roles final :
    public Collection,
    public LazySource,
    public NamedEntity<Roles>
{
    static constexpr const char* guestRoleName = "Guest";

  public:
    enum class Privileges
    {
        login,
        configureManager,
        configureUsers,
        configureSelf,
        configureComponents,
        noAuth
    };
    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD_DEF(std::vector<int>, Privileges, {})
  private:
    class RolesProxyQuery : public IQuery, public ISingleton<RolesProxyQuery>
    {
        const EntitySupplementProviderPtr provider;

      public:
        explicit RolesProxyQuery(const EntitySupplementProviderPtr& provider) :
            provider(provider)
        {}
        ~RolesProxyQuery() override = default;

        const IEntity::InstanceCollection process() override
        {
            static const std::map<std::string, std::string> privToRoles{
                {"priv-admin", "Administrator"},
                {"priv-operator", "Operator"},
                {"priv-user", "ReadOnly"},
            };

            IEntity::InstanceCollection instances;
            provider->populate();
            for (const auto accInstance : provider->getInstances())
            {
                const auto& privs =
                    AccountSettingsProvider::getFieldAllPrivileges(accInstance);
                for (const auto& priv : privs)
                {
                    auto roleIt = privToRoles.find(priv);
                    if (roleIt == privToRoles.end())
                    {
                        continue;
                    }
                    auto instance = std::make_shared<StaticInstance>(priv);
                    setFieldId(instance, priv);
                    setFieldName(instance, roleIt->second);
                    this->setPrivileges(instance);
                    instances.push_back(instance);
                }
            }
            this->addStaticRoles(instances);
            return instances;
        }

        /**
         * @brief Get the collection of query fields
         *
         * @return const QueryFields - The collection of fields
         */
        const QueryFields getFields() const override
        {
            return {
                fieldId,
                fieldName,
                fieldPrivileges,
            };
        }

      protected:
        void addStaticRoles(IEntity::InstanceCollection& instances)
        {
            auto instance = std::make_shared<StaticInstance>(guestRoleName);
            setFieldName(instance, guestRoleName);
            setFieldId(instance, guestRoleName);
            this->setPrivileges(instance);
            instances.push_back(instance);
        }
        void setPrivileges(const InstancePtr& instance)
        {
            static const std::map<std::string, std::vector<int>> privToRoles{
                {
                    "Administrator",
                    privsToList<Privileges::login, Privileges::configureManager,
                                Privileges::configureUsers,
                                Privileges::configureSelf,
                                Privileges::configureComponents>(),
                },
                {
                    "Operator",
                    privsToList<Privileges::login, Privileges::configureSelf,
                                Privileges::configureComponents>(),
                },
                {
                    "ReadOnly",
                    privsToList<Privileges::login, Privileges::configureSelf>(),
                },
                {
                    guestRoleName,
                    privsToList<Privileges::noAuth>(),
                },
            };
            setFieldPrivileges(instance,
                               privToRoles.at(getFieldName(instance)));
        }
        template <Privileges... priv>
        inline static const std::vector<int> privsToList()
        {
            return {static_cast<int>(priv)...};
        }
    };

  public:
    Roles() :
        Collection(), query(std::make_shared<RolesProxyQuery>(
                          AccountSettingsProvider::getSingleton()))
    {}
    ~Roles() override = default;
    static const ConditionPtr nonGuest()
    {
        return Condition::buildNonEqual<std::string>(fieldName, guestRoleName);
    }
    static const IRelation::RelationRulesList
        roleDestRelation(const std::string& destField)
    {
        return std::forward<IRelation::RelationRulesList>({
            {
                destField,
                Roles::fieldId,
                [destField](const auto& roleIdInstance,
                            const auto& userPrivValue) -> bool {
                    if (!std::holds_alternative<std::string>(userPrivValue) ||
                        !std::holds_alternative<std::string>(
                            roleIdInstance->getValue()))
                    {
                        log<level::WARNING>(
                            "Account role relation: invalid type of field "
                            "index fields",
                            entry("DEST_FIELD=%s", destField.c_str()),
                            entry("ROLE_FIELD=%s", Roles::fieldId));
                        return false;
                    }
                    return roleIdInstance->getStringValue() ==
                           std::get<std::string>(userPrivValue);
                },
            },
        });
    }

  protected:
    ENTITY_DECL_QUERY(query)

  private:
    std::shared_ptr<RolesProxyQuery> query;
};

} // namespace entity
} // namespace obmc
} // namespace app
