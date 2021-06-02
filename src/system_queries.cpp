// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/exceptions.hpp>
#include <system_queries.hpp>

namespace app
{
namespace query
{
namespace obmc
{

const std::map<std::string, std::string> Chassis::chassisTypesNames{
    {"23", "Rack Mount"},
};

} // namespace obmc
} // namespace query
} // namespace app
