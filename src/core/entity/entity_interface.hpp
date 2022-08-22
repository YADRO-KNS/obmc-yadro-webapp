// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <core/exceptions.hpp>
#include <phosphor-logging/log.hpp>

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <variant>
#include <functional>

using namespace phosphor::logging;

namespace app
{
namespace query
{
class IQuery;
using QueryPtr = std::shared_ptr<IQuery>;
using QueryCollection = std::vector<QueryPtr>;
using QueryField = std::string;
using QueryFields = std::vector<QueryField>;
} // namespace query

namespace entity
{

/**
 * The Entity is a most central abstraction which describes a user-consumption
 * payload. The IEntity interface defines the user-object, e.g. Server, Sensors,
 * Chassis, etc.
 * Also, the defined data should be stored in someplace, and that
 * is 'IInstance' abstraction, which the IEntity includes. The described IEntity
 * wants to have a description of its own fields, which is IEntityMebmber.
 * Like the IEntity has the IInstance, the IEntityMember also has its own
 * IInstance containing the Fields value.
 */

class IEntity;
class IEntityMapper;
using EntityPtr = std::shared_ptr<IEntity>;
using EntityPtrConst = std::shared_ptr<const IEntity>;
using EntityWeak = std::weak_ptr<IEntity>;

using EntityName = std::string;
using MemberName = std::string;

template <class T, typename... Args>
class ISingleton
{
  public:
    virtual ~ISingleton() = default;

    static const std::shared_ptr<T>& getSingleton(const Args&... args)
    {
        static std::shared_ptr<T> instance(std::make_shared<T>(args...));
        return instance;
    }
};

class IEntity
{
  public:
    class IEntityMember;
    class IInstance;
    class ISupplementProvider;
    class IRelation;
    class ICondition;
    class IFormatter;
    class IValidator;

    using EntityMemberPtr = std::shared_ptr<IEntity::IEntityMember>;
    using EntityMemberPtrConst = std::shared_ptr<const IEntity::IEntityMember>;
    using InstancePtr = std::shared_ptr<IInstance>;
    using InstancePtrConst = std::shared_ptr<const IInstance>;
    using RelationPtr = std::shared_ptr<IRelation>;
    using ConditionPtr = std::shared_ptr<ICondition>;
    using ConditionsList = std::vector<ConditionPtr>;
    using FormatterPtr = std::shared_ptr<IFormatter>;
    using ValidatorPtr = std::shared_ptr<IValidator>;
    using Relations = std::vector<RelationPtr>;

    using MemberMap = std::map<const std::string, EntityMemberPtr>;
    using InstanceHash = std::size_t;
    using InstanceCollection = std::vector<InstancePtr>;

    enum class Type
    {
        object,
        array,
        reference
    };

    class IEntityMember
    {
      public:
        class IInstance;
        using InstancePtr = std::shared_ptr<IInstance>;
        using InstancePtrConst = std::shared_ptr<const IInstance>;

        class IInstance
        {
          public:
            using Association =
                std::tuple<std::string, std::string, std::string>;
            using Associations = std::vector<Association>;
            using FieldType =
                std::variant<Associations, std::vector<std::string>,
                             std::vector<double>, std::vector<int>, std::string,
                             int64_t, uint64_t, double, int32_t, uint32_t,
                             int16_t, uint16_t, uint8_t, bool, std::nullptr_t>;

            virtual const FieldType& getValue() const noexcept = 0;
            virtual const std::string& getStringValue() const = 0;
            virtual int getIntValue() const = 0;
            virtual double getFloatValue() const = 0;
            virtual bool getBoolValue() const = 0;
            virtual bool isNull() const = 0;
            virtual const std::string getType() const = 0;

            virtual void setValue(const FieldType&) = 0;

            virtual ~IInstance() = default;
        };

        virtual const std::string getName() const noexcept = 0;
        virtual const InstancePtr& getInstance() const = 0;

        virtual ~IEntityMember() = default;
    };

    class IInstance
    {
      public:
        using MemberInstancesMap =
            std::map<entity::MemberName, IEntity::IEntityMember::InstancePtr>;

        virtual ~IInstance() = default;
        /**
         * @brief Get the Field of Entity Instance
         *
         * @param entityMemberName - name of an Entity Member to seach the Field
         *
         * @return const Entity::IEntityMember::InstancePtr& The Field Instance
         * of specified Entity Member
         *
         */
        virtual const IEntity::IEntityMember::InstancePtr&
            getField(const IEntity::EntityMemberPtr&) const = 0;

        virtual const IEntity::IEntityMember::InstancePtr&
            getField(const MemberName&) const = 0;

        virtual const std::vector<MemberName> getMemberNames() const = 0;

        virtual void supplement(const MemberName&,
                                const IEntityMember::IInstance::FieldType&) = 0;
        virtual void
            supplementOrUpdate(const MemberName&,
                               const IEntityMember::IInstance::FieldType&) = 0;
        virtual void supplementOrUpdate(const InstancePtr&) = 0;

        virtual bool hasField(const MemberName&) const = 0;
        virtual bool checkCondition(const ConditionPtr) const = 0;

        virtual const std::map<std::size_t, InstancePtr> getComplex() const = 0;
        virtual bool isComplex() const = 0;

        virtual void initDefaultFieldsValue() = 0;
        /**
         * @brief Get the Hash of Entity Instance
         *
         * @return std::size_t hash value
         */
        virtual std::size_t getHash() const = 0;
    };

    /**
     * @class ISupplementProvider
     * @brief The abstraction that contains partial logic of acquire data
     *        filling a subject abstraction.
     *
     */
    class ISupplementProvider
    {
      public:
        using ProviderLinkRule = std::function<void(
            const IEntity::InstancePtr&, const IEntity::InstancePtr&)>;

        virtual ~ISupplementProvider() = default;

        virtual void supplementInstance(const IEntity::InstancePtr&,
                                        ProviderLinkRule) = 0;
    };

    class ICondition
    {
      public:
        using CompareCallback =
            std::function<bool(const IEntityMember::InstancePtr&,
                               const IEntityMember::IInstance::FieldType&)>;
        using CustomCompareCallback =
            std::function<bool(const IEntityMember::InstancePtr&)>;

        template<typename TValue = std::string>
        struct Equal
        {
            virtual ~Equal() = default;
            virtual bool
                operator()(const IEntityMember::InstancePtr& instance,
                           const IEntityMember::IInstance::FieldType& value)
            {
                if (!instance)
                {
                    return false;
                }
                try
                {
                    const auto left = instance->getValue();
                    if constexpr (std::is_enum_v<TValue>)
                    {
                        if (std::holds_alternative<int>(left) &&
                            std::holds_alternative<int>(value))
                        {
                            return left == value;
                        }
                    }
                    else
                    {
                        if (std::holds_alternative<TValue>(left) &&
                            std::holds_alternative<TValue>(value))
                        {
                            return left == value;
                        }
                    }
                }
                catch (std::exception& e)
                {
                    log<level::DEBUG>(
                        "Error while comparing values of some condition.",
                        entry("ERROR=%s", e.what()));
                }
                return false;
            }
        };

        template <typename TValue = std::string>
        struct NonEqual : public Equal<TValue>
        {
            ~NonEqual() override = default;
            bool operator()(
                const IEntityMember::InstancePtr& instance,
                const IEntityMember::IInstance::FieldType& value) override
            {
                return !Equal<TValue>::operator()(instance, value);
            }
        };

        virtual void addRule(const MemberName&,
                             const IEntityMember::IInstance::FieldType&,
                             CompareCallback) = 0;

        virtual void addRule(const MemberName&, CustomCompareCallback) = 0;
        virtual bool check(const IEntity::IInstance&) const = 0;

        virtual ~ICondition() = default;
    };

    class IRelation
    {
      public:
        using RuleSet =
            std::tuple<MemberName, MemberName, ICondition::CompareCallback>;
        using RelationRulesList = std::vector<RuleSet>;

        enum class LinkWay : uint8_t
        {
            oneToOne,
            oneToMany,
            manyToOne,
            none
        };
        virtual ~IRelation() = default;

        virtual const EntityPtr getDestinationTarget() const = 0;
        virtual const std::vector<ConditionPtr>
            getConditions(InstanceHash) const = 0;
        virtual const std::vector<ConditionPtr>
            getConditions(const InstancePtr) const = 0;
        virtual LinkWay getLinkWay() const = 0;
    };

    class IFormatter
    {
      public:
        virtual ~IFormatter() = default;

        virtual const IEntityMember::IInstance::FieldType
            format(const MemberName& member,
                   const IEntityMember::IInstance::FieldType& value);
    };

    class IValidator
    {
      public:
        virtual ~IValidator() = default;
    };

    virtual const EntityName getName() const = 0;

    virtual bool addMember(const EntityMemberPtr&) = 0;
    virtual bool createMember(const MemberName&) = 0;

    virtual const IEntity::EntityMemberPtr
        getMember(const std::string& memberName) const = 0;

    virtual const MemberMap& getMembers() const = 0;

    virtual bool hasMember(const MemberName&) const = 0;

    virtual const InstancePtr getInstance(std::size_t) const = 0;
    virtual const std::vector<InstancePtr>
        getInstances(const ConditionsList& = ConditionsList()) const = 0;
    virtual void setInstances(std::vector<InstancePtr>) = 0;
    virtual InstancePtr mergeInstance(InstancePtr) = 0;
    virtual void removeInstance(InstanceHash) = 0;

    virtual const Relations& getRelations() const = 0;
    virtual const RelationPtr getRelation(const EntityName&) const = 0;

    virtual const query::QueryCollection& getQueries() const = 0;
    virtual void processQueries() = 0;
    virtual void configure(const query::QueryPtr) = 0;
    
    /**
     * @brief Fill the entity from the configured data source.
     *
     * The data might come from different places and in different ways.
     * The end-entity class should configure the way of filling.
     */
    virtual void populate() = 0;
    virtual void resetCache() = 0;
    /**
     * @brief The Entity required to initialize each relevant source that is
     *        will use to populate IEntity.
     */
    virtual void initialize() = 0;

    virtual ~IEntity() = default;

    /**
     * @brief Get Type of the current Entity object.
     *
     * @return IEntity::Type  - The type of Entity.
     */
    virtual IEntity::Type getType() const = 0;
};
} // namespace entity

namespace query
{
class IQuery
{
  public:
    class QueryException: public app::core::exceptions::ObmcAppException{
      public:
        explicit QueryException(const std::string& error) :
            ObmcAppException("Query failure: " + error) _GLIBCXX_TXN_SAFE
        {}
        virtual ~QueryException() = default;
    };
    virtual ~IQuery() = default;
    /**
     * @brief Configure IQuery for specified entity
     * 
     * @param entity - the IEntity to define appropriate behavior
     */
    virtual void configure(std::reference_wrapper<entity::IEntity>)
    {}

    /**
     * @brief Run the configured quiry.
     *
     * @return std::vector<TInstance> The list of retrieved data which
     *                                incapsulated at TInstance objects
     */
    virtual const entity::IEntity::InstanceCollection process() = 0;

    /**
     * @brief Get the collection of query fields
     *
     * @return const QueryFields - The collection of fields
     */
    virtual const QueryFields getFields() const = 0;
};

} // namespace query
} // namespace app
