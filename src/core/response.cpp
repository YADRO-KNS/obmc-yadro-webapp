
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/response.hpp>

#include <logger/logger.hpp>
#include <http/headers.hpp>

namespace app
{
namespace core
{

size_t Response::totalSize() const
{
    return internalBuffer.length();
}

const statuses::Code& Response::getStatus()
{
    return this->status;
}

void Response::setStatus(const statuses::Code& status)
{
    this->status = status;
}

void Response::setHeader(const std::string& headerName, const std::string& value)
{
    headerBuffer += (app::http::header(headerName, value) + endHeaderLine);
}

const std::string Response::getHeaders() const
{
    return (headerBuffer + endHeaderLine);
}

const std::string& Response::getBody() const
{
    return internalBuffer;
}

void Response::push(const std::string& buffer)
{
    internalBuffer += buffer;
}

void Response::clear()
{
    internalBuffer.clear();
}

} // namespace core
} // namespace app
