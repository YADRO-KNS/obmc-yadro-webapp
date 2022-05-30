// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <string>
#include <sstream>
#include <filesystem>
#include <vector>

namespace app
{
namespace helpers
{
namespace utils
{

inline long int countExtraSegmentsOfPath(const std::string& namespacePath,
                                       const std::string& path)
{
    if (namespacePath.length() >= path.length())
    {
        return 0;
    }

    return std::count(path.begin() +
                          static_cast<long int>(namespacePath.length()),
                      path.end(), '/');
}

inline bool constantTimeStringCompare(const std::string_view a,
                                      const std::string_view b)
{
    // Important note, this function is ONLY constant time if the two input
    // sizes are the same
    if (a.size() != b.size())
    {
        return false;
    }
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

struct ConstantTimeCompare
{
    bool operator()(const std::string_view a, const std::string_view b) const
    {
        return constantTimeStringCompare(a, b);
    }
};

inline bool base64Decode(const std::string_view input, std::string& output)
{
    if (input.empty())
    {
        return false;
    }

    const auto predictedLen = 3 * input.length() / 4;
    output.resize(predictedLen + 1);
    const std::vector<unsigned char> vectorChars{input.begin(), input.end()};

    const auto outputLength = EVP_DecodeBlock(
        reinterpret_cast<unsigned char*>(output.data()),
        vectorChars.data(), static_cast<int>(vectorChars.size()));

    if (predictedLen != static_cast<unsigned long>(outputLength))
    {
        return false;
    }

    return true;
}

/**
 * @brief Get string view of current date in the GMT view
 *
 * @return const std::string a literal view of current data.
 */
inline const std::string getFormattedCurrentDate(const std::string& format)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream timeStringStream;
    timeStringStream << std::put_time(std::localtime(&time), format.c_str());
    return timeStringStream.str();
}

inline std::string urlEncode(const std::string_view value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (const char c : value)
    {
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2)
                << static_cast<int>(static_cast<unsigned char>(c));
        escaped << std::nouppercase;
    }

    return escaped.str();
}

inline const std::string getNameFromLastSegmentObjectPath(const std::string& objectPath)
{
    std::size_t found = objectPath.rfind("/");
    if (found == std::string::npos)
    {
        throw std::logic_error(
            "Object path should not be ends by the '/'");
    }
    auto name = objectPath.substr(found + 1);
    std::replace_if(
        name.begin(), name.end(), [](char letter) { return letter == '_'; },
        ' ');
    return std::forward<std::string>(name);
}

inline const std::string toLower(const std::string& data)
{
    std::string lowerString;
    lowerString.resize(data.size());
    std::transform(data.begin(), data.end(), lowerString.begin(), ::tolower);
    return std::forward<const std::string>(lowerString);
}

} // namespace utils
} // namespace helpers
} // namespace app
