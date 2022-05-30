// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

namespace app
{
namespace connect
{

class IConnect
{
  public:
    /**
     * @brief Connect to the target service.
     */
    virtual void connect() = 0;
    /**
     * @brief Close already established connection
     *
     * @return true   - sucess
     * @return false  - something went wrong
     */
    virtual bool disconnect() noexcept = 0;
    virtual ~IConnect() = default;
};

} // namespace connect
} // namespace app
