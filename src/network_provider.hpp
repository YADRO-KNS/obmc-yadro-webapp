// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __NETWORK_PROVIDER_H__
#define __NETWORK_PROVIDER_H__

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <definitions.hpp>
#include <logger/logger.hpp>

namespace app
{
namespace query
{
namespace obmc
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;

using namespace std::placeholders;
using namespace app::entity::obmc::definitions;

static const DbusVariantType formatStringValueFromDict(
    const std::map<std::string, std::string>& formatterDict,
    const PropertyName& propertyName, const DbusVariantType& value,
    DBusInstancePtr)
{
    auto formattedValue = std::visit(
        [propertyName,
         &formatterDict](auto&& property) -> const DbusVariantType {
            using TProperty = std::decay_t<decltype(property)>;

            if constexpr (std::is_same_v<TProperty, std::string>)
            {
                auto findTypeIt = formatterDict.find(property);
                if (findTypeIt == formatterDict.end())
                {
                    throw app::core::exceptions::InvalidType(propertyName);
                }

                return DbusVariantType(findTypeIt->second);
            }

            throw app::core::exceptions::InvalidType(propertyName);
        },
        value);
    return formattedValue;
}

class NetworkSysConfiguration final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* networkServiceName =
        "xyz.openbmc_project.Network";
    static constexpr const char* netSysConfigInterfaceName =
        "xyz.openbmc_project.Network.SystemConfiguration";

    static constexpr const char* namePropertyHostName = "HostName";
    static constexpr const char* namePropertyDefaultIPv4Gateway =
        "DefaultGateway";
    static constexpr const char* namePropertyDefaultIPv6Gateway =
        "DefaultGateway6";

    static constexpr const char* fieldHostName = "HostName";
    static constexpr const char* fieldDefaultIPv4Gateway = "DefaultIPv4Gateway";
    static constexpr const char* fieldDefaultIPv6Gateway = "DefaultIPv6Gateway";

  public:
    static const std::vector<std::string> fields;
    static constexpr const char* entityName = "NetworkConfig";

    NetworkSysConfiguration() : dbus::FindObjectDBusQuery()
    {}
    ~NetworkSysConfiguration() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                netSysConfigInterfaceName,
                {
                    {namePropertyHostName, fieldHostName},
                    {namePropertyDefaultIPv4Gateway, fieldDefaultIPv4Gateway},
                    {namePropertyDefaultIPv6Gateway, fieldDefaultIPv6Gateway},
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/network",
            {
                netSysConfigInterfaceName,
            },
            nextOneDepth,
            networkServiceName,
        };

        return criteria;
    }
};

const std::vector<std::string> NetworkSysConfiguration::fields = {
    NetworkSysConfiguration::fieldHostName,
    NetworkSysConfiguration::fieldDefaultIPv4Gateway,
    NetworkSysConfiguration::fieldDefaultIPv6Gateway,
};

class NetworkDHCPConfiguration final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* networkServiceName =
        "xyz.openbmc_project.Network";
    static constexpr const char* netDHCPConfigInterfaceName =
        "xyz.openbmc_project.Network.DHCPConfiguration";

    static constexpr const char* namePropertyDNSEnabled = "DNSEnabled";
    static constexpr const char* namePropertyHostNameEnabled =
        "HostNameEnabled";
    static constexpr const char* namePropertyNTPEnabled = "NTPEnabled";
    static constexpr const char* namePropertySendHostNameEnabled =
        "SendHostNameEnabled";

    static constexpr const char* fieldDNSEnabled = "DNSEnabled";
    static constexpr const char* fieldHostNameEnabled = "HostNameEnabled";
    static constexpr const char* fieldNTPEnabled = "NTPEnabled";
    static constexpr const char* fieldSendHostNameEnabled =
        "SendHostNameEnabled";

  public:
    static const std::vector<std::string> fields;
    static constexpr const char* entityName = "NetworkDHCPConfig";

    NetworkDHCPConfiguration() : dbus::FindObjectDBusQuery()
    {}
    ~NetworkDHCPConfiguration() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                netDHCPConfigInterfaceName,
                {
                    {namePropertyDNSEnabled, fieldDNSEnabled},
                    {namePropertyHostNameEnabled, fieldHostNameEnabled},
                    {namePropertyNTPEnabled, fieldNTPEnabled},
                    {namePropertySendHostNameEnabled, fieldSendHostNameEnabled},
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/network/config",
            {
                netDHCPConfigInterfaceName,
            },
            nextOneDepth,
            networkServiceName,
        };

        return criteria;
    }
};

const std::vector<std::string> NetworkDHCPConfiguration::fields = {
    NetworkDHCPConfiguration::fieldDNSEnabled,
    NetworkDHCPConfiguration::fieldHostNameEnabled,
    NetworkDHCPConfiguration::fieldNTPEnabled,
    NetworkDHCPConfiguration::fieldSendHostNameEnabled,
};

class NetworkIPInterface final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* networkServiceName =
        "xyz.openbmc_project.Network";
    static constexpr const char* netIpInterfaceName =
        "xyz.openbmc_project.Network.IP";

    static constexpr const char* namePropertyAddress = "Address";
    static constexpr const char* namePropertyGateway = "Gateway";
    static constexpr const char* namePropertyOrigin = "Origin";
    static constexpr const char* namePropertyPrefixLength = "PrefixLength";
    static constexpr const char* namePropertyType = "Type";

    static constexpr const char* fieldAddress = "Address";
    static constexpr const char* fieldGateway = "Gateway";
    static constexpr const char* fieldOrigin = "Origin";
    static constexpr const char* fieldMask = "SubnetMask";
    static constexpr const char* fieldType = "Type";

    static const std::map<std::string, std::string> typeDict;
    static const std::map<std::string, std::string> originsDict;

  public:
    static const std::vector<std::string> fields;
    static constexpr const char* entityName = "IP";

    NetworkIPInterface() : dbus::FindObjectDBusQuery()
    {}
    ~NetworkIPInterface() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                netIpInterfaceName,
                {
                    {namePropertyAddress, fieldAddress},
                    {namePropertyGateway, fieldGateway},
                    {namePropertyOrigin, fieldOrigin},
                    {namePropertyPrefixLength, fieldMask},
                    {namePropertyType, fieldType},
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/network",
            {
                netIpInterfaceName,
            },
            noDepth,
            networkServiceName,
        };

        return criteria;
    }

    const FieldsFormattingMap& getFormatters() const override
    {
        using namespace std::placeholders;
        static const FieldsFormattingMap formatters{
            {
                namePropertyOrigin,
                {std::bind(formatStringValueFromDict, originsDict, _1, _2, _3)},
            },
            {
                namePropertyType,
                {std::bind(formatStringValueFromDict, typeDict, _1, _2, _3)},
            },
        };

        return formatters;
    }
};

const std::map<std::string, std::string> NetworkIPInterface::typeDict{
    {"xyz.openbmc_project.Network.IP.Protocol.IPv4", "IPv4"},
    {"xyz.openbmc_project.Network.IP.Protocol.IPv6", "IPv6"},
};
const std::map<std::string, std::string> NetworkIPInterface::originsDict{
    {"xyz.openbmc_project.Network.IP.AddressOrigin.Static", "Static"},
    {"xyz.openbmc_project.Network.IP.AddressOrigin.DHCP", "DHCP"},
    {"xyz.openbmc_project.Network.IP.AddressOrigin.LinkLocal", "LinkLocal"},
    {"xyz.openbmc_project.Network.IP.AddressOrigin.SLAAC", "SLAAC"},
};

const std::vector<std::string> NetworkIPInterface::fields = {
    NetworkIPInterface::fieldAddress, NetworkIPInterface::fieldGateway,
    NetworkIPInterface::fieldMask,    NetworkIPInterface::fieldOrigin,
    NetworkIPInterface::fieldType,
};

class NetworkEthInterface final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* networkServiceName =
        "xyz.openbmc_project.Network";
    static constexpr const char* netEthInterfaceName =
        "xyz.openbmc_project.Network.EthernetInterface";
    static constexpr const char* netMACInterfaceName =
        "xyz.openbmc_project.Network.MACAddress";

    static constexpr const char* namePropertyAutoNeg = "AutoNeg";
    static constexpr const char* namePropertyDHCPEnabled = "DHCPEnabled";
    static constexpr const char* namePropertyDomainName = "DomainName";
    static constexpr const char* namePropertyIPv6AcceptRA = "IPv6AcceptRA";
    static constexpr const char* namePropertyInterfaceName = "InterfaceName";
    static constexpr const char* namePropertyLinkLocalAutoConf =
        "LinkLocalAutoConf";
    static constexpr const char* namePropertyLinkUp = "LinkUp";
    static constexpr const char* namePropertyNICEnabled = "NICEnabled";
    static constexpr const char* namePropertyNTPServers = "NTPServers";
    static constexpr const char* namePropertyNameservers = "Nameservers";
    static constexpr const char* namePropertySpeed = "Speed";
    static constexpr const char* namePropertyStaticNameServers =
        "StaticNameServers";
    static constexpr const char* namePropertyMACAddress = "MACAddress";

    static constexpr const char* fieldAutoNeg = "AutoNeg";
    static constexpr const char* fieldDHCPEnabled = "DHCPEnabled";
    static constexpr const char* fieldDomainName = "DomainName";
    static constexpr const char* fieldIPv6AcceptRA = "IPv6AcceptRA";
    static constexpr const char* fieldInterfaceName = "InterfaceName";
    static constexpr const char* fieldLinkLocalAutoConf = "LinkLocalAutoConf";
    static constexpr const char* fieldLinkUp = "LinkUp";
    static constexpr const char* fieldNICEnabled = "NICEnabled";
    static constexpr const char* fieldNTPServers = "NTPServers";
    static constexpr const char* fieldNameservers = "Nameservers";
    static constexpr const char* fieldSpeed = "Speed";
    static constexpr const char* fieldStaticNameServers = "StaticNameServers";
    static constexpr const char* fieldMACAddress = "MACAddress";

    static const std::map<std::string, std::string> dhcpConfDict;
    static const std::map<std::string, std::string> LinkTypeDict;

  public:
    static const std::vector<std::string> fields;
    static constexpr const char* entityName = "Ethernet";

    NetworkEthInterface() : dbus::FindObjectDBusQuery()
    {}
    ~NetworkEthInterface() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                netEthInterfaceName,
                {
                    {namePropertyAutoNeg, fieldAutoNeg},
                    {namePropertyDHCPEnabled, fieldDHCPEnabled},
                    {namePropertyDomainName, fieldDomainName},
                    {namePropertyIPv6AcceptRA, fieldIPv6AcceptRA},
                    {namePropertyInterfaceName, fieldInterfaceName},
                    {namePropertyLinkLocalAutoConf, fieldLinkLocalAutoConf},
                    {namePropertyLinkUp, fieldLinkUp},
                    {namePropertyNICEnabled, fieldNICEnabled},
                    {namePropertyNTPServers, fieldNTPServers},
                    {namePropertyNameservers, fieldNameservers},
                    {namePropertySpeed, fieldSpeed},
                    {namePropertyStaticNameServers, fieldStaticNameServers},
                },
            },
            {
                netMACInterfaceName,
                {
                    {namePropertyMACAddress, fieldMACAddress},
                },
            },
        };

        return dictionary;
    }

    static const IEntity::IRelation::RelationRulesList& relationToIp()
    {
        using namespace std::placeholders;
        using namespace app::entity::obmc::definitions;

        static const IEntity::IRelation::RelationRulesList relations{
            {
                metaObjectPath,
                metaObjectPath,
                [](const IEntity::IEntityMember::InstancePtr& instance,
                   const IEntity::IEntityMember::IInstance::FieldType& value)
                    -> bool {
                    auto objectPath = std::get<std::string>(value);
                    BMC_LOG_DEBUG << "[CHECK RELATION] "
                                  << instance->getStringValue() << "|"
                                  << objectPath;
                    //  An ethernet-interface object path includes its own name
                    //  at the last segment, but the VLAN name start with the
                    //  corresponding physical interface name, e.g.,
                    //  - ethernet object path:
                    //    `/xyz/openbmc_project/network/eth0`
                    //  - vlan object path:
                    //    `/xyz/openbmc_project/network/eth0_999`.
                    // Hence, for the ip-interface object
                    // /xyz/openbmc_project/network/eth0_999/ipv4/4bf59855` the
                    // relation will be established for both ethernet
                    // interfaces, which is incorrect.
                    // Add the closure segment delimiter '/' to the end of the
                    // ethernet-interface object path to be certain that the
                    // relation matched for the entire dbus-path, not for part
                    // of the ethernet-interface name.
                    return instance->getStringValue().starts_with(objectPath + "/");
                },
            },
        };
        return relations;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/network",
            {
                netEthInterfaceName,
            },
            nextOneDepth,
            networkServiceName,
        };

        return criteria;
    }

    const FieldsFormattingMap& getFormatters() const override
    {
        using namespace std::placeholders;
        static const FieldsFormattingMap formatters{
            {
                namePropertyDHCPEnabled,
                {
                    std::bind(formatStringValueFromDict, dhcpConfDict, _1, _2,
                              _3),
                },
            },
            {
                namePropertyLinkLocalAutoConf,
                {
                    std::bind(formatStringValueFromDict, LinkTypeDict, _1, _2,
                              _3),
                },
            },
        };

        return formatters;
    }
};
const std::map<std::string, std::string> NetworkEthInterface::dhcpConfDict{
    {
        "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.both",
        "Both",
    },
    {
        "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.v4",
        "V4",
    },
    {
        "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.v6",
        "V6",
    },
    {
        "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.none",
        "None",
    },
};

const std::map<std::string, std::string> NetworkEthInterface::LinkTypeDict{
    {
        "xyz.openbmc_project.Network.EthernetInterface.LinkLocalConf.fallback",
        "Fallback",
    },
    {
        "xyz.openbmc_project.Network.EthernetInterface.LinkLocalConf.both",
        "Both",
    },
    {
        "xyz.openbmc_project.Network.EthernetInterface.LinkLocalConf.v4",
        "V4",
    },
    {
        "xyz.openbmc_project.Network.EthernetInterface.LinkLocalConf.v6",
        "V6",
    },
    {
        "xyz.openbmc_project.Network.EthernetInterface.LinkLocalConf.none",
        "None",
    },
};

const std::vector<std::string> NetworkEthInterface::fields = {
    NetworkEthInterface::fieldAutoNeg,
    NetworkEthInterface::fieldDHCPEnabled,
    NetworkEthInterface::fieldDomainName,
    NetworkEthInterface::fieldIPv6AcceptRA,
    NetworkEthInterface::fieldInterfaceName,
    NetworkEthInterface::fieldLinkLocalAutoConf,
    NetworkEthInterface::fieldLinkUp,
    NetworkEthInterface::fieldNICEnabled,
    NetworkEthInterface::fieldNTPServers,
    NetworkEthInterface::fieldMACAddress,
    NetworkEthInterface::fieldStaticNameServers,
};

} // namespace obmc
} // namespace query
} // namespace app

#endif // __NETWORK_PROVIDER_H__
