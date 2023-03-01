// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#include "dbus_connect.hpp"

#include <systemd/sd-bus.h>
#include <unistd.h>

#include <core/connect/connect.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/exceptions.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>

#include <chrono>
#include <cstring>
#include <functional>
#include <memory>

namespace app
{
namespace connect
{

using namespace sdbusplus::bus;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::literals;
using namespace phosphor::logging;

DBusConnect::DBusConnect() : alive(false), dbusConnect(nullptr)
{}

DBusConnect::~DBusConnect() noexcept
{
    this->terminate();
    this->disconnect();
}

void DBusConnect::run()
{
    alive.store(true);
    thread =
        std::make_unique<std::thread>(std::bind(&DBusConnect::process, this));
}

void DBusConnect::terminate() noexcept
{
    alive.store(false);
    if (thread && thread->joinable())
    {
        thread->join();
        log<level::DEBUG>("DBus connection thread is terminated");
    }
}

match::match DBusConnect::createWatcher(const std::string& rule,
                                        match::match::callback_t handler)
{
    DBusCallGuard<match::match> guard(capturedByThreadId, [&] {
        log<level::DEBUG>("Create DBus signal watcher",
                          entry("RULE=%s", rule.c_str()));
        return std::forward<match::match>(
            match::match(*getConnect(), rule, handler));
    });
    return std::forward<match::match>(guard());
}

void DBusConnect::process()
{
    while (alive.load())
    {
        query::dbus::DBusInstance::cleanupInstacesWatchers();
        try
        {
            {
                // non-blocking wait to allow atomic exchange dbus-call-window
                // state
                DBusCallGuard<void> guard(
                    capturedByThreadId,
                    [&]() {
                        getConnect()->wait(0);
                        getConnect()->process_discard();
                    },
                    10);
                guard();
            }
            std::this_thread::sleep_for(20ms);
        }
        catch (const sdbusplus::exception_t& ex)
        {
            log<level::CRIT>("Fail to process DBus handlers",
                             entry("ERROR=%s", ex.what()),
                             entry("DESC=%s", ex.description()));
            if (errno == -EIO || errno == -EBADMSG)
            {
                log<level::CRIT>(
                    "DBus connection is corrupt. No way to continue processing "
                    "with the current context.",
                    entry("ERROR=%s", ex.what()),
                    entry("DESC=%s", ex.description()));
                exit(EXIT_FAILURE);
            }
        }
    }
    log<level::DEBUG>("Finish task of dbus connection handler");
}

std::unique_ptr<sdbusplus::bus::bus>& DBusConnect::getConnect()
{
    if (!sdbusConnect)
    {
        throw app::core::exceptions::ObmcAppException(
            "The DBus connection was lost");
    }
    return sdbusConnect;
}

bool DBusConnect::disconnect() noexcept
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

void DBusConnect::updateWellKnownServiceNameDict()
{
    auto listNamesAcquired = sdbusConnect->list_names_acquired();
    for (auto& serviceName : listNamesAcquired)
    {
        // There is unique name of DBus service. Skip
        if (serviceName.starts_with(":"))
        {
            continue;
        }

        try
        {
            auto uniqueServiceName = this->callMethodAndRead<std::string>(
                "org.freedesktop.DBus", "/", "org.freedesktop.DBus",
                "GetNameOwner", serviceName);
            setServiceName(uniqueServiceName, serviceName);
        }
        catch (const sdbusplus::exception_t& ex)
        {
            log<level::ERR>("Fail to acquire DBus services list",
                            entry("ERROR=%s", ex.what()));
        }
    }
}

void DBusConnect::initSdBusConnection()
{
    if (!dbusConnect)
    {
        throw std::logic_error("The dbus connection is not initialized");
    }
    if (sdbusConnect)
    {
        throw std::logic_error("The SDBus connection already initialized");
    }

    sdbusConnect = std::make_unique<sdbusplus::bus::bus>(dbusConnect);
    updateWellKnownServiceNameDict();
    initServiceNamesWatcher();
    configureObjectManagingHandlers();
}

void DBusConnect::initServiceNamesWatcher()
{
    using namespace sdbusplus::bus::match;
    static auto watcher = createWatcher(
        rules::nameOwnerChanged(),
        [this](sdbusplus::message::message& message) {
            std::string name;     // well-known
            std::string oldOwner; // unique-name
            std::string newOwner; // unique-name

            try
            {
                message.read(name, oldOwner, newOwner);
            }
            catch (sdbusplus::exception_t& ex)
            {

                log<level::ERR>("Fail to observe services list changes",
                                entry("ERROR=%s", ex.what()));
                return;
            }
            if (name.starts_with(":"))
            {
                return;
            }
            setServiceName(newOwner.size() ? newOwner : oldOwner, name);
        });
}

void DBusConnect::setServiceName(const std::string& uniqueName,
                                 const std::string& wellKnownName)
{
    log<level::DEBUG>("Set Well-Known service name",
                      entry("SVC_UNI_NAME=%s", uniqueName.c_str()),
                      entry("SVC_WK_NAME=%s", wellKnownName.c_str()));
    serviceNamesDict.insert_or_assign(uniqueName, wellKnownName);
}

void DBusConnect::configureObjectManagingHandlers()
{
    using namespace sdbusplus::bus::match;
    using namespace app::query::dbus;

    globalSignalHandlers.push_back(std::move(createWatcher(
        rules::interfacesAdded(), [](sdbusplus::message::message& message) {
            DBusQuery::processObjectCreate(message);
        })));

    globalSignalHandlers.push_back(std::move(createWatcher(
        rules::interfacesRemoved(), [](sdbusplus::message::message& message) {
            DBusQuery::processObjectRemove(message);
        })));
}

const std::string&
    DBusConnect::getWellKnownServiceName(const std::string& uniqueName)
{
    using namespace std::literals;

    size_t tryCount = 0;
    auto it = serviceNamesDict.find(uniqueName);
    while (it == serviceNamesDict.end())
    {
        std::this_thread::sleep_for(1s);
        // try to acquire directly updating well-known service dict.
        updateWellKnownServiceNameDict();
        it = serviceNamesDict.find(uniqueName);
        if (tryCount > 15)
        {
            throw std::runtime_error("Required service '" + uniqueName +
                                     "' not provided by DBus");
        }
        tryCount++;
    }
    return it->second;
}

DBusConnectUni DBusConnect::createDbusConnection()
{
#ifdef BMC_DBUS_CONNECT_SYSTEM
    connect::DBusConnectUni connect = std::make_unique<SystemDBusConnect>();
#elif defined(BMC_DBUS_CONNECT_REMOTE)
    connect::DBusConnectUni connect =
        std::make_unique<RemoteHostDbusConnect>(BMC_DBUS_REMOTE_HOST);
#else
#error "Unknown DBus connection type"
#endif
    connect->connect();

    return std::forward<connect::DBusConnectUni>(connect);
}

void SystemDBusConnect::connect()
{
    auto status = sd_bus_open_system(&dbusConnect);
    if (status < 0)
    {
        throw std::runtime_error(
            std::string("Can't establish dbus connection: ") +
            std::strerror(-status));
    }

    log<level::DEBUG>("Established DBus connection on system bus");
    initSdBusConnection();
}

#ifdef BMC_DBUS_CONNECT_REMOTE
void RemoteHostDbusConnect::connect()
{
    auto status = sd_bus_open_system_remote(&dbusConnect, hostname.c_str());
    if (status < 0)
    {
        throw std::runtime_error(
            std::string("Can't establish dbus connection: ") +
            std::strerror(-status));
    }
    log<level::DEBUG>("Established DBus connection to Host",
                      entry("HOST=%s", hostname.c_str()));
    initSdBusConnection();
}
#endif

} // namespace connect
} // namespace app
