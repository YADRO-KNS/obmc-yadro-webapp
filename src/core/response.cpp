
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/response.hpp>

#include <http/headers.hpp>
#include <core/helpers/utils.hpp>

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

void Response::setContentType(const std::string& contentType)
{
    this->contentType = contentType;
}

void Response::setStatus(const statuses::Code& status)
{
    this->status = status;
}

void Response::setHeader(const std::string& headerName, const std::string& value)
{
    headerBuffer += (app::http::header(headerName, value) + endHeaderLine);
}

const std::string Response::getHead() const
{
    const auto contentTypeHeader =
        app::http::header(headers::contentType, contentType) + endHeaderLine;
    return (headerStatus(status) + endHeaderLine + contentTypeHeader +
            headerBuffer + endHeaderLine);
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
