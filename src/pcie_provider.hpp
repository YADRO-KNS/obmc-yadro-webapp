// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <definitions.hpp>
#include <logger/logger.hpp>
#include <status_provider.hpp>

#include <regex>

namespace app
{
namespace query
{
namespace obmc
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;

using namespace app::entity::obmc::definitions;

class PCIeProvider final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* pcieServiceName = "xyz.openbmc_project.PCIe";
    static constexpr const char* pcieInterfaceName =
        "xyz.openbmc_project.PCIe.Device";

    static constexpr const char* propertyNameDevice = "Device";
    static constexpr const char* propertyNameDeviceType = "DeviceType";
    static constexpr const char* propertyManufacturer = "Manufacturer";
    static constexpr const char* propertySubsystem = "Subsystem";

  public:
    static constexpr size_t pcieFunctionsCount = 8;
    static constexpr const char* pcieFunctionFieldPrefix = "Function";
    static const std::vector<MemberName> pcieFunctionsDynamicFields;

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
        return PCIeProvider::pcieFunctionFieldPrefix + std::to_string((index)) +
               dynMemberName;
    }

    PCIeProvider() : dbus::FindObjectDBusQuery()
    {}
    ~PCIeProvider() override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static auto pcieInterfaceProperties = getPCIeMemberNameDict();
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                pcieInterfaceName,
                pcieInterfaceProperties,
            },
        };

        return dictionary;
    }

    static const std::map<PropertyName, MemberName> getPCIeMemberNameDict()
    {
        std::map<PropertyName, MemberName> pcieFunctionsFields{
            {
                propertyNameDevice,
                supplement_providers::pcie::fieldDevice,
            },
            {
                propertyNameDeviceType,
                supplement_providers::pcie::fieldDeviceType,
            },
            {
                propertyManufacturer,
                supplement_providers::pcie::fieldManufacturer,
            },
            {
                propertySubsystem,
                supplement_providers::pcie::fieldSubsystem,
            },
        };

        for (size_t index = 0; index < pcieFunctionsCount; ++index)
        {
            for (auto dynMemberName : pcieFunctionsDynamicFields)
            {
                auto fieldName =
                    getComplexFunctionMemberName(index, dynMemberName);
                pcieFunctionsFields.insert_or_assign(fieldName, fieldName);
            }
        }

        return std::forward<std::map<PropertyName, MemberName>>(
            pcieFunctionsFields);
    }

    static const std::vector<MemberName> getPCIeMemberNameList()
    {
        static auto memberDict = getPCIeMemberNameDict();
        std::vector<MemberName> memberList{
            supplement_providers::pcie::metaSBD,
            supplement_providers::pcie::fieldSocket,
            supplement_providers::pcie::fieldBus,
            supplement_providers::pcie::fieldAddress,
            supplement_providers::status::fieldStatus,
        };

        std::transform(memberDict.begin(), memberDict.end(),
                       std::back_inserter(memberList),
                       [](auto& it) { return it.second; });
        return std::forward<std::vector<MemberName>>(memberList);
    }

    void supplementByStaticFields(DBusInstancePtr& instance) const override
    {
        this->setSBD(instance);
        this->setStatus(instance);
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/PCIe",
            {
                pcieInterfaceName,
            },
            noDepth,
            pcieServiceName,
        };

        return criteria;
    }

    /**
     * @brief Set Socket, Bus and Address fields
     *
     * @param instance tartget instance
     */
    void setSBD(DBusInstancePtr& instance) const
    {
        using namespace supplement_providers::pcie;

        auto& objectPath = instance->getObjectPath();
        auto SBD =
            app::helpers::utils::getNameFromLastSegmentObjectPath(objectPath);

        instance->supplementOrUpdate(supplement_providers::pcie::metaSBD, SBD);

        constexpr const std::array fieldMatchIndexed{
            fieldSocket,
            fieldBus,
            fieldAddress,
        };
        BMC_LOG_DEBUG << "SBD object: " << SBD;

        std::regex SBDRegex("^S(\\d+)B(\\d+)D(\\d+)$");
        std::smatch SBDMatch;
        if (std::regex_search(SBD, SBDMatch, SBDRegex))
        {
            BMC_LOG_DEBUG << "count SBD matched: " << SBDMatch.size();
            for (size_t index = 1; index <= SBDMatch.size(); ++index)
            {
                // SBD match groups indexed from 1, but fieldMatchIndexed from 0.
                // Hence, need to reduct the index to verefy
                auto memberIndex = index - 1;
                if (memberIndex > fieldMatchIndexed.size())
                {
                    return;
                }
                BMC_LOG_DEBUG << "SBD segment:";
                BMC_LOG_DEBUG << fieldMatchIndexed[memberIndex] << "="
                              << SBDMatch[index];
                instance->supplementOrUpdate(fieldMatchIndexed[memberIndex],
                                             std::string(SBDMatch[index]));
            }
        }
    }

    void setStatus(DBusInstancePtr& instance) const
    {
        // Currently, doesn't have a way to determine the PCIe status.
        // Set 'OK' as default.
        instance->supplementOrUpdate(supplement_providers::status::fieldStatus,
                                     Status::statusOK);
    }
};

const std::vector<MemberName> PCIeProvider::pcieFunctionsDynamicFields{
    supplement_providers::pcie::functions::fieldClassCode,
    supplement_providers::pcie::functions::fieldDeviceClass,
    supplement_providers::pcie::functions::fieldDeviceId,
    supplement_providers::pcie::functions::fieldDeviceName,
    supplement_providers::pcie::functions::fieldFunctionType,
    supplement_providers::pcie::functions::fieldRevisionId,
    supplement_providers::pcie::functions::fieldSubsystemId,
    supplement_providers::pcie::functions::fieldSubsystemName,
    supplement_providers::pcie::functions::fieldSubsystemVendorId,
    supplement_providers::pcie::functions::fieldVendorId,
    supplement_providers::pcie::functions::fieldVendorName,
};

class PCIeBaseEntity : public Entity
{
  public:
    explicit PCIeBaseEntity(const EntityName& entityName) : Entity(entityName)
    {}
    ~PCIeBaseEntity() override = default;

  protected:
    const EntityPtr getPCIeProvider()
    {
        return app::core::application.getEntityManager().getProvider(
            supplement_providers::pcie::providerPCIe);
    }
};

class PCIeDeviceEntity final : public PCIeBaseEntity
{
  public:
    explicit PCIeDeviceEntity(const EntityName& entityName) :
        PCIeBaseEntity(entityName)
    {}
    ~PCIeDeviceEntity() override = default;

    void fillEntity() override
    {
        auto instances = getPCIeProvider()->getInstances();
        // set all providers instances directly
        this->setInstances(instances);
    }

    static const IEntity::IRelation::RelationRulesList& realtionToFunctions()
    {
        using namespace std::placeholders;
        using namespace app::entity::obmc::definitions;

        static const IEntity::IRelation::RelationRulesList relations{
            {
                supplement_providers::pcie::metaSBD,
                supplement_providers::pcie::metaSBD,
                [](const IEntity::IEntityMember::InstancePtr& instance,
                   const IEntity::IEntityMember::IInstance::FieldType& value)
                    -> bool {
                    const auto& destSBD = std::get<std::string>(value);
                    BMC_LOG_DEBUG << "[CHECK RELATION] "
                                  << instance->getStringValue() << "|"
                                  << destSBD;
                    return instance->getStringValue() == destSBD;
                },
            },
        };
        return relations;
    }
};

class PCIeFunctionEntity final : public PCIeBaseEntity
{
  public:
    explicit PCIeFunctionEntity(const EntityName& entityName) :
        PCIeBaseEntity(entityName)
    {}
    ~PCIeFunctionEntity() override = default;

    void fillEntity() override
    {
        using namespace supplement_providers::pcie;

        std::vector<InstancePtr> instancesToInit;
        constexpr const std::array directlyCopyEntityMembers{
            metaSBD,
        };
        auto& providerInstances = getPCIeProvider()->getInstances();
        for (auto providerInstance : providerInstances)
        {
            for (uint8_t funcIndex = 0;
                 funcIndex < PCIeProvider::pcieFunctionsCount; ++funcIndex)
            {
                std::string identifyField =
                    PCIeProvider::pcieFunctionFieldPrefix +
                    std::to_string(funcIndex) + functions::fieldDeviceId;
                auto identifyMembery =
                    providerInstance->getField(identifyField);
                if (identifyMembery->getStringValue().empty())
                {
                    continue;
                }

                auto targetInstance = std::make_shared<Entity::StaticInstance>(
                    identifyField + identifyMembery->getStringValue() +
                    providerInstance->getField(metaSBD)->getStringValue());

                // Set the PCI Function ID
                targetInstance->supplementOrUpdate(
                    pcie::functions::fieldFunctionId, funcIndex);

                // Directly copy member instance from provider
                for (auto directlyCopyMember : directlyCopyEntityMembers)
                {
                    targetInstance->supplementOrUpdate(
                        directlyCopyMember,
                        providerInstance->getField(directlyCopyMember)
                            ->getValue());
                }

                // copy complex members
                for (auto dynamicField :
                     PCIeProvider::pcieFunctionsDynamicFields)
                {
                    const auto pcieFuncMemberName =
                        PCIeProvider::getComplexFunctionMemberName(
                            funcIndex, dynamicField);
                    auto field = providerInstance->getField(pcieFuncMemberName);
                    try
                    {
                        if (field->getStringValue().empty())
                        {
                            // Don't set the value of an empty string to pass
                            // through the 'N/A' value.
                            continue;
                        }
                    }
                    catch (std::bad_variant_access&)
                    {
                        // pass through if the field is not
                    }

                    targetInstance->supplementOrUpdate(dynamicField,
                                                       field->getValue());
                }

                instancesToInit.emplace_back(targetInstance);
            }
        }
        setInstances(instancesToInit);
    }
};

} // namespace obmc
} // namespace query
} // namespace app
