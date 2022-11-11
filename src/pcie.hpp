// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <common_fields.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/entity/proxy_query.hpp>
#include <core/helpers/utils.hpp>
#include <phosphor-logging/log.hpp>
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
using namespace app::query::proxy;
using namespace phosphor::logging;

class PCIeFunction;

class PCIeProvider final :
    public entity::EntitySupplementProvider,
    public ISingleton<PCIeProvider>,
    public CachedSource,
    public NamedEntity<PCIeProvider>
{
  public:
    enum class DeviceType
    {
        singleFunction,
        multiFunction,
        unkown
    };

    enum class DeviceClass
    {
        unclassifiedDevice,
        massStorageController,
        networkController,
        displayController,
        multimediaController,
        memoryController,
        bridge,
        communicationController,
        genericSystemPeripheral,
        inputDeviceController,
        dockingStation,
        processor,
        serialBusController,
        wirelessController,
        intelligentController,
        satelliteCommunicationsController,
        encryptionController,
        signalProcessingController,
        processingAccelerators,
        nonEssentialInstrumentation,
        coprocessor,
        unassignedClass,
        other
    };

    enum class FunctionType
    {
        physicalFunction,
        virtualFunction,
        unkownFunction
    };

    static constexpr const uint8_t otherDeviceClassId = 0xFFU;

    ENTITY_DECL_FIELD(std::string, Id)
    ENTITY_DECL_FIELD(std::string, Function)
    ENTITY_DECL_FIELD(std::string, Socket)
    ENTITY_DECL_FIELD(std::string, Bus)
    ENTITY_DECL_FIELD(std::string, Address)
    ENTITY_DECL_FIELD(std::string, Device)
    ENTITY_DECL_FIELD_ENUM(DeviceType, DeviceType, unkown)
    ENTITY_DECL_FIELD(std::string, Subsystem)
    /* Dynamic fields */
    ENTITY_DECL_FIELD(std::string, FunctionId)
    ENTITY_DECL_FIELD_DEF(uint8_t, FunctionIdNumerical, 0U)
    ENTITY_DECL_FIELD(std::string, ClassCode)
    ENTITY_DECL_FIELD_DEF(uint8_t, DeviceClassId, otherDeviceClassId)
    ENTITY_DECL_FIELD_ENUM(DeviceClass, DeviceClass, other)
    ENTITY_DECL_FIELD(std::string, DeviceSubClass)
    ENTITY_DECL_FIELD(std::string, DeviceId)
    ENTITY_DECL_FIELD(std::string, DeviceName)
    ENTITY_DECL_FIELD_ENUM(FunctionType, FunctionType, unkownFunction)
    ENTITY_DECL_FIELD(std::string, RevisionId)
    ENTITY_DECL_FIELD(std::string, SubsystemId)
    ENTITY_DECL_FIELD(std::string, SubsystemName)
    ENTITY_DECL_FIELD(std::string, SubsystemVendorId)
    ENTITY_DECL_FIELD(std::string, VendorId)
    ENTITY_DECL_FIELD(std::string, VendorName)

    static constexpr const char* fieldSBD = fieldId;
    static constexpr const char* pcieFunctionFieldPrefix = "Function";
    static constexpr size_t pcieFunctionsCount = 8;

    static const MembersList& getDynamicFields()
    {
        static const MembersList fields{
            fieldClassCode,     fieldDeviceId,          fieldDeviceClassId,
            fieldDeviceClass,   fieldDeviceSubClass,    fieldDeviceName,
            fieldFunctionType,  fieldRevisionId,        fieldSubsystemId,
            fieldSubsystemName, fieldSubsystemVendorId, fieldVendorId,
            fieldVendorName,
        };
        return fields;
    }

    /**
     * @brief Get the complex PCIe Function member name
     *
     * Complex member pattern: `Function{functionIndex}{MemberName}`
     *
     * @param index function index
     * @param dynMemberName member name
     * @return const std::string complex member name
     */
    static const std::string
        getComplexFunctionMemberName(size_t index,
                                     const MemberName& dynMemberName)
    {
        return pcieFunctionFieldPrefix + std::to_string((index)) +
               dynMemberName;
    }

  private:
    class PCIeQuery final :
        public dbus::FindObjectDBusQuery,
        public ISingleton<PCIeQuery>
    {
        static constexpr const char* pcieServiceName =
            "xyz.openbmc_project.PCIe";
        static constexpr const char* pcieInterfaceName =
            "xyz.openbmc_project.PCIe.Device";

      public:
        PCIeQuery() : dbus::FindObjectDBusQuery()
        {}
        ~PCIeQuery() override = default;

        const dbus::DBusPropertyEndpointMap&
            getSearchPropertiesMap() const override
        {
            static auto pcieInterfaceProperties = getPCIeMemberNameDict();
            static const dbus::DBusPropertyEndpointMap dictionary{
                DBUS_QUERY_EP_IFACES(pcieInterfaceName,
                                     pcieInterfaceProperties),
            };

            return dictionary;
        }
        class FormatDeviceType : public query::dbus::IFormatter
        {
          public:
            ~FormatDeviceType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                static const std::map<std::string, DeviceType> types{
                    {"SingleFunction", DeviceType::singleFunction},
                    {"MultiFunction", DeviceType::multiFunction},
                };

                return formatValueFromDict(types, property, value,
                                           DeviceType::unkown);
            }
        };
        class FormatDeviceClass : public query::dbus::IFormatter
        {
          public:
            ~FormatDeviceClass() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<uint8_t, DeviceClass> types{
                    {0x00, DeviceClass::unclassifiedDevice},
                    {0x01, DeviceClass::massStorageController},
                    {0x02, DeviceClass::networkController},
                    {0x03, DeviceClass::displayController},
                    {0x04, DeviceClass::multimediaController},
                    {0x05, DeviceClass::memoryController},
                    {0x06, DeviceClass::bridge},
                    {0x07, DeviceClass::communicationController},
                    {0x08, DeviceClass::genericSystemPeripheral},
                    {0x09, DeviceClass::inputDeviceController},
                    {0x0a, DeviceClass::dockingStation},
                    {0x0b, DeviceClass::processor},
                    {0x0c, DeviceClass::serialBusController},
                    {0x0d, DeviceClass::wirelessController},
                    {0x0e, DeviceClass::intelligentController},
                    {0x0f, DeviceClass::satelliteCommunicationsController},
                    {0x10, DeviceClass::encryptionController},
                    {0x11, DeviceClass::signalProcessingController},
                    {0x12, DeviceClass::processingAccelerators},
                    {0x13, DeviceClass::nonEssentialInstrumentation},
                    {0x40, DeviceClass::coprocessor},
                    {0xff, DeviceClass::unassignedClass}};
                // clang-format: on

                return formatValueFromDict(types, property, value,
                                           DeviceClass::other);
            }
        };

        class FormatFunctionType : public query::dbus::IFormatter
        {
          public:
            ~FormatFunctionType() override = default;

            const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) override
            {
                // clang-format: off
                static const std::map<std::string, FunctionType> types{
                    {"Physical", FunctionType::physicalFunction},
                    {"Virtual", FunctionType::virtualFunction},
                };
                // clang-format: on

                return formatValueFromDict(types, property, value,
                                           FunctionType::unkownFunction);
            }
        };

        static const DBusPropertySetters& getPCIeMemberNameDict()
        {
            static DBusPropertySetters pcieFunctionsFields{
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldDevice),
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::manufacturer),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSubsystem),
                DBUS_QUERY_EP_SET_FORMATTERS2(
                    fieldDeviceType, DBUS_QUERY_EP_CSTR(FormatDeviceType)),
            };
            static const std::map<std::string, DBusPropertyFormatters> fmtList{
                {fieldFunctionType, DBUS_QUERY_EP_CSTR(FormatFunctionType)},
            };
            for (size_t index = 0; index < pcieFunctionsCount; ++index)
            {
                for (auto dynMemberName : getDynamicFields())
                {
                    DBusPropertyCasters casters;
                    auto fieldName =
                        getComplexFunctionMemberName(index, dynMemberName);
                    auto it = fmtList.find(dynMemberName);
                    if (it != fmtList.end())
                    {
                        casters.first = std::move(it->second);
                    }
                    pcieFunctionsFields.emplace_back(
                        DBusPropertyReflection(fieldName, fieldName), casters);
                }
            }

            return pcieFunctionsFields;
        }

        void supplementByStaticFields(
            const DBusInstancePtr& instance) const override
        {
            this->setSBD(instance);
            this->setStatus(instance);
            this->setDeviceSubClass(instance);
            this->setDeviceClassId(instance);
            this->setDeviceClass(instance);
        }

      protected:
        /* clang-format off */
        DBUS_QUERY_DECLARE_CRITERIA(
            "/xyz/openbmc_project/PCIe",
            DBUS_QUERY_CRIT_IFACES(pcieInterfaceName),
            noDepth,
            pcieServiceName
        )
        /* clang-format on */

        /**
         * @brief Set Socket, Bus and Address fields
         *
         * @param instance tartget instance
         */
        void setSBD(const DBusInstancePtr& instance) const
        {
            const auto& objectPath = instance->getObjectPath();
            auto SBD = app::helpers::utils::getNameFromLastSegmentObjectPath(
                objectPath);

            instance->supplementOrUpdate(fieldSBD, SBD);

            constexpr const std::array fieldMatchIndexed{
                fieldSocket,
                fieldBus,
                fieldAddress,
            };

            std::regex SBDRegex("^S(\\d+)B(\\d+)D(\\d+)$");
            std::smatch SBDMatch;
            if (std::regex_search(SBD, SBDMatch, SBDRegex))
            {
                log<level::DEBUG>("PCIe device have several SBD matches",
                                  entry("PCI_DBUS_PATH=%s", objectPath.c_str()),
                                  entry("SBD=%s", SBD.c_str()),
                                  entry("COUNT=%ld", SBDMatch.size()));
                for (size_t index = 1; index <= SBDMatch.size(); ++index)
                {
                    // SBD match groups indexed from 1, but
                    // fieldMatchIndexed from 0. Hence, need to reduct the
                    // index to verefy
                    auto memberIndex = index - 1;
                    if (memberIndex >= fieldMatchIndexed.size())
                    {
                        return;
                    }
                    instance->supplementOrUpdate(fieldMatchIndexed[memberIndex],
                                                 std::string(SBDMatch[index]));
                }
            }
        }

        void setStatus(const DBusInstancePtr& instance) const
        {
            // Currently, doesn't have a way to determine the PCIe status.
            // Set 'OK' as default.
            StatusProvider::setFieldStatus(instance,
                                           StatusProvider::Status::ok);
        }

        void setDeviceClassId(const DBusInstancePtr& instance) const
        {
            StrHexToNumberFormatter formatter;
            for (size_t index = 0; index < pcieFunctionsCount; ++index)
            {
                const auto classCodeFieldName =
                    getComplexFunctionMemberName(index, fieldClassCode);
                const auto deviceClassIdFieldName =
                    getComplexFunctionMemberName(index, fieldDeviceClassId);
                const auto classCodeField =
                    instance->getField(classCodeFieldName);
                auto cc = formatter.format(classCodeFieldName,
                                           classCodeField->getStringValue());

                uint8_t dci = otherDeviceClassId;
                if (std::holds_alternative<uint64_t>(cc))
                {
                    dci = static_cast<uint8_t>((std::get<uint64_t>(cc) >> 16) &
                                               0xFFU);
                }
                instance->supplementOrUpdate(deviceClassIdFieldName, dci);
            }
        }

        void setDeviceClass(const DBusInstancePtr& instance) const
        {
            FormatDeviceClass formatter;
            for (size_t index = 0; index < pcieFunctionsCount; ++index)
            {
                const auto deviceClassFieldName =
                    getComplexFunctionMemberName(index, fieldDeviceClass);
                const auto deviceClassIdFieldName =
                    getComplexFunctionMemberName(index, fieldDeviceClassId);
                const auto dci = std::get<uint8_t>(
                    instance->getField(deviceClassIdFieldName)->getValue());
                const auto dbusValue =
                    formatter.format(deviceClassFieldName, dci);
                int intEnumValue = static_cast<int>(DeviceClass::other);
                if (std::holds_alternative<int>(dbusValue))
                {
                    intEnumValue = std::get<int>(dbusValue);
                }
                instance->supplementOrUpdate(deviceClassFieldName,
                                             intEnumValue);
            }
        }

        void setDeviceSubClass(const DBusInstancePtr& instance) const
        {
            for (size_t index = 0; index < pcieFunctionsCount; ++index)
            {
                const auto deviceClassFieldName =
                    getComplexFunctionMemberName(index, fieldDeviceClass);
                const auto deviceSubClassFieldName =
                    getComplexFunctionMemberName(index, fieldDeviceSubClass);
                const auto dsc =
                    instance->getField(deviceClassFieldName)->getStringValue();
                instance->supplementOrUpdate(deviceSubClassFieldName, dsc);
            }
        }
    };

  public:
    PCIeProvider() : EntitySupplementProvider()
    {
        createMember(fieldSBD);
        createMember(fieldSocket);
        createMember(fieldBus);
        createMember(fieldAddress);
        createMember(StatusProvider::fieldStatus);
    }
    ~PCIeProvider() override = default;

  protected:
    ENTITY_DECL_QUERY(PCIeQuery::getSingleton())
};

class PCIeDevice final :
    public Collection,
    public LazySource,
    public NamedEntity<PCIeDevice>
{
  public:
    explicit PCIeDevice() :
        Collection(),
        query(std::make_shared<ProxyQuery>(PCIeProvider::getSingleton()))
    {}
    ~PCIeDevice() override = default;

    static const IEntity::IRelation::RelationRulesList& realtionToFunctions()
    {
        using namespace std::placeholders;
        static const IEntity::IRelation::RelationRulesList relations{
            {
                PCIeProvider::fieldSBD,
                PCIeProvider::fieldSBD,
                [](const IEntity::IEntityMember::InstancePtr& instance,
                   const IEntity::IEntityMember::IInstance::FieldType& value)
                    -> bool {
                    const auto& destSBD = std::get<std::string>(value);
                    log<level::DEBUG>(
                        "Linking 'PCIDevice' and 'PCIFunction' entities",
                        entry("FIELD=%s", PCIeProvider::fieldSBD),
                        entry("PCIEDEV=%s", instance->getStringValue().c_str()),
                        entry("PCIEFUN=%s", destSBD.c_str()));
                    return instance->getStringValue() == destSBD;
                },
            },
        };
        return relations;
    }

  protected:
    ENTITY_DECL_RELATIONS(ENTITY_DEF_RELATION(PCIeFunction,
                                              realtionToFunctions()))
    ENTITY_DECL_QUERY(query)
  private:
    ProxyQueryPtr query;
};

class PCIeFunction final :
    public Collection,
    public LazySource,
    public NamedEntity<PCIeFunction>
{
    class FunctionProxyQuery :
        public IQuery,
        public ISingleton<FunctionProxyQuery>
    {
        const EntitySupplementProviderPtr provider;

      public:
        FunctionProxyQuery(const EntitySupplementProviderPtr& provider) :
            provider(provider)
        {}
        ~FunctionProxyQuery() override = default;
        /**
         * @brief Run the configured quiry.
         *
         * @return std::vector<TInstance> The list of retrieved data which
         *                                incapsulated at TInstance objects
         */
        const entity::IEntity::InstanceCollection process() override
        {
            provider->populate();
            std::vector<InstancePtr> instancesToInit;
            constexpr const std::array directlyCopyEntityMembers{
                PCIeProvider::fieldSBD,
            };
            const auto& providerInstances = provider->getInstances();
            for (auto providerInstance : providerInstances)
            {
                for (uint8_t funcIndex = 0;
                     funcIndex < PCIeProvider::pcieFunctionsCount; ++funcIndex)
                {
                    const auto funcIndexStr = std::to_string(funcIndex);
                    std::string identifyField =
                        PCIeProvider::pcieFunctionFieldPrefix + funcIndexStr +
                        PCIeProvider::fieldDeviceId;
                    auto identifyMembery =
                        providerInstance->getField(identifyField);
                    if (identifyMembery->getStringValue().empty())
                    {
                        continue;
                    }

                    auto targetInstance =
                        std::make_shared<Entity::StaticInstance>(
                            identifyField + identifyMembery->getStringValue() +
                            providerInstance->getField(PCIeProvider::fieldSBD)
                                ->getStringValue());

                    // Set the PCI Function ID
                    PCIeProvider::setFieldFunctionId(targetInstance,
                                                     funcIndexStr);
                    PCIeProvider::setFieldFunctionIdNumerical(targetInstance,
                                                              funcIndex);

                    // Directly copy member instance from provider
                    for (auto directlyCopyMember : directlyCopyEntityMembers)
                    {
                        targetInstance->supplementOrUpdate(
                            directlyCopyMember,
                            providerInstance->getField(directlyCopyMember)
                                ->getValue());
                    }

                    // copy complex members
                    for (auto dynamicField : PCIeProvider::getDynamicFields())
                    {
                        const auto pcieFuncMemberName =
                            PCIeProvider::getComplexFunctionMemberName(
                                funcIndex, dynamicField);
                        auto field =
                            providerInstance->getField(pcieFuncMemberName);

                        targetInstance->supplementOrUpdate(dynamicField,
                                                           field->getValue());
                    }

                    instancesToInit.emplace_back(targetInstance);
                }
            }
            return instancesToInit;
        }

        /**
         * @brief Get the collection of query fields
         *
         * @return const QueryFields - The collection of fields
         */
        const QueryFields getFields() const override
        {
            QueryFields fields{
                PCIeProvider::fieldClassCode,
                PCIeProvider::fieldDeviceClassId,
                PCIeProvider::fieldDeviceClass,
                PCIeProvider::fieldDeviceSubClass,
                PCIeProvider::fieldDeviceId,
                PCIeProvider::fieldDeviceName,
                PCIeProvider::fieldFunctionType,
                PCIeProvider::fieldRevisionId,
                PCIeProvider::fieldSubsystemId,
                PCIeProvider::fieldFunctionId,
                PCIeProvider::fieldFunctionIdNumerical,
                PCIeProvider::fieldSubsystemName,
                PCIeProvider::fieldSubsystemVendorId,
                PCIeProvider::fieldVendorId,
                PCIeProvider::fieldVendorName,
            };

            return std::forward<QueryFields>(fields);
        }
    };

  public:
    explicit PCIeFunction() :
        Collection(), query(std::make_shared<FunctionProxyQuery>(
                          PCIeProvider::getSingleton()))
    {
        createMember(PCIeProvider::fieldSBD);
    }
    ~PCIeFunction() override = default;

  protected:
    ENTITY_DECL_QUERY(query)
    ENTITY_DECL_RELATIONS(
        ENTITY_DEF_RELATION(PCIeDevice, PCIeDevice::realtionToFunctions()))
  private:
    std::shared_ptr<FunctionProxyQuery> query;
};

} // namespace entity
} // namespace obmc
} // namespace app
