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

class PIDProfile final :
    public entity::Entity,
    public CachedSource,
    public NamedEntity<PIDProfile>
{
  public:
    enum class Profile
    {
        acoustic,
        performance
    };
    ENTITY_DECL_FIELD_ENUM(Profile, Profile, acoustic)
    ENTITY_DECL_FIELD_DEF(std::vector<int>, SupportedProfiles, {})

  private:
    class Query : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* thermalModeIface =
            "xyz.openbmc_project.Control.ThermalMode";

        class FormatProfile : public query::dbus::IFormatter
        {
          public:
            ~FormatProfile() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, Profile> profiles{
                    {"Acoustic", Profile::acoustic},
                    {"Performance", Profile::performance},
                };
                // clang-format: on
                return formatValueFromDict(profiles, property, value,
                                           Profile::acoustic);
            }
        };

        class FormatSupportedProfiles : public query::dbus::IFormatter
        {
          public:
            ~FormatSupportedProfiles() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                std::vector<int> supportedProfiles;
                FormatProfile formatter;
                if (std::holds_alternative<std::vector<std::string>>(value))
                {
                    const auto& valueDict =
                        std::get<std::vector<std::string>>(value);
                    for (const auto& profile : valueDict)
                    {
                        auto intEnumVal =
                            std::get<int>(formatter.format(property, profile));
                        supportedProfiles.emplace_back(intEnumVal);
                    }
                }
                return supportedProfiles;
            }
        };

      public:
        Query() : FindObjectDBusQuery()
        {}
        ~Query() override = default;

        // clang-format: off
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                thermalModeIface,
                DBUS_QUERY_EP_SET_FORMATTERS("Current", fieldProfile,
                                             DBUS_QUERY_EP_CSTR(FormatProfile)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS("Supported", fieldSupportedProfiles,
                                             DBUS_QUERY_EP_CSTR(FormatSupportedProfiles)
                )
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/control/",
            DBUS_QUERY_CRIT_IFACES(thermalModeIface),
            noDepth,
            std::nullopt
        )
        // clang-format: on
    };

  public:
    PIDProfile() :
        Entity(), query(std::make_shared<Query>())
    {}
    ~PIDProfile() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

class PID final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<PID>
{
  public:
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD(std::string, Type)
    ENTITY_DECL_FIELD(std::string, Class)

    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, Inputs, {})
    ENTITY_DECL_FIELD_DEF(std::vector<std::string>, Outputs, {})

    ENTITY_DECL_FIELD_DEF(double, FeedForwardGainCoef, 0);
    ENTITY_DECL_FIELD_DEF(double, FeedForwardOffsetCoef, 0)

    ENTITY_DECL_FIELD_DEF(double, IntegralCoef, 0)
    ENTITY_DECL_FIELD_DEF(double, IntegralLimitMax, 0)
    ENTITY_DECL_FIELD_DEF(double, IntegralLimitMin, 0)
    ENTITY_DECL_FIELD_DEF(double, NegativeHyst, 0)
    ENTITY_DECL_FIELD_DEF(double, PositiveHyst, 0)
    ENTITY_DECL_FIELD_DEF(double, OutputMax, 0)
    ENTITY_DECL_FIELD_DEF(double, OutputMin, 0)
    ENTITY_DECL_FIELD_DEF(double, ProportionalCoef, 0)
    ENTITY_DECL_FIELD_DEF(double, SlewNegative, 0)
    ENTITY_DECL_FIELD_DEF(double, SlewPositive, 0)

  private:
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
                DBUS_QUERY_EP_FIELDS_ONLY(propFFGainCoefficient, fieldFeedForwardGainCoef),
                DBUS_QUERY_EP_FIELDS_ONLY(propFFOffCoefficient, fieldFeedForwardOffsetCoef),
                DBUS_QUERY_EP_FIELDS_ONLY(propICoefficient, fieldIntegralCoef),
                DBUS_QUERY_EP_FIELDS_ONLY(propILimitMax, fieldIntegralLimitMax),
                DBUS_QUERY_EP_FIELDS_ONLY(propILimitMin, fieldIntegralLimitMin),
                DBUS_QUERY_EP_FIELDS_ONLY(propNegativeHysteresis, fieldNegativeHyst),
                DBUS_QUERY_EP_FIELDS_ONLY(propOutLimitMax, fieldOutputMax),
                DBUS_QUERY_EP_FIELDS_ONLY(propOutLimitMin, fieldOutputMin),
                DBUS_QUERY_EP_FIELDS_ONLY(propPCoefficient, fieldProportionalCoef),
                DBUS_QUERY_EP_FIELDS_ONLY(propPositiveHysteresis, fieldPositiveHyst),
                DBUS_QUERY_EP_FIELDS_ONLY(propSlewNeg, fieldSlewNegative),
                DBUS_QUERY_EP_FIELDS_ONLY(propSlewPos, fieldSlewPositive),
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
    ENTITY_DECL_RELATIONS(ENTITY_DEF_RELATION(PIDZone, relationRules()))
  private:
    DBusQueryPtr query;
};

class PIDZone final :
    public entity::Collection,
    public CachedSource,
    public NamedEntity<PIDZone>
{
  public:
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD_DEF(double, MinOutputPercent, 0)
    ENTITY_DECL_FIELD_DEF(double, FailSafePercent, 0)

  private:
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
                    propFailSafePercent, fieldFailSafePercent),
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
    ENTITY_DECL_RELATIONS(ENTITY_DEF_RELATION(PID, PID::relationRules()))
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
