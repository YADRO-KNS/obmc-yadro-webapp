// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/entity/entity_interface.hpp>
#include <core/entity/entity_manager.hpp>
#include <core/exceptions.hpp>
#include <phosphor-logging/log.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace app
{
namespace entity
{

using namespace phosphor::logging;
using namespace std::chrono;

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

class BaseEntity : virtual public IEntity
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

#define ENTITY_DEF_RELATION(dest, rules)                                       \
    Relation::build<dest>(getEntityManager().getEntity(getName()), rules)
#define ENTITY_DEF_RELATION2(dest, rules)                                      \
    Relation::build(getEntityManager().getEntity(getName()),                   \
                    getEntityManager().getEntity(#dest), rules)

#define ENTITY_DEF_RELATION_DIRECT(dest)                                       \
    ENTITY_DEF_RELATION(dest, IRelation::directLinkingRule())
#define ENTITY_DEF_RELATION_DIRECT2(dest)                                      \
    ENTITY_DEF_RELATION2(dest, IRelation::directLinkingRule())

#define ENTITY_DECL_RELATIONS(...)                                             \
    const std::vector<IEntity::RelationPtr>& getRelations() const override     \
    {                                                                          \
        static const Relations relations{__VA_ARGS__};                         \
        return relations;                                                      \
    }

#define ENTITY_DECL_RESET_FIELD(name)                                          \
    static void resetField##name(const InstancePtr instance)                   \
    {                                                                          \
        instance->supplementOrUpdate(field##name, std::nullptr_t(nullptr));    \
    }
#define ENTITY_DECL_FIELD_DEF(type, name, defval)                              \
    static constexpr const char* field##name = #name;                          \
    static type getField##name(const InstancePtr instance)                     \
    {                                                                          \
        const auto fieldPtr = instance->getField(field##name);                 \
        if (fieldPtr->isNull())                                                \
        {                                                                      \
            return defval;                                                     \
        }                                                                      \
        const auto& value = fieldPtr->getValue();                              \
        return std::get<type>(value);                                          \
    }                                                                          \
    static void setField##name(const InstancePtr instance, const type& value)  \
    {                                                                          \
        instance->supplementOrUpdate(field##name, value);                      \
    }                                                                          \
    ENTITY_DECL_RESET_FIELD(name)

#define ENTITY_DECL_FIELD(type, name)                                          \
    ENTITY_DECL_FIELD_DEF(type, name,                                          \
                          Entity::EntityMember::fieldValueNotAvailable)

#define ENTITY_DECL_FIELD_ENUM(type, name, defval)                             \
    static constexpr const char* field##name = #name;                          \
    static type getField##name(const InstancePtr instance)                     \
    {                                                                          \
        const auto fieldPtr = instance->getField(field##name);                 \
        if (fieldPtr->isNull())                                                \
        {                                                                      \
            return type::defval;                                               \
        }                                                                      \
        const auto& value = fieldPtr->getValue();                              \
        if constexpr (!std::is_enum_v<type>)                                   \
        {                                                                      \
            throw std::logic_error("The defined entity field is not enum");    \
        }                                                                      \
        return static_cast<type>(std::get<int>(value));                        \
    }                                                                          \
    static void setField##name(const InstancePtr instance, const type& value)  \
    {                                                                          \
        if constexpr (!std::is_enum_v<type>)                                   \
        {                                                                      \
            throw std::logic_error("The defined entity field is not enum");    \
        }                                                                      \
        instance->supplementOrUpdate(field##name, static_cast<int>(value));    \
    }                                                                          \
    ENTITY_DECL_RESET_FIELD(name)

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
            {}
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
                if (!std::holds_alternative<std::string>(getValue()) ||
                    isNull())
                {
                    return naVal;
                }
                return std::get<std::string>(getValue());
            }
            int getIntValue() const override
            {
                if (!std::holds_alternative<int>(getValue()) || isNull())
                {
                    return 0;
                }
                return std::get<int>(getValue());
            }
            double getFloatValue() const override
            {
                if (!std::holds_alternative<double>(getValue()) || isNull())
                {
                    return 0.0;
                }
                return std::get<double>(getValue());
            }
            bool getBoolValue() const override
            {
                if (!std::holds_alternative<bool>(getValue()) || isNull())
                {
                    return false;
                }
                return std::get<bool>(getValue());
            }
            bool isNull() const override
            {
                return !value.has_value() ||
                       std::holds_alternative<nullptr_t>(*value);
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
            return build<Equal<TRightValue>, TRightValue>(fieldName, value);
        }

        template <typename TRightValue>
        static const ConditionPtr buildNonEqual(const std::string& fieldName,
                                                TRightValue value)
        {
            return build<NonEqual<TRightValue>, TRightValue>(fieldName, value);
        }

      protected:
        bool fieldValueCompare(const IEntity::IInstance&) const;

        template <class TComparer, typename TRightValue>
        static const ConditionPtr build(const std::string& fieldName,
                                        TRightValue value)
        {
            if constexpr (std::is_enum_v<TRightValue>)
            {
                return std::make_shared<Condition>(
                    fieldName, static_cast<int>(value), TComparer());
            }
            else
            {
                return std::make_shared<Condition>(fieldName, value,
                                                   TComparer());
            }
        }
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
        void mergeInternalMetadata(const InstancePtr&) override
        {}
        bool hasField(const MemberName&) const override;
        bool checkCondition(const ConditionPtr) const override;
        const InstanceCollection
            getRelatedInstances(const RelationPtr,
                                const ConditionsList& = ConditionsList(),
                                bool skipEmpty = false) const override;

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

    BaseEntity(const BaseEntity&) = delete;
    BaseEntity& operator=(const BaseEntity&) = delete;
    BaseEntity(BaseEntity&&) = delete;
    BaseEntity& operator=(BaseEntity&&) = delete;

    explicit BaseEntity() noexcept;

    ~BaseEntity() override = default;

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

  protected:
    virtual const MembersList getMembersNames() const;
    virtual const ProviderRulesDict& getProviders() const;

    inline void initMembers();
    inline void initRelations();
    inline void initProviders();

  protected:
    static void defaultLinkProvider(const IEntity::InstancePtr& supplement,
                                    const IEntity::InstancePtr& target);

  private:
    mutable std::mutex mutex;
};

template <typename TEntity>
class NamedEntity : public virtual IEntity
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

    static const EntityPtr getEntity()
    {
        return BaseEntity::getEntityManager().getEntity<TEntity>();
    }

    ~NamedEntity() override = default;
};

/**
 * @class CachedSource
 * @brief The behavior of populating IEntity data.
 *        The data should be filled by an cache update engine.
 */
class CachedSource : public virtual IEntity
{
    bool initialized;
    bool hasErrors;
    /**
     * BRIEF: the guard to avoid double processing of the populate action.
     * NOTE:  we must avoid blocking synchronization to prevent failures on
     *        low-level dbus sockets.
     */
    std::atomic<bool> cachingInProgress;

  public:
    CachedSource() :
        initialized(false), hasErrors(false), cachingInProgress(false)
    {}
    void populate() override
    {
        // if another thread has already accepted an IEntity filling process, then
        // skip populating and immediately return uninitialized stuff.
        if (!cachingInProgress && (!initialized || hasErrors))
        {
            cachingInProgress = true;
            hasErrors = false;
            resetCache();
            processQueries();
            cachingInProgress = false;

            if (!hasErrors)
            {
                initialized = true;
            }
        }
    }
    void configure(const query::QueryPtr query) override
    {
        query->configure(std::ref(*this));
        this->configureQueryEvents();
    }
    ~CachedSource() override = default;

  protected:
    inline void invalidateCache() noexcept
    {
        hasErrors = true;
    }

    inline void configureQueryEvents() noexcept
    {
        using QueryEvent = app::query::QueryEvent;
        for (const auto& query : this->getQueries())
        {
            query->addEventSubscriber([this](QueryEvent event) {
                if (event == QueryEvent::hasFailures)
                {
                    log<level::DEBUG>(
                        "Invalidate cache of IQuery",
                        entry("IQUERY=%s", this->getName().c_str()));
                    this->invalidateCache();
                }
            });
        }
    }
};

/**
 * @class ShortTimeCachedSource
 * @brief The behavior of populating IEntity data.
 *        The data will be filled by an cache update engine with a defined
 *        lifetime.
 */
template <int64_t timeToCacheInSec>
class ShortTimeCachedSource : public virtual IEntity
{
    system_clock::time_point cacheTimePoint;

  public:
    ShortTimeCachedSource() :
        cacheTimePoint(system_clock::now() - timeToCache())
    {}
    void populate() override
    {
        if (isCacheExpiried())
        {
            resetCache();
            processQueries();
            setTimePoint();
        }
    }
    void configure(const query::QueryPtr query) override
    {
        query->configure(std::ref(*this));
    }
    ~ShortTimeCachedSource() override = default;

  private:
    void setTimePoint()
    {
        this->cacheTimePoint = system_clock::now();
    }

    inline bool isCacheExpiried() const
    {
        std::time_t current = std::time(nullptr);
        const auto calculated =
            system_clock::to_time_t(cacheTimePoint + timeToCache());
        return calculated < current;
    }

    constexpr const auto timeToCache() const
    {
        return std::chrono::seconds(timeToCacheInSec);
    }
};

/**
 * @class LazySource
 * @brief The behavior of populating IEntity data.
 *        The data will be filled per request by 'populate()' via configured
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
class Entity : public BaseEntity
{
  public:
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity(Entity&&) = delete;
    Entity& operator=(Entity&&) = delete;

    explicit Entity() noexcept : BaseEntity()
    {}

    ~Entity() override = default;
    /**
     * @brief Get Entity type.
     *
     * @return Type IEntity::Type::object
     */
    Type getType() const override
    {
        return Type::object;
    }

    bool isNull() const
    {
        return this->getInstances().size() == 0;
    }

    const InstancePtr get() const
    {
        const auto instances = this->getInstances();
        if (instances.size() == 0)
        {
            return std::make_shared<StaticInstance>(
                EntityMember::fieldValueNotAvailable);
        }
        else if (instances.size() > 1)
        {
            log<level::WARNING>(
                "The given instance of IEntity is not object type. Please, "
                "review data source and the entity configuration.",
                entry("ENTITY=%s", getName().c_str()));
        }

        return instances.at(0);
    }

    const InstancePtr operator()() const
    {
        return this->get();
    }
};

/**
 * @brief Class Collection provides the list of Entities specified object type.
 *        The current abstraction is needed to explicitly define an Entity set.
 */
class Collection : public BaseEntity
{
  public:
    Collection(const Collection&) = delete;
    Collection& operator=(const Collection&) = delete;
    Collection(Collection&&) = delete;
    Collection& operator=(Collection&&) = delete;

    explicit Collection() noexcept : BaseEntity()
    {}

    ~Collection() override = default;
    /**
     * @brief Get Colleciton type
     *
     * @return Type IEntity::Type::array
     */
    Type getType() const override
    {
        return Type::array;
    }
};

/**
 * @class EntitySupplementProvider
 * @brief The abstraction that contains partial logic of acquire data filling an
 *        target Entity.
 *
 */
class EntitySupplementProvider :
    public BaseEntity,
    public IEntity::ISupplementProvider
{
  public:
    explicit EntitySupplementProvider() noexcept : BaseEntity()
    {}
    ~EntitySupplementProvider() override = default;
    /** @inherit */
    void supplementInstance(const IEntity::InstancePtr& instance,
                            ProviderLinkRule) override;

    /**
     * @brief Get Rerefernce type.
     *        Providers has a formless type that is depends on the requesting
     *        resources.
     *
     * @return Type IEntity::Type::reference
     */
    Type getType() const override
    {
        return Type::reference;
    }
};

} // namespace entity
} // namespace app
