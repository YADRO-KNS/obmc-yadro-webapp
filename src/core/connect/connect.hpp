// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __CONNECT_H__
#define __CONNECT_H__

namespace app
{
namespace connect
{

class IConnect
{
  public:
    virtual void connect() = 0;
    virtual bool disconnect() = 0;
    virtual ~IConnect() = default;
};

} // namespace connect
} // namespace app
#endif // __CONNECT_H__
