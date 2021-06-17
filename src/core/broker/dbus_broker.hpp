// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __DBUSBROKER_H__
#define __DBUSBROKER_H__

#include <core/broker/broker.hpp>
#include <core/entity/entity.hpp>

#include <core/connect/dbus_connect.hpp>
#include <core/entity/query.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace app
{

namespace broker
{

using namespace entity;
using namespace app::query;

class EntityDbusBroker;

using QueryEntityPtr =
    std::shared_ptr<IQuery<IEntity::InstancePtr, connect::DBusConnectUni>>;
using DBusBrokerPtr = std::shared_ptr<EntityDbusBroker>;

class EntityDbusBroker : public Broker
{
    std::mutex guardMutex;
    entity::EntityPtr entity;
    QueryEntityPtr entityQuery;

  public:
    EntityDbusBroker(entity::EntityPtr entityPointer,
                     QueryEntityPtr queryPointer, bool watch = true,
                     std::chrono::seconds timeout = minutes(0)) noexcept :
        Broker(timeout, watch),
        entity(entityPointer), entityQuery(queryPointer)
    {}
    ~EntityDbusBroker() override = default;

    bool tryProcess(const connect::DBusConnectionPoolUni&);

    void registerObjectsListener(const connect::DBusConnectionPoolUni&);
};

class DBusBrokerManager : public IBrokerManager
{
    std::vector<std::thread> threads;
    std::atomic_bool active;

    static constexpr size_t queryBrokerThreadCount = 10;
    static constexpr size_t singleThreadTask = 1;
  public:
    DBusBrokerManager(const DBusBrokerManager&) = delete;
    DBusBrokerManager& operator=(const DBusBrokerManager&) = delete;
    DBusBrokerManager(DBusBrokerManager&&) = delete;
    DBusBrokerManager& operator=(DBusBrokerManager&&) = delete;

    explicit DBusBrokerManager() :
        active(false),
        connectionPool(std::make_unique<connect::DBusConnectionPool>())
    {
        // TODO(IK) Needs to define `Connection Pool` abstraction to thread-safe
        // access to dbus-resources and safe processing dbus-handlers.
    }
    ~DBusBrokerManager() override
    {
    }

    void start() override;
    void terminate() override;
    /**
     * @brief Bind a new one dbus broker
     * @param DBusBrokerPtr pointer of IBroker
     *
     */
    void bind(DBusBrokerPtr);

  protected:
    void doCaptureDbus();
    void doManageringObjects();
    void doWatchDBusServiceOwners();

    void setupDBusServiceNames();

    connect::DBusConnectUni createDbusConnection();
    bool hasReadyTask() const;
    const connect::DBusConnectionPoolUni& getConnectionPool() const;
  private:
    std::vector<DBusBrokerPtr> brokers;
    connect::DBusConnectionPoolUni connectionPool;
};

} // namespace broker

} // namespace app

#endif // __DBUSBROKER_H__
