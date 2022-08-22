// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>
#include <common_fields.hpp>
#include <formatters.hpp>

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

class StatusProvider: public virtual IEntity
{
  public:
    enum class Status {
        ok,
        warning,
        critical
    };

    ENTITY_DECL_FIELD_ENUM(Status, Status, ok)
    ENTITY_DECL_FIELD_ENUM(Status, StatusRollup, ok)
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, Causer, {})

    StatusProvider() = default;
    ~StatusProvider() override = default;

    static Status getHigherStatus(Status left, Status right)
    {
        if (static_cast<int>(left) >= static_cast<int>(right))
        {
            return left;
        }
        return right;
    }

    static Status getHigherStatus(const InstancePtr instance, Status right)
    {
        if (static_cast<int>(getFieldStatus(instance)) >= static_cast<int>(right))
        {
            return getFieldStatus(instance);
        }
        return right;
    }

    class StatusByFunctional : public query::dbus::IFormatter
    {
      public:
        ~StatusByFunctional() override = default;

        const DbusVariantType format(const PropertyName& property,
                                     const DbusVariantType& value) override
        {
            Status status = Status::ok;
            if (std::holds_alternative<bool>(value) && !std::get<bool>(value))
            {
                status = Status::warning;
            }
            return static_cast<int>(status);
        }
    };
};

class StatusRollup final
{
    const EntityPtr entity;

  public:
    StatusRollup(const EntityPtr& entity) : entity(entity)
    {
        if (!entity->hasMember(StatusProvider::fieldStatusRollup))
        {
            entity->createMember(StatusProvider::fieldStatusRollup);
        }
    }
    ~StatusRollup() = default;

    IEntity::IEntityMember::IInstance::FieldType
        operator()(const IEntity::InstancePtr& instance)
    {
        StatusProvider::Status result = StatusProvider::Status::ok;
        const auto relations = entity->getRelations();
        for (const auto& relation : relations)
        {
            const auto dest = relation->getDestinationTarget();
            if (!dest->hasMember(StatusProvider::fieldStatus))
            {
                continue;
            }
            const auto instancesOfStatus =
                instance->getRelatedInstances(relation, {}, true);
            for (const auto instance : instancesOfStatus)
            {
                result = StatusProvider::getHigherStatus(instance, result);
                if (dest->hasMember(StatusProvider::fieldStatusRollup))
                {
                    result = StatusProvider::getHigherStatus(
                        StatusProvider::getFieldStatusRollup(instance), result);
                }
            }
        }
        return static_cast<int>(result);
    }

    template <typename TEntity>
    static std::pair<MemberName, DBusQuery::DefaultValueSetter> defaultGetter()
    {
        return {StatusProvider::fieldStatusRollup,
                StatusRollup(TEntity::getEntity())};
    }
    static std::pair<MemberName, DBusQuery::DefaultValueSetter>
        defaultGetter(const EntityPtr entity)
    {
        return {StatusProvider::fieldStatusRollup, StatusRollup(entity)};
    }
};

class StatusFromRollup final
{
  public:
    StatusFromRollup(const EntityPtr& entity)
    {
        if (!entity->hasMember(StatusProvider::fieldStatus))
        {
            entity->createMember(StatusProvider::fieldStatus);
        }
    }
    ~StatusFromRollup() = default;

    IEntity::IEntityMember::IInstance::FieldType
        operator()(const IEntity::InstancePtr& instance)
    {
        return static_cast<int>(StatusProvider::getFieldStatusRollup(instance));
    }

    template <typename TEntity>
    static std::pair<MemberName, DBusQuery::DefaultValueSetter> defaultGetter()
    {
        return {StatusProvider::fieldStatus,
                StatusFromRollup(TEntity::getEntity())};
    }
    static std::pair<MemberName, DBusQuery::DefaultValueSetter>
        defaultGetter(const EntityPtr entity)
    {
        return {StatusProvider::fieldStatus, StatusFromRollup(entity)};
    }
};

template <typename TTargetEntity>
class StatusByCallbackManager final :
    public virtual StatusProvider,
    public entity::EntitySupplementProvider,
    public ISingleton<StatusByCallbackManager<TTargetEntity>>,
    public CachedSource,
    public NamedEntity<StatusByCallbackManager<TTargetEntity>>
{
    private:
    class Query final : public GetObjectDBusQuery
    {
        static constexpr const char* globalIface =
            "xyz.openbmc_project.Inventory.Item.Global";
        static constexpr const char* assocIface =
            "xyz.openbmc_project.Association.Definitions";
        static constexpr const char* cbManagerService =
            "xyz.openbmc_project.CallbackManager";
        static constexpr const char* cbManagerGlobalPath =
            "/xyz/openbmc_project/CallbackManager";

        static constexpr const char* compStatusOK = "ok";
        static constexpr const char* compStatusWarning = "warning";
        static constexpr const char* compStatusCritical = "critical";
        class BaseStatusFormatter : public query::dbus::IFormatter
        {
            const std::vector<std::string> observedCausers;

          public:
            BaseStatusFormatter(
                const std::vector<std::string>& observedCausers) :
                observedCausers(observedCausers)
            {}
            ~BaseStatusFormatter() override = default;

          protected:
            inline bool isCauserObserved(const std::string& causer)
            {
                if (!observedCausers.empty())
                {
                    return std::any_of(observedCausers.begin(),
                                       observedCausers.end(),
                                       [causer](const auto& value) {
                                           return causer == value;
                                       });
                }
                return true;
            }
        };

        class FormatCauser : public BaseStatusFormatter
        {
          public:
            FormatCauser(const std::vector<std::string>& observedCausers) :
                BaseStatusFormatter(observedCausers)
            {}
            ~FormatCauser() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                std::vector<std::string> causers;
                if (std::holds_alternative<DBusAssociationsType>(value))
                {
                    const auto associations =
                        std::get<DBusAssociationsType>(value);
                    for (const auto& association : associations)
                    {
                        const auto causer = std::get<2>(association);
                        if (this->isCauserObserved(causer))
                        {
                            causers.push_back(causer);
                        }
                    }
                }

                return causers;
            }
        };

        class FormatStatus : public BaseStatusFormatter
        {
          public:
            FormatStatus(const std::vector<std::string>& observedCausers) :
                BaseStatusFormatter(observedCausers)
            {}
            ~FormatStatus() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static std::map<std::string, Status> statusDict{
                    {compStatusOK, Status::ok},
                    {compStatusWarning, Status::warning},
                    {compStatusCritical, Status::critical},
                };
                Status result = Status::ok;
                if (std::holds_alternative<DBusAssociationsType>(value))
                {
                    const auto associations =
                        std::get<DBusAssociationsType>(value);
                    for (const auto& association : associations)
                    {
                        const std::string level = std::get<0>(association);
                        const auto causer = std::get<2>(association);
                        if (!this->isCauserObserved(causer))
                        {
                            continue;
                        }
                        Status currentStatus = static_cast<Status>(
                            std::get<int>(formatValueFromDict(
                                statusDict, property, level, Status::ok)));
                        result = StatusProvider::getHigherStatus(currentStatus,
                                                                 result);
                    }
                }

                return static_cast<int>(result);
            }
        };

      public:
        Query(): GetObjectDBusQuery(cbManagerService, cbManagerGlobalPath)
        {}
        ~Query() override = default;
        // clang-format: off
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                assocIface,
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "Associations", fieldStatus,
                    DBUS_QUERY_EP_CSTR(FormatStatus, TTargetEntity::observedCauserList())
                )
            )
        )
        // clang-format: on
      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                globalIface,
                assocIface,
            };
            return interfaces;
        }
    };

  public:
    StatusByCallbackManager() : query(std::make_shared<Query>())
    {
        this->createMember(fieldStatusRollup);
    }
    ~StatusByCallbackManager() override = default;

  protected:
    ENTITY_DECL_QUERY(query)

  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
