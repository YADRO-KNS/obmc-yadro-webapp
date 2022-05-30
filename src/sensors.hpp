// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <phosphor-logging/log.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/helpers/utils.hpp>

#include <formatters.hpp>
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

class Sensors :
    public entity::Collection,
    public NamedEntity<Sensors>,
    public CachedSource
{
  public:
    static constexpr const char* fieldName = "Name";
    static constexpr const char* fieldValue = "Reading";
    static constexpr const char* fieldUnit = "Unit";
    static constexpr const char* fieldLowCritical = "LowCritical";
    static constexpr const char* fieldLowWarning = "LowWarning";
    static constexpr const char* fieldHighWarning = "HighWarning";
    static constexpr const char* fieldHighCritical = "HighCritical";

    static constexpr const char* fieldWarningAlarmHigh = "WarningAlarmHigh";
    static constexpr const char* fieldWarningAlarmLow = "WarningAlarmLow";
    static constexpr const char* fieldCriticalAlarmHigh = "CriticalAlarmHigh";
    static constexpr const char* fieldCriticalAlarmLow = "CriticalAlarmLow";

    static constexpr const char* fieldAvailable = "Available";
    static constexpr const char* fieldFunctional = "Functional";

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

        static constexpr const char* propCriticalLow = "CriticalLow";
        static constexpr const char* propCriticalHigh = "CriticalHigh";
        static constexpr const char* propWarningLow = "WarningLow";
        static constexpr const char* propWarningHigh = "WarningHigh";

        static constexpr const char* propWarningAlarmHigh = "WarningAlarmHigh";
        static constexpr const char* propWarningAlarmLow = "WarningAlarmLow";
        static constexpr const char* propCriticalAlarmHigh = "CriticalAlarmHigh";
        static constexpr const char* propCriticalAlarmLow = "CriticalAlarmLow";

        static constexpr const char* propValue = "Value";
        static constexpr const char* defaultUnitLabel = "Unit";

      private:
        class FormatUnit : public query::dbus::IFormatter
        {
          public:
            ~FormatUnit() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const StringFormatterDict units{
                    {"xyz.openbmc_project.Sensor.Value.Unit.DegreesC", "°C"},
                    {"xyz.openbmc_project.Sensor.Value.Unit.Volts", "Volts"},
                    {"xyz.openbmc_project.Sensor.Value.Unit.RPMS", "RPM"},
                    {"xyz.openbmc_project.Sensor.Value.Unit.Meters", "Meters"},
                    {"xyz.openbmc_project.Sensor.Value.Unit.Amperes",
                     "Amperes"},
                    {"xyz.openbmc_project.Sensor.Value.Unit.Watts", "Watts"},
                    {"xyz.openbmc_project.Sensor.Value.Unit.Joules", "Joules"},
                    {"xyz.openbmc_project.Sensor.Value.Unit.Percent", "%"},
                };

                return formatStringValueFromDict(units, property, value);
            }
        };

      public:
        SensorQuery() : dbus::FindObjectDBusQuery()
        {}
        ~SensorQuery() override = default;

        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                sensorValueInterface,
                DBUS_QUERY_EP_FIELDS_ONLY(propValue, fieldValue),
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
                                 DBUS_QUERY_EP_FIELDS_ONLY2(fieldFunctional)))
      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/sensors",
            DBUS_QUERY_CRIT_IFACES(sensorValueInterface), noDepth, std::nullopt)

        void setSensorName(const DBusInstancePtr& instance) const
        {
            auto& objectPath = instance->getObjectPath();
            instance->supplementOrUpdate(
                fieldName,
                app::helpers::utils::getNameFromLastSegmentObjectPath(
                    objectPath));
        }

        void setStatus(const DBusInstancePtr& instance) const
        {
            std::string status = StatusProvider::OK;
            DBusInstanceWeak instanceWeak(instance);
            static const std::map<std::string, std::vector<std::string>>
                alarmFieldsDict{
                    {
                        StatusProvider::critical,
                        {fieldCriticalAlarmHigh, fieldCriticalAlarmLow},
                    },
                    {
                        StatusProvider::warning,
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
                    // property, so feel is their state is OK.
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
            instance->supplementOrUpdate(StatusProvider::fieldStatus, status);
        }

        void supplementByStaticFields(const DBusInstancePtr& instance) const override
        {
            this->setSensorName(instance);
            this->setStatus(instance);
        }
    };

  public:
    Sensors() : Collection(), query(std::make_shared<SensorQuery>())
    {
        this->createMember(fieldName);
        this->createMember(StatusProvider::fieldStatus);
    }
    ~Sensors() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
