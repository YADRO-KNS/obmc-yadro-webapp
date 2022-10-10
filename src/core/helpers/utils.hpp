// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <systemd/sd-id128.h>

#include <phosphor-logging/log.hpp>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace app
{
namespace helpers
{
namespace utils
{
using namespace phosphor::logging;

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

inline auto base64Decode(const std::string& input) -> const std::string
{
    const auto predictedLen = 3 * input.length() / 4; // predict output size
    const auto outputBuffer{std::make_unique<char[]>(predictedLen + 1)};
    const std::vector<unsigned char> vecChars{
        input.begin(), input.end()}; // convert to decode into uchar container

    const auto outputLen =
        EVP_DecodeBlock(reinterpret_cast<unsigned char*>(outputBuffer.get()),
                        vecChars.data(), static_cast<int>(vecChars.size()));

    if (predictedLen != static_cast<unsigned long>(outputLen))
    {
        throw std::runtime_error("DecodeBase64 error");
    }

    return outputBuffer.get();
}

/**
 * @brief Get string view of current date
 * @param format    - The format of string view of datetime
 * @param time      - The time to formatting
 * @return const std::string a literal view of current data.
 */
inline const std::string getFormattedDate(const std::string& format,
                                          const std::time_t& time)
{
    std::stringstream timeStringStream;
    timeStringStream << std::put_time(std::localtime(&time), format.c_str());
    return timeStringStream.str();
}

/**
 * @brief Get string view of current date
 * @param format - The format of string view of datetime
 * @return const std::string a literal view of current data.
 */
inline const std::string getFormattedCurrentDate(const std::string& format)
{
    std::time_t time = std::time(nullptr);
    return getFormattedDate(format, time);
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

inline const std::string
    getNameFromLastSegmentObjectPath(const std::string& objectPath,
                                     bool cleanup = true)
{
    std::size_t found = objectPath.rfind("/");
    if (found == std::string::npos)
    {
        log<level::DEBUG>("The object to parse is invalid",
                          entry("OBJPATH=%s", objectPath.c_str()));
        return objectPath;
    }
    auto name = objectPath.substr(found + 1);
    if (cleanup)
    {
        std::replace_if(
            name.begin(), name.end(), [](char letter) { return letter == '_'; },
            ' ');
    }
    return std::forward<std::string>(name);
}

inline const std::string toLower(const std::string& data)
{
    std::string lowerString;
    lowerString.resize(data.size());
    std::transform(data.begin(), data.end(), lowerString.begin(), ::tolower);
    return std::forward<const std::string>(lowerString);
}

/**
 * @brief Retrieve service root UUID
 *
 * @return Service root UUID
 */
inline std::string getUuid()
{
    std::string ret;
    // This ID needs to match the one in ipmid
    sd_id128_t appId{{0Xe0, 0Xe1, 0X73, 0X76, 0X64, 0X61, 0X47, 0Xda, 0Xa5,
                      0X0c, 0Xd0, 0Xcc, 0X64, 0X12, 0X45, 0X78}};
    sd_id128_t machineId{};

    if (sd_id128_get_machine_app_specific(appId, &machineId) == 0)
    {
        std::array<char, SD_ID128_STRING_MAX> str;
        ret = sd_id128_to_string(machineId, str.data());
        ret.insert(8, 1, '-');
        ret.insert(13, 1, '-');
        ret.insert(18, 1, '-');
        ret.insert(23, 1, '-');
    }

    return ret;
}

/**
 * @brief Trim the string from start
 *
 * @param s
 */
static inline void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

// Trim the string from end
static inline void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

// Trim the string both start and end
static inline const std::string trim(std::string s)
{
    ltrim(s);
    rtrim(s);
    return s;
}

inline const auto getNetmask(uint32_t bits)
{
    uint32_t value = 0xffffffff << (32 - bits);
    return std::to_string((value >> 24) & 0xff) + "." +
           std::to_string((value >> 16) & 0xff) + "." +
           std::to_string((value >> 8) & 0xff) + "." +
           std::to_string(value & 0xff);
}

static inline const std::vector<std::string> splitToVector(std::stringstream ss,
                                                           char delimiter)
{
    std::vector<std::string> result;
    while (ss.good())
    {
        std::string substr;
        getline(ss, substr, delimiter);
        result.push_back(substr);
    }
    return result;
}

static inline const std::pair<std::string, std::string>
    splitToPair(const std::string& string, char delimiter)
{
    auto pos = string.find(delimiter);
    if (pos == std::string::npos || string.length() <= pos)
    {
        return std::make_pair(string, "");
    }
    return std::make_pair(string.substr(0, pos), string.substr(pos + 1));
}

struct OpenSSLGenerator
{
    uint8_t operator()()
    {
        uint8_t index = 0;
        int rc = RAND_bytes(&index, sizeof(index));
        if (rc != opensslSuccess)
        {
            log<level::DEBUG>("Cannot get random number");
            err = true;
        }

        return index;
    }

    uint8_t max()
    {
        return std::numeric_limits<uint8_t>::max();
    }
    uint8_t min()
    {
        return std::numeric_limits<uint8_t>::min();
    }

    bool error()
    {
        return err;
    }

    // all generators require this variable
    using result_type = uint8_t;

  private:
    // RAND_bytes() returns 1 on success, 0 otherwise. -1 if bad function
    static constexpr int opensslSuccess = 1;
    bool err = false;
};

} // namespace utils
} // namespace helpers
} // namespace app
