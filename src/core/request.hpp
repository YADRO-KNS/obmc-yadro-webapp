// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <fastcgi++/http.hpp>

#include <memory>

#include <service/session.hpp>

namespace app
{
namespace core
{

using namespace Fastcgipp;
using namespace Fastcgipp::Http;

/**
 * @brief the HTTP-request interface
 *
 */
class IRequest
{
  public:
    virtual ~IRequest() = default;

    virtual const Environment<char>& environment() const = 0;
    virtual const Environment<char>& environment() = 0;
    /**
     * @brief Checks the input HTTP request headers
     */
    virtual bool validate() = 0;

    virtual void setSession(const service::session::UserSessionPtr&) = 0;
    virtual const service::session::UserSessionPtr& getSession() const = 0;
    virtual bool isSessionEmpty() const = 0;
};

/**
 * @brief the HTTP Request class
 * Contains HTTP request Headers and related information.
 */
class Request : public IRequest
{
    const Http::Environment<char>& env;
    service::session::UserSessionPtr userSession;

  public:
    explicit Request() = delete;

    Request(const Environment<char>& argEnv) : env(argEnv), userSession(){};
    Request(const Request&) = delete;
    Request(const Request&&) = delete;

    Request& operator=(const Request&) = delete;
    Request& operator=(const Request&&) = delete;

    ~Request() = default;

    const Environment<char>& environment() const override;
    const Environment<char>& environment() override;

    /** @overload */
    bool validate() override;

    void setSession(const service::session::UserSessionPtr&) override;
    const service::session::UserSessionPtr& getSession() const override;
    bool isSessionEmpty() const override;
};

using RequestUni = std::unique_ptr<IRequest>;
using RequestPtr = std::shared_ptr<IRequest>;

} // namespace core

} // namespace app
