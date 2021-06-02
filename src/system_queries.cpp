// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/exceptions.hpp>
#include <system_queries.hpp>

namespace app
{
namespace query
{
namespace obmc
{

const std::map<std::string, std::string> Chassis::chassisTypesNames{
    {"23", "Rack Mount"},
};

const std::map<std::string, std::string> Sensors::sensorUnitsMap{
    {"xyz.openbmc_project.Sensor.Value.Unit.DegreesC", "°C"},
    {"xyz.openbmc_project.Sensor.Value.Unit.Volts", "Volts"},
    {"xyz.openbmc_project.Sensor.Value.Unit.RPMS", "RPM"},
    {"xyz.openbmc_project.Sensor.Value.Unit.Meters", "Meters"},
    {"xyz.openbmc_project.Sensor.Value.Unit.Amperes", "Amperes"},
    {"xyz.openbmc_project.Sensor.Value.Unit.Watts", "Watts"},
    {"xyz.openbmc_project.Sensor.Value.Unit.Joules", "Joules"},
    {"xyz.openbmc_project.Sensor.Value.Unit.Percent", "%"},
};

} // namespace obmc
} // namespace query
} // namespace app
