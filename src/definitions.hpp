// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __SYSTEM_DEFINITIONS_H__
#define __SYSTEM_DEFINITIONS_H__

namespace app
{
namespace entity
{
namespace obmc
{
namespace definitions
{

constexpr const char* entityChassis = "Chassis";
constexpr const char* entityServer = "Server";
constexpr const char* entitySensors = "Sensors";
constexpr const char* entityBaseboard = "Baseboard";
constexpr const char* entityNetwork = "Network";
constexpr const char* fieldName = "Name";
constexpr const char* fieldType = "Type";
constexpr const char* fieldModel = "Model";
constexpr const char* fieldManufacturer = "Manufacturer";
constexpr const char* fieldSerialNumber = "SerialNumber";
constexpr const char* fieldPartNumber = "PartNumber";
constexpr const char* fieldDatetime = "Datetime";

constexpr const char* metaFieldPrefix = "__meta_";

constexpr const char* metaObjectPath = "__meta_field__object_path";
constexpr const char* metaObjectService = "__meta_field__object_service";

constexpr const char* metaRelation = "__meta_relations__";

namespace sensors
{
constexpr const char* fieldName = "Name";
constexpr const char* fieldValue = "Reading";
constexpr const char* fieldUnit = "Unit";
constexpr const char* fieldLowCritical = "LowCritical";
constexpr const char* fieldLowWarning = "LowWarning";
constexpr const char* fieldHighWarning = "HighWarning";
constexpr const char* fieldHightCritical = "HighCritical";

constexpr const char* fieldAvailable = "Available";
constexpr const char* fieldFunctional = "Functional";

} // namespace sensors

namespace version
{
constexpr const char* fieldVersionBios = "BiosVersion";
constexpr const char* fieldVersionBmc = "BmcVersion";
} // namespace version

namespace power
{
constexpr const char* entityHostPower = "HostPower";
constexpr const char* fieldState = "PowerState";
constexpr const char* fieldStatus = "Status";
constexpr const char* metaStatus = "__meta__raw_dbus_status";
} // namespace power

namespace network
{

namespace config
{

constexpr const char* entityNetConf = "NetworkConfig";
constexpr const char* entityDHCPConf = "NetworkDHCPConfig";

static constexpr const char* fieldHostName = "HostName";
static constexpr const char* fieldDefaultIPv4Gateway = "DefaultIPv4Gateway";
static constexpr const char* fieldDefaultIPv6Gateway = "DefaultIPv6Gateway";

static constexpr const char* fieldDNSEnabled = "DNSEnabled";
static constexpr const char* fieldHostNameEnabled = "HostNameEnabled";
static constexpr const char* fieldNTPEnabled = "NTPEnabled";
static constexpr const char* fieldSendHostNameEnabled = "SendHostNameEnabled";

} // namespace config

namespace ip
{

constexpr const char* entityIpIface = "IP";

static constexpr const char* fieldAddress = "Address";
static constexpr const char* fieldGateway = "Gateway";
static constexpr const char* fieldOrigin = "Origin";
static constexpr const char* fieldMask = "SubnetMask";
static constexpr const char* fieldType = "Type";

}

namespace ethernet
{
constexpr const char* entityEthIface = "Ethernet";

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
} // namespace ethernet

} // namespace network

namespace pcie
{
constexpr const char* entityPCIe = "PCIe";

namespace functions
{
constexpr const char* entityFunction = "Function";
constexpr const char* fieldFunctionId = "FunctionId";
} // namespace functions

} // namespace pcie

namespace supplement_providers
{

namespace relations
{
constexpr const char* providerRelations = "Relations";
constexpr const char* fieldAssociations = "Associations";
constexpr const char* fieldEndpoint = "Endpoint";
constexpr const char* fieldSource = "Source";
constexpr const char* fieldDestination = "Destination";

} // namespace relations

namespace pcie
{
constexpr const char* providerPCIe = "__provider__PCIe";
constexpr const char* metaSBD = "__meta__SBD";

constexpr const char* fieldSocket = "Socket";
constexpr const char* fieldBus = "Bus";
constexpr const char* fieldAddress = "Address";
constexpr const char* fieldDevice = "Device";
constexpr const char* fieldDeviceType = "DeviceType";
constexpr const char* fieldManufacturer = "Manufacturer";
constexpr const char* fieldSubsystem = "Subsystem";

namespace functions
{
constexpr const char* fieldClassCode = "ClassCode";
constexpr const char* fieldDeviceClass = "DeviceClass";
constexpr const char* fieldDeviceId = "DeviceId";
constexpr const char* fieldDeviceName = "DeviceName";
constexpr const char* fieldFunctionType = "FunctionType";
constexpr const char* fieldRevisionId = "RevisionId";
constexpr const char* fieldSubsystemId = "SubsystemId";
constexpr const char* fieldSubsystemName = "SubsystemName";
constexpr const char* fieldSubsystemVendorId = "SubsystemVendorId";
constexpr const char* fieldVendorId = "VendorId";
constexpr const char* fieldVendorName = "VendorName";
} // namespace functions

} // namespace pcie

namespace status
{
constexpr const char* providerStatus = "__provider__Status";

constexpr const char* fieldStatus = "Status";
constexpr const char* fieldObjectCauthPath = "Causer";

} // namespace status
namespace version
{
constexpr const char* providerVersion = "__provider__Version";
constexpr const char* fieldVersion = "Version";
constexpr const char* fieldPurpose = "Purpose";
} // namespace version

namespace assetTag
{
constexpr const char* providerAssetTag = "__provider__AssetTag";
constexpr const char* fieldTag = "AssetTag";
} // namespace assetTag

namespace indicatorLed
{
constexpr const char* providerIndicatorLed = "__provider__IndicatorLed";
constexpr const char* fieldLed = "Led";
} // namespace assetTag


} // namespace supplement_providers

} // namespace definitions
} // namespace obmc
} // namespace entity
} // namespace app
#endif // __SYSTEM_DEFINITIONS_H__
