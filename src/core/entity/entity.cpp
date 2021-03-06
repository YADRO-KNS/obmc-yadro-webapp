// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/application.hpp>
#include <core/entity/entity.hpp>

namespace app
{
namespace entity
{

using namespace exceptions;

const MemberName Entity::EntityMember::getName() const noexcept
{
    return name;
}

const IEntity::IEntityMember::InstancePtr&
    Entity::EntityMember::getInstance() const
{
    return this->instance;
}

void Entity::Relation::addConditionBuildRules(const RelationRulesList& rules)
{
    conditionBuildRules.insert(conditionBuildRules.end(), rules.begin(),
                               rules.end());
}

const EntityPtr Entity::Relation::getDestinationTarget() const
{
    return destination;
}

const std::vector<IEntity::ConditionPtr>
    Entity::Relation::getConditions(InstanceHash sourceInstanceHash) const
{
    std::vector<IEntity::ConditionPtr> conditions;
    for(auto [memberSource, memberDest, compareLiteral]: conditionBuildRules)
    {
        auto sourceMember = this->source.lock()->getMember(memberSource);
        auto sourceInstance = this->source.lock()->getInstance(sourceInstanceHash);
        if (!sourceMember || !sourceInstance)
        {
            // to be certain that we haven't invalid pointers
            continue;
        }
        auto compareValue =
            sourceInstance->getField(sourceMember)->getValue();

        BMC_LOG_DEBUG << "[RELATION] ADD RULE FOR CONDITION: " << memberDest
                      << "=" << std::get<std::string>(compareValue);

        auto condition = std::make_shared<Entity::Condition>();
        condition->addRule(memberDest, compareValue, compareLiteral);
        conditions.push_back(condition);
    }
    return std::forward<const std::vector<IEntity::ConditionPtr>>(conditions);
}

IEntity::IRelation::LinkWay Entity::Relation::getLinkWay() const
{
    // TODO(IK) should we remove the LinkWay abstraction?
    return linkWay;
}

const IEntity::IEntityMember::InstancePtr& Entity::StaticInstance::getField(
    const IEntity::EntityMemberPtr& entityMember) const
{
    return getField(entityMember->getName());
}

const IEntity::IEntityMember::InstancePtr&
    Entity::StaticInstance::getField(const MemberName& entityMemberName) const
{
    auto findInstanceIt = memberInstances.find(entityMemberName);
    if (findInstanceIt == memberInstances.end())
    {
        return instanceNotFound();
    }

    return findInstanceIt->second;
}

const std::vector<MemberName> Entity::StaticInstance::getMemberNames() const
{
    std::vector<MemberName> result;
    for (auto& [memberName, memberInstance] : memberInstances)
    {
        result.emplace_back(memberName);
    }

    return std::forward<const std::vector<MemberName>>(result);
}

void Entity::StaticInstance::supplement(
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

void Entity::StaticInstance::supplementOrUpdate(const MemberName& memberName,
                                const IEntity::IEntityMember::IInstance::FieldType& value)
{
    if (hasField(memberName))
    {
        getField(memberName)->setValue(value);
        return;
    }

    supplement(memberName, value);
}

void Entity::StaticInstance::supplementOrUpdate(
    const IEntity::InstancePtr& destination)
{
    BMC_LOG_DEBUG << "count destination members: "
                  << destination->getMemberNames().size();
    for (auto& memberName : destination->getMemberNames())
    {
        this->supplementOrUpdate(memberName,
                                 destination->getField(memberName)->getValue());
    }
}

bool Entity::StaticInstance::hasField(const MemberName& memberName) const
{
    return memberInstances.find(memberName) != memberInstances.end();
}

bool Entity::StaticInstance::checkCondition(
    const IEntity::ConditionPtr condition) const
{
    BMC_LOG_DEBUG << "Checking condition";
    return !condition || condition->check(*this);
}

const std::map<std::size_t, IEntity::InstancePtr> Entity::StaticInstance::getComplex() const
{
    return std::map<std::size_t, IEntity::InstancePtr>();
}

bool Entity::StaticInstance::isComplex() const
{
    return false;
}

void Entity::StaticInstance::initDefaultFieldsValue()
{
    // nothing to do
}

std::size_t Entity::StaticInstance::getHash() const
{
    auto hash = std::hash<std::string>{}(identity);
    return hash;
}

const IEntity::IEntityMember::InstancePtr&
    Entity::StaticInstance::instanceNotFound() const
{
    static IEntity::IEntityMember::InstancePtr notAvailable =
        std::make_shared<Entity::EntityMember::StaticInstance>(
            std::string(Entity::EntityMember::fieldValueNotAvailable));

    return notAvailable;
}

void Entity::Condition::addRule(
    const MemberName& destinationMember,
    const IEntity::IEntityMember::IInstance::FieldType& value,
    CompareCallback compareCallback)
{
    this->rules.push_back(
        std::make_pair(std::make_pair(destinationMember, value), compareCallback));
}

bool Entity::Condition::check(const IEntity::IInstance& sourceInstance) const
{
    return fieldValueCompare(sourceInstance);
}

bool Entity::Condition::fieldValueCompare(
    const IEntity::IInstance& sourceInstance) const
{
    bool result = true;
    for (auto& [ruleMeta, compareCallback] : rules)
    {
        MemberName memberName;
        IEntityMember::IInstance::FieldType rightValue;
        std::tie(memberName, rightValue) = ruleMeta;
        auto memberInstance = sourceInstance.getField(memberName);
        result &= std::invoke(compareCallback, memberInstance, rightValue);
    }
    return result;
}

bool Entity::addMember(const EntityMemberPtr& member)
{
    if (!member)
    {
        throw EntityException(
            "Invalid member pattern. The passed memeber is empty");
    }

    bool initialized = members.emplace(member->getName(), member).second;
    if (!initialized)
    {
        BMC_LOG_WARNING << "Invalid member pattern. The " + member->getName() +
                           " member of object already registered";
    }

    return initialized;
}

const EntityName Entity::getName() const noexcept
{
    return this->name;
}

const IEntity::EntityMemberPtr
    Entity::getMember(const MemberName& memberName) const
{
    auto it = this->members.find(memberName);
    if (it == this->members.end())
    {
        throw EntityException("The object Member <" + memberName +
                              "> not found");
    }
    return it->second;
}

const IEntity::InstancePtr Entity::getInstance(std::size_t hash) const
{
    auto findInstanceIt = this->instances.find(hash);
    if (findInstanceIt == instances.end())
    {
        return IEntity::InstancePtr();
    }

    return findInstanceIt->second;
}

const std::vector<IEntity::InstancePtr>
    Entity::getInstances(const ConditionsList& conditions) const
{
    // FIXME need optimization to prevent high load on CPU per request
    std::vector<IEntity::InstancePtr> result;
    for (auto [_, instanceObject] : instances)
    {
        instanceObject->initDefaultFieldsValue();

        auto complexInstances = instanceObject->getComplex();
        complexInstances.insert_or_assign(instanceObject->getHash(),
                                          instanceObject);
        for (auto [_, instance] : complexInstances)
        {
            for (auto& provider : this->providers)
            {
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

void Entity::setInstances(std::vector<InstancePtr> instancesList)
{
    instances.clear();
    for (auto& inputInstance : instancesList)
    {
        this->instances.insert_or_assign(inputInstance->getHash(),
                                         inputInstance);
    }
}

IEntity::InstancePtr Entity::mergeInstance(InstancePtr instance)
{
    InstancesHashmap::iterator foundIt = instances.find(instance->getHash());
    if (foundIt == instances.end())
    {
        BMC_LOG_DEBUG << "[MERGING] Insert new instance";
        this->instances.insert_or_assign(instance->getHash(),
                                         instance);
        return instance;
    }
    BMC_LOG_DEBUG << "[MERGING] Supplement";
    foundIt->second->supplementOrUpdate(instance);
    return foundIt->second;
}

void Entity::removeInstance(InstanceHash hash)
{
    auto instance = this->instances.extract(hash);
    BMC_LOG_DEBUG << "Remove instance: " << hash << " of entity " << this->getName()
              << " is OK";
}

void Entity::linkSupplementProvider(
    const EntitySupplementProviderPtr& provider,
    ISupplementProvider::ProviderLinkRule linkRule)
{
    BMC_LOG_DEBUG << "Link provider: " << provider->getName();
    providers.push_back(std::make_pair(provider, linkRule));
}

void Entity::addRelation(const RelationPtr relation)
{
    using namespace std::literals;
    if (!relation)
    {
        BMC_LOG_ERROR << "Attempt to register nullptr_t of the relation object.";
        return;
    }

    BMC_LOG_DEBUG << "The entity " << this->getName() << " will accept to the "
              << relation->getDestinationTarget()->getName()
              << "destination entity";

    relations.push_back(relation);
}

const std::vector<IEntity::RelationPtr>& Entity::getRelations() const
{
    return relations;
}

Entity::Type Entity::getType() const
{
    return Type::object;
}

const IEntity::RelationPtr
    Entity::getRelation(const EntityName& entityName) const
{
    BMC_LOG_DEBUG << "Count relations: " << relations.size();
    for (auto relation: relations)
    {
        BMC_LOG_DEBUG << "Check relation relations: "
                      << relation->getDestinationTarget()->getName();
        if (relation->getDestinationTarget()->getName() == entityName)
        {
            // force update instance before checking relations
            relation->getDestinationTarget()->fillEntity();
            return relation;
        }
    }

    return RelationPtr();
}

void EntitySupplementProvider::supplementInstance(
    IEntity::InstancePtr& entityInstance, ProviderLinkRule linkRuleFn)
{
    for (auto supplementInstance : this->getInstances())
    {
        std::invoke(linkRuleFn, supplementInstance, entityInstance);
    }
}

EntityManager::EntityBuilder& EntityManager::EntityBuilder::addMembers(
    const std::vector<std::string>& memberNames)
{
    for (const auto& memberName : memberNames)
    {
        entity->addMember(std::make_shared<Entity::EntityMember>(memberName));
    }
    return *this;
}

EntityManager::EntityBuilder&
    EntityManager::EntityBuilder::linkSupplementProvider(
        const std::string& providerName,
        IEntity::ISupplementProvider::ProviderLinkRule linkRule)
{
    using namespace app::entity::obmc::definitions;
    auto findProviderIt = providers.find(providerName);
    if (findProviderIt == providers.end())
    {
        throw exceptions::EntityException(
            "Requested provider is not registered: " + providerName);
    }

    this->entity->linkSupplementProvider(findProviderIt->second, linkRule);

    auto providerMembers =
        this->entityManager.getProvider(providerName)->getMembers();
    for (auto [memberName, memberInstance] : providerMembers)
    {
        if (memberName.starts_with(metaFieldPrefix))
        {
            continue;
        }
        this->entity->addMember(memberInstance);
    }

    return *this;
}

EntityManager::EntityBuilder&
    EntityManager::EntityBuilder::linkSupplementProvider(
        const std::string& providerName)
{
    using namespace std::placeholders;

    this->linkSupplementProvider(
        providerName,
        std::bind(&EntityManager::EntityBuilder::defaultLinkProvider, _1, _2));
    return *this;
}

EntityManager::EntityBuilder& EntityManager::EntityBuilder::addRelations(
    const std::string& destinationEntityName,
    const IEntity::IRelation::RelationRulesList& ruleBuilders)
{
    auto destinationEntity =
        this->entityManager.getEntity(destinationEntityName, false);

    auto relation =
        std::make_shared<Entity::Relation>(this->entity, destinationEntity);

    relation->addConditionBuildRules(ruleBuilders);
    entity->addRelation(relation);
    return *this;
}

void EntityManager::EntityBuilder::defaultLinkProvider(
    const IEntity::InstancePtr& supplement, const IEntity::InstancePtr& target)
{
    target->supplementOrUpdate(supplement);
}

EntityManager::EntityBuilderPtr EntityManager::buildSupplementProvider(
    const std::string& supplementProviderName)
{
    auto provider =
        std::make_shared<EntitySupplementProvider>(supplementProviderName);
    supplementProviders.emplace(supplementProviderName, provider);
    auto builder = std::make_shared<EntityManager::EntityBuilder>(
        std::move(provider), supplementProviders, *this);
    return std::forward<EntityManager::EntityBuilderPtr>(builder);
}

EntityManager::EntityBuilderPtr
    EntityManager::buildCollection(const std::string& name)
{
    return std::forward<EntityManager::EntityBuilderPtr>(
        buildEntity<Collection>(name));
}

EntityManager::EntityBuilderPtr
    EntityManager::buildEntity(const std::string& name)
{
    return std::forward<EntityManager::EntityBuilderPtr>(
        buildEntity<Entity>(name));
}

void EntityManager::addEntity(EntityPtr entity)
{
    if (!entityDictionary.emplace(entity->getName(), std::move(entity)).second)
    {
        throw exceptions::EntityException(
            "The name of object already registered. Object name is " +
            entity->getName());
    }
}

const EntityPtr EntityManager::getEntity(const std::string& entityName,
                                         bool forceFillInstances) const
{
    auto it = entityDictionary.find(entityName);
    if (it == entityDictionary.end())
    {
        throw EntityException("The Object <" + entityName + "> not found");
    }
    if (forceFillInstances)
    {
        it->second->fillEntity();
    }
    return it->second;
}

const EntityPtr EntityManager::getProvider(const EntityName& providerName) const
{
    auto it = supplementProviders.find(providerName);
    if (it == supplementProviders.end())
    {
        throw EntityException("The Provider <" + providerName + "> not found");
    }
    return it->second;
}

Collection::Type Collection::getType() const
{
    return Type::array;
}

} // namespace entity
} // namespace app
