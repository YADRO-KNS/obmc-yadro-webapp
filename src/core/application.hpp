// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef BMC_APPLICATION_HPP
#define BMC_APPLICATION_HPP

#include <fastcgi++/manager.hpp>
#include <logger/logger.hpp>
#include <core/broker/dbus_broker.hpp>
#include <core/entity/entity.hpp>

#include <memory>

namespace app
{
namespace core
{

/**
 * @brief the `Application` class contains and configures common abstractions,
 * e.g Broker, Entity, Routes, etc.
 */
class Application final
{
  public:
    Application() : dbusBrokerManager(app::broker::DBusBrokerManager())
    {
        BMC_LOG_DEBUG << "Init application";
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
  protected:
    void initEntityMap();
    void initBrokers();
    void registerAllRoutes();
    void waitBootingBmc();

    static void handleSignals(int signal);
  private:
    app::broker::DBusBrokerManager dbusBrokerManager;
    entity::EntityManager entityManager;
};

extern Application application;

} // namespace core

} // namespace app

#endif //! BMC_APPLICATION_HPP
