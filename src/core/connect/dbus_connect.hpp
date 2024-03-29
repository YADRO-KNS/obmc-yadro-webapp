// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021, KNS Group LLC (YADRO)

#pragma once

#include <config.h>

#include <core/connect/connect.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <thread>

namespace app
{
namespace connect
{
class DBusConnect;

using namespace sdbusplus::bus;
using namespace phosphor::logging;

using DBusConnectUni = std::unique_ptr<DBusConnect>;
/**
 * @class DBusConnect
 * @brief The base DBus connection class that is contains general logic of
 *        query-observe dbus things.
 *
 */
class DBusConnect : protected IConnect
{
    /** sdbusplus connection object wrapper around `sd_bus*` */
    std::unique_ptr<sdbusplus::bus::bus> sdbusConnect;
    std::atomic<std::thread::id> capturedByThreadId;

  private:
    /**
     * @brief DBus call may be requested from different threads and we must
     *        protect dbus-resources access.
     *        The DBusCallGuard performs non-blocking wait until configured
     *        operation from anather thread is completed.
     */
    template <typename TOpRet>
    class DBusCallGuard
    {
        std::atomic<std::thread::id>& threadIdRef;
        std::function<TOpRet()> operation;
        const uint8_t priority;
        bool calledUnsafe;

      public:
        DBusCallGuard(std::atomic<std::thread::id>& capturedByThreadId,
                      std::function<TOpRet()>&& operation,
                      uint8_t priority = 5) :
            threadIdRef(capturedByThreadId),
            operation(operation), priority(priority), calledUnsafe(false)
        {
            if (priority == 0)
            {
                throw std::logic_error("Priority cannot be zero");
            }
        }

        TOpRet operator()()
        {
            using namespace std::chrono;
            using namespace std::chrono_literals;
            auto currentThreadId = std::this_thread::get_id();
            auto vocationThreadId = std::thread::id(0);

            while (!threadIdRef.compare_exchange_strong(
                vocationThreadId, currentThreadId, std::memory_order_seq_cst))
            {
                if (isSameThread(vocationThreadId))
                {
                    auto currentThreadHash =
                        std::hash<std::thread::id>{}(currentThreadId);
                    log<level::DEBUG>(
                        "Call DBus unsafe",
                        entry("THREAD_ID=0x%08X", currentThreadHash));
                    calledUnsafe = true;
                    return std::invoke(operation);
                }
                std::this_thread::sleep_for(priority * 20us);
                vocationThreadId = std::thread::id(0);
            }
            return std::invoke(operation);
        }

        /**
         * @brief Destroy the DBusCallGuard object
         *        The configured operation may raise an exception and
         *        the threadIdRef might be frozen in a busy state.
         *        Destructor cared about safe reverting threadIdRef to ready
         *        state.
         */
        ~DBusCallGuard()
        {
            if (calledUnsafe)
            {
                return;
            }
            threadIdRef.store(std::thread::id(0), std::memory_order_acq_rel);
        }

      protected:
        inline bool
            isSameThread(std::atomic<std::thread::id> capturedByThreadId,
                         std::thread::id currentThreadId =
                             std::this_thread::get_id()) noexcept
        {
            // After comparing it doesn't matter which value of
            // `currentThreadId` is.
            // So do memory_order_relaxed without synchronization.
            auto tmpThreadId = capturedByThreadId.load();
            return capturedByThreadId.compare_exchange_strong(
                currentThreadId, tmpThreadId, std::memory_order_relaxed);
        }
    };

  public:
    DBusConnect(const DBusConnect&) = delete;
    DBusConnect& operator=(const DBusConnect&) = delete;
    DBusConnect(DBusConnect&&) = delete;
    DBusConnect& operator=(DBusConnect&&) = delete;

    explicit DBusConnect();
    ~DBusConnect() noexcept override;
    /**
     * @brief Start observe DBus signals
     *
     */
    void run();
    /**
     * @brief Terminate DBus signals observing
     *
     */
    void terminate() noexcept;

    /** @brief Directly call DBus method with specified return type
     *
     * @tparam Ret                    - Type of method call result
     * @tparam Args                   - A set of types of the dbus-method
     *                                  parameters
     *
     * @param busName                 - DBus service name
     * @param path                    - DBus object path
     * @param interface               - DBus interface
     * @param method                  - DBus method name
     * @param args                    - DBus method parameters
     *
     * @throw std::runtime_error      - Failure of thread call.
     * @throw sdbusplus::exception_t  - DBus error
     *
     * @return auto                   - The result of DBus method call.
     */
    template <typename Ret, typename... Args>
    Ret callMethodAndRead(const std::string& busName, const std::string& path,
                          const std::string& interface,
                          const std::string& method, Args... args)
    {
        DBusCallGuard<Ret> guard(
            capturedByThreadId,
            [&]() -> Ret {
                return callMethodAndReadUnsafe<Ret, Args...>(
                    busName, path, interface, method,
                    std::forward<Args>(args)...);
            },
            1);
        return guard();
    }

    /** @brief Directly call DBus method with specified return type
     *
     * @tparam Ret                    - Type of method call result
     * @tparam Args                   - A set of types of the dbus-method
     *                                  parameters
     *
     * @param busName                 - DBus service name
     * @param path                    - DBus object path
     * @param interface               - DBus interface
     * @param method                  - DBus method name
     * @param args                    - DBus method parameters
     *
     * @throw std::runtime_error      - Faulture of thread call.
     * @throw sdbusplus::exception_t  - DBus error
     *
     * @return auto                   - The yield of DBus method call.
     */
    template <typename Ret, typename... Args>
    Ret callMethodAndReadUnsafe(const std::string& busName,
                                const std::string& path,
                                const std::string& interface,
                                const std::string& method, Args&&... args)
    {
        Ret resp;
        getConnect()->wait(0);
        auto reqMsg = getConnect()->new_method_call(
            busName.c_str(), path.c_str(), interface.c_str(), method.c_str());
        reqMsg.append(std::forward<Args>(args)...);
        auto respMsg = getConnect()->call(reqMsg);
        respMsg.read(resp);
        return resp;
    }

    /**
     * @brief Directly call DBus method
     *
     * @tparam Args                   - A set of types of the dbus-method
     *                                  parameters
     *
     * @param guard                   - DBus request guard
     * @param busName                 - DBus service name
     * @param path                    - DBus object path
     * @param interface               - DBus interface
     * @param method                  - DBus method name
     * @param args                    - DBus method parameters
     *
     * @throw std::runtime_error      - Faulture of thread call.
     * @throw sdbusplus::exception_t  - DBus error
     * @return auto                   - The yield of DBus method call.
     */
    template <typename... Args>
    auto callMethod(const std::string& busName, const std::string& path,
                    const std::string& interface, const std::string& method,
                    Args&&... args)
    {
        auto reqMsg = getConnect()->new_method_call(
            busName.c_str(), path.c_str(), interface.c_str(), method.c_str());
        reqMsg.append(std::forward<Args>(args)...);
        getConnect()->call_noreply(reqMsg);
    }

    /**
     * @brief Create a DBus signals watcher
     *
     * @param rule            - DBus rule to match signal
     * @param handler         - The callback to handle signal
     *
     * @return match::match&& - The rvalue of sdbusplus::match::match_t
     */
    match::match createWatcher(const std::string& rule,
                               match::match::callback_t handler);
    /**
     * @brief Get the Well-Known DBus-service name by specified dbus-unique name
     *
     * @param uniqueName          - DBus unique service name
     * @return const std::string& - DBus well-known service name
     */
    const std::string& getWellKnownServiceName(const std::string& uniqueName);
    /**
     * @brief Force update DBus well-known to unique service names dictionary
     */
    void updateWellKnownServiceNameDict();
    /**
     * @brief Create a Dbus Connection object
     *
     * @return DBusConnectUni
     */
    static DBusConnectUni createDbusConnection();

  protected:
    /**
     * @brief Main process of observing dbus signals.
     */
    void process();
    /**
     * @brief Initialize DBus connection.
     */
    inline void initSdBusConnection();
    /**
     * @brief Initialize DBus signal watcher to update well-known service names
     *        dictionary.
     */
    inline void initServiceNamesWatcher();
    /**
     * @brief Get the DBus connection object
     *
     * @return std::unique_ptr<sdbusplus::bus::bus>& connection object
     */
    std::unique_ptr<sdbusplus::bus::bus>& getConnect();
    /**
     * @brief Force disconnect DBus connection
     *
     * @return true   - sucess
     * @return false  - already disconnected
     */
    bool disconnect() noexcept override;
    /**
     * @brief Set the DBus well-known service name by its unique name.
     *
     * @param uniqueName      - DBus unique service name
     * @param wellKnownName   - DBus well-known service name
     */
    void setServiceName(const std::string& uniqueName,
                        const std::string& wellKnownName);
    /**
     * @brief Configure dbus-object adding/removing hanlders
     */
    void configureObjectManagingHandlers();

  protected:
    /** flag to indicate is dbus-signals watcher thread alive */
    std::atomic_bool alive;
    /** low-level dbus-connection object. */
    sd_bus* dbusConnect;
    /** dbus-signal watcher thread */
    std::unique_ptr<std::thread> thread;
    /** DBus unique to well-known service names dictionary */
    std::map<std::string, std::string> serviceNamesDict;
    /** Global singal handlers dict to store sdbusplus matchers */
    std::vector<sdbusplus::bus::match::match> globalSignalHandlers;
};

/**
 * @class SystemDBusConnect.
 * @brief DBus connection to the System bus.
 */
class SystemDBusConnect final : public DBusConnect
{
  public:
    SystemDBusConnect() noexcept
    {}
    ~SystemDBusConnect() override = default;
    /** @inherit */
    void connect() override;
};

#ifdef BMC_DBUS_CONNECT_REMOTE
/**
 * @class RemoteHostDbusConnect.
 * @brief DBus connection to the remote system via SSH
 *
 * @note  The non-secure way to connect DBus remote system DBus!
 *        Use only for the debugging in the development mode.
 */
class RemoteHostDbusConnect final : public DBusConnect
{
    const std::string hostname;

  public:
    RemoteHostDbusConnect(const std::string& dbusHostname) noexcept :
        hostname(dbusHostname)
    {}
    ~RemoteHostDbusConnect() override = default;
    /** @inherit */
    void connect() override;
};
#endif

} // namespace connect
} // namespace app
