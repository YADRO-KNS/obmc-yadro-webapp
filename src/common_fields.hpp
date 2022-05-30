// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

namespace app
{
namespace obmc
{
namespace entity
{
namespace general
{
namespace assets
{
constexpr const char* assetInterface =
    "xyz.openbmc_project.Inventory.Decorator.Asset";

constexpr const char* manufacturer = "Manufacturer";
constexpr const char* model = "Model";
constexpr const char* partNumber = "PartNumber";
constexpr const char* serialNumber = "SerialNumber";

} // namespace assets
} // namespace general
namespace relations
{
constexpr const char* fieldAssociations = "Associations";
constexpr const char* fieldEndpoint = "Endpoint";
constexpr const char* fieldSource = "Source";
constexpr const char* fieldDestination = "Destination";
} // namespace relations
} // namespace entity
} // namespace obmc
} // namespace app
