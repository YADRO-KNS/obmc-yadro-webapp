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
  public:
    ENTITY_DECL_FIELD(std::string, HostName)
    ENTITY_DECL_FIELD(std::string, DefaultIPv4Gateway)
    ENTITY_DECL_FIELD(std::string, DefaultIPv6Gateway)

  private:
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
  public:
    ENTITY_DECL_FIELD_DEF(bool, DNSEnabled, false)
    ENTITY_DECL_FIELD_DEF(bool, HostNameEnabled, false)
    ENTITY_DECL_FIELD_DEF(bool, NTPEnabled, false)
    ENTITY_DECL_FIELD_DEF(bool, SendHostNameEnabled, false)
  private:
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
  public:
   enum class Type
   {
      ipv4,
      ipv6
   };
   enum class Origin
   {
      staticOrigin,
      dhcp,
      linkLocal,
      slaac
   };
    ENTITY_DECL_FIELD_ENUM(Origin, Origin, staticOrigin)
    ENTITY_DECL_FIELD_ENUM(Type, Type, ipv4)
    ENTITY_DECL_FIELD(std::string, Address)
    ENTITY_DECL_FIELD(std::string, Gateway)
    ENTITY_DECL_FIELD(std::string, SubnetMask)
    ENTITY_DECL_FIELD_DEF(uint8_t, PrefixLength, 0U)
  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* netIpInterfaceName =
            "xyz.openbmc_project.Network.IP";
        class FormatOrigin : public query::dbus::IFormatter
        {
            inline const std::string dbusEnum(const std::string& enumStr)
            {
                return "xyz.openbmc_project.Network.IP.AddressOrigin." +
                       enumStr;
            }

          public:
            ~FormatOrigin() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, Origin> origins{
                    {dbusEnum("Static"), Origin::staticOrigin},
                    {dbusEnum("DHCP"), Origin::dhcp},
                    {dbusEnum("LinkLocal"), Origin::linkLocal},
                    {dbusEnum("SLAAC"), Origin::slaac},
                };

                return formatValueFromDict(origins, property, value,
                                           Origin::staticOrigin);
            }
        };

        class FormatType : public query::dbus::IFormatter
        {
            inline const std::string dbusEnum(const std::string& enumStr)
            {
                return "xyz.openbmc_project.Network.IP.Protocol." + enumStr;
            }

          public:
            ~FormatType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, Type> types{
                    {dbusEnum("IPv4"), Type::ipv4},
                    {dbusEnum("IPv6"), Type::ipv6},
                };

                return formatValueFromDict(types, property, value, Type::ipv4);
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
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldPrefixLength),
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
        void supplementByStaticFields(const DBusInstancePtr& instance) const override
        {
            this->setSubnetMask(instance);
            this->setGateway(instance);
        }
        inline void setSubnetMask(const DBusInstancePtr& instance) const
        {
            if (getFieldType(instance) == Type::ipv4)
            {
                auto subnetMask =
                    helpers::utils::getNetmask(getFieldPrefixLength(instance));
                setFieldSubnetMask(instance, subnetMask);
            }
        }
        inline void setGateway(const DBusInstancePtr& instance) const
        {
            if (instance->getField(fieldGateway)->isNull() ||
                getFieldGateway(instance).empty())
            {
                setFieldGateway(instance, "0.0.0.0");
            }
        }
    };

  public:
    static const ConditionPtr nonStaticIp()
    {
       return Condition::buildNonEqual(fieldOrigin, Origin::staticOrigin);
    }

    static const ConditionPtr staticIp()
    {
       return Condition::buildEqual(fieldOrigin, Origin::staticOrigin);
    }

    IP() : Collection(), query(std::make_shared<Query>())
    {
        this->createMember(fieldSubnetMask);
    }
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
  public:
    enum class DHCPState
    {
        both,
        v4,
        v6,
        none
    };

    enum class OperatingMode
    {
        stateful,
        disabled,
    };

    enum class LinkConfiguration
    {
        fallback,
        both,
        v4,
        v6,
        none
    };
    
    enum class LinkStatus
    {
        linkUp,
        linkDown,
        noLink
    };
        
    enum class State
    {
        enabled,
        disabled
    };

    enum class Kind
    {
        physical,
        vlan,
    };

    ENTITY_DECL_FIELD_DEF(bool, DHCPv4Enabled, false)
    ENTITY_DECL_FIELD_ENUM(DHCPState, DHCPEnabled, none)
    ENTITY_DECL_FIELD_ENUM(OperatingMode, OperatingMode, disabled)
    ENTITY_DECL_FIELD_ENUM(LinkConfiguration, LinkLocalAutoConf, none)
    ENTITY_DECL_FIELD_ENUM(Kind, Kind, physical)
    ENTITY_DECL_FIELD_ENUM(State, State, disabled)
    ENTITY_DECL_FIELD_ENUM(LinkStatus, LinkStatus, noLink)

    ENTITY_DECL_FIELD(std::string, FQDN)
    ENTITY_DECL_FIELD(std::string, InterfaceName)
    ENTITY_DECL_FIELD(std::string, VLANName)
    ENTITY_DECL_FIELD(std::string, MACAddress)
    ENTITY_DECL_FIELD_DEF(bool, AutoNeg, false)
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, DomainName, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, NTPServers, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, Nameservers, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, StaticNameServers, {})
    ENTITY_DECL_FIELD_DEF(bool, IPv6AcceptRA, false)
    ENTITY_DECL_FIELD_DEF(bool, LinkUp, false)
    ENTITY_DECL_FIELD_DEF(bool, NICEnabled, false)
    ENTITY_DECL_FIELD_DEF(uint32_t, Speed, 0U)
    ENTITY_DECL_FIELD_DEF(uint32_t, VLANId, 0U)

  private:
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
            inline const std::string dbusEnum(const std::string& enumStr)
            {
                return "xyz.openbmc_project.Network.EthernetInterface."
                       "DHCPConf." +
                       enumStr;
            }

          public:
            ~FormatDHCPEnabled() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, DHCPState> dhcpConf{
                    {dbusEnum("both"), DHCPState::both},
                    {dbusEnum("v4"), DHCPState::v4},
                    {dbusEnum("v6"), DHCPState::v6},
                    {dbusEnum("none"), DHCPState::none},
                };

                return formatValueFromDict(dhcpConf, property, value,
                                           DHCPState::none);
            }
        };

        class FormatLinkConf : public query::dbus::IFormatter
        {
            inline const std::string dbusEnum(const std::string& enumStr)
            {
                return "xyz.openbmc_project.Network.EthernetInterface."
                       "LinkLocalConf." + enumStr;
            }
          public:
            ~FormatLinkConf() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, LinkConfiguration> linkConf{
                    {dbusEnum("fallback"), LinkConfiguration::fallback},
                    {dbusEnum("both"), LinkConfiguration::both},
                    {dbusEnum("v4"), LinkConfiguration::v4},
                    {dbusEnum("v6"), LinkConfiguration::v6},
                    {dbusEnum("none"), LinkConfiguration::none},
                };

                return formatValueFromDict(linkConf, property, value,
                                           LinkConfiguration::none);
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
        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            static const DefaultFieldsValueDict defaults{
                {fieldFQDN, setFQDN},
            };
            return defaults;
        }
        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            this->setKind(instance);
            this->setState(instance);
            this->setLinkStatus(instance);
            this->setDHCPv4Enabled(instance);
            this->setOperatingMode(instance);
        }

        inline void setKind(const DBusInstancePtr& instance) const
        {
            if (instance->getField(fieldVLANId)->isNull())
            {
                setFieldKind(instance, Kind::physical);
                return;
            }
            setFieldKind(instance, Kind::vlan);
        }
        inline void setLinkStatus(const DBusInstancePtr& instance) const
        {
            if (getFieldState(instance) == State::disabled)
            {
                setFieldLinkStatus(instance, LinkStatus::noLink);
                return;
            }
            if (getFieldLinkUp(instance))
            {
                setFieldLinkStatus(instance, LinkStatus::linkUp);
                return;
            }
            setFieldLinkStatus(instance, LinkStatus::linkDown);
        }
        inline void setState(const DBusInstancePtr& instance) const
        {
            if (getFieldNICEnabled(instance))
            {
                setFieldState(instance, State::enabled);
                return;
            }
            setFieldState(instance, State::disabled);
        }
        inline void setDHCPv4Enabled(const DBusInstancePtr& instance) const
        {
            auto dhcpState = getFieldDHCPEnabled(instance);
            if (dhcpState == DHCPState::both || dhcpState == DHCPState::v4)
            {
                setFieldDHCPv4Enabled(instance, true);
                return;
            }
            setFieldDHCPv4Enabled(instance, false);
        }
        inline void setOperatingMode(const DBusInstancePtr& instance) const
        {
            auto dhcpState = getFieldDHCPEnabled(instance);
            if (dhcpState == DHCPState::both || dhcpState == DHCPState::v4)
            {
                setFieldOperatingMode(instance, OperatingMode::stateful);
                return;
            }
            setFieldOperatingMode(instance, OperatingMode::disabled);
        }
        static IEntityMember::IInstance::FieldType
            setFQDN(const IEntity::InstancePtr& instance)
        {
            const auto netCfgEntity = NetworkConfig::getEntity();
            const auto netCfgInstances = netCfgEntity->getInstances();
            if (netCfgInstances.empty())
            {
                return std::nullptr_t(nullptr);
            }
            const auto hostname =
                NetworkConfig::getFieldHostName(netCfgInstances.front());
            const auto domainNames = getFieldDomainName(instance);
            if (domainNames.empty())
            {
                return hostname;
            }
            return hostname + "." + domainNames.front();
        }
    };

  public:
    Ethernet() : Collection(), query(std::make_shared<Query>())
    {
      this->createMember(fieldKind);
      this->createMember(fieldFQDN);
      this->createMember(fieldLinkStatus);
      this->createMember(fieldState);
      this->createMember(fieldDHCPv4Enabled);
      this->createMember(fieldOperatingMode);
    }
    ~Ethernet() override = default;

    static const ConditionPtr vlanEthernet()
    {
       return Condition::buildEqual(fieldKind, Kind::vlan);
    }

    static const ConditionPtr physicalEthernet()
    {
       return Condition::buildEqual(fieldKind, Kind::physical);
    }

  protected:
    static const IEntity::IRelation::RelationRulesList& relationToVLAN()
    {
        static const IEntity::IRelation::RelationRulesList relations{
            {fieldInterfaceName, fieldVLANName,
             [](const auto& instance, const auto& value) -> bool {
                 if (instance->isNull() ||
                     !std::holds_alternative<std::string>(value))
                 {
                     return false;
                 }

                 const auto destVlanName = instance->getStringValue();
                 const auto sourceIfaceName = std::get<std::string>(value);
                 return destVlanName.starts_with(sourceIfaceName + ".");
             }},
        };
        return relations;
    }

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
                    // xyz /openbmc_project/network/eth0_999/ipv4/4bf59855`
                    // the relation will be established for both
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
    ENTITY_DECL_RELATIONS(
      ENTITY_DEF_RELATION(IP, relationToIp()),
      ENTITY_DEF_RELATION2(Ethernet, relationToVLAN())
    )
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
