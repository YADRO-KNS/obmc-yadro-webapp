// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <stdexcept>
#include <string>

namespace app
{
namespace core
{
namespace exceptions
{

class ObmcAppException : public std::runtime_error
{
  public:
    explicit ObmcAppException(const std::string& arg) :
        std::runtime_error(arg) _GLIBCXX_TXN_SAFE
    {}
    virtual ~ObmcAppException() = default;
};

class NotImplemented : public ObmcAppException
{
  public:
    explicit NotImplemented(const std::string& target) :
        ObmcAppException("The function '" + target +
                         "' not implemented") _GLIBCXX_TXN_SAFE
    {}
    virtual ~NotImplemented() = default;
};

class InvalidType : public ObmcAppException
{
  public:
    explicit InvalidType(const std::string& member) :
        ObmcAppException("The member '" + member +
                         "' has invalid type") _GLIBCXX_TXN_SAFE
    {}
    virtual ~InvalidType() = default;
};

} // namespace exceptions
} // namespace core
} // namespace app
