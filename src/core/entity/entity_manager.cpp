// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#include "core/entity/entity_manager.hpp"
#include <core/entity/entity.hpp>
#include "core/exceptions.hpp"

namespace app
{
namespace entity
{

using namespace exceptions;

void EntityManager::configure()
{
    for(auto entity : entityDictionary)
    {
        entity.second->initialize();
    }
}

const EntityPtr& EntityManager::addEntity(EntityPtr entity)
{
    auto state = entityDictionary.emplace(entity->getName(), std::move(entity));
    if (!state.second)
    {
        throw EntityException(
            "The name of object already registered. Object name is " +
            entity->getName());
    }

    return state.first->second;
}

const EntityPtr EntityManager::getEntity(const std::string& entityName) const
{
    auto it = entityDictionary.find(entityName);
    if (it == entityDictionary.end())
    {
        throw EntityException("The Object <" + entityName + "> not found");
    }
    // The default behavior of acquiring Entity instance is to provide the most
    // actualized data. The Entity instance should make it own decisions how the
    // data will be filled.
    it->second->populate();
    return it->second;
}

void EntityManager::update()
{
    for(auto entity : entityDictionary)
    {
        entity.second->populate();
        log<level::INFO>(
            (entity.second->getName() + " entity internal cache initialized")
                .c_str());
    }
}

} // namespace entity
} // namespace app
