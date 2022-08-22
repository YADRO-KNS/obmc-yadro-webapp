// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/application.hpp>
#include <core/entity/entity.hpp>
#include <core/entity/dbus_query.hpp>

#include "assert.h"

namespace app
{
namespace entity
{

using namespace exceptions;
using namespace phosphor::logging;

const MemberName BaseEntity::EntityMember::getName() const noexcept
{
    return name;
}

const IEntity::IEntityMember::InstancePtr&
    BaseEntity::EntityMember::getInstance() const
{
    return this->instance;
}

void BaseEntity::Relation::addConditionBuildRules(const RelationRulesList& rules)
{
    conditionBuildRules.insert(conditionBuildRules.end(), rules.begin(),
                               rules.end());
}

const EntityPtr BaseEntity::Relation::getDestinationTarget() const
{
    return destination;
}

const std::vector<IEntity::ConditionPtr>
    BaseEntity::Relation::getConditions(InstanceHash sourceInstanceHash) const
{
    std::vector<IEntity::ConditionPtr> conditions;
    for(auto [memberSource, memberDest, compareLiteral]: conditionBuildRules)
    {
        if (memberSource == dummyField)
        {
            conditions.emplace_back(
                std::make_shared<Condition>(memberDest, compareLiteral));
            continue;
        }
        auto sourceMember = this->source.lock()->getMember(memberSource);
        auto sourceInstance = this->source.lock()->getInstance(sourceInstanceHash);
        if (!sourceMember || !sourceInstance)
        {
            // to be certain that we haven't invalid pointers
            assert("Some pointers of important object are invalid");
            continue;
        }
        auto compareValue =
            sourceInstance->getField(sourceMember)->getValue();

        log<level::DEBUG>("Acquire conditions for rule",
                          entry("DEST_MEMBER=%s", memberDest.c_str()));
        conditions.emplace_back(std::make_shared<Condition>(
            memberDest, compareValue, compareLiteral));
    }
    return std::forward<const std::vector<IEntity::ConditionPtr>>(conditions);
}

const std::vector<IEntity::ConditionPtr>
    BaseEntity::Relation::getConditions(const InstancePtr instance) const
{
    return std::forward<const std::vector<IEntity::ConditionPtr>>(
        getConditions(instance->getHash()));
}

const IEntity::InstanceCollection
    BaseEntity::StaticInstance::getRelatedInstances(
        const IEntity::RelationPtr relation, const ConditionsList& conditions,
        bool skipEmpty) const
{
    if (!relation)
    {
        return {};
    }
    auto relConditions = relation->getConditions(getHash());
    relConditions.insert(relConditions.end(), conditions.begin(),
                         conditions.end());
    if (skipEmpty && relConditions.empty())
    {
        return {};
    }
    return relation->getDestinationTarget()->getInstances(relConditions);
}

IEntity::IRelation::LinkWay BaseEntity::Relation::getLinkWay() const
{
    // TODO(IK) should we remove the LinkWay abstraction?
    return linkWay;
}

const IEntity::IEntityMember::InstancePtr& BaseEntity::StaticInstance::getField(
    const IEntity::EntityMemberPtr& entityMember) const
{
    return getField(entityMember->getName());
}

const IEntity::IEntityMember::InstancePtr&
    BaseEntity::StaticInstance::getField(const MemberName& entityMemberName) const
{
    auto findInstanceIt = memberInstances.find(entityMemberName);
    if (findInstanceIt == memberInstances.end())
    {
        return instanceNotFound();
    }

    return findInstanceIt->second;
}

const std::vector<MemberName> BaseEntity::StaticInstance::getMemberNames() const
{
    std::vector<MemberName> result;
    for (auto& [memberName, memberInstance] : memberInstances)
    {
        result.emplace_back(memberName);
    }

    return std::forward<const std::vector<MemberName>>(result);
}

void BaseEntity::StaticInstance::supplement(
    const MemberName& member,
    const IEntity::IEntityMember::IInstance::FieldType& value)
{
    auto memberInstance = std::make_shared<EntityMember::StaticInstance>(value);
    if (!memberInstances.emplace(member, std::move(memberInstance)).second)
    {
        throw std::logic_error("The requested member '" + member +
                               "' is already registried.");
    }
}

void BaseEntity::StaticInstance::supplementOrUpdate(const MemberName& memberName,
                                const IEntity::IEntityMember::IInstance::FieldType& value)
{
    if (hasField(memberName))
    {
        getField(memberName)->setValue(value);
        return;
    }

    supplement(memberName, value);
}

void BaseEntity::StaticInstance::supplementOrUpdate(
    const IEntity::InstancePtr& destination)
{
    for (auto& memberName : destination->getMemberNames())
    {
        this->supplementOrUpdate(memberName,
                                 destination->getField(memberName)->getValue());
    }
}

bool BaseEntity::StaticInstance::hasField(const MemberName& memberName) const
{
    return memberInstances.find(memberName) != memberInstances.end();
}

bool BaseEntity::StaticInstance::checkCondition(
    const IEntity::ConditionPtr condition) const
{
    return !condition || condition->check(*this);
}

const std::map<std::size_t, IEntity::InstancePtr> BaseEntity::StaticInstance::getComplex() const
{
    return std::map<std::size_t, IEntity::InstancePtr>();
}

bool BaseEntity::StaticInstance::isComplex() const
{
    return false;
}

void BaseEntity::StaticInstance::initDefaultFieldsValue()
{
    // nothing to do
}

std::size_t BaseEntity::StaticInstance::getHash() const
{
    auto hash = std::hash<std::string>{}(identity);
    return hash;
}

const IEntity::IEntityMember::InstancePtr&
    BaseEntity::StaticInstance::instanceNotFound() const
{
    static IEntity::IEntityMember::InstancePtr notAvailable =
        std::make_shared<Entity::EntityMember::StaticInstance>();

    return notAvailable;
}

void BaseEntity::Condition::addRule(
    const MemberName& destinationMember,
    const IEntity::IEntityMember::IInstance::FieldType& value,
    CompareCallback compareCallback)
{
    this->rules.emplace_back(std::make_pair(destinationMember, value), compareCallback);
}

void BaseEntity::Condition::addRule(const MemberName& destinationMember,
                                    CustomCompareCallback compareCallback)
{
    this->rules.emplace_back(
        std::make_pair(destinationMember, std::nullptr_t(nullptr)),
        [compareCallback](const IEntityMember::InstancePtr& instance,
                          const IEntityMember::IInstance::FieldType&) {
            return compareCallback(instance);
        });
}

bool BaseEntity::Condition::check(const IEntity::IInstance& sourceInstance) const
{
    return fieldValueCompare(sourceInstance);
}

bool BaseEntity::Condition::fieldValueCompare(
    const IEntity::IInstance& sourceInstance) const
{
    bool result = true;
    for (auto& [ruleMeta, compareCallback] : rules)
    {
        MemberName memberName;
        IEntityMember::IInstance::FieldType rightValue;
        std::tie(memberName, rightValue) = ruleMeta;
        IEntity::IEntityMember::InstancePtr memberInstance;
        if (IRelation::dummyField != memberName)
        {
            memberInstance = sourceInstance.getField(memberName);
        }
        result &= std::invoke(compareCallback, memberInstance, rightValue);
    }
    return result;
}

BaseEntity::BaseEntity() noexcept
{
    this->createMember(app::query::dbus::metaObjectPath);
    this->createMember(app::query::dbus::metaObjectService);
}

bool BaseEntity::addMember(const EntityMemberPtr& memberPtr)
{
    if (!memberPtr)
    {
        throw EntityException(
            "Invalid member pattern. The passed memeber is empty");
    }

    bool initialized = members.emplace(memberPtr->getName(), memberPtr).second;
    if (!initialized)
    {
        log<level::WARNING>(
            "Invalid pattern to add member of entity.",
            entry("ENTITY=%s", this->getName().c_str()),
            entry("MEMBER=%s", memberPtr->getName().c_str()),
            entry("DESC=%s", "Member of object already registered"));
    }

    return initialized;
}

bool BaseEntity::createMember(const MemberName& member)
{
    return addMember(std::make_shared<Entity::EntityMember>(member));    
}

const IEntity::EntityMemberPtr
    BaseEntity::getMember(const MemberName& memberName) const
{
    auto it = this->members.find(memberName);
    if (it == this->members.end())
    {
        auto& providersRules = getProviders();
        for (auto [provider, _] : providersRules)
        {
            try
            {
                return provider->getMember(memberName);
            }
            catch (EntityException&)
            {
                // don't handle this excecption, there is the first-level
                // exception bellow
            }
        }
        throw EntityException("The object Member <" + memberName +
                              "> not found");
    }
    return it->second;
}

const IEntity::InstancePtr BaseEntity::getInstance(std::size_t hash) const
{
    auto findInstanceIt = this->instances.find(hash);
    if (findInstanceIt == instances.end())
    {
        return IEntity::InstancePtr();
    }

    return findInstanceIt->second;
}

const std::vector<IEntity::InstancePtr>
    BaseEntity::getInstances(const ConditionsList& conditions) const
{
    // FIXME need optimization to prevent high load on CPU per request
    std::vector<IEntity::InstancePtr> result;
    for (const auto [_, instanceObject] : instances)
    {
        instanceObject->initDefaultFieldsValue();

        auto complexInstances = instanceObject->getComplex();
        complexInstances.insert_or_assign(instanceObject->getHash(),
                                          instanceObject);
        for (const auto [_, instance] : complexInstances)
        {
            const auto& providers = getProviders();
            for (auto provider : providers)
            {
                log<level::DEBUG>(
                    "Supplement instance by provider",
                    entry("PROVIDER=%s", provider.first->getName().c_str()));
                provider.first->supplementInstance(instance, provider.second);
            }
            bool conditionPassed = true;
            for (auto condition : conditions)
            {
                conditionPassed =
                    conditionPassed && instance->checkCondition(condition);
                if (!conditionPassed)
                {
                    break;
                }
            }
            if (conditionPassed)
            {
                result.push_back(instance);
            }
        }
    }
    return std::forward<const std::vector<IEntity::InstancePtr>>(result);
}

void BaseEntity::setInstances(std::vector<InstancePtr> instancesList)
{
    for (auto& inputInstance : instancesList)
    {
        this->instances.insert_or_assign(inputInstance->getHash(),
                                         inputInstance);
    }
}

IEntity::InstancePtr BaseEntity::mergeInstance(InstancePtr instance)
{
    InstancesHashmap::iterator foundIt = instances.find(instance->getHash());
    if (foundIt == instances.end())
    {
        this->instances.insert_or_assign(instance->getHash(),
                                         instance);
        return instance;
    }
    foundIt->second->supplementOrUpdate(instance);
    return foundIt->second;
}

void BaseEntity::removeInstance(InstanceHash hash)
{
    auto instance = this->instances.extract(hash);
    log<level::DEBUG>("Entity instance successfully removed",
                      entry("INSTANCE_HASH=%ld", hash),
                      entry("ENTITY=%s", getName().c_str()));
}

const std::vector<IEntity::RelationPtr>& BaseEntity::getRelations() const
{
    static const Relations noRelations;
    return noRelations;
}

void BaseEntity::initialize()
{
    log<level::DEBUG>("Entity initialize",
                      entry("ENTITY=%s", getName().c_str()));
    resetCache();
    initMembers();
    initRelations();
    initProviders();
}

void BaseEntity::initMembers()
{
    const auto membersNames = getMembersNames();
    log<level::DEBUG>("Entity members initialize",
                      entry("ENTITY=%s", getName().c_str()),
                      entry("MEMBERS_CNT=%ld", membersNames.size()));
    for (auto member: membersNames)
    {
        createMember(member);
    }
}

void BaseEntity::initRelations()
{
}

void BaseEntity::initProviders()
{
    for (auto providerRule: getProviders())
    {
        providerRule.first->initialize();
    }
}

void BaseEntity::processQueries()
{
    log<level::DEBUG>("Processing entity queries",
                      entry("ENTITY=%s", getName().c_str()));
    try
    {
        for (auto provider : getProviders())
        {
            provider.first->processQueries();
        }
        for (auto query : getQueries())
        {
            configure(query);
            setInstances(query->process());
        }
    }
    catch (sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("Failed to process entity queries",
                        entry("ENTITY=%s", getName().c_str()),
                        entry("ERROR=%s", ex.what()));
    }
}

void BaseEntity::resetCache()
{
    this->instances.clear();

    for (auto provider : getProviders())
    {
        provider.first->resetCache();
    }
}

const BaseEntity::MembersList BaseEntity::getMembersNames() const
{
    BaseEntity::MembersList membersList;
    for (auto query : getQueries())
    {
        for (auto memberName: query->getFields())
        {
            membersList.emplace_back(memberName);
        }
    }
    return std::forward<Entity::MembersList>(membersList);
}

const BaseEntity::ProviderRulesDict& BaseEntity::getProviders() const
{
    static const BaseEntity::ProviderRulesDict noProviders;
    return noProviders;
}

const IEntity::RelationPtr
    BaseEntity::getRelation(const EntityName& entityName) const
{
    const auto& relations = getRelations();
    log<level::DEBUG>("Get relation for destination entity",
                      entry("ENTITY=%s", getName().c_str()),
                      entry("DESTINATION=%s", entityName.c_str()),
                      entry("TOTAL_RELS=%ld", relations.size()));
    for (auto relation: relations)
    {
        if (relation->getDestinationTarget()->getName() == entityName)
        {
            // force update instance before checking relations
            log<level::DEBUG>("Entity relation found",
                              entry("ENTITY=%s", getName().c_str()),
                              entry("DESTINATION=%s", entityName.c_str()),
                              entry("TOTAL_RELS=%ld", relations.size()));
            return relation;
        }
    }

    return RelationPtr();
}

const EntityManager& BaseEntity::getEntityManager()
{
    return app::core::application.getEntityManager();
}

void EntitySupplementProvider::supplementInstance(
    const IEntity::InstancePtr& entityInstance, ProviderLinkRule linkRuleFn)
{
    const auto providerInstance = this->getInstances();
    for (auto instance : providerInstance)
    {
        std::invoke(linkRuleFn, instance, entityInstance);
    }
}

void BaseEntity::defaultLinkProvider(const IEntity::InstancePtr& supplement,
                                 const IEntity::InstancePtr& target)
{
    target->supplementOrUpdate(supplement);
}

} // namespace entity
} // namespace app
