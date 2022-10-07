// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <formatters.hpp>
#include <status_provider.hpp>
#include <regex>

namespace app
{
namespace obmc
{
namespace entity
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;

class VirtualMedia final :
    public entity::Collection,
    /* Intel virtual-media implementation such ugly either it can't be used to
       watch of objects changes, because their don't sends signals. Too bad...*/
    public ShortTimeCachedSource<1>,
    public NamedEntity<VirtualMedia>
{
  public:
    enum class InterfaceType
    {
        usb,
        hdd,
        cdrom
    };

    enum class DeviceType
    {
        proxy,
        legacy,
        direct,
        unknown
    };

    enum class TransferProtocol
    {
        cifs,
        ftp,
        sftp,
        http,
        https,
        nfs,
        scp,
        tftp,
        oem
    };

    enum class OemTransferProtocol
    {
        nbd,
        absent
    };

    enum class ConnectionType
    {
        notConnected,
        uri,
        applet
    };

    enum class State
    {
        enabled,
        disabled,
        standbyOffline
    };

    ENTITY_DECL_FIELD_ENUM(InterfaceType, InterfaceType, usb)
    ENTITY_DECL_FIELD_ENUM(DeviceType, DeviceType, unknown)
    ENTITY_DECL_FIELD_ENUM(TransferProtocol, TransferProtocol, oem)
    ENTITY_DECL_FIELD_ENUM(OemTransferProtocol, OemTransferProtocol, absent)
    ENTITY_DECL_FIELD_ENUM(ConnectionType, ConnectionType, notConnected)
    ENTITY_DECL_FIELD_ENUM(State, State, standbyOffline)

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Device)
    ENTITY_DECL_FIELD(std::string, EndpointId)
    ENTITY_DECL_FIELD(std::string, ExportName)
    ENTITY_DECL_FIELD(std::string, ImageURL)
    ENTITY_DECL_FIELD(std::string, Socket)
    ENTITY_DECL_FIELD(std::string, WebSocketAddress)
    ENTITY_DECL_FIELD_DEF(int32_t, RemainingInactivityTimeout, 0)
    ENTITY_DECL_FIELD_DEF(bool, Active, false)
    ENTITY_DECL_FIELD_DEF(bool, WriteProtected, false)
  private:
    class Query final : public dbus::FindObjectDBusQuery
    {
        static constexpr const char* service =
            "xyz.openbmc_project.VirtualMedia";
        static constexpr const char* vmMountPointIface =
            "xyz.openbmc_project.VirtualMedia.MountPoint";
        static constexpr const char* vmProcessIface =
            "xyz.openbmc_project.VirtualMedia.Process";
        class FormatInterfaceType : public query::dbus::IFormatter
        {
          public:
            ~FormatInterfaceType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<uint8_t, InterfaceType> types{
                    {0, InterfaceType::usb},
                    {1, InterfaceType::hdd},
                    {2, InterfaceType::cdrom},
                };
                // clang-format: on
                return formatValueFromDict(types, property, value,
                                           InterfaceType::usb);
            }
        };
        class FormatStatus : public query::dbus::IFormatter
        {
          public:
            ~FormatStatus() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<int32_t, StatusProvider::Status>
                    statusList{
                        {0, StatusProvider::Status::ok},
                        {-1, StatusProvider::Status::warning},
                    };
                // clang-format: on
                return formatValueFromDict(statusList, property, value,
                                           StatusProvider::Status::critical);
            }
        };
        class FormatState : public query::dbus::IFormatter
        {
          public:
            ~FormatState() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<bool, State> states{
                    {false, State::standbyOffline},
                    {true, State::enabled},
                };
                // clang-format: on
                return formatValueFromDict(states, property, value,
                                           State::disabled);
            }
        };

      public:
        Query() : dbus::FindObjectDBusQuery()
        {}
        ~Query() override = default;

        /* clang-format off */
        DBUS_QUERY_DECL_EP(
            DBUS_QUERY_EP_IFACES(
                vmMountPointIface,
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldDevice),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldEndpointId),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldExportName),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldImageURL),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSocket),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldRemainingInactivityTimeout),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldWriteProtected),
                DBUS_QUERY_EP_SET_FORMATTERS2(fieldInterfaceType,
                                             DBUS_QUERY_EP_CSTR(FormatInterfaceType)
                )
            ),
            DBUS_QUERY_EP_IFACES(
                vmProcessIface,
                DBUS_QUERY_EP_SET_FORMATTERS("ExitCode", StatusProvider::fieldStatus,
                                             DBUS_QUERY_EP_CSTR(FormatStatus)
                ),
                DBUS_QUERY_EP_SET_FORMATTERS("Active", fieldState,
                                             DBUS_QUERY_EP_CSTR(FormatState)
                )
            )
        )

      protected:
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/VirtualMedia/",
            DBUS_QUERY_CRIT_IFACES(vmMountPointIface),
            noDepth,
            service
        )
        /* clang-format on */
        const DefaultFieldsValueDict& getDefaultFieldsValue() const override
        {
            static const DefaultFieldsValueDict defaults{
                {StatusProvider::fieldStatus,
                 [](const auto& instance)
                     -> IEntityMember::IInstance::FieldType {
                     if (getFieldActive(instance))
                     {
                         return static_cast<int>(StatusProvider::Status::ok);
                     }
                     return static_cast<int>(
                         StatusProvider::getFieldStatus(instance));
                 }}};
            return defaults;
        }

        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            this->setId(instance);
            this->setDeviceType(instance);
            this->setConnectionType(instance);
            this->setTransferProtocol(instance);
            this->setOemTransferProtocol(instance);
            this->setActive(instance);
            this->setImageName(instance);
            this->setWSAddress(instance);
        }

        inline void setId(const DBusInstancePtr& instance) const
        {
            const auto& objectPath = instance->getObjectPath();
            const auto id = getNameFromLastSegmentObjectPath(objectPath);
            setFieldId(instance, id);
        }
        inline void setImageName(const DBusInstancePtr& instance) const
        {
            if (getFieldDeviceType(instance) == DeviceType::legacy)
            {
                const auto imageUrl = getFieldImageURL(instance);
                if (!instance->getField(fieldImageURL)->isNull() || imageUrl.empty())
                {
                    return;
                }
                const auto imageName =
                    getNameFromLastSegmentObjectPath(imageUrl, false);
                setFieldExportName(instance, imageName);
            }
            else if (getFieldDeviceType(instance) == DeviceType::proxy)
            {
                const auto imageUrl = getFieldImageURL(instance);
                const auto cleanImageName = std::regex_replace(imageUrl, std::regex("\""), "");
                setFieldExportName(instance, cleanImageName);
                instance->getField(fieldImageURL)->setValue(nullptr);
            }
        }
        inline void setWSAddress(const DBusInstancePtr& instance) const
        {
            static const std::map<std::string, std::string> wsepDict{
                {"nbd0", "/nbd/0"},
            };
            const auto device = getFieldDevice(instance);
            auto wsepIt = wsepDict.find(device);
            if (wsepIt != wsepDict.end())
            {
                setFieldWebSocketAddress(instance, wsepIt->second);
            }
        }
        inline void setDeviceType(const DBusInstancePtr& instance) const
        {
            static const std::map<std::string, DeviceType> deviceTypes{
                {"/Proxy/", DeviceType::proxy},
                {"/Legacy/", DeviceType::legacy},
                {"/Direct/", DeviceType::direct},
            };
            auto type = DeviceType::unknown;
            const auto objectPath = instance->getObjectPath();
            for (const auto& [segment, deviceType] : deviceTypes)
            {
                if (objectPath.find(segment) != std::string::npos)
                {
                    type = deviceType;
                    break;
                }
            }
            setFieldDeviceType(instance, type);
        }

        inline void setTransferProtocol(const DBusInstancePtr& instance) const
        {
            static const std::map<std::string, TransferProtocol> protocolDict{
                {"http://", TransferProtocol::http},
                {"https://", TransferProtocol::https},
                {"ftp://", TransferProtocol::ftp},
                {"sftp://", TransferProtocol::sftp},
                {"scp://", TransferProtocol::scp},
                {"cifs://", TransferProtocol::cifs},
                {"smb://", TransferProtocol::cifs},
                {"tftp://", TransferProtocol::tftp},
                {"nfs://", TransferProtocol::nfs},
            };
            const auto imageURL = getFieldImageURL(instance);
            if (!imageURL.empty())
            {
                for (const auto& [uriProto, proto] : protocolDict)
                {
                    if (imageURL.starts_with(uriProto))
                    {
                        setFieldTransferProtocol(instance, proto);
                        return;
                    }
                }
            }
            setFieldTransferProtocol(instance, TransferProtocol::oem);
        }
        inline void
            setOemTransferProtocol(const DBusInstancePtr& instance) const
        {
            if (getFieldTransferProtocol(instance) != TransferProtocol::oem ||
                getFieldConnectionType(instance) ==
                    ConnectionType::notConnected)
            {
                setFieldOemTransferProtocol(instance,
                                            OemTransferProtocol::absent);
                return;
            }
            setFieldOemTransferProtocol(instance, OemTransferProtocol::nbd);
        }
        inline void setConnectionType(const DBusInstancePtr& instance) const
        {
            if (getFieldState(instance) != State::enabled)
            {
                setFieldConnectionType(instance, ConnectionType::notConnected);
                return;
            }
            if (getFieldDeviceType(instance) == DeviceType::legacy ||
                getFieldDeviceType(instance) == DeviceType::direct)
            {
                setFieldConnectionType(instance, ConnectionType::uri);
                return;
            }
            setFieldConnectionType(instance, ConnectionType::applet);
        }
        inline void setActive(const DBusInstancePtr& instance) const
        {
            setFieldActive(instance, getFieldState(instance) == State::enabled);
        }
    };

  public:
    static const ConditionPtr vmByIfaceType(InterfaceType type)
    {
        return Condition::buildEqual(fieldInterfaceType, type);
    }
    VirtualMedia() : Collection(), query(std::make_shared<Query>())
    {
        this->createMember(fieldId);
        this->createMember(fieldDeviceType);
        this->createMember(fieldTransferProtocol);
        this->createMember(fieldOemTransferProtocol);
        this->createMember(fieldConnectionType);
        this->createMember(fieldActive);
        this->createMember(fieldWebSocketAddress);
    }
    ~VirtualMedia() override = default;

  protected:
    ENTITY_DECL_QUERY(query)

  private:
    DBusQueryPtr query;
};

} // namespace entity
} // namespace obmc
} // namespace app
