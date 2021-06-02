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

static const DbusVariantType
    formatStringValueFromDict(const std::map<std::string, std::string>& formatterDict,
                   const PropertyName& propertyName,
                   const DbusVariantType& value, DBusInstancePtr)
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
    static constexpr const char* namePropertyDefaultIPv4Gateway = "DefaultGateway";
    static constexpr const char* namePropertyDefaultIPv6Gateway = "DefaultGateway6";

  public:
    NetworkSysConfiguration() : dbus::FindObjectDBusQuery()
    {}
    ~NetworkSysConfiguration() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                netSysConfigInterfaceName,
                {
                    {
                        namePropertyHostName,
                        network::config::fieldHostName,
                    },
                    {
                        namePropertyDefaultIPv4Gateway,
                        network::config::fieldDefaultIPv4Gateway,
                    },
                    {
                        namePropertyDefaultIPv6Gateway,
                        network::config::fieldDefaultIPv6Gateway,
                    },
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

class NetworkDHCPConfiguration final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* networkServiceName =
        "xyz.openbmc_project.Network";
    static constexpr const char* netDHCPConfigInterfaceName =
        "xyz.openbmc_project.Network.DHCPConfiguration";

    static constexpr const char* namePropertyDNSEnabled = "DNSEnabled";
    static constexpr const char* namePropertyHostNameEnabled = "HostNameEnabled";
    static constexpr const char* namePropertyNTPEnabled = "NTPEnabled";
    static constexpr const char* namePropertySendHostNameEnabled = "SendHostNameEnabled";

  public:
    NetworkDHCPConfiguration() : dbus::FindObjectDBusQuery()
    {}
    ~NetworkDHCPConfiguration() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                netDHCPConfigInterfaceName,
                {
                    {
                        namePropertyDNSEnabled,
                        network::config::fieldDNSEnabled,
                    },
                    {
                        namePropertyHostNameEnabled,
                        network::config::fieldHostNameEnabled,
                    },
                    {
                        namePropertyNTPEnabled,
                        network::config::fieldNTPEnabled,
                    },
                    {
                        namePropertySendHostNameEnabled,
                        network::config::fieldSendHostNameEnabled,
                    },
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

    static const std::map<std::string, std::string> typeDict;
    static const std::map<std::string, std::string> originsDict;

  public:
    NetworkIPInterface() : dbus::FindObjectDBusQuery()
    {}
    ~NetworkIPInterface() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                netIpInterfaceName,
                {
                    {
                        namePropertyAddress,
                        network::ip::fieldAddress,
                    },
                    {
                        namePropertyGateway,
                        network::ip::fieldGateway,
                    },
                    {
                        namePropertyOrigin,
                        network::ip::fieldOrigin,
                    },
                    {
                        namePropertyPrefixLength,
                        network::ip::fieldMask,
                    },
                    {
                        namePropertyType,
                        network::ip::fieldType,
                    },
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
                {
                    std::bind(formatStringValueFromDict, originsDict, _1, _2,
                              _3),
                },
            },
            {
                namePropertyType,
                {
                    std::bind(formatStringValueFromDict, typeDict, _1, _2, _3),
                },
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

    static const std::map<std::string, std::string> dhcpConfDict;
    static const std::map<std::string, std::string> LinkTypeDict;
  public:
    NetworkEthInterface() : dbus::FindObjectDBusQuery()
    {}
    ~NetworkEthInterface() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                netEthInterfaceName,
                {
                    {
                        namePropertyAutoNeg,
                        network::ethernet::fieldAutoNeg,
                    },
                    {
                        namePropertyDHCPEnabled,
                        network::ethernet::fieldDHCPEnabled,
                    },
                    {
                        namePropertyDomainName,
                        network::ethernet::fieldDomainName,
                    },
                    {
                        namePropertyIPv6AcceptRA,
                        network::ethernet::fieldIPv6AcceptRA,
                    },
                    {
                        namePropertyInterfaceName,
                        network::ethernet::fieldInterfaceName,
                    },
                    {
                        namePropertyLinkLocalAutoConf,
                        network::ethernet::fieldLinkLocalAutoConf,
                    },
                    {
                        namePropertyLinkUp,
                        network::ethernet::fieldLinkUp,
                    },
                    {
                        namePropertyNICEnabled,
                        network::ethernet::fieldNICEnabled,
                    },
                    {
                        namePropertyNTPServers,
                        network::ethernet::fieldNTPServers,
                    },
                    {
                        namePropertyNameservers,
                        network::ethernet::fieldNameservers,
                    },
                    {
                        namePropertySpeed,
                        network::ethernet::fieldSpeed,
                    },
                    {
                        namePropertyStaticNameServers,
                        network::ethernet::fieldStaticNameServers,
                    },
                },
            },
            {
                netMACInterfaceName,
                {
                    {
                        namePropertyMACAddress,
                        network::ethernet::fieldMACAddress,
                    },
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
                    return instance->getStringValue().starts_with(objectPath);
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
                    std::bind(formatStringValueFromDict, dhcpConfDict,
                              _1, _2, _3),
                },
            },
            {
                namePropertyLinkLocalAutoConf,
                {
                    std::bind(formatStringValueFromDict, LinkTypeDict,
                              _1, _2, _3),
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

} // namespace obmc
} // namespace query
} // namespace app

#endif // __NETWORK_PROVIDER_H__
