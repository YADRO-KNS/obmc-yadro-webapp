// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

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
class PID final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* emServiceName =
        "xyz.openbmc_project.EntityManager";
    static constexpr const char* pidInterfaceName =
        "xyz.openbmc_project.Configuration.Pid";
    static constexpr const char* propClass = "Class";
    static constexpr const char* propFFGainCoefficient = "FFGainCoefficient";
    static constexpr const char* propFFOffCoefficient = "FFOffCoefficient";
    static constexpr const char* propICoefficient = "ICoefficient";
    static constexpr const char* propILimitMax = "ILimitMax";
    static constexpr const char* propILimitMin = "ILimitMin";
    static constexpr const char* propInputs = "Inputs";
    static constexpr const char* propName = "Name";
    static constexpr const char* propNegativeHysteresis = "NegativeHysteresis";
    static constexpr const char* propOutLimitMax = "OutLimitMax";
    static constexpr const char* propOutLimitMin = "OutLimitMin";
    static constexpr const char* propOutputs = "Outputs";
    static constexpr const char* propPCoefficient = "PCoefficient";
    static constexpr const char* propPositiveHysteresis = "PositiveHysteresis";
    static constexpr const char* propSlewNeg = "SlewNeg";
    static constexpr const char* propSlewPos = "SlewPos";
    static constexpr const char* propType = "Type";
    static constexpr const char* propZones = "Zones";

    static constexpr const char* fieldName = "name";
    static constexpr const char* fieldClass = "class";
    static constexpr const char* fieldType = "type";
    static constexpr const char* fieldInputs = "inputs";
    static constexpr const char* fieldFFGainCoefficient = "feedForwardGainCoef";
    static constexpr const char* fieldFFOffCoefficient =
        "feedForwardOffsetCoef";
    static constexpr const char* fieldICoefficient = "integralCoef";
    static constexpr const char* fieldILimitMax = "integralLimitMax";
    static constexpr const char* fieldILimitMin = "integralLimitMin";
    static constexpr const char* fieldNegativeHysteresis = "negativeHyst";
    static constexpr const char* fieldPositiveHysteresis = "positiveHyst";
    static constexpr const char* fieldOutLimitMax = "outputMax";
    static constexpr const char* fieldOutLimitMin = "outputMin";
    static constexpr const char* fieldOutputs = "outputs";
    static constexpr const char* fieldPCoefficient = "proportionalCoef";
    static constexpr const char* fieldSlewNeg = "slewNegative";
    static constexpr const char* fieldSlewPos = "slewPositive";

  public:
    static constexpr const char* fieldZones = "zones";
    static constexpr const char* entityName = "PID";
    static const std::vector<std::string> fields;

    PID() : dbus::FindObjectDBusQuery()
    {}
    ~PID() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                pidInterfaceName,
                {
                    {propClass, fieldClass},
                    {propFFGainCoefficient, fieldFFGainCoefficient},
                    {propFFOffCoefficient, fieldFFOffCoefficient},
                    {propICoefficient, fieldICoefficient},
                    {propILimitMax, fieldILimitMax},
                    {propILimitMin, fieldILimitMin},
                    {propInputs, fieldInputs},
                    {propName, fieldName},
                    {propNegativeHysteresis, fieldNegativeHysteresis},
                    {propOutLimitMax, fieldOutLimitMax},
                    {propOutLimitMin, fieldOutLimitMin},
                    {propOutputs, fieldOutputs},
                    {propPCoefficient, fieldPCoefficient},
                    {propPositiveHysteresis, fieldPositiveHysteresis},
                    {propSlewNeg, fieldSlewNeg},
                    {propSlewPos, fieldSlewPos},
                    {propType, fieldType},
                    {propZones, fieldZones},
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/",
            {
                pidInterfaceName,
            },
            noDepth,
            emServiceName,
        };

        return criteria;
    }
};

const std::vector<std::string> PID::fields = {
    fieldClass,
    fieldFFGainCoefficient,
    fieldFFOffCoefficient,
    fieldICoefficient,
    fieldILimitMax,
    fieldILimitMin,
    fieldInputs,
    fieldName,
    fieldNegativeHysteresis,
    fieldOutLimitMax,
    fieldOutLimitMin,
    fieldOutputs,
    fieldPCoefficient,
    fieldPositiveHysteresis,
    fieldSlewNeg,
    fieldSlewPos,
    fieldType,
    fieldZones,
};

class PIDZone final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* emServiceName =
        "xyz.openbmc_project.EntityManager";
    static constexpr const char* pidZoneInterfaceName =
        "xyz.openbmc_project.Configuration.Pid.Zone";
    static constexpr const char* propPidName = "Name";
    static constexpr const char* propFailSafePercent = "FailSafePercent";
    static constexpr const char* propMinThermalOutput = "MinThermalOutput";

  public:
    static constexpr const char* entityName = "PIDZone";

    static constexpr const char* fieldName = "name";
    static constexpr const char* fieldFailsafePercent = "failSafePercent";
    static constexpr const char* fieldMinOutputPercent = "minOutputPercent";

    PIDZone() : dbus::FindObjectDBusQuery()
    {}
    ~PIDZone() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {pidZoneInterfaceName,
             {
                 {propPidName, fieldName},
                 {propFailSafePercent, fieldFailsafePercent},
                 {propMinThermalOutput, fieldMinOutputPercent},
             }},
        };

        return dictionary;
    }

    static const IEntity::IRelation::RelationRulesList& relationToPID()
    {
        static const IEntity::IRelation::RelationRulesList relations{
            {
                fieldName,
                PID::fieldZones,
                [](const IEntity::IEntityMember::InstancePtr& instance,
                   const IEntity::IEntityMember::IInstance::FieldType& value)
                    -> bool {
                    try
                    {
                        const auto& searchZone = std::get<std::string>(value);
                        const auto& zones = std::get<std::vector<std::string>>(
                            instance->getValue());

                        return std::any_of(
                            zones.begin(), zones.end(),
                            [&searchZone](const std::string& zone) -> bool {
                                return zone == searchZone;
                            });
                    }
                    catch (std::exception& ex)
                    {
                        BMC_LOG_ERROR << "Error retreive PID relation fields: "
                                      << ex.what();
                    }

                    return false;
                },
            },
        };
        return relations;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/",
            {pidZoneInterfaceName},
            noDepth,
            emServiceName,
        };

        return criteria;
    }
};

} // namespace obmc
} // namespace query
} // namespace app
