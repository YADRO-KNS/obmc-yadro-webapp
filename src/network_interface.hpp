// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <phosphor-logging/log.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>

#include <formatters.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::entity;
using namespace app::query;
using namespace phosphor::logging;

constexpr const char* networkServiceName = "xyz.openbmc_project.Network";

class NetworkConfig final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<NetworkConfig>
{
    static constexpr const char* fieldHostName = "HostName";
    static constexpr const char* fieldDefaultIPv4Gateway = "DefaultIPv4Gateway";
    static constexpr const char* fieldDefaultIPv6Gateway = "DefaultIPv6Gateway";

    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* netSysConfigInterfaceName =
            "xyz.openbmc_project.Network.SystemConfiguration";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            netSysConfigInterfaceName,
            DBUS_QUERY_EP_FIELDS_ONLY2(fieldHostName),
            DBUS_QUERY_EP_FIELDS_ONLY("DefaultGateway",
                                      fieldDefaultIPv4Gateway),
            DBUS_QUERY_EP_FIELDS_ONLY("DefaultGateway6",
                                      fieldDefaultIPv6Gateway)
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/network",
            DBUS_QUERY_CRIT_IFACES(netSysConfigInterfaceName),
            nextOneDepth,
            networkServiceName
        )
    };

  public:
    NetworkConfig() : Entity(), query(std::make_shared<Query>())
    {}
    ~NetworkConfig() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class NetworkDHCPConfig final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<NetworkDHCPConfig>
{
    static constexpr const char* fieldDNSEnabled = "DNSEnabled";
    static constexpr const char* fieldHostNameEnabled = "HostNameEnabled";
    static constexpr const char* fieldNTPEnabled = "NTPEnabled";
    static constexpr const char* fieldSendHostNameEnabled =
        "SendHostNameEnabled";

    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* netDHCPConfigInterfaceName =
            "xyz.openbmc_project.Network.DHCPConfiguration";
      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        DBUS_QUERY_DECL_EP(DBUS_QUERY_EP_IFACES(
            netDHCPConfigInterfaceName,
            DBUS_QUERY_EP_FIELDS_ONLY2(fieldDNSEnabled),
            DBUS_QUERY_EP_FIELDS_ONLY2(fieldHostNameEnabled),
            DBUS_QUERY_EP_FIELDS_ONLY2(fieldNTPEnabled),
            DBUS_QUERY_EP_FIELDS_ONLY2(fieldSendHostNameEnabled)))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/network/config",
            DBUS_QUERY_CRIT_IFACES(netDHCPConfigInterfaceName),
            nextOneDepth,
            networkServiceName
        )
    };

  public:
    NetworkDHCPConfig() : Entity(), query(std::make_shared<Query>())
    {}
    ~NetworkDHCPConfig() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class IP final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<IP>
{
    static constexpr const char* fieldAddress = "Address";
    static constexpr const char* fieldGateway = "Gateway";
    static constexpr const char* fieldOrigin = "Origin";
    static constexpr const char* fieldMask = "SubnetMask";
    static constexpr const char* fieldType = "Type";

    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* netIpInterfaceName =
            "xyz.openbmc_project.Network.IP";
        class FormatOrigin : public query::dbus::IFormatter
        {
          public:
            ~FormatOrigin() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const StringFormatterDict origins{
                    {"xyz.openbmc_project.Network.IP.AddressOrigin.Static",
                     "Static"},
                    {"xyz.openbmc_project.Network.IP.AddressOrigin.DHCP",
                     "DHCP"},
                    {"xyz.openbmc_project.Network.IP.AddressOrigin.LinkLocal",
                     "LinkLocal"},
                    {"xyz.openbmc_project.Network.IP.AddressOrigin.SLAAC",
                     "SLAAC"},
                };

                return formatStringValueFromDict(origins, property, value);
            }
        };

        class FormatType : public query::dbus::IFormatter
        {
          public:
            ~FormatType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const StringFormatterDict types{
                    {"xyz.openbmc_project.Network.IP.Protocol.IPv4", "IPv4"},
                    {"xyz.openbmc_project.Network.IP.Protocol.IPv6", "IPv6"},
                };

                return formatStringValueFromDict(types, property, value);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                netIpInterfaceName,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldAddress),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldGateway),
                DBUS_QUERY_EP_FIELDS_ONLY("PrefixLength", fieldMask),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldOrigin, 
                    DBUS_QUERY_EP_CSTR(FormatOrigin)),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldType, 
                    DBUS_QUERY_EP_CSTR(FormatType)),
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/network",
            DBUS_QUERY_CRIT_IFACES(netIpInterfaceName), 
            noDepth, 
            networkServiceName
        )
    };

  public:
    IP() : Collection(), query(std::make_shared<Query>())
    {}
    ~IP() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class Ethernet final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<Ethernet>
{
    static constexpr const char* fieldAutoNeg = "AutoNeg";
    static constexpr const char* fieldDHCPEnabled = "DHCPEnabled";
    static constexpr const char* fieldDomainName = "DomainName";
    static constexpr const char* fieldIPv6AcceptRA = "IPv6AcceptRA";
    static constexpr const char* fieldInterfaceName = "InterfaceName";
    static constexpr const char* fieldLinkLocalAutoConf =
        "LinkLocalAutoConf";
    static constexpr const char* fieldLinkUp = "LinkUp";
    static constexpr const char* fieldNICEnabled = "NICEnabled";
    static constexpr const char* fieldNTPServers = "NTPServers";
    static constexpr const char* fieldNameservers = "Nameservers";
    static constexpr const char* fieldSpeed = "Speed";
    static constexpr const char* fieldStaticNameServers =
        "StaticNameServers";
    static constexpr const char* fieldMACAddress = "MACAddress";
    static constexpr const char* fieldVLANId = "VLANId";
    static constexpr const char* fieldVLANName = "VLANName";

    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* netEthInterfaceName =
            "xyz.openbmc_project.Network.EthernetInterface";
        static constexpr const char* netMACInterfaceName =
            "xyz.openbmc_project.Network.MACAddress";
        static constexpr const char* netVLANInterfaceName =
            "xyz.openbmc_project.Network.VLAN";

        class FormatDHCPEnabled : public query::dbus::IFormatter
        {
          public:
            ~FormatDHCPEnabled() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const StringFormatterDict dhcpConf{
                    {"xyz.openbmc_project.Network.EthernetInterface."
                     "DHCPConf.both",
                     "Both"},
                    {"xyz.openbmc_project.Network.EthernetInterface."
                     "DHCPConf.v4",
                     "V4"},
                    {"xyz.openbmc_project.Network.EthernetInterface."
                     "DHCPConf.v6",
                     "V6"},
                    {"xyz.openbmc_project.Network.EthernetInterface."
                     "DHCPConf.none",
                     "None"},
                };

                return formatStringValueFromDict(dhcpConf, property, value);
            }
        };

        class FormatLinkConf : public query::dbus::IFormatter
        {
          public:
            ~FormatLinkConf() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const StringFormatterDict linkConf{
                    {"xyz.openbmc_project.Network.EthernetInterface."
                     "LinkLocalConf.fallback",
                     "Fallback"},
                    {"xyz.openbmc_project.Network.EthernetInterface."
                     "LinkLocalConf.both",
                     "Both"},
                    {"xyz.openbmc_project.Network.EthernetInterface."
                     "LinkLocalConf.v4",
                     "V4"},
                    {"xyz.openbmc_project.Network.EthernetInterface."
                     "LinkLocalConf.v6",
                     "V6"},
                    {"xyz.openbmc_project.Network.EthernetInterface."
                     "LinkLocalConf.none",
                     "None"},
                };

                return formatStringValueFromDict(linkConf, property, value);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                netEthInterfaceName,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldAutoNeg),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldDHCPEnabled, 
                    DBUS_QUERY_EP_CSTR(FormatDHCPEnabled)),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldDomainName),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldIPv6AcceptRA),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldInterfaceName),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldLinkLocalAutoConf, 
                    DBUS_QUERY_EP_CSTR(FormatLinkConf)),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldLinkUp),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldNICEnabled),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldNTPServers),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldNameservers),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSpeed),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldStaticNameServers)
            ),
            DBUS_QUERY_EP_IFACES(
                netMACInterfaceName,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMACAddress),
            ),
            DBUS_QUERY_EP_IFACES(
                netVLANInterfaceName,
                DBUS_QUERY_EP_FIELDS_ONLY("Id", fieldVLANId),
                DBUS_QUERY_EP_FIELDS_ONLY("InterfaceName", fieldVLANName),
            )
        )
      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/network",
            DBUS_QUERY_CRIT_IFACES(netEthInterfaceName), 
            nextOneDepth, 
            networkServiceName
        )
    };

  public:
    Ethernet() : Collection(), query(std::make_shared<Query>())
    {
    }
    ~Ethernet() override = default;


  protected:
    static const IEntity::IRelation::RelationRulesList& relationToIp()
    {
        using namespace std::placeholders;

        static const IEntity::IRelation::RelationRulesList relations{
            {
                metaObjectPath,
                metaObjectPath,
                [](const IEntity::IEntityMember::InstancePtr& instance,
                   const IEntity::IEntityMember::IInstance::FieldType& value)
                    -> bool {
                    auto objectPath = std::get<std::string>(value);
                    log<level::DEBUG>(
                        "Linking 'IP' and 'Ethernet' entities",
                        entry("FIELD=%s", metaObjectPath),
                        entry("IP=%s", instance->getStringValue().c_str()),
                        entry("Ethernet=%s", objectPath.c_str()));
                    //  An ethernet-interface object path includes its own name
                    //  at the last segment, but the VLAN name start
                    //  with the
                    //  corresponding physical interface name, e.g.,
                    //  - ethernet object path:
                    //    `/xyz/openbmc_project/network/eth0`
                    //  - vlan object path:
                    //    `/xyz/openbmc_project/network/eth0_999`.
                    // Hence, for the ip-interface object
                    //
                    // xyz / openbmc_project / network / eth0_999 / ipv4 /
                    // 4bf59855` the relation will be established for both
                    // ethernet interfaces, which is incorrect. Add the closure
                    // segment delimiter '/' to the end of the
                    // ethernet-interface object path to be certain
                    // that the relation matched for the entire
                    // dbus-path, not for part of the ethernet-interface name.
                    return instance->getStringValue().starts_with(objectPath +
                                                                  "/");
                },
            },
        };
        return relations;
    }
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATION(IP, relationToIp())
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
