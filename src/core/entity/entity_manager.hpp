// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include "core/exceptions.hpp"

#include <cxxabi.h>

#include <core/entity/entity_interface.hpp>

namespace app
{
namespace entity
{

/**
 * @class EntityManager
 * @brief The manager to centralize handling each Entity instance.
 *
 */
class EntityManager final
{
    using EntityMap = std::map<const std::string, EntityPtr>;

#define ENTITY_DECL(t, ...)                                                    \
    using t##ShrPtr = std::shared_ptr<t>;                                      \
    const t##ShrPtr get##t() const                                             \
    {                                                                          \
        static const t##ShrPtr entity = buildEntity<t>(##__VA_ARGS__);         \
        return entity;                                                         \
    }

  public:
    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;
    EntityManager(EntityManager&&) = delete;
    EntityManager& operator=(EntityManager&&) = delete;

    explicit EntityManager() = default;
    ~EntityManager() = default;

    /**
     * @brief Add IEntity instance to the EntityManager registry
     *
     * @param entity - The IEntity instance
     */
    const EntityPtr& addEntity(EntityPtr entity);

    /**
     * @brief Build and add IEntity instance to the EntityManager registry
     *
     * @tparam TIEntity - The derived type of IEntity
     * @param Args      - Arguments to create IEntity object
     *
     * @return const std::shared_ptr<TIEntity>
     */
    template <typename TIEntity, typename... Args>
    const std::shared_ptr<TIEntity> buildEntity(Args... args)
    {
        static_assert(std::is_base_of_v<IEntity, TIEntity>);
        return std::dynamic_pointer_cast<TIEntity>(
            addEntity(std::make_shared<TIEntity>(args...)));
    }

    /**
     * @brief Configure each registried enitty
     *
     */
    void configure();

    /**
     * @brief Get the Entity instance that having specified literal name
     *
     * @param entityName            - The entity name to retrieve the
     *                                corresponding an Entity Objects instances
     * @return const EntityPtr      - Entity object which contains the target
     *                           instances
     */
    const EntityPtr getEntity(const EntityName& entityName) const;
    /**
     * @brief Get the Entity instance that having specified literal name
     *
     * @param entityName            - The entity name to retrieve the
     *                                corresponding an Entity Objects instances
     * @return const EntityPtr      - Entity object which contains the target
     *                           instances
     */
    template <typename TEntity>
    const std::shared_ptr<TEntity> getEntity() const
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
        auto entityPtr = std::dynamic_pointer_cast<TEntity>(getEntity(en));
        if (!entityPtr)
        {
            throw core::exceptions::ObmcAppException(
                "Invalid access to the entity on casting to "
                "an unrecognized type");
        }
        free(className);
        return entityPtr;
    }

    /**
     * @brief Update all IEntity isntances (Entity, Collection, Providers, etc.)
     */
    void update();

  protected:
  private:
    EntityMap entityDictionary;
};

/**
 * @brief The class of mapping the actual Entity literal name with the protocol
 * specific naming.
 *
 * TODO: by actual this abstraction unimplemented. But for the future protocols
 * implementation, e.g. DMTF REDFISH, this is must have.
 *
 * The Entity instances do registered as:
 * + `Entity object literal name` => EntityObject instance
 *    - `Entity Object Member literal name` => EntityObjectMember instance
 *    - ...
 * + ...
 *
 * A some protocols may want to extract the EntityObject/EntityMember by own
 * specific literal naming.
 * The IEntityMapper mappings the requested literal
 * name to the registered literal name.
 *
 */
class IEntityMapper
{
  public:
    virtual ~IEntityMapper() = default;
};

} // namespace entity
} // namespace app
