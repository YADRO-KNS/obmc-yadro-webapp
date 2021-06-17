// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __QUERY_H__
#define __QUERY_H__

#include <core/entity/entity.hpp>
#include <logger/logger.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <map>
#include <utility>
#include <variant>
#include <vector>

namespace app
{
namespace query
{

template <class TInstance, class TConnect>
class IQuery;

template <class TInstance, class TConnect>
class IQuery
{
  public:
    typedef TConnect ConnectType;

    virtual ~IQuery() = default;
    /**
     * @brief Run the configured quiry.
     *
     * @return std::vector<TInstance> The list of retrieved data which
     * incapsulated at TInstance objects
     */
    virtual std::vector<TInstance> process(const TConnect&) = 0;
};

} // namespace query
} // namespace app

#endif // __QUERY_H__
