// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/helpers/utils.hpp>
#include <formatters.hpp>
#include <phosphor-logging/log.hpp>
#include <status_provider.hpp>

namespace app
{
namespace obmc
{
namespace entity
{
using namespace app::entity;
using namespace app::query;
using namespace phosphor::logging;
using namespace app::helpers::utils;
class Sensors :
    public entity::Collection,
    public NamedEntity<Sensors>,
    public CachedSource
{
  public:
    enum class Group
    {
        power,
        thermal,
        sensors,
    };

    enum class Unit
    {
        temperature,
        voltage,
        rotational,
        altitude,
        current,
        power,
        energyJoules,
        percent,
        unkown
    };

    enum class State
    {
        enabled,
        absent
    };

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Name)
    ENTITY_DECL_FIELD_DEF(double, Reading, 0)
    ENTITY_DECL_FIELD_ENUM(Unit, Unit, unkown)
    ENTITY_DECL_FIELD_ENUM(Group, Group, sensors)

    ENTITY_DECL_FIELD_DEF(double, MaxValue, 0)
    ENTITY_DECL_FIELD_DEF(double, MinValue, 0)
    ENTITY_DECL_FIELD_DEF(double, LowCritical, 0)
    ENTITY_DECL_FIELD_DEF(double, LowWarning, 0)
    ENTITY_DECL_FIELD_DEF(double, HighWarning, 0)
    ENTITY_DECL_FIELD_DEF(double, HighCritical, 0)

    ENTITY_DECL_FIELD_DEF(double, WarningAlarmHigh, 0)
    ENTITY_DECL_FIELD_DEF(double, WarningAlarmLow, 0)
    ENTITY_DECL_FIELD_DEF(double, CriticalAlarmHigh, 0)
    ENTITY_DECL_FIELD_DEF(double, CriticalAlarmLow, 0)

    ENTITY_DECL_FIELD_DEF(bool, Available, false)
    ENTITY_DECL_FIELD_DEF(bool, Functional, false)

    ENTITY_DECL_FIELD_ENUM(State, State, absent)
    ENTITY_DECL_FIELD_ENUM(StatusProvider::Status, Status, ok)
    ENTITY_DECL_FIELD(std::string, AssociatedInventoryId)

    class SensorQuery final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* sensorThresholdWarningInterface =
            "xyz.openbmc_project.Sensor.Threshold.Warning";
        static constexpr const char* sensorThresholdCriticalInterface =
            "xyz.openbmc_project.Sensor.Threshold.Critical";
        static constexpr const char* sensorValueInterface =
            "xyz.openbmc_project.Sensor.Value";
        static constexpr const char* sensorAvailabilityInterface =
            "xyz.openbmc_project.State.Decorator.Availability";
        static constexpr const char* sensorOperationalStatusInterface =
            "xyz.openbmc_project.State.Decorator.OperationalStatus";
        static constexpr const char* assocDefIface =
            "xyz.openbmc_project.Association.Definitions";

        static constexpr const char* propCriticalLow = "CriticalLow";
        static constexpr const char* propCriticalHigh = "CriticalHigh";
        static constexpr const char* propWarningLow = "WarningLow";
        static constexpr const char* propWarningHigh = "WarningHigh";

        static constexpr const char* propWarningAlarmHigh = "WarningAlarmHigh";
        static constexpr const char* propWarningAlarmLow = "WarningAlarmLow";
        static constexpr const char* propCriticalAlarmHigh =
            "CriticalAlarmHigh";
        static constexpr const char* propCriticalAlarmLow = "CriticalAlarmLow";

        static constexpr const char* propValue = "Value";
        static constexpr const char* defaultUnitLabel = "Unit";

      private:
        class FormatAssociatedInventory : public query::dbus::IFormatter
        {
          public:
            ~FormatAssociatedInventory() override = default;

            const DbusVariantType format(const PropertyName&,
                                         const DbusVariantType& value) override
            {
                constexpr const std::array inventorySlugList{
                    "inventory",
                    "cpu",
                };
                if (std::holds_alternative<DBusAssociationsType>(value))
                {
                    const auto associations =
                        std::get<DBusAssociationsType>(value);
                    for (const auto& association : associations)
                    {
                        const auto purpose = std::get<0>(association);
                        if (std::end(inventorySlugList) !=
                            std::find(std::begin(inventorySlugList),
                                      std::end(inventorySlugList), purpose))
                        {
                            const auto inventory = std::get<2>(association);
                            return getNameFromLastSegmentObjectPath(inventory,
                                                                    false);
                        }
                    }
                }
                return std::string(EntityMember::fieldValueNotAvailable);
            }
        };
        class GroupParser
        {
            const DBusInstancePtr instance;

            enum class Index
            {
                voltage = 0,
                power,
                current,
                fanTach,
                temperature,
                fanPwm,
                utilization,
                max
            };

            inline size_t index(Index index)
            {
                return static_cast<size_t>(index);
            }

          public:
            explicit GroupParser(const DBusInstancePtr& instance) :
                instance(instance)
            {}
            ~GroupParser() = default;

            Group operator()()
            {
                constexpr const size_t maxIndexes =
                    static_cast<size_t>(Index::max);
                static constexpr std::array<const char*, maxIndexes> paths = {
                    "/xyz/openbmc_project/sensors/voltage/",
                    "/xyz/openbmc_project/sensors/power/",
                    "/xyz/openbmc_project/sensors/current/",
                    "/xyz/openbmc_project/sensors/fan_tach/",
                    "/xyz/openbmc_project/sensors/temperature/",
                    "/xyz/openbmc_project/sensors/fan_pwm/",
                    "/xyz/openbmc_project/sensors/utilization/",
                };

                static const std::map<Group, std::vector<const char*>> groups =
                    {
                        {
                            Group::power,
                            {
                                paths[index(Index::voltage)],
                                paths[index(Index::power)],
                            },
                        },
                        {
                            Group::thermal,
                            {
                                paths[index(Index::fanTach)],
                                paths[index(Index::temperature)],
                                paths[index(Index::fanPwm)],
                            },
                        },
                    };

                const auto& objectPath = instance->getObjectPath();
                for (const auto& [group, identity] : groups)
                {
                    auto found =
                        std::any_of(identity.begin(), identity.end(),
                                    [objectPath](auto&& value) -> bool {
                                        return objectPath.starts_with(value);
                                    });
                    if (found)
                    {
                        return group;
                    }
                }
                return Group::sensors;
            }
        };

        class FormatUnit : public query::dbus::IFormatter
        {
            inline const std::string longDBusEnum(const std::string& enumStr)
            {
                return "xyz.openbmc_project.Sensor.Value.Unit." + enumStr;
            }

          public:
            ~FormatUnit() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, Unit> units{
                    {longDBusEnum("DegreesC"), Unit::temperature},
                    {longDBusEnum("Volts"), Unit::voltage},
                    {longDBusEnum("RPMS"), Unit::rotational},
                    {longDBusEnum("Meters"), Unit::altitude},
                    {longDBusEnum("Amperes"), Unit::current},
                    {longDBusEnum("Watts"), Unit::power},
                    {longDBusEnum("Joules"), Unit::energyJoules},
                    {longDBusEnum("Percent"), Unit::percent},
                };

                return formatValueFromDict(units, property, value,
                                           Unit::unkown);
            }
        };

      public:
        SensorQuery() : dbus::FindObjectDBusQuery()
        {}
        ~SensorQuery() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                sensorValueInterface,
                DBUS_QUERY_EP_FIELDS_ONLY(propValue, fieldReading),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMinValue),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldMaxValue),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldUnit,
                                              DBUS_QUERY_EP_CSTR(FormatUnit))),
            DBUS_QUERY_EP_IFACES(
                sensorThresholdWarningInterface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldWarningAlarmLow),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldWarningAlarmHigh),
                DBUS_QUERY_EP_FIELDS_ONLY(propWarningLow, fieldLowWarning),
                DBUS_QUERY_EP_FIELDS_ONLY(propWarningHigh, fieldHighWarning)),
            DBUS_QUERY_EP_IFACES(
                sensorThresholdCriticalInterface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldCriticalAlarmHigh),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldCriticalAlarmLow),
                DBUS_QUERY_EP_FIELDS_ONLY(propCriticalLow, fieldLowCritical),
                DBUS_QUERY_EP_FIELDS_ONLY(propCriticalHigh, fieldHighCritical)),
            DBUS_QUERY_EP_IFACES(sensorAvailabilityInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldAvailable)),
            DBUS_QUERY_EP_IFACES(sensorOperationalStatusInterface,
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldFunctional)),
            DBUS_QUERY_EP_IFACES(
                assocDefIface,
                DBUS_QUERY_EP_SET_FORMATTERS(
                    "Associations", fieldAssociatedInventoryId,
                    DBUS_QUERY_EP_CSTR(FormatAssociatedInventory))))

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/sensors",
            DBUS_QUERY_CRIT_IFACES(sensorValueInterface), noDepth, std::nullopt)

        void setSensorName(const DBusInstancePtr& instance) const
        {
            const auto& objectPath = instance->getObjectPath();
            setFieldId(instance,
                       getNameFromLastSegmentObjectPath(objectPath, false));
            setFieldName(instance,
                         getNameFromLastSegmentObjectPath(objectPath));
        }

        void setStatus(const DBusInstancePtr& instance) const
        {
            using AlarmStatusDict =
                std::map<StatusProvider::Status, std::vector<std::string>>;
            auto status = StatusProvider::Status::ok;
            DBusInstanceWeak instanceWeak(instance);
            static const AlarmStatusDict alarmFieldsDict{
                {
                    StatusProvider::Status::ok,
                    {fieldCriticalAlarmHigh, fieldCriticalAlarmLow},
                },
                {
                    StatusProvider::Status::warning,
                    {fieldWarningAlarmHigh, fieldWarningAlarmLow},
                },
            };
            const auto compareAlarmFn =
                [instanceWeak](const std::string& value) -> bool {
                auto instance = instanceWeak.lock();
                try
                {
                    if (instance)
                    {
                        return instance->getField(value)->getBoolValue();
                    }
                }
                catch (std::bad_variant_access& ex)
                {
                    // Several sensors might haven't critical/warning
                    // property, so feel is their status is OK.
                    log<level::DEBUG>(
                        "Fail to calculate sensor status",
                        entry("SENSOR=%s", instance->getField(fieldName)
                                               ->getStringValue()
                                               .c_str()),
                        entry("FIELD=%s", value.c_str()),
                        entry("ERROR=%s", ex.what()));
                }
                return false;
            };

            for (const auto& [alarm, fields] : alarmFieldsDict)
            {
                if (std::any_of(fields.begin(), fields.end(), compareAlarmFn))
                {
                    status = alarm;
                    break;
                }
            }
            setFieldStatus(instance, status);
        }

        void setGroup(const DBusInstancePtr& instance) const
        {
            GroupParser parser(instance);
            setFieldGroup(instance, parser());
        }

        void setState(const DBusInstancePtr& instance) const
        {
            setFieldState(instance, State::absent);
            try
            {
                if (instance &&
                    instance->getField(fieldAvailable)->getBoolValue())
                {
                    setFieldState(instance, State::enabled);
                }
            }
            catch (std::bad_variant_access& ex)
            {
                log<level::DEBUG>(
                    "Fail to calculate sensor state",
                    entry("SENSOR=%s", instance->getField(fieldName)
                                           ->getStringValue()
                                           .c_str()),
                    entry("FIELD=%s", fieldAvailable),
                    entry("ERROR=%s", ex.what()));
            }
        }

        void setAvailable(const DBusInstancePtr& instance) const
        {
            if (instance->getField(fieldAvailable)->isNull())
            {
                // Some sensors don't have the
                // `sensorAvailabilityInterface`, e.g. fan PWM.
                // Force set to true for them
                setFieldAvailable(instance, true);
            }
        }

        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            this->setSensorName(instance);
            this->setAvailable(instance);
            this->setStatus(instance);
            this->setState(instance);
            this->setGroup(instance);
        }
    };

  public:
    Sensors() : Collection(), query(std::make_shared<SensorQuery>())
    {
        this->createMember(fieldId);
        this->createMember(fieldName);
        this->createMember(fieldStatus);
        this->createMember(fieldState);
        this->createMember(fieldGroup);
    }
    ~Sensors() override = default;

    static const IRelation::RelationRulesList& inventoryRelation()
    {
        static const IRelation::RelationRulesList relations{
            {
                fieldId,
                Sensors::fieldAssociatedInventoryId,
                [](const IEntityMember::InstancePtr& instance,
                   const IEntityMember::IInstance::FieldType& value) -> bool {
                    try
                    {
                        if (std::holds_alternative<std::string>(value) &&
                            std::holds_alternative<std::string>(
                                instance->getValue()))
                        {
                            const auto fanName = instance->getStringValue();
                            const auto sensorInventory =
                                std::get<std::string>(value);
                            return fanName == sensorInventory;
                        }
                    }
                    catch (std::exception& ex)
                    {
                        log<level::ERR>("Fail to configure relation Inventory "
                                        "to Reading sensor",
                                        entry("ERROR=%s", ex.what()));
                    }

                    return false;
                },
            },
        };
        return relations;
    }

    static const ConditionPtr availableSensors()
    {
        return Condition::buildEqual(fieldAvailable, true);
    }

    static const ConditionPtr sensorsByType(Unit unit)
    {
        return Condition::buildEqual(fieldUnit, unit);
    }

    static const ConditionPtr genericSensors()
    {
        return Condition::buildEqual(fieldGroup, Group::sensors);
    }

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
