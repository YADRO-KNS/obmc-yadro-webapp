// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <phosphor-logging/log.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/entity/proxy_query.hpp>
#include <core/helpers/utils.hpp>

#include <status_provider.hpp>
#include <common_fields.hpp>

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
    static constexpr const char* fieldSBD = "Id";
    static constexpr size_t pcieFunctionsCount = 8;
    static constexpr const char* pcieFunctionFieldPrefix = "Function";
    static constexpr const char* fieldSocket = "Socket";
    static constexpr const char* fieldBus = "Bus";
    static constexpr const char* fieldAddress = "Address";
    static constexpr const char* fieldDevice = "Device";
    static constexpr const char* fieldDeviceType = "DeviceType";
    static constexpr const char* fieldSubsystem = "Subsystem";
    /* Dynamic fields */
    static constexpr const char* fieldFunctionId = "FunctionId";
    static constexpr const char* fieldClassCode = "ClassCode";
    static constexpr const char* fieldDeviceClass = "DeviceClass";
    static constexpr const char* fieldDeviceId = "DeviceId";
    static constexpr const char* fieldDeviceName = "DeviceName";
    static constexpr const char* fieldFunctionType = "FunctionType";
    static constexpr const char* fieldRevisionId = "RevisionId";
    static constexpr const char* fieldSubsystemId = "SubsystemId";
    static constexpr const char* fieldSubsystemName = "SubsystemName";
    static constexpr const char* fieldSubsystemVendorId = "SubsystemVendorId";
    static constexpr const char* fieldVendorId = "VendorId";
    static constexpr const char* fieldVendorName = "VendorName";

    static const MembersList& getDynamicFields()
    {
        static const MembersList fields{
            fieldClassCode,   fieldDeviceClass,   fieldDeviceId,
            fieldDeviceName,  fieldFunctionType,  fieldRevisionId,
            fieldSubsystemId, fieldSubsystemName, fieldSubsystemVendorId,
            fieldVendorId,    fieldVendorName,
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

        static const DBusPropertySetters& getPCIeMemberNameDict()
        {
            static DBusPropertySetters pcieFunctionsFields{
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldDevice),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldDeviceType),
                DBUS_QUERY_EP_FIELDS_ONLY2(general::assets::manufacturer),
                DBUS_QUERY_EP_FIELDS_ONLY2(fieldSubsystem),
            };

            for (size_t index = 0; index < pcieFunctionsCount; ++index)
            {
                for (auto dynMemberName : getDynamicFields())
                {
                    auto fieldName =
                        getComplexFunctionMemberName(index, dynMemberName);
                    pcieFunctionsFields.insert_or_assign(
                        DBusPropertyReflection(fieldName, fieldName),
                        DBusPropertyCasters());
                }
            }

            return pcieFunctionsFields;
        }

        void supplementByStaticFields(const DBusInstancePtr& instance) const override
        {
            this->setSBD(instance);
            this->setStatus(instance);
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
            auto& objectPath = instance->getObjectPath();
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
            instance->supplementOrUpdate(StatusProvider::fieldStatus,
                                         StatusProvider::OK);
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
    {
        PCIeProvider::getSingleton()->initialize();
        createMember(PCIeProvider::fieldSBD);
        createMember(PCIeProvider::fieldSocket);
        createMember(PCIeProvider::fieldBus);
        createMember(PCIeProvider::fieldAddress);
        createMember(StatusProvider::fieldStatus);
    }
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
                        entry("PCIDevice=%s", instance->getStringValue().c_str()),
                        entry("PCIFunction=%s", destSBD.c_str()));
                    return instance->getStringValue() == destSBD;
                },
            },
        };
        return relations;
    }

  protected:
    ENTITY_DECL_RELATION(PCIeFunction, realtionToFunctions())
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
                    std::string identifyField =
                        PCIeProvider::pcieFunctionFieldPrefix +
                        std::to_string(funcIndex) + PCIeProvider::fieldDeviceId;
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
                    targetInstance->supplementOrUpdate(
                        PCIeProvider::fieldFunctionId, funcIndex);

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
                        try
                        {
                            if (field->getStringValue().empty())
                            {
                                // Don't set the value of an empty string to
                                // pass through the 'N/A' value.
                                continue;
                            }
                        }
                        catch (std::bad_variant_access&)
                        {
                            // pass through if the field is not valid
                        }

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
                PCIeProvider::fieldDeviceClass,
                PCIeProvider::fieldDeviceId,
                PCIeProvider::fieldDeviceName,
                PCIeProvider::fieldFunctionType,
                PCIeProvider::fieldRevisionId,
                PCIeProvider::fieldSubsystemId,
                PCIeProvider::fieldFunctionId,
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
    ENTITY_DECL_RELATION(PCIeDevice, PCIeDevice::realtionToFunctions())
  private:
    std::shared_ptr<FunctionProxyQuery> query;
};

} // namespace entity
} // namespace obmc
} // namespace app
