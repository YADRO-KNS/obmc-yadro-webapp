// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __DBUSCONNECT_H__
#define __DBUSCONNECT_H__

#include <logger/logger.hpp>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <core/connect/connect.hpp>

#include <cstring>
#include <functional>
#include <memory>

namespace app
{
namespace connect
{
class DBusConnect;
using DBusConnectUni = std::unique_ptr<DBusConnect>;

class DBusConnect : public IConnect
{
    std::unique_ptr<sdbusplus::bus::bus> sdbusConnect;
  public:
    DBusConnect(const DBusConnect&) = delete;
    DBusConnect& operator=(const DBusConnect&) = delete;
    DBusConnect(DBusConnect&&) = delete;
    DBusConnect& operator=(DBusConnect&&) = delete;

    explicit DBusConnect() = default;

    std::unique_ptr<sdbusplus::bus::bus>& getConnect()
    {
        if (!sdbusConnect)
        {
            throw app::core::exceptions::ObmcAppException(
                "The DBus connection was lost");
        }
        return sdbusConnect;
    }

    bool disconnect() override
    {
        if (sdbusConnect)
        {
            sdbusConnect.reset();
        }
        if (dbusConnect)
        {
            sd_bus_close(dbusConnect);
            return true;
        }

        return false;
    }

    ~DBusConnect() noexcept override
    {
        this->disconnect();
    };

    protected:
        sd_bus* dbusConnect;

        void initSdBusConnection() {
            if (!dbusConnect) {
                throw std::logic_error(
                    "The dbus connection is not initialized");
            }
            if (sdbusConnect) {
                throw std::logic_error(
                    "The SDBus connection already initialized");
            }
            sdbusConnect = std::make_unique<sdbusplus::bus::bus>(dbusConnect);
        }
};

class SystemDBusConnect : public DBusConnect
{
  public:
    SystemDBusConnect() noexcept
    {}
    ~SystemDBusConnect() override = default;

    void connect() override
    {
        auto status = sd_bus_open_system(&dbusConnect);
        if (status < 0)
        {
            throw std::runtime_error(
                std::string("Can't establish dbus connection. Reason: ") +
                std::strerror(status));
        }
        BMC_LOG_DEBUG << "Establish DBus connection on system bus";
        initSdBusConnection();
    }
};

class RemoteHostDbusConnect : public DBusConnect
{
    const std::string hostname;

  public:
    RemoteHostDbusConnect(const std::string& dbusHostname) noexcept :
        hostname(dbusHostname)
    {
        BMC_LOG_DEBUG << "DBUS hostname:  " << hostname;
    }
    ~RemoteHostDbusConnect() override = default;

    void connect() override
    {
        auto status = sd_bus_open_system_remote(&dbusConnect, hostname.c_str());
        if (status < 0)
        {
            throw std::runtime_error(
                std::string("Can't establish dbus connection. Reason: ") +
                std::strerror(status));
        }
        BMC_LOG_DEBUG << "Establish DBus connection to Host: " << hostname;
        initSdBusConnection();
    }
};

} // namespace connect
} // namespace app
#endif // __DBUSCONNECT_H__
