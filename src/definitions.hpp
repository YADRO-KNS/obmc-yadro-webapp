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


namespace version
{
constexpr const char* fieldVersionBios = "BiosVersion";
constexpr const char* fieldVersionBmc = "BmcVersion";
} // namespace version


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
