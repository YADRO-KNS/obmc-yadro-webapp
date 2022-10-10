// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <config.h>

#include <core/request.hpp>
#include <core/response.hpp>
#include <core/router.hpp>
#include <fastcgi++/request.hpp>

namespace app
{
namespace core
{

class Connection : public Fastcgipp::Request<char>
{
    static constexpr const size_t maxBodySizeByte =
        (HTTP_REQ_BODY_LIMIT_MB << 20U);

  public:
    Connection();

    Connection(const Connection&) = delete;
    Connection(const Connection&&) = delete;

    Connection& operator=(const Connection&) = delete;
    Connection& operator=(const Connection&&) = delete;

    ~Connection() = default;

  protected:
    void inHandler(int) override;
    bool response() override;
    bool inProcessor() override;

  private:
    size_t totalBytesRecived;

    RequestPtr request;
    RouteUni router;
};

} // namespace core

} // namespace app
