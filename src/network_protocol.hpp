// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <formatters.hpp>
#include <phosphor-logging/log.hpp>

#include <chrono>
namespace app
{
namespace obmc
{
namespace entity
{
using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;
using namespace phosphor::logging;
using namespace std::chrono_literals;

class SnmpProtocol final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<SnmpProtocol>
{
    static constexpr const char* service = "xyz.openbmc_project.SNMPCfg";
    static constexpr const char* object = "/xyz/openbmc_project/snmpcfg";

  public:
    ENTITY_DECL_FIELD(std::string, AuthenticationProtocol)
    ENTITY_DECL_FIELD(std::string, CommunityString)
    ENTITY_DECL_FIELD(std::string, CommunityAccessMode)
    ENTITY_DECL_FIELD(std::string, CommunityName)
    ENTITY_DECL_FIELD(std::string, EncryptionProtocol)
    ENTITY_DECL_FIELD(std::string, EngineId)
    ENTITY_DECL_FIELD_DEF(bool, EnableSNMPv1, false)
    ENTITY_DECL_FIELD_DEF(bool, EnableSNMPv2c, false)
    ENTITY_DECL_FIELD_DEF(bool, EnableSNMPv3, false)
    ENTITY_DECL_FIELD_DEF(bool, HideCommunityStrings, false)
    ENTITY_DECL_FIELD_DEF(uint16_t, Port, 0U)

  private:
    class Query : public dbus::GetObjectDBusQuery
    {
        static constexpr const char* snmpCfgIface =
            "xyz.openbmc_project.SNMPCfg";

      public:
        Query() : GetObjectDBusQuery(service, object)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            snmpCfgIface,
            DBUS_QUERY_EP_FIELDS_ONLY("Community", fieldCommunityString)))

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                snmpCfgIface,
            };
            return interfaces;
        }
        // clang-format: on
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            setFieldAuthenticationProtocol(instance, "CommunityString");
            setFieldCommunityAccessMode(instance, "Limited");
            setFieldCommunityName(instance, "default");
            setFieldEncryptionProtocol(instance, "None");
            setFieldEnableSNMPv1(instance, false);
            setFieldEnableSNMPv2c(instance, true);
            setFieldEnableSNMPv3(instance, false);
            setFieldHideCommunityStrings(instance, false);
            // Using the socket unit for SNMP is not recommended by the authors
            // of the net-snmp package. This hack fixes the port number
            // determination that doesn't work without the socket unit.
            setFieldPort(instance, 161);
        }
    };

  public:
    SnmpProtocol() : Entity(), query(std::make_shared<Query>())
    {
        this->createMember(fieldAuthenticationProtocol);
        this->createMember(fieldCommunityAccessMode);
        this->createMember(fieldCommunityName);
        this->createMember(fieldEncryptionProtocol);
        this->createMember(fieldEngineId);
        this->createMember(fieldEnableSNMPv1);
        this->createMember(fieldEnableSNMPv2c);
        this->createMember(fieldEnableSNMPv3);
        this->createMember(fieldHideCommunityStrings);
        this->createMember(fieldPort);
    }
    ~SnmpProtocol() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class NtpProtocol final :
    public Entity,
    public CachedSource,
    public NamedEntity<NtpProtocol>
{
    static constexpr const char* service = "xyz.openbmc_project.Settings";
    static constexpr const char* object =
        "/xyz/openbmc_project/time/sync_method";

  public:
    enum class TimeSyncMethod
    {
        ntp,
        manual
    };

    ENTITY_DECL_FIELD_ENUM(TimeSyncMethod, TimeSyncMethod, manual);

  private:
    class Query : public dbus::GetObjectDBusQuery
    {
        static constexpr const char* timeSyncIface =
            "xyz.openbmc_project.Time.Synchronization";
        class FormatTimeSyncMethod : public query::dbus::IFormatter
        {
            inline const std::string dbusEnum(const std::string& enumStr)
            {
                return "xyz.openbmc_project.Time.Synchronization.Method." +
                       enumStr;
            }

          public:
            ~FormatTimeSyncMethod() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, TimeSyncMethod>
                    syncMethodMap{
                        {dbusEnum("NTP"), TimeSyncMethod::ntp},
                        {dbusEnum("Manual"), TimeSyncMethod::manual},
                    };

                return formatValueFromDict(syncMethodMap, property, value,
                                           TimeSyncMethod::manual);
            }
        };

      public:
        Query() : GetObjectDBusQuery(service, object)
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            timeSyncIface,
            DBUS_QUERY_EP_SET_FORMATTERS2(
                fieldTimeSyncMethod, DBUS_QUERY_EP_CSTR(FormatTimeSyncMethod))))

      protected:
        const InterfaceList& searchInterfaces() const override
        {
            static const InterfaceList interfaces{
                timeSyncIface,
            };
            return interfaces;
        }
        // clang-format: on
    };

  public:
    NtpProtocol() : Entity(), query(std::make_shared<Query>())
    {}
    ~NtpProtocol() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class IpmiSolProtocolProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<IpmiSolProtocolProvider>,
    public CachedSource,
    public NamedEntity<IpmiSolProtocolProvider>
{
  public:
    ENTITY_DECL_FIELD_DEF(bool, Enabled, false)
  private:
    class Query : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* ipmisolIface =
            "xyz.openbmc_project.Ipmi.SOL";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            ipmisolIface, DBUS_QUERY_EP_FIELDS_ONLY("Enable", fieldEnabled)))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA("/xyz/openbmc_project/ipmi/sol",
                                    DBUS_QUERY_CRIT_IFACES(ipmisolIface),
                                    noDepth, std::nullopt)
        // clang-format: on
    };

  public:
    IpmiSolProtocolProvider() :
        EntitySupplementProvider(), query(std::make_shared<Query>())
    {}
    ~IpmiSolProtocolProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class NetworkProtocol final :
    public entity::Collection,
    public ShortTimeCachedSource<10>,
    public NamedEntity<NetworkProtocol>
{
    // The primary unit name as string
    using PrimaryUnit = std::string;
    // The human readable description string
    using UnitDescription = std::string;
    // The load state (i.e. whether the unit file has been loaded successfully)
    using UnitLoadState = std::string;
    // The active state (i.e. whether the unit is currently started or not)
    using UnitActiveState = std::string;
    // The sub state (a more fine-grained version of the active state that is
    // specific to the unit type, which the active state is not)
    using UnitSubState = std::string;
    // A unit that is being followed in its state by this unit, if there is any,
    // otherwise the empty string.
    using UnitBeingFollowed = std::string;
    // The unit object path
    using UnitObjectPath = sdbusplus::message::object_path;
    // If there is a job queued for the job unit, the numeric job id, 0
    // otherwise
    using UnitJobQueued = uint32_t;
    // The job type as string
    using UnitJobType = std::string;
    // The job object path
    using UnitJobPath = sdbusplus::message::object_path;
    // The unit metadata structure(std::tuple)
    using UnitMeta =
        std::tuple<PrimaryUnit, UnitDescription, UnitLoadState, UnitActiveState,
                   UnitSubState, UnitBeingFollowed, UnitObjectPath,
                   UnitJobQueued, UnitJobType, UnitJobPath>;

    using ListUnits = std::vector<UnitMeta>;

  public:
    // Kind of protocol
    enum class Purpose
    {
        ssh,
        ipmi,
        ipmisol,
        kvmip,
        https,
        sol,
        snmp,
        ntp,
        unknown
    };

    ENTITY_DECL_FIELD_ENUM(Purpose, Purpose, unknown)
    ENTITY_DECL_FIELD_DEF(bool, Enabled, false)
    ENTITY_DECL_FIELD_DEF(uint16_t, Port, 0U)
  private:
    class Query final : public DBusQueryViaMethodCall<ListUnits>
    {
        static constexpr const char* serviceName = "org.freedesktop.systemd1";
        static constexpr const char* interfaceName =
            "org.freedesktop.systemd1.Manager";
        static constexpr const char* object = "/org/freedesktop/systemd1";

        using SocketListen = std::tuple<std::string, std::string>;
        using SocketListenList = std::vector<SocketListen>;
        using SocketListenListVariant = std::variant<SocketListenList>;

        class PortQuery final :
            public DBusQueryViaMethodCall<SocketListenListVariant,
                                          InterfaceName, std::string>
        {
            static constexpr const char* socketIface =
                "org.freedesktop.systemd1.Socket";
            // Network protocol listen field index in a tuple
            enum ListenElementsIndex
            {
                netProtoListenType,
                netProtoListenStream
            };

          public:
            PortQuery(const ObjectPath& objectPath) :
                DBusQueryViaMethodCall<SocketListenListVariant, InterfaceName,
                                       std::string>(
                    serviceName, objectPath, "org.freedesktop.DBus.Properties",
                    "Get", socketIface, "Listen")
            {}
            ~PortQuery() override = default;

            InstanceCollection populateInstances(
                const SocketListenListVariant& responseVariant) override
            {
                if (!std::holds_alternative<SocketListenList>(responseVariant))
                {
                    return {};
                }
                const auto& response =
                    std::get<SocketListenList>(responseVariant);
                if (response.empty())
                {
                    return {};
                }
                auto linstenSocketMeta = response.front();
                const auto& listenStream =
                    std::get<netProtoListenStream>(linstenSocketMeta);
                std::size_t lastColonPos = listenStream.rfind(':');
                if (lastColonPos == std::string::npos)
                {
                    // Not a port
                    return {};
                }
                std::string portStr = listenStream.substr(lastColonPos + 1);
                if (portStr.empty())
                {
                    return {};
                }
                char* endPtr = nullptr;
                // Use strtol instead of stroi to avoid
                // exceptions
                errno = 0;
                uint16_t port = std::strtol(portStr.c_str(), &endPtr, 10);
                if ((errno == 0) && (*endPtr == '\0'))
                {
                    const auto instance =
                        std::make_shared<StaticInstance>(listenStream);
                    setFieldPort(instance, port);
                    return {instance};
                }
                return {};
            }

            void setPort(const InstancePtr& instance)
            {
                // Several services can't provides port information.
                // This hack is workaround to determine the managed port by a
                // service.
                static const std::map<Purpose, uint16_t> portForService{
                    {Purpose::https, 443},
                };

                auto purpose = getFieldPurpose(instance);
                auto portIt = portForService.find(purpose);
                if (portIt != portForService.end())
                {
                    setFieldPort(instance, portIt->second);
                    return;
                }
                const auto portProvidesInstances = process();
                if (portProvidesInstances.empty())
                {
                    return;
                }
                setFieldPort(instance,
                             getFieldPort(portProvidesInstances.front()));
            }

            const QueryFields getFields() const override
            {
                return {fieldPort};
            }
        };

      public:
        Query() :
            DBusQueryViaMethodCall<ListUnits>(serviceName, object,
                                              interfaceName, "ListUnits")
        {}
        ~Query() override = default;

        InstanceCollection populateInstances(const ListUnits& response) override
        {
            // Network protocol field index in a tuple
            enum UnitFieldIndex
            {
                netProtoUnitName = 0,
                netProtoUnitSubState = 4,
                netProtoUnitObjPath = 6,
            };

            static const std::vector<std::string> protocolFromService{
                "start-ipkvm", "snmpd", "lighttpd."};
            static const std::map<Purpose, std::string> observedServices = {
                {Purpose::ssh, "dropbear"},
                {Purpose::https, "lighttpd."},
                {Purpose::ipmi, "phosphor-ipmi-net"},
                {Purpose::sol, "obmc-console-ssh"},
                {Purpose::kvmip, "start-ipkvm"},
                {Purpose::snmp, "snmpd"},
            };

            try
            {
                InstanceCollection instances;
                for (const auto& unitMeta : response)
                {
                    const std::string& unitName =
                        std::get<netProtoUnitName>(unitMeta);
                    const std::string& socketPath =
                        std::get<netProtoUnitObjPath>(unitMeta);
                    const std::string& unitState =
                        std::get<netProtoUnitSubState>(unitMeta);

                    if ((!unitName.ends_with(".socket") &&
                         !std::any_of(protocolFromService.begin(),
                                      protocolFromService.end(),
                                      [unitName](const std::string& svcName) {
                                          return unitName.starts_with(svcName);
                                      })))
                    {
                        continue;
                    }

                    for (const auto [purpose, svcStartName] : observedServices)
                    {
                        if (!unitName.starts_with(svcStartName))
                        {
                            continue;
                        }
                        PortQuery portQuery(socketPath);
                        const auto instance = buildInstance(purpose);
                        setFieldEnabled(instance, (unitState == "running" ||
                                                   unitState == "listening"));
                        portQuery.setPort(instance);
                        instances.push_back(instance);
                    }
                }
                setStaticProto(instances);
                return instances;
            }
            catch (const sdbusplus::exception_t& ex)
            {
                log<level::DEBUG>(
                    "Fail to process DBus query (DBusQueryViaMethodCall)",
                    entry("ERROR=%s", ex.what()));
                std::cerr << "Error: " << ex.what() << std::endl;
            }
            return {};
        }

        const QueryFields getFields() const override
        {
            return {fieldPurpose, fieldEnabled, fieldPort};
        }

      protected:
        inline const std::string buildIdentityField(Purpose purpose)
        {
            return object + ("NetworkProtoId_" +
                             std::to_string(static_cast<int>(purpose)));
        }
        inline const DBusInstancePtr buildInstance(Purpose purpose)
        {
            const auto instance = this->createInstance(
                serviceName, buildIdentityField(purpose), {});
            setFieldPurpose(instance, purpose);
            setFieldEnabled(instance, false);
            return instance;
        }
        inline void setStaticProto(InstanceCollection& instances)
        {
            static const std::vector<Purpose> defaultProtoList{Purpose::ipmisol,
                                                               Purpose::ntp};
            for (auto purpose : defaultProtoList)
            {
                auto instance = buildInstance(purpose);
                setNtpEnabled(instance);
                setSnmpPort(instance);
                instances.push_back(instance);
            }
        }

        inline void setNtpEnabled(const IEntity::InstancePtr& instance)
        {
            if (getFieldPurpose(instance) != Purpose::ntp)
            {
                return;
            }
            const auto ntpEntity = NtpProtocol::getEntity();
            const auto ntpInstances = ntpEntity->getInstances();
            if (ntpInstances.empty())
            {
                return;
            }
            auto enabled =
                NtpProtocol::getFieldTimeSyncMethod(ntpInstances.front()) ==
                NtpProtocol::TimeSyncMethod::ntp;
            setFieldEnabled(instance, enabled);
        }

        inline void setSnmpPort(const IEntity::InstancePtr& instance)
        {
            if (getFieldPurpose(instance) != Purpose::snmp)
            {
                return;
            }
            const auto entity = SnmpProtocol::getEntity();
            const auto snmpInstances = entity->getInstances();
            if (snmpInstances.empty())
            {
                return;
            }
            setFieldPort(instance, getFieldPort(snmpInstances.front()));
        }
    };

    template <Purpose targetPurpose>
    static void linkProviderByPurpose(const IEntity::InstancePtr& supplementer,
                                      const IEntity::InstancePtr& target)
    {
        if (getFieldPurpose(target) == targetPurpose)
        {
            target->supplementOrUpdate(supplementer);
        }
    }

    template <Purpose targetPurpose>
    static const IRelation::RelationRulesList& additionalProtoCapRelation()
    {
        static const IRelation::RelationRulesList relations{
            {fieldPurpose, IRelation::dummyField,
             [](const auto, const auto& purposeVal) -> bool {
                 if (!std::holds_alternative<int>(purposeVal))
                 {
                     log<level::WARNING>("additionalProtoCapRelation: invalid "
                                         "type of field 'Purpose'");
                     return false;
                 }

                 auto firmwarePurpose =
                     static_cast<Purpose>(std::get<int>(purposeVal));
                 return targetPurpose == firmwarePurpose;
             }},
        };
        return relations;
    }

  public:
    NetworkProtocol() : Collection(), query(std::make_shared<Query>())
    {}
    ~NetworkProtocol() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_PROVIDERS(ENTITY_PROVIDER_LINK(
        IpmiSolProtocolProvider, linkProviderByPurpose<Purpose::ipmisol>))
    ENTITY_DECL_RELATIONS(
        ENTITY_DEF_RELATION(SnmpProtocol,
                            additionalProtoCapRelation<Purpose::snmp>()),
        ENTITY_DEF_RELATION(NtpProtocol,
                            additionalProtoCapRelation<Purpose::ntp>()))
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
