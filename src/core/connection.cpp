
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/connection.hpp>
#include <http/headers.hpp>
#include <logger/logger.hpp>

namespace app
{
namespace core
{

Connection::Connection() :
    Fastcgipp::Request<char>(maxBodySizeByte), totalBytesRecived(0), request(),
    router()
{}

void Connection::inHandler(int bytesReceived)
{
    BMC_LOG_DEBUG << "In handler, bytes count=" << bytesReceived;
    totalBytesRecived += static_cast<size_t>(bytesReceived);
    if (!request)
    {
        request = std::make_shared<app::core::Request>(this->environment());
    }
}

bool Connection::inProcessor()
{
    BMC_LOG_DEBUG << "Custom 'Content Type' detected";

    if (environment().contentType.empty())
    {
        BMC_LOG_ALERT << "Unknown Content Type. Immediate close";
        return false;
    }
    BMC_LOG_DEBUG << "Content Type: " << environment().contentType;
    if (!request)
    {
        BMC_LOG_CRITICAL << "The request not initialized.";
        return false;
    }

    if (!router)
    {
        router = std::make_unique<app::core::Router>(this->request);
    }

    // Processing the 'preHandlers' in the 'inProcessor', because after that
    // procedure the Post Data Buffer will be cleared.
    return request->validate() && router->preHandler();
}

bool Connection::response()
{
    BMC_LOG_DEBUG << "New Connection. Response";
    using namespace Fastcgipp::Http;

    if (!router)
    {
        BMC_LOG_CRITICAL << "The route handler not initialized.";
        return false;
    }

    BMC_LOG_DEBUG << "Process Route.";
    const auto& responsePtr = router->process();
    BMC_LOG_DEBUG << "Write Headers.";
    writeHeader(responsePtr);
    BMC_LOG_DEBUG << "Write response to out.";

    out << *responsePtr;
    BMC_LOG_DEBUG << "Immediate flush data.";
    out.flush();

    return true;
}

void Connection::writeHeader(const ResponseUni& responsePointer)
{
    using namespace app::http;
    constexpr const char* headerDateFormat = "%a, %d %b %Y %H:%M:%S GMT";
    auto status = responsePointer->getStatus();

    BMC_LOG_DEBUG << "Write status header." << static_cast<int>(status);
    out << headerStatus(status) << std::endl;

    responsePointer->setHeader(headers::contentType,
                               content_types::applicationJson);
    responsePointer->setHeader(headers::contentLength,
                               std::to_string(responsePointer->totalSize()));
    responsePointer->setHeader(
        headers::date,
        app::helpers::utils::getFormattedCurrentDate(headerDateFormat));

    out << responsePointer->getHeaders();
}

} // namespace core

} // namespace app
