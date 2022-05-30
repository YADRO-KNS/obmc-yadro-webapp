// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <fastcgi++/http.hpp>
#include <http/headers.hpp>
#include <nlohmann/json.hpp>

#include <iostream>
#include <memory>
#include <string>

namespace app
{
namespace core
{

using namespace Fastcgipp;
using namespace app::http;

class IResponse;

using ResponseUni = std::unique_ptr<IResponse>;
using ResponsePtr = std::shared_ptr<IResponse>;

/**
 * @brief the HTTP Response Interface
 */
class IResponse
{
  public:
    /**
     * @brief get tatal size of response data
     *
     * @return size_t
     */
    virtual size_t totalSize() const = 0;
    /**
     * @brief get the buffer of body
     *
     * @return std::string
     */
    virtual const std::string& getBody() const = 0;
    /**
     * @brief Set the HTTP response code
     *
     * @return statuses::Code
     */
    virtual const statuses::Code& getStatus() = 0;
    /**
     * @brief Set the Status object
     *
     * @param status
     */
    virtual void setStatus(const statuses::Code&) = 0;
    /**
     * @brief Set the Content Type of response
     * 
     * @param contentType - the type of content contains at the response
     */
    virtual void setContentType(const std::string&) = 0;
    /**
     * @brief Set the HTTP Header
     *
     * @param headerName - name of header
     * @param headerValue - value of header
     */
    virtual void setHeader(const std::string&, const std::string&) = 0;

    /**
     * @brief Get the Headers buffer
     *
     * @return const std::string& the buffer of all headers.
     */
    virtual const std::string getHead() const = 0;
    /**
     * @brief Destroy the IResponse object
     *
     */
    virtual ~IResponse() = default;

    /**
     * @brief Add the response data which will be written to the output fastCGI
     *        pipe
     *
     * @param buffer data
     */
    virtual void push(const std::string&) = 0;
    /**
     * @brief clear the internal output buffer
     */
    virtual void clear() = 0;

    /**
     * @brief Pipe of HTTP-payload stream
     *
     * @param os out stream
     * @param response target
     * @return std::ostream& out stream
     */
    friend std::ostream& operator<<(std::ostream& os, const IResponse& response)
    {
        os << response.getHead();
        os << response.getBody();
        return os;
    }
};

class Response : public IResponse
{
    static constexpr const char* endHeaderLine = "\r\n";

  public:
    explicit Response() :
        status(statuses::Code::NotFound),
        contentType(content_types::textPlain){};
    Response(const Response&) = delete;
    Response(const Response&&) = delete;

    Response& operator=(const Response&) = delete;
    Response& operator=(const Response&&) = delete;
    /**
     * @brief Construct a new Response object
     *
     * @param argEnv
     */
    ~Response() override = default;

    size_t totalSize() const override;

    const std::string& getBody() const override;

    const statuses::Code& getStatus() override;
    void setContentType(const std::string&) override;
    void setStatus(const statuses::Code&) override;

    void setHeader(const std::string&, const std::string&) override;
    const std::string getHead() const override;

    void push(const std::string&) override;

    void clear() override;

  private:
    std::string headerBuffer;
    std::string internalBuffer;
    statuses::Code status;
    std::string contentType;
};

} // namespace core

} // namespace app
