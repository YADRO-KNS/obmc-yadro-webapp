// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/entity/entity.hpp>

#include <variant>
#include <vector>

namespace app
{
namespace query
{
namespace proxy
{

using namespace app::entity;

class ProxyQuery;
using ProxyQueryPtr = std::shared_ptr<ProxyQuery>;

/**
 * @class ProxyQuery
 * @brief The IInstance of data might incoming from different points and
 *        different ways. The specific engine of IEntity data representation
 *        might requires to interpert the same data that acquired a IQuery by
 *        different ways. E.g:
 *          * `FooQuery` obtained five instances, each provides `FooField`
 *          * `FooEntity` configure `FooMember` that is directly provide
 *            `FooQuery::Instance::FooField`
 *          * `BarEntity` requires provide `FooQuery::Instance::FooField` too,
 *            but formatting the field shoud be done by specific logic and the
 *            field name desires to be `BarField`
 *          * To prevent double-processing `FooQuery`, the `ProxyQuery` can
 *            proxy all `IInstances` form `FooEntity` to `BarEntity`.
 *
 * @note  The ProxyQuery handles only with IEntityProvider, to make easer
 *        obtaining relevant proxying data and directly supplement target
 *        IEntity.
 */
class ProxyQuery : public IQuery
{
    const EntitySupplementProviderPtr provider;

  public:
    ProxyQuery(const EntitySupplementProviderPtr& provider) : provider(provider)
    {
        provider->initialize();
    }
    ~ProxyQuery() override = default;
    /**
     * @brief Run the configured quiry.
     *
     * @return std::vector<TInstance> The list of retrieved data which
     *                                incapsulated at TInstance objects
     */
    const entity::IEntity::InstanceCollection process() override
    {
        log<level::DEBUG>("Processing proxy-query",
                          entry("SUPLPRVDR=%s", provider->getName().c_str()));
        provider->populate();
        return provider->getInstances();
    }

    /**
     * @brief Get the collection of query fields
     *
     * @return const QueryFields - The collection of fields
     */
    const QueryFields getFields() const override
    {
        QueryFields fields;
        const auto& members = provider->getMembers();
        for (const auto& [memberName, _] : members)
        {
            fields.push_back(memberName);
        }
        return std::forward<QueryFields>(fields);
    }

  protected:
    const EntitySupplementProviderPtr& getProvider() const
    {
        return provider;
    }
};

} // namespace proxy
} // namespace query
} // namespace app
