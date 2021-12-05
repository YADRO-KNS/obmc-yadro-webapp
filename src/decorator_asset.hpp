// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

namespace app
{
namespace query
{
namespace obmc
{
namespace general
{
namespace assets
{
constexpr const char* assetInterface =
    "xyz.openbmc_project.Inventory.Decorator.Asset";

constexpr const char* propertyManufacturer = "Manufacturer";
constexpr const char* propertyModel = "Model";
constexpr const char* propertyPartNumber = "PartNumber";
constexpr const char* propertySerialNumber = "SerialNumber";

} // namespace assets
} // namespace general
} // namespace obmc
} // namespace query
} // namespace app
