// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <formatters.hpp>
#include <phosphor-logging/log.hpp>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::entity;
using namespace app::query;
using namespace phosphor::logging;

class PIDZone;

class PID final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<PID>
{
    static constexpr const char* fieldName = "Name";
    static constexpr const char* fieldClass = "Class";
    static constexpr const char* fieldType = "Type";
    static constexpr const char* fieldInputs = "Inputs";
    static constexpr const char* fieldFFGainCoefficient = "FeedForwardGainCoef";
    static constexpr const char* fieldFFOffCoefficient =
        "FeedForwardOffsetCoef";
    static constexpr const char* fieldICoefficient = "IntegralCoef";
    static constexpr const char* fieldILimitMax = "IntegralLimitMax";
    static constexpr const char* fieldILimitMin = "IntegralLimitMin";
    static constexpr const char* fieldNegativeHysteresis = "NegativeHyst";
    static constexpr const char* fieldPositiveHysteresis = "PositiveHyst";
    static constexpr const char* fieldOutLimitMax = "OutputMax";
    static constexpr const char* fieldOutLimitMin = "OutputMin";
    static constexpr const char* fieldOutputs = "Outputs";
    static constexpr const char* fieldPCoefficient = "ProportionalCoef";
    static constexpr const char* fieldSlewNeg = "SlewNegative";
    static constexpr const char* fieldSlewPos = "SlewPositive";

    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* emServiceName =
            "xyz.openbmc_project.EntityManager";
        static constexpr const char* pidInterfaceName =
            "xyz.openbmc_project.Configuration.Pid";

        static constexpr const char* propFFGainCoefficient =
            "FFGainCoefficient";
        static constexpr const char* propFFOffCoefficient = "FFOffCoefficient";
        static constexpr const char* propICoefficient = "ICoefficient";
        static constexpr const char* propILimitMax = "ILimitMax";
        static constexpr const char* propILimitMin = "ILimitMin";
        static constexpr const char* propNegativeHysteresis =
            "NegativeHysteresis";
        static constexpr const char* propOutLimitMax = "OutLimitMax";
        static constexpr const char* propOutLimitMin = "OutLimitMin";
        static constexpr const char* propPCoefficient = "PCoefficient";
        static constexpr const char* propPositiveHysteresis =
            "PositiveHysteresis";
        static constexpr const char* propSlewNeg = "SlewNeg";
        static constexpr const char* propSlewPos = "SlewPos";

      public:
        static constexpr const char* fieldZones = "Zones";
        static const std::vector<std::string> fields;

        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                pidInterfaceName,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldName),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldClass),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldType),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldZones),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldInputs),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldOutputs),
                DBUS_QUERY_EP_FIELDS_ONLY(propFFGainCoefficient, fieldFFGainCoefficient),
                DBUS_QUERY_EP_FIELDS_ONLY(propFFOffCoefficient, fieldFFOffCoefficient),
                DBUS_QUERY_EP_FIELDS_ONLY(propICoefficient, fieldICoefficient),
                DBUS_QUERY_EP_FIELDS_ONLY(propILimitMax, fieldILimitMax),
                DBUS_QUERY_EP_FIELDS_ONLY(propILimitMin, fieldILimitMin),
                DBUS_QUERY_EP_FIELDS_ONLY(propNegativeHysteresis, fieldNegativeHysteresis),
                DBUS_QUERY_EP_FIELDS_ONLY(propOutLimitMax, fieldOutLimitMax),
                DBUS_QUERY_EP_FIELDS_ONLY(propOutLimitMin, fieldOutLimitMin),
                DBUS_QUERY_EP_FIELDS_ONLY(propPCoefficient, fieldPCoefficient),
                DBUS_QUERY_EP_FIELDS_ONLY(propPositiveHysteresis, fieldPositiveHysteresis),
                DBUS_QUERY_EP_FIELDS_ONLY(propSlewNeg, fieldSlewNeg),
                DBUS_QUERY_EP_FIELDS_ONLY(propSlewPos, fieldSlewPos),
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/",
            DBUS_QUERY_CRIT_IFACES(pidInterfaceName),
            noDepth,
            emServiceName
        )
        /* clang-format on */
    };

  public:
    PID() : Collection(), query(std::make_shared<Query>())
    {}
    ~PID() override = default;

    static const IEntity::IRelation::RelationRulesList& relationRules()
    {
        static const IEntity::IRelation::RelationRulesList relations{
            {
                fieldName,
                "Zones",
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
                        log<level::ERR>(
                            "Fail to configure relation Zone to PID",
                            entry("ERROR=%s", ex.what()));
                    }

                    return false;
                },
            },
        };
        return relations;
    }
  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATION(PIDZone, relationRules())
  private:
    DBusQueryPtr query;
};

class PIDZone final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<PIDZone>
{
    static constexpr const char* fieldName = "Name";
    static constexpr const char* fieldFailsafePercent = "FailSafePercent";
    static constexpr const char* fieldMinOutputPercent = "MinOutputPercent";

    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* emServiceName =
            "xyz.openbmc_project.EntityManager";
        static constexpr const char* pidZoneInterfaceName =
            "xyz.openbmc_project.Configuration.Pid.Zone";

        static constexpr const char* propPidName = "Name";
        static constexpr const char* propFailSafePercent = "FailSafePercent";
        static constexpr const char* propMinThermalOutput = "MinThermalOutput";

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                pidZoneInterfaceName,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldName),
                DBUS_QUERY_EP_FIELDS_ONLY(
                    propFailSafePercent, fieldFailsafePercent),
                DBUS_QUERY_EP_FIELDS_ONLY(
                    propMinThermalOutput, fieldMinOutputPercent)
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/",
            DBUS_QUERY_CRIT_IFACES(pidZoneInterfaceName),
            noDepth,
            emServiceName
        )
        /* clang-format on */
    };

  public:
    PIDZone() : Collection(), query(std::make_shared<Query>())
    {}
    ~PIDZone() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATION(PID, PID::relationRules())
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
