// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#include <config.h>
#include <core/helpers/utils.hpp>

#ifdef LOG_ENGINE_PHOSPHOR
#include <phosphor-logging/log.hpp>

#define CURRENT_PHOSPHOR_LOG_LEVEL()                                           \
    phosphor::logging::level::BMC_LOGGING_LEVEL
#endif

#define CURRENT_LOG_LEVEL_VALUE() (LogLevel::BMC_LOGGING_LEVEL)
namespace app
{

enum class LogLevel
{
    DEBUG = 0,
    INFO,
    NOTICE,
    WARNING,
    ERR,
    CRIT,
    ALERT,
    EMERG
};

class Logger
{
    static constexpr const char* timestampDateFormat = "%Y-%m-%d %H:%M:%S";

  public:
    Logger([[maybe_unused]] const std::string& prefix,
           [[maybe_unused]] const std::string& filename,
           [[maybe_unused]] const size_t line, LogLevel levelIn) :
        level(levelIn)
    {
#ifndef LOG_ENGINE_PHOSPHOR
        stringstream << "("
                     << app::helpers::utils::getFormattedCurrentDate(
                            timestampDateFormat)
                     << ")";
#endif
        stringstream << "[" << prefix << " "
                     << std::filesystem::path(filename).filename() << ":"
                     << line << "] ";
    }
    ~Logger()
    {
        if (level >= getCurrentLogLevel())
        {
            stringstream << std::endl;

#ifdef LOG_ENGINE_PHOSPHOR
            phosphor::logging::log<CURRENT_PHOSPHOR_LOG_LEVEL()>(
                stringstream.str().c_str());
#else
            std::cerr << stringstream.str();
#endif
        }
    }

    template <typename T>
    Logger& operator<<([[maybe_unused]] T const& value)
    {
        if (level >= getCurrentLogLevel())
        {
            stringstream << value;
        }
        return *this;
    }

    static void setLogLevel(LogLevel level)
    {
        getLogLevelRef() = level;
    }

    static LogLevel getCurrentLogLevel()
    {
        return getLogLevelRef();
    }

  private:
    static LogLevel& getLogLevelRef()
    {
        static auto currentLevel = CURRENT_LOG_LEVEL_VALUE();
        return currentLevel;
    }

    std::ostringstream stringstream;
    app::LogLevel level;
};

#define BMC_LOG_EMERG                                                              \
    app::Logger("EMERGENCY", __FILE__, __LINE__, app::LogLevel::EMERG)
#define BMC_LOG_ALERT app::Logger("ALERT", __FILE__, __LINE__, app::LogLevel::ALERT)
#define BMC_LOG_CRITICAL                                                           \
    app::Logger("CRITICAL", __FILE__, __LINE__, app::LogLevel::CRIT)
#define BMC_LOG_ERROR app::Logger("ERROR", __FILE__, __LINE__, app::LogLevel::ERR)
#define BMC_LOG_WARNING                                                            \
    app::Logger("WARNING", __FILE__, __LINE__, app::LogLevel::WARNING)
#define BMC_LOG_INFO app::Logger("INFO", __FILE__, __LINE__, app::LogLevel::INFO)
#define BMC_LOG_DEBUG app::Logger("DEBUG", __FILE__, __LINE__, app::LogLevel::DEBUG)

} // namespace app
#endif // __LOGGER_H__
