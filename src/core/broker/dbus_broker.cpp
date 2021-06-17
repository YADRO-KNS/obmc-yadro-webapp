// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <config.h>

#include <core/broker/dbus_broker.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/exceptions.hpp>
#include <sdbusplus/bus/match.hpp>

#include <thread>
#include <vector>

namespace app
{
namespace broker
{

bool EntityDbusBroker::tryProcess(const connect::DBusConnectionPoolUni& connections)
{
    std::unique_lock<std::mutex> lock(guardMutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        BMC_LOG_DEBUG
            << "Attempt to process a broker's task which already runs.";
        return false;
    }

    if (!isTimeout())
    {
        BMC_LOG_DEBUG << "Attempt to process the task which is not timed out yet.";
        return false;
    }

    if (!this->entityQuery)
    {
        throw app::core::exceptions::ObmcAppException(
            "Query not registried! Entity=" + entity->getName());
    }
    BMC_LOG_INFO << "Accept broker task of Entity '" << entity->getName() << "'";

    try
    {
        auto instances =
            this->entityQuery->process(connections->getQueryConnection());

        // Register watchers if the broker is configured to watch of DBus
        // signals.
        if (this->isWatch())
        {
            for (auto& instance : instances)
            {
                auto dbusInstance =
                    std::dynamic_pointer_cast<dbus::DBusInstance>(instance);
                if (!dbusInstance)
                {
                    throw std::logic_error("Found a non-DBusInstance during "
                                           "DBusBroker query processin.");
                }
                dbusInstance->bindListeners(connections->getWatcherConnection());
            }
        }
        this->entity->setInstances(instances);
    }
    catch (sdbusplus::exception::SdBusError& ex)
    {
        BMC_LOG_ERROR << "Can't process task: " << ex.what();
        return false;
    }
    this->setExecutionTime();

    BMC_LOG_DEBUG << "Query processed successfuly. Entity '" << entity->getName()
              << "'";
    return true;
}

void EntityDbusBroker::registerObjectsListener(
    const connect::DBusConnectionPoolUni& connections)
{
    auto dbusQuery =
        std::dynamic_pointer_cast<query::dbus::EntityDBusQuery>(entityQuery);

    if (!dbusQuery)
    {
        throw std::logic_error(
            "At the DBusBroker the entity query is not EntityDBusQuery.");
    }

    dbusQuery->registerObjectCreationObserver(connections, this->entity);
    dbusQuery->registerObjectRemovingObserver(connections, this->entity);
}

void DBusBrokerManager::start()
{
    static const std::vector<std::pair<size_t, std::function<void()>>>
        countTaskThreadsDict{
            {queryBrokerThreadCount,
             std::bind(&DBusBrokerManager::doCaptureDbus, this)},
            {singleThreadTask,
             std::bind(&DBusBrokerManager::doManageringObjects, this)},
            {singleThreadTask,
             std::bind(&DBusBrokerManager::doWatchDBusServiceOwners, this)},
        };

    setupDBusServiceNames();

    active = true;
    size_t totalTasks = 0;
    for (auto& [count, handler] : countTaskThreadsDict)
    {
        totalTasks += count;
        for (size_t threadNumber = 0; threadNumber < count; threadNumber++)
        {
            BMC_LOG_DEBUG << "init broker task #" << threadNumber;
            threads.push_back(std::thread(handler));
        }
    }
    BMC_LOG_INFO << "Total " << totalTasks << " DBus brokers tasks started";
}

void DBusBrokerManager::terminate()
{
    active = false;
    BMC_LOG_INFO << "Request to terminate DBus brokers";

    for (auto& thread : threads)
    {
        thread.join();
    }
}

void DBusBrokerManager::bind(DBusBrokerPtr broker)
{
    if (broker->isWatch())
    {
        broker->registerObjectsListener(getConnectionPool());
    }
    this->brokers.push_back(broker);
}

void DBusBrokerManager::doCaptureDbus()
{
    // Since the access to the dbus-connection is not thread-safe, adding
    // the object listeners from the processing query might be when another
    // thread does `sdbusplus::bus::bus::process()`, which will
    // cause a segfault.
    // Open a new connection pool for each thread.
    connect::DBusConnectionPoolUni connections =
        std::make_unique<connect::DBusConnectionPool>();
    while (active)
    {
        for (const auto& broker : brokers)
        {
            if (broker->isTimeout())
            {
                BMC_LOG_DEBUG << "Try process broker task: #"
                          << std::this_thread::get_id();
                if (!broker->tryProcess(connections))
                {
                    BMC_LOG_DEBUG << "Can't process broker task";
                }
            }
        }

        // Process watcher while haven't ready tasks
        do
        {
            connections->getWatcherConnection()->getConnect()->wait(500ms);
            try
            {
                connections->getWatcherConnection()
                    ->getConnect()
                    ->process_discard();
            }
            catch (sdbusplus::exception_t& ex)
            {
                BMC_LOG_CRITICAL << "Can't process dbus handler: " << ex.what();
            }
        } while (active && !hasReadyTask());
    }
    BMC_LOG_INFO << "Terminate broker ID #" << std::this_thread::get_id();
}

void DBusBrokerManager::doManageringObjects()
{
    while (active)
    {
        auto& connection =
            getConnectionPool()->getWatcherConnection()->getConnect();
        connection->wait(500ms);
        try
        {
            connection->process_discard();
        }
        catch (sdbusplus::exception_t& ex)
        {
            BMC_LOG_CRITICAL << "Can't process dbus handler: " << ex.what();
        }
    }
    BMC_LOG_INFO << "Terminate object watcher ID #" << std::this_thread::get_id();
}

void DBusBrokerManager::doWatchDBusServiceOwners()
{
    using namespace sdbusplus::bus::match;
    // See to the DBusBrokerManager::doCaptureDbus()
    auto watcherConnect = connect::DBusConnectionPool::createDbusConnection();
    auto& connection = watcherConnect->getConnect();
    static match serviceOwnerWatcher(
        *connection, rules::nameOwnerChanged(),
        [](sdbusplus::message::message& message) {
            std::string name;     // well-known
            std::string oldOwner; // unique-name
            std::string newOwner; // unique-name

            try
            {
                message.read(name, oldOwner, newOwner);
            }
            catch (sdbusplus::exception_t& ex)
            {
                BMC_LOG_ERROR << "Can't read name owner property changed: "
                              << ex.what();
                return;
            }
            if (name.starts_with(":"))
            {
                return;
            }
            app::query::dbus::EntityDBusQuery::setServiceName(
                newOwner.size() ? newOwner : oldOwner, name);
        });

    while (active)
    {
        connection->wait(1s);
        try
        {
            connection->process_discard();
        }
        catch (sdbusplus::exception_t& ex)
        {
            BMC_LOG_CRITICAL << "Can't process dbus handler: " << ex.what();
        }
    }
    BMC_LOG_INFO << "Terminate dbus service names watcher ID #"
             << std::this_thread::get_id();
}

void DBusBrokerManager::setupDBusServiceNames()
{
    constexpr const char* freedesktopServiceName = "org.freedesktop.DBus";
    constexpr const char* freedesktopInterfaceName = "org.freedesktop.DBus";

    auto& connection = getConnectionPool()->getQueryConnection()->getConnect();
    for (auto& wellKnownServiceName : connection->list_names_acquired())
    {
        std::string uniqueServiceName;
        // There is unique name of DBus service. Skip
        if (wellKnownServiceName.starts_with(":"))
        {
            continue;
        }
        auto mapperCall = connection->new_method_call(
            freedesktopServiceName, "/",
            freedesktopInterfaceName, "GetNameOwner");

        mapperCall.append(wellKnownServiceName);
        try
        {
            connection->call(mapperCall).read(uniqueServiceName);
        }
        catch (sdbusplus::exception_t& ex)
        {
            BMC_LOG_CRITICAL << "Can't call dbus GetNameOwner: " << ex.what();
        }
        app::query::dbus::EntityDBusQuery::setServiceName(uniqueServiceName,
                                                          wellKnownServiceName);
    }
}

bool DBusBrokerManager::hasReadyTask() const
{
    return std::any_of(
        brokers.begin(), brokers.end(),
        [](IBrokerManager::BrokerPtr&& broker) { return broker->isTimeout(); });
}

const connect::DBusConnectionPoolUni&
    DBusBrokerManager::getConnectionPool() const
{
    return connectionPool;
}

} // namespace broker
} // namespace app
