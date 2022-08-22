// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <phosphor-logging/log.hpp>
#include <core/exceptions.hpp>

#include <core/entity/entity_interface.hpp>
#include <core/entity/enitty_manager.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace app
{
namespace entity
{

using namespace phosphor::logging;

class EntitySupplementProvider;
class Collection;
using EntitySupplementProviderPtr = std::shared_ptr<EntitySupplementProvider>;
namespace exceptions
{

class EntityException : public core::exceptions::ObmcAppException
{
  public:
    explicit EntityException(const std::string& arg) :
        core::exceptions::ObmcAppException(arg)
    {}
    virtual ~EntityException() = default;
};

} // namespace exceptions

class Entity : virtual public IEntity
{
#define ENTITY_DECL_QUERY(...)                                                 \
    const query::QueryCollection& getQueries() const override                  \
    {                                                                          \
        static const query::QueryCollection queries{__VA_ARGS__};              \
        return queries;                                                        \
    }

#define ENTITY_PROVIDER_LINK(prv, link)                                        \
    {                                                                          \
        prv::getSingleton(), link                                              \
    }
#define ENTITY_PROVIDER_LINK_DEFAULT(prv)                                      \
    ENTITY_PROVIDER_LINK(prv, defaultLinkProvider)
#define ENTITY_DECL_PROVIDERS(...)                                             \
    const ProviderRulesDict& getProviders() const override                     \
    {                                                                          \
        static const Entity::ProviderRulesDict providers{__VA_ARGS__};         \
        return providers;                                                      \
    }
#define ENTITY_DECL_RELATION(dest, rule)                                       \
    const std::vector<IEntity::RelationPtr>& getRelations() const override     \
    {                                                                          \
        auto relation = std::make_shared<Relation>(                            \
            getEntityManager().getEntity(getName()),                           \
            getEntityManager().getEntity<dest>());                             \
        relation->addConditionBuildRules(rule);                                \
        static const Relations relations{relation};                            \
        return relations;                                                      \
    }

    using InstancesHashmap = std::map<InstanceHash, InstancePtr>;
    MemberMap members;
    InstancesHashmap instances;

  protected:
    using ProviderRule = std::pair<EntitySupplementProviderPtr,
                                   ISupplementProvider::ProviderLinkRule>;
    using ProviderRulesDict = std::vector<ProviderRule>;
    using MembersList = std::vector<MemberName>;

  public:
    class EntityMember : public IEntityMember
    {
      public:
        static constexpr const char* fieldValueNotAvailable = "N/A";
        class StaticInstance : public IEntityMember::IInstance
        {
            std::optional<FieldType> value;
          public:
            StaticInstance(const StaticInstance&) = delete;
            StaticInstance& operator=(const StaticInstance&) = delete;
            StaticInstance(StaticInstance&&) = delete;
            StaticInstance& operator=(StaticInstance&&) = delete;

            explicit StaticInstance() noexcept = default;
            explicit StaticInstance(const FieldType& fieldValue) noexcept :
                value(fieldValue)
            {
            }
            ~StaticInstance() override = default;

            const FieldType& getValue() const noexcept
            {
                static const FieldType na = std::string(fieldValueNotAvailable);
                return !isNull() ? value.value() : na;
            }
            void setValue(const FieldType& fieldValue)
            {
                value = fieldValue;
            }
            const std::string& getStringValue() const override
            {
                static const std::string naVal(fieldValueNotAvailable);
                return isNull() ? naVal : std::get<std::string>(getValue());
            }
            int getIntValue() const override
            {
                return isNull() ? 0 : std::get<int>(getValue());
            }
            double getFloatValue() const override
            {
                return isNull() ? 0.0 : std::get<double>(getValue());
            }
            bool getBoolValue() const override
            {
                return isNull() ? false : std::get<bool>(getValue());
            }
            bool isNull() const override
            {
                return !value.has_value();
            }
            const std::string getType() const override
            {
                if (isNull())
                {
                    return "Null";
                }
                auto visitCallback = [](auto&& value) -> const std::string {
                    using TProperty = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<std::string, TProperty>)
                    {
                        return "String";
                    }
                    else if constexpr (std::is_same_v<int, TProperty>)
                    {
                        return "Integer";
                    }
                    else if constexpr (std::is_same_v<double, TProperty>)
                    {
                        return "Double";
                    }
                    else if constexpr (std::is_same_v<bool, TProperty>)
                    {
                        return "Boolean";
                    }

                    return "Unknown";
                };
                return std::visit(std::move(visitCallback), getValue());
            }
        };

        explicit EntityMember(const std::string& memberName) noexcept :
            name(memberName), instance(std::make_shared<StaticInstance>(
                                  std::string(fieldValueNotAvailable)))
        {}

        explicit EntityMember() = delete;
        EntityMember(const EntityMember&) = delete;
        EntityMember& operator=(const EntityMember&) = delete;
        EntityMember(EntityMember&&) = delete;
        EntityMember& operator=(EntityMember&&) = delete;

        ~EntityMember() override = default;

        const MemberName getName() const noexcept override;

        const InstancePtr& getInstance() const override;

      private:
        const MemberName name;
        InstancePtr instance;
    };

    class Condition : public ICondition
    {
        using RuleMeta =
            std::pair<MemberName, IEntityMember::IInstance::FieldType>;
        using Rule = std::pair<RuleMeta, CompareCallback>;
        std::vector<Rule> rules;

      public:
        Condition(const Condition&) = delete;
        Condition& operator=(const Condition&) = delete;
        Condition(Condition&&) = delete;
        Condition& operator=(Condition&&) = delete;

        explicit Condition() = default;
        Condition(const MemberName& member,
                  const IEntityMember::IInstance::FieldType& value,
                  CompareCallback comparer)
        {
            addRule(member, value, comparer);
        }
        Condition(const MemberName& member, CustomCompareCallback comparer)
        {
            addRule(member, comparer);
        }
        Condition(const MemberName& member, CompareCallback comparer)
        {
            addRule(member,
                    [comparer](const IEntityMember::InstancePtr& instance) {
                        return comparer(instance, nullptr_t(nullptr));
                    });
        }
        ~Condition() override = default;

        void addRule(const MemberName&,
                     const IEntityMember::IInstance::FieldType&,
                     CompareCallback) override;

        void addRule(const MemberName&, CustomCompareCallback) override;
        bool check(const IEntity::IInstance&) const override;

        template <typename TRightValue>
        static const ConditionPtr buildEqual(const std::string& fieldName,
                                             TRightValue value)
        {
            if constexpr (std::is_enum_v<TRightValue>)
            {
                return std::make_shared<Condition>(
                    fieldName, static_cast<int>(value), Equal<TRightValue>());
            }
            else
            {
                return std::make_shared<Condition>(fieldName, value,
                                                   Equal<TRightValue>());
            }
        }
      protected:
        bool fieldValueCompare(const IEntity::IInstance&) const;
    };

    class Relation : public IRelation
    {
        const EntityWeak source;
        const EntityPtr destination;
        LinkWay linkWay;
        RelationRulesList conditionBuildRules;

      public:
        Relation(const Relation&) = delete;
        Relation& operator=(const Relation&) = delete;
        Relation(Relation&&) = delete;
        Relation& operator=(Relation&&) = delete;

        explicit Relation(const EntityWeak sourceEntity,
                          const EntityPtr destinationEntity) noexcept :
            source(sourceEntity),
            destination(destinationEntity), linkWay(LinkWay::oneToOne)
        {}
        ~Relation() override = default;

        void addConditionBuildRules(const RelationRulesList&);

        const EntityPtr getDestinationTarget() const override;
        const std::vector<ConditionPtr>
            getConditions(InstanceHash) const override;
        const std::vector<ConditionPtr>
            getConditions(const InstancePtr) const override;
        LinkWay getLinkWay() const override;
      public:
        static const RelationPtr build(const EntityPtr source,
                                       const EntityPtr dest,
                                       const RelationRulesList& rules)
        {
            auto relation = std::make_shared<Relation>(source, dest);
            relation->addConditionBuildRules(rules);
            return relation;
        }
        template <typename TDestination>
        static const RelationPtr build(const EntityPtr source,
                                       const RelationRulesList& rules)
        {
            return build(source, getEntityManager().getEntity<TDestination>(),
                         rules);
        }
    };

    class StaticInstance : public IInstance
    {
        const std::string identity;

      public:
        StaticInstance(const StaticInstance&) = delete;
        StaticInstance& operator=(const StaticInstance&) = delete;
        StaticInstance(StaticInstance&&) = delete;
        StaticInstance& operator=(StaticInstance&&) = delete;

        explicit StaticInstance(const std::string& identityFieldValue) noexcept
            :
            identity(identityFieldValue)
        {}

        virtual ~StaticInstance() = default;

        const IEntity::IEntityMember::InstancePtr&
            getField(const IEntity::EntityMemberPtr&) const override;

        const IEntity::IEntityMember::InstancePtr&
            getField(const MemberName&) const override;

        const std::vector<MemberName> getMemberNames() const override;

        void supplement(
            const MemberName&,
            const IEntity::IEntityMember::IInstance::FieldType&) override;
        void supplementOrUpdate(
            const MemberName&,
            const IEntity::IEntityMember::IInstance::FieldType&) override;
        void supplementOrUpdate(const InstancePtr&) override;

        bool hasField(const MemberName&) const override;
        bool checkCondition(const ConditionPtr) const override;

        const std::map<std::size_t, InstancePtr> getComplex() const override;
        bool isComplex() const override;
        void initDefaultFieldsValue() override;
        /**
         * @brief Get the Hash of Entity Instance
         *
         * @return std::size_t hash value
         */
        std::size_t getHash() const override;

      protected:
        virtual const IEntity::IEntityMember::InstancePtr&
            instanceNotFound() const;
        MemberInstancesMap memberInstances;
    };

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity(Entity&&) = delete;
    Entity& operator=(Entity&&) = delete;

    explicit Entity() noexcept;

    ~Entity() override = default;

    bool addMember(const EntityMemberPtr&) override;
    bool createMember(const MemberName& member) override;

    const IEntity::EntityMemberPtr
        getMember(const std::string& memberName) const override;

    const MemberMap& getMembers() const override
    {
        return this->members;
    }

    bool hasMember(const MemberName& memberName) const override
    {
        return (this->members.find(memberName) != this->members.end());
    }

    const InstancePtr getInstance(std::size_t) const override;
    const std::vector<InstancePtr>
        getInstances(const ConditionsList& = ConditionsList()) const override;
    void setInstances(std::vector<InstancePtr>) override;
    InstancePtr mergeInstance(InstancePtr) override;
    void removeInstance(InstanceHash) override;
    const Relations& getRelations() const override;
    const RelationPtr getRelation(const EntityName&) const override;
    static const EntityManager& getEntityManager();

    void processQueries() override;

    /**
     * @brief Clear a cache of instances
     * All isntances of entity will be removed
     */
    void resetCache() override;
    /**
     * @brief The Entity expect to initialize:
     *        + Target Entity members
     *        + Relations to anather Entities
     */
    void initialize() override;
    /**
     * @brief Get Entity type. The Enitty is object type of IEntity.
     *
     * @return Type IEntity::object
     */
    Type getType() const override;

  protected:
    virtual const MembersList getMembersNames() const;
    virtual const ProviderRulesDict& getProviders() const;

    inline void initMembers();
    inline void initRelations();
    inline void initProviders();

  protected:
    static void defaultLinkProvider(const IEntity::InstancePtr& supplement,
                                    const IEntity::InstancePtr& target);
};

template<typename TEntity>
class NamedEntity: public virtual IEntity
{
  public:
    const EntityName getName() const override
    {
        int status;
        char* className;
        const std::type_info& ti = typeid(TEntity);
        className = abi::__cxa_demangle(ti.name(), 0, 0, &status);
        if (status < 0 || !className)
        {
            throw core::exceptions::ObmcAppException(
                "Can't acquire entityName by symbols");
        }
        EntityName en(className);
        auto pos = en.find_last_of("::"); 
        if (pos != std::string::npos)
        {
            en = en.substr(pos + 1);
        }
        free(className);
        return en;
    }
    ~NamedEntity() override = default;
};

/**
 * @class CachedSource
 * @brief The behavior of populating IEntity data.
 *        The data should be filled by an cache update engine.
 */
class CachedSource: public virtual IEntity
{
    bool initialized;
  public:
    CachedSource() : initialized(false)
    {}
    void populate() override
    {
        if (!initialized)
        {
            resetCache();
            processQueries();
        }
        initialized = true;
    }
    void configure(const query::QueryPtr query) override
    {
        query->configure(std::ref(*this));
    }
    ~CachedSource() override = default;
};

/**
 * @class LazySource
 * @brief The behavior of populating IEntity data.
 *        The data will be filled per request of 'populate()' via configured
 *        datasource
 */
class LazySource : public virtual IEntity
{
  public:
    void populate() override
    {
        this->resetCache();
        this->processQueries();
    }
    void configure(const query::QueryPtr) override
    {}
    ~LazySource() override = default;
};

/**
 * @brief Class Collection provides the list of Entities specified object type.
 *        The current abstraction is needed to explicitly define an Entity set.
 */
class Collection : public Entity
{
  public:
    Collection(const Collection&) = delete;
    Collection& operator=(const Collection&) = delete;
    Collection(Collection&&) = delete;
    Collection& operator=(Collection&&) = delete;

    explicit Collection() noexcept : Entity()
    {}

    ~Collection() override = default;
    /**
     * @brief Get Entity type. The Enitty is object type of IEntity.
     *
     * @return Type IEntity::array
     */
    Type getType() const override;
};

/**
 * @class EntitySupplementProvider
 * @brief The abstraction that contains partial logic of acquire data filling an
 *        target Entity.
 *
 */
class EntitySupplementProvider :
    public Entity,
    public IEntity::ISupplementProvider
{
  public:
    explicit EntitySupplementProvider() noexcept : Entity()
    {}
    ~EntitySupplementProvider() override = default;
    /** @inherit */
    void supplementInstance(const IEntity::InstancePtr& instance,
                            ProviderLinkRule) override;
};

} // namespace entity
} // namespace app
