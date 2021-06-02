// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __BROKER_H__
#define __BROKER_H__

#include <logger/logger.hpp>

#include <map>
#include <memory>
#include <chrono>

namespace app
{
namespace broker
{
class IBrokerManager;

using namespace std::chrono;
using namespace std::literals;

class IBrokerManager
{
  public:
    class IBroker
    {
      public:
        virtual ~IBroker() = default;

        virtual bool isTimeout() = 0;
        virtual bool isWatch() const = 0;
    };

    using BrokerPtr = std::shared_ptr<IBroker>;
    /**
     * @brief Start all broker threads
     *
     */
    virtual void start() = 0;
    /**
     * @brief Terminate all broker threads.
     *
     * @return true
     * @return false
     */
    virtual void terminate() = 0;

    virtual ~IBrokerManager() = default;
};

class Broker : public IBrokerManager::IBroker
{
    seconds lastExecutionTime;
    seconds timeout;
    bool watch;

  public:
    Broker() = delete;
    Broker(const Broker&) = delete;
    Broker& operator=(const Broker&) = delete;
    Broker(Broker&&) = delete;
    Broker& operator=(Broker&&) = delete;

    /**
     * @brief Construct a new Broker object
     *
     * @param inTimeout - The time between brokers process.
     *                    Single Shot for the zero value.
     * @param inWatch   - Need to register watcher.
     */
    explicit Broker(seconds inTimeout, bool inWatch) :
        lastExecutionTime(system_clock::duration::zero() / 1s),
        timeout(inTimeout), watch(inWatch)
    {}
    ~Broker() override = default;

    bool isWatch() const override
    {
        return watch;
    }

    bool isTimeout() override
    {
        bool isNeverRun = lastExecutionTime == system_clock::duration::zero();
        bool isSingleShot = getTimeout() == system_clock::duration::zero();
        bool isActualTimeout = system_clock::now().time_since_epoch() / 1s >
                               (lastExecutionTime + getTimeout()).count();

        return isNeverRun || (!isSingleShot && isActualTimeout);
    }

    protected:
      void setExecutionTime()
      {
          lastExecutionTime =
              seconds(std::chrono::system_clock::now().time_since_epoch() / 1s);
      }
      const std::chrono::seconds& getTimeout() const
      {
          return timeout;
      }
};

} // namespace broker
} // namespace app

#endif // __BROKER_H__
