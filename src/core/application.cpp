// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021, KNS Group LLC (YADRO)

#include <core/application.hpp>
#include <phosphor-logging/log.hpp>

#include <core/route/handlers/graphql_handler.hpp>

#include <csignal>
#include <filesystem>

#include <includes.hpp>

namespace app
{
namespace core
{

using namespace phosphor::logging;

void Application::initDBusConnect()
{
    dbusConnection = std::move(connect::DBusConnect::createDbusConnection());
}

void Application::runDBusObserve()
{
    dbusConnection->run();
}

void Application::configure()
{
    waitBootingBmc();
    registerAllRoutes();
    initDBusConnect();
    initEntities();
    runDBusObserve();
    // run observe dbus signals
    // register handlers
    std::signal(SIGINT, &Application::handleSignals);

    // TODO include IProtocol abstraction which incapsulate own specific
    // handlers. Need to remove a static class/methods.
    route::handlers::VisitorFactory::registerGqlVisitors();
}

void Application::start()
{
    /* Now just call the object handler function. It will sleep quietly when
     * there are no requests and efficiently manage them when there are many.
     */
    fastCgiManager.setupSignals();
    fastCgiManager.listen();
    fastCgiManager.start();
    fastCgiManager.join();
}

void Application::terminate()
{
    fastCgiManager.terminate();
    dbusConnection.reset();
}

void Application::waitBootingBmc()
{
    constexpr const char * bootingFilePath = "/run/bmc-booting";
    using namespace std::literals;

    // Don't initialize the application until the BMC boot processing to avoid
    // query to dbus-services which an incomplete state.
    while(std::filesystem::exists(bootingFilePath))
    {
        log<level::INFO>("Wait for BMC boot completed...");
        std::this_thread::sleep_for(1s);
    }

    log<level::DEBUG>("The BMC boot state is OK");
}

void Application::handleSignals(int signal)
{
    log<level::DEBUG>("Signal caught", entry("SIGNAL=%d", strsignal(signal)));
    if (SIGINT == signal)
    {
        log<level::INFO>("Terminate application", entry("SIGNAL=SIGINT"));
        application.terminate();
    }
}

const connect::DBusConnectUni& Application::getDBusConnect() const
{
    if (!dbusConnection)
    {
        throw std::runtime_error("The DBus connection is not initialized.");
    }
    return dbusConnection;
}

void Application::initEntities()
{
    using namespace app::obmc::entity;
    entityManager.buildEntity<Sensors>();
    entityManager.buildEntity<Processor>();
    entityManager.buildEntity<ProcessorSummary>();
    entityManager.buildEntity<Memory>();
    entityManager.buildEntity<MemorySummary>();
    entityManager.buildEntity<Fan>();
    entityManager.buildEntity<PSU>();
    entityManager.buildEntity<Drive>();
    entityManager.buildEntity<StorageControllers>();
    entityManager.buildEntity<NetAdapter>();
    entityManager.buildEntity<Settings>();
    entityManager.buildEntity<PCIeDevice>();
    entityManager.buildEntity<PCIeFunction>();
    entityManager.buildEntity<HostPower>();
    entityManager.buildEntity<PIDProfile>();
    entityManager.buildEntity<PIDZone>();
    entityManager.buildEntity<PID>();
    entityManager.buildEntity<SnmpProtocol>();
    entityManager.buildEntity<NtpProtocol>();
    entityManager.buildEntity<NetworkProtocol>();
    entityManager.buildEntity<NetworkConfig>();
    entityManager.buildEntity<NetworkDHCPConfig>();
    entityManager.buildEntity<IP>();
    entityManager.buildEntity<Ethernet>();

    entityManager.buildEntity<SessionManager>();
    entityManager.buildEntity<Accounts>();
    entityManager.buildEntity<DomainGroups>();
    entityManager.buildEntity<DomainAccounts>();
    entityManager.buildEntity<AccountSettings>();
    entityManager.buildEntity<TrustStoreCertificates>();
    entityManager.buildEntity<LDAPCertificates>();
    entityManager.buildEntity<HTTPSCertificates>();
    entityManager.buildEntity<VirtualMedia>();
    entityManager.buildEntity<Firmware>();
    entityManager.buildEntity<FirmwareManagment>();
    entityManager.buildEntity<BmcManager>();
    entityManager.buildEntity<IntrusionSensor>();
    entityManager.buildEntity<Chassis>();
    entityManager.buildEntity<Baseboard>();
    entityManager.buildEntity<Server>();

    entityManager.configure();
    entityManager.update();
    log<level::INFO>("Internal cache initialization is complete.");
}

} // namespace core
} // namespace app
