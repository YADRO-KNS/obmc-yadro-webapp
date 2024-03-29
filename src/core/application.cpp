// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021, KNS Group LLC (YADRO)

#include <core/application.hpp>
#include <core/route/handlers/graphql_handler.hpp>
#include <includes.hpp>
#include <phosphor-logging/log.hpp>

#include <csignal>
#include <filesystem>

namespace app
{
namespace core
{

namespace fs = std::filesystem;
using namespace std::literals;
using namespace phosphor::logging;
using namespace app::obmc::entity;

constexpr const char* yawebInitGuradFile = YAWEB_INIT_GUARD_FILE;

void Application::initDBusConnect()
{
    dbusConnection = std::move(connect::DBusConnect::createDbusConnection());
    createInitGuardFile();
}

void Application::runDBusObserve()
{
    dbusConnection->run();
}

void Application::createInitGuardFile()
{
    const auto filePath = fs::path(yawebInitGuradFile);

    if (!fs::exists(filePath.parent_path()))
    {
        throw std::runtime_error(
            "The path to create init-guard file doesn't exist. Filename: " +
            filePath.string());
    }

    std::ofstream file{filePath};
    fs::permissions(filePath, (fs::perms::owner_read | fs::perms::owner_write |
                               fs::perms::group_read));
}

void Application::removeInitGuardFile()
{
    const auto filePath = fs::path(yawebInitGuradFile);

    if (!fs::exists(filePath))
    {
        log<level::DEBUG>("The file of init-guard doesn't exist.",
                          entry("FILENAME=%s", filePath.c_str()));
        return;
    }

    fs::remove(filePath);
}

void Application::finishInitialization()
{
    while (!isBaseEntitiesInitialized())
    {
        log<level::DEBUG>("Waits for important Entity instances initialization "
                          "are completed...");
        std::this_thread::sleep_for(5s);
    }
    removeInitGuardFile();
}

void Application::configure()
{
    waitBootingBmc();
    registerAllRoutes();
    initDBusConnect();
    initEntities();
    runDBusObserve();
    finishInitialization();
    // run observe dbus signals
    // register handlers
    std::signal(SIGINT, &Application::handleSignals);
}

void Application::start()
{
    /* Now just call the object handler function. It will sleep quietly when
     * there are no requests and efficiently manage them when there are many.
     */
    fastCgiManager.setupSignals();
    fastCgiManager.listen(FASTCGI_SOCKET_PATH);
    fastCgiManager.start();
    fastCgiManager.join();
}

void Application::terminate()
{
    fastCgiManager.terminate();
    dbusConnection.reset();
}

bool Application::isBaseEntitiesInitialized()
{
    using namespace app::obmc::entity;
    return !Server::getEntity()->getInstances().empty() &&
           !Chassis::getEntity()->getInstances().empty() &&
           !Baseboard::getEntity()->getInstances().empty();
}

void Application::waitBootingBmc()
{
    constexpr const char* bootingFilePath = "/run/bmc-booting";

    // Don't initialize the application until the BMC boot processing to avoid
    // query to dbus-services which an incomplete state.
    while (std::filesystem::exists(bootingFilePath))
    {
        log<level::DEBUG>("Wait for BMC boot completed...");
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
    entityManager.buildEntity<Roles>();
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
