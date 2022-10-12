// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021, KNS Group LLC (YADRO)

#include <http/headers.hpp>

#include <iostream>
#include <map>

#include <gtest/gtest.h>

using namespace app::http;
using namespace app::http::statuses;

static std::map<const char*, const std::string> testHeadesSamples{
    {"statusOkTest", "HTTP/1.1 200 OK"},
    {"contentTypeOkTest", "Content-Type: application/json; charset=UTF-8"},
    {"contentLengthOkTest", "Content-Length: "},
    {"contentTypeDateTest", "Date: "},
};

TEST(header, testHeaderStatus)
{
    EXPECT_EQ(testHeadesSamples["statusOkTest"], headerStatus(Code::OK));
}

TEST(header, testHeaderContentType)
{
    EXPECT_EQ(testHeadesSamples["contentTypeOkTest"],
              header(headers::contentType, content_types::applicationJson));
}

TEST(header, testHeaderContentLength)
{
    EXPECT_TRUE(header(headers::contentType, content_types::applicationJson)
                    .starts_with(testHeadesSamples["contentTypeOkTest"]));
}

TEST(header, testHeaderContentDate)
{
    EXPECT_TRUE(header(headers::date, "Wed, 14 Apr 2021 11:28:34 GMT")
                    .starts_with(testHeadesSamples["contentTypeDateTest"]));
}
