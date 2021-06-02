
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/request.hpp>
#include <logger/logger.hpp>

#include <set>
#include <string>

namespace app
{
namespace core
{
const Environment<char>& Request::environment() const
{
    return env;
}

const Environment<char>& Request::environment()
{
    return env;
}

bool Request::validate()
{
    static const std::set<std::string> allowedContentTypes{
        "application/json",
        "text/plain",
    };

    return !environment().contentType.empty() &&
           allowedContentTypes.contains(environment().contentType);
}

void Request::setSession(const service::session::UserSessionPtr& instance)
{
    this->userSession = instance;
}

const service::session::UserSessionPtr& Request::getSession() const
{
    return this->userSession;
}

bool Request::isSessionEmpty() const
{
    return !this->userSession;
}

} // namespace core
} // namespace app
