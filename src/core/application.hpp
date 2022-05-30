// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <fastcgi++/manager.hpp>
#include <phosphor-logging/log.hpp>
#include <core/connect/dbus_connect.hpp>
#include <core/entity/enitty_manager.hpp>
#include <memory>

namespace app
{
namespace core
{

using namespace phosphor::logging;
/**
 * @brief the `Application` class contains and configures common abstractions,
 * e.g Broker, Entity, Routes, etc.
 */
class Application final
{
  public:
    Application()
    {
        log<level::INFO>("Init application");
    };
    ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application& old) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&& old) = delete;

    /**
     * @brief Configure Web Application.
     * @throw Invalid Argument
     */
    void configure();

    /**
     * @brief Start Web Application
     * @throw std::exception
     */
    void start();

    void terminate();

    /**
     * @brief Get the Entity Manager object
     *
     * @return const entity::EntityManager&
     */
    const entity::EntityManager& getEntityManager()
    {
        return this->entityManager;
    }

    const connect::DBusConnectUni& getDBusConnect() const;
  protected:
    void initEntities();
    void initDBusConnect();
    void runDBusObserve();
    void registerAllRoutes();
    void waitBootingBmc();

    static void handleSignals(int signal);
  private:
    connect::DBusConnectUni dbusConnection;
    entity::EntityManager entityManager;
};

extern Application application;

} // namespace core

} // namespace app
