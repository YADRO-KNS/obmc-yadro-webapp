// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/entity/dbus_query.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>

namespace app
{
namespace query
{
namespace dbus
{
using namespace app::core::exceptions;

template <class TInstance>
std::map<std::string, std::string> DBusQuery<TInstance>::serviceNamesDict;

EntityManager::EntityBuilderPtr DBusQueryBuilder::complete()
{
    return std::move(this->entityBuilder);
}

std::size_t FindObjectDBusQuery::DBusObjectEndpoint::getHash() const
{
    std::size_t hashPath = std::hash<std::string>{}(path);
    std::size_t hashService =
        std::hash<std::string>{}(service.has_value() ? *service : "");
    std::size_t hashDepth = std::hash<int32_t>{}(depth);
    std::size_t hashInterfaces;

    for (auto& interface : interfaces)
    {
        hashInterfaces ^= std::hash<std::string>{}(interface);
    }

    return (hashPath ^ (hashInterfaces << 1) ^ (hashDepth << 2) ^
            (hashService << 3));
}

std::vector<IEntity::InstancePtr>
    FindObjectDBusQuery::process(sdbusplus::bus::bus& connect)
{
    using DBusSubTreeOut =
        std::vector<std::pair<std::string, DBusServiceInterfaces>>;
    std::vector<DBusInstancePtr> dbusInstances;

    auto mapperCall = connect.new_method_call(
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree");

    mapperCall.append(getQueryCriteria().path);
    mapperCall.append(getQueryCriteria().depth);
    mapperCall.append(getQueryCriteria().interfaces);

    auto mapperResponseMsg = connect.call(mapperCall);

    if (mapperResponseMsg.is_method_error())
    {
        BMC_LOG_ERROR << "Error of mapper call";
        throw ObmcAppException("ERROR of mapper call");
    }

    DBusSubTreeOut mapperResponse;
    mapperResponseMsg.read(mapperResponse);
    mapperResponseMsg.release();
    mapperCall.release();

    BMC_LOG_DEBUG << "DBus Objects read sucess.";
    for (auto& [objectPath, serviceInfoList] : mapperResponse)
    {
        for (auto& [serviceName, interfaces] : serviceInfoList)
        {
            if (getQueryCriteria().service.has_value() &&
                getQueryCriteria().service.value() != serviceName)
            {
                BMC_LOG_DEBUG << "Skip service " << serviceName
                          << " because it was not specified";
                continue;
            }
            BMC_LOG_DEBUG << "Emplace new entity instance.";
            auto instance = this->createInstance(connect, serviceName,
                                                 objectPath, interfaces);
            dbusInstances.push_back(instance);
        }
    }

    BMC_LOG_DEBUG << "Process Found DBus Object query is sucess. Count instance: "
              << dbusInstances.size();
    std::vector<IEntity::InstancePtr> result(dbusInstances.begin(),
                                             dbusInstances.end());
    return result;
}

bool FindObjectDBusQuery::checkCriteria(
    const ObjectPath& objectPath, const std::vector<InterfaceName>& interface,
    std::optional<ServiceName> optionalServiceName) const
{
    std::vector<InterfaceName> notMatchedInterfaces;
    auto& activeInterfaces = getQueryCriteria().interfaces;

    if (optionalServiceName.has_value() &&
        getQueryCriteria().service.has_value() &&
        *optionalServiceName != *getQueryCriteria().service)
    {
        return false;
    }

    if (getQueryCriteria().depth > 0 &&
        app::helpers::utils::countExtraSegmentsOfPath(
            getQueryCriteria().path, objectPath) > getQueryCriteria().depth)
    {
        return false;
    }

    std::set_difference(
        activeInterfaces.begin(), activeInterfaces.end(), interface.begin(),
        interface.end(),
        std::inserter(notMatchedInterfaces, notMatchedInterfaces.begin()));
    if (notMatchedInterfaces.size() == activeInterfaces.size())
    {
        // No one interface matched
        return false;
    }

    return true;
}

EntityDBusQueryConstWeakPtr FindObjectDBusQuery::getWeakPtr() const
{
    return weak_from_this();
}

EntityDBusQueryPtr FindObjectDBusQuery::getSharedPtr()
{
    return shared_from_this();
}

const ObjectPath& FindObjectDBusQuery::getObjectPathNamespace() const
{
    return this->getQueryCriteria().path;
}

std::vector<IEntity::InstancePtr>
    IntrospectServiceDBusQuery::process(sdbusplus::bus::bus& connect)
{
    using ObjectValueTree =
        std::map<sdbusplus::message::object_path, DBusInterfacesMap>;
    std::vector<DBusInstancePtr> dbusInstances;
    ObjectValueTree interfacesResponse;

    auto mapperCall = connect.new_method_call(
        serviceName.c_str(), "/", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");

    auto mapperResponseMsg = connect.call(mapperCall);

    if (mapperResponseMsg.is_method_error())
    {
        BMC_LOG_ERROR << "Error of mapper call";
        throw ObmcAppException("ERROR of mapper call");
    }

    mapperResponseMsg.read(interfacesResponse);
    mapperResponseMsg.release();
    mapperCall.release();
    for (auto& [objectPath, interfaces] : interfacesResponse)
    {
        auto instance = std::make_shared<DBusInstance>(
            serviceName, objectPath.str, getSearchPropertiesMap(),
            getWeakPtr());

        for (auto& [interfaceName, propertiesMap] : interfaces)
        {
            instance->fillMembers(interfaceName, propertiesMap);
        }
        dbusInstances.push_back(instance);
    }

    BMC_LOG_DEBUG << "Process Found DBus Object query is sucess.";

    std::vector<IEntity::InstancePtr> result(dbusInstances.begin(),
                                             dbusInstances.end());
    return std::forward<std::vector<IEntity::InstancePtr>>(result);
}

bool IntrospectServiceDBusQuery::checkCriteria(
    const ObjectPath&, const std::vector<InterfaceName>& inputInterfaces,
    std::optional<ServiceName> optionalServiceName) const
{
    std::vector<InterfaceName> notMatchedInterfaces;
    std::vector<InterfaceName> activeInterfaces;
    for (auto& seachPropIt : getSearchPropertiesMap())
    {
        activeInterfaces.push_back(seachPropIt.first);
    }

    if (!optionalServiceName.has_value() || *optionalServiceName != serviceName)
    {
        return false;
    }

    std::set_difference(
        activeInterfaces.begin(), activeInterfaces.end(),
        inputInterfaces.begin(), inputInterfaces.end(),
        std::inserter(notMatchedInterfaces, notMatchedInterfaces.begin()));
    if (notMatchedInterfaces.size() == activeInterfaces.size())
    {
        // No one interface matched
        return false;
    }

    return true;
}

template <class TInstance>
void DBusQuery<TInstance>::setServiceName(const std::string uniqueName,
                                          const std::string wellKnownName)
{
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    BMC_LOG_DEBUG << "Set Well-Known service name [" << uniqueName << "] "
              << wellKnownName;
    serviceNamesDict.insert_or_assign(uniqueName, wellKnownName);
}

template <class TInstance>
const ServiceName&
    DBusQuery<TInstance>::getWellKnownServiceName(const std::string uniqueName)
{
    using namespace std::literals;

    size_t tryCount = 0;
    while (serviceNamesDict.find(uniqueName) == serviceNamesDict.end() &&
           tryCount < 15)
    {
        tryCount++;
        std::this_thread::sleep_for(1s);
    }
    return serviceNamesDict.at(uniqueName);
}

EntityDBusQueryConstWeakPtr IntrospectServiceDBusQuery::getWeakPtr() const
{
    return weak_from_this();
}

EntityDBusQueryPtr IntrospectServiceDBusQuery::getSharedPtr()
{
    return shared_from_this();
}

const ObjectPath& IntrospectServiceDBusQuery::getObjectPathNamespace() const
{
    static const ObjectPath globalNamespace("/");
    return globalNamespace;
}

template <class TInstance>
DBusInstancePtr DBusQuery<TInstance>::createInstance(
    sdbusplus::bus::bus& connect, const ServiceName& serviceName,
    const ObjectPath& objectPath, const std::vector<InterfaceName>& interfaces)
{
    auto entityInstance = std::make_shared<DBusInstance>(
        serviceName, objectPath, getSearchPropertiesMap(), getWeakPtr());

    BMC_LOG_DEBUG << "ObjectPath='" << objectPath << "', Service='" << serviceName
              << "'";
    for (auto& interface : interfaces)
    {
        auto properties = entityInstance->queryProperties(connect, interface);
        entityInstance->fillMembers(interface, properties);
    }

    return std::forward<DBusInstancePtr>(entityInstance);
}

template <class TInstance>
void DBusQuery<TInstance>::addObserver(sdbusplus::bus::match::match&& observer)
{
    observers.push_back(std::move(observer));
}

const std::vector<DBusInstancePtr> DBusInstance::getComplexInstances() const
{
    std::vector<DBusInstancePtr> childs;
    for (auto [_, instance] : complexInstances)
    {
        childs.push_back(instance);
    }
    return std::forward<const std::vector<DBusInstancePtr>>(childs);
}

bool DBusInstance::fillMembers(
    const InterfaceName& interfaceName,
    const std::map<PropertyName, DbusVariantType>& properties)
{
    auto self = shared_from_this();
    auto propertyMemberDict = getPropertyMemberDict(interfaceName);
    if (propertyMemberDict.empty())
    {
        BMC_LOG_DEBUG << "Properties of interface not provided: " << interfaceName;
        return false;
    }

    for (auto& [propertyName, memberName] : propertyMemberDict)
    {
        auto findProperty = properties.find(propertyName);
        if (findProperty == properties.end())
        {
            continue;
        }
        auto formattedValue = dbusQuery.lock()->processFormatters(
            propertyName, findProperty->second, shared_from_this());

        this->resolveDBusVariant(memberName, formattedValue);
    }
    dbusQuery.lock()->supplementByStaticFields(self);
    return true;
}

const IEntity::IEntityMember::InstancePtr&
    DBusInstance::getField(const IEntity::EntityMemberPtr& member) const
{
    return Entity::StaticInstance::getField(member);
}

const IEntity::IEntityMember::InstancePtr&
    DBusInstance::getField(const MemberName& memberName) const
{
    return Entity::StaticInstance::getField(memberName);
}

const std::vector<MemberName> DBusInstance::getMemberNames() const
{
    return std::forward<const std::vector<MemberName>>(
        Entity::StaticInstance::getMemberNames());
}

bool DBusInstance::hasField(const MemberName& memberName) const
{
    return Entity::StaticInstance::hasField(memberName);
}

const DBusPropertiesMap
    DBusInstance::queryProperties(sdbusplus::bus::bus& connect,
                                  const InterfaceName& interface)
{
    std::map<PropertyName, DbusVariantType> properties;

    BMC_LOG_DEBUG << "Create DBUs 'GetAll' properties call. Service='"
              << serviceName << "', ObjectPath='" << objectPath
              << "', Interface='" << interface << "'";

    sdbusplus::message::message getProperties = connect.new_method_call(
        this->serviceName.c_str(), this->objectPath.c_str(),
        "org.freedesktop.DBus.Properties", "GetAll");
    getProperties.append(interface);

    try
    {
        sdbusplus::message::message response = connect.call(getProperties);
        response.read(properties);
        response.release();
        BMC_LOG_DEBUG << "DBus Properties read SUCCESS. Count properties: "
                  << properties.size();
    }
    catch (const std::exception& e)
    {
        BMC_LOG_CRITICAL << "Failed to GetAll properties. PATH=" << objectPath
                     << ", INTF=" << interface << ", WHAT=" << e.what();
    }
    getProperties.release();

    return std::forward<const DBusPropertiesMap>(properties);
}

void DBusInstance::bindListeners(sdbusplus::bus::bus& connection)
{
    using namespace sdbusplus::bus::match;

    auto self = shared_from_this();

    for (auto [interface, _] : targetProperties)
    {
        match matcher(
            connection,
            rules::propertiesChanged(this->getObjectPath(), interface),
            [self](sdbusplus::message::message& message) {
                std::map<PropertyName, DbusVariantType> changedValues;
                InterfaceName interfaceName;
                message.read(interfaceName, changedValues);

                self->fillMembers(interfaceName, changedValues);
            });
        BMC_LOG_DEBUG << "Registried watcher for Object=" << this->getObjectPath()
                  << ", Interface=" << interface;
        listeners.push_back(std::forward<match>(matcher));
    }
}

const ObjectPath& DBusInstance::getObjectPath() const
{
    return objectPath;
}

const InterfaceName& DBusInstance::getService() const
{
    return serviceName;
}

void DBusInstance::supplement(const MemberName& memberName,
        const IEntity::IEntityMember::IInstance::FieldType& value)
{
    Entity::StaticInstance::supplement(memberName, value);
}

void DBusInstance::supplementOrUpdate(const MemberName& memberName,
        const IEntity::IEntityMember::IInstance::FieldType& value)
{
    Entity::StaticInstance::supplementOrUpdate(memberName, value);
}

void DBusInstance::supplementOrUpdate(const IEntity::InstancePtr& destination)
{
    using namespace app::query::dbus::obmc::definitions;
    BMC_LOG_DEBUG << "count destination members: "
              << destination->getMemberNames().size();
    for (auto& memberName : destination->getMemberNames())
    {
        if (memberName.starts_with(metaFieldPrefix))
        {
            continue;
        }
        BMC_LOG_DEBUG << "Merge Field: " << memberName << " of instance "
                  << this->objectPath;
        this->supplementOrUpdate(memberName,
                                 destination->getField(memberName)->getValue());
    }
}

bool DBusInstance::checkCondition(const IEntity::ConditionPtr condition) const
{
    return Entity::StaticInstance::checkCondition(condition);
}

std::size_t DBusInstance::getHash() const
{
    return getHash(this->getService(), this->getObjectPath());
}

std::size_t DBusInstance::getHash(const ServiceName& serviceName,
                                  const ObjectPath& objectPath)
{
    std::size_t hashServiceName = std::hash<std::string>{}(serviceName);
    std::size_t hashPath = std::hash<std::string>{}(objectPath);

    return (hashPath ^ (hashServiceName << 1));
}

template <typename TProperty>
void DBusInstance::captureComplexDBusProperty(const MemberName& memberName,
                                              const TProperty& property)

{
    BMC_LOG_DEBUG << "Complex Primitive capture, member name=" << memberName;
    for (auto& value : property)
    {
        DBusPropertiesMap properties{
            {memberName, value},
        };
        auto complexInstance = std::make_shared<DBusInstance>(
            serviceName, objectPath, targetProperties, dbusQuery);
        complexInstance->fillMembers(memberName, properties);
        this->complexInstances.insert_or_assign(complexInstance->getHash(),
                                                complexInstance);
    }
    return;
}

void DBusInstance::captureDBusAssociations(
    const InterfaceName& interfaceName,
    const DBusAssociationsType& associations)
{
    using namespace app::entity::obmc::definitions::supplement_providers;

    BMC_LOG_DEBUG << "Complex Association values process, member name="
              << interfaceName;

    this->complexInstances.clear();
    // Since the complex association is a disclose of shadow one DBus Property,
    // we should clear outdated instances with the same specified interface
    for (auto& [source, destination, destObjectPath] : associations)
    {
        BMC_LOG_DEBUG << "Loop association: Source=" << source
                  << ", Destination=" << destination
                  << ", ObjectPath=" << destObjectPath;

        auto childInstance = std::make_shared<DBusInstance>(
            serviceName, destObjectPath, targetProperties, dbusQuery);
        DBusPropertiesMap properties{
            {relations::fieldSource, source},
            {relations::fieldDestination, destination},
            {relations::fieldEndpoint, destObjectPath},
        };
        // Don't care about outdated instances. The 'fillMemeber' method now be
        // able to update stored instances.
        childInstance->fillMembers(interfaceName, properties);

        this->complexInstances.insert_or_assign(childInstance->getHash(),
                                                childInstance);
    }
}

void DBusInstance::resolveDBusVariant(const MemberName& memberName,
                                      const DbusVariantType& dbusVariant)
{
    auto self = shared_from_this();
    auto visitCallback = [instance = self, memberName](auto&& value) {
        using TProperty = std::decay_t<decltype(value)>;

        if constexpr (std::is_constructible_v<DBusMemberInstance::FieldType,
                                              TProperty>)
        {
            instance->supplementOrUpdate(memberName,
                                         DBusMemberInstance::FieldType(value));
            return;
        }
        else if constexpr (std::is_same_v<std::vector<std::string>,
                                          TProperty> ||
                           std::is_same_v<std::vector<double>, TProperty>)
        {
            instance->captureComplexDBusProperty(memberName, value);
            return;
        }
        else if constexpr (std::is_same_v<DBusAssociationsType, TProperty>)
        {
            instance->captureDBusAssociations(memberName, value);
            return;
        }

        throw std::invalid_argument(
            "Try to retrieve the dbus property of an unsupported type");
    };
    std::visit(std::move(visitCallback), dbusVariant);
}

const std::map<std::size_t, IEntity::InstancePtr>
    DBusInstance::getComplex() const
{
    std::map<std::size_t, IEntity::InstancePtr> result(complexInstances.begin(),
                                                       complexInstances.end());
    return std::forward<std::map<std::size_t, IEntity::InstancePtr>>(result);
}

bool DBusInstance::isComplex() const
{
    return complexInstances.empty();
}

void DBusInstance::initDefaultFieldsValue()
{
    auto& defaultFields = dbusQuery.lock()->getDefaultFieldsValue();

    for (auto& [memberName, memberValue] : defaultFields)
    {
        this->supplementOrUpdate(memberName, memberValue);
    }
}

const DBusPropertyMemberDict
    DBusInstance::getPropertyMemberDict(const InterfaceName& interface) const
{
    auto findInterface = this->targetProperties.find(interface);
    if (findInterface == this->targetProperties.end())
    {
        BMC_LOG_DEBUG << "Interface '" << interface << "' missmatch. Skipping";
        return DBusPropertyMemberDict();
    }

    return std::move(findInterface->second);
}

const IEntity::IEntityMember::IInstance::FieldType&
    DBusMemberInstance::getValue() const noexcept
{
    return value;
}

const std::string& DBusMemberInstance::getStringValue() const
{
    return std::get<std::string>(value);
}

int DBusMemberInstance::getIntValue() const
{
    return std::get<int>(value);
}

double DBusMemberInstance::getFloatValue() const
{
    return std::get<double>(value);
}

bool DBusMemberInstance::getBoolValue() const
{
    return std::get<bool>(value);
}

void DBusMemberInstance::setValue(const FieldType& value)
{
    this->value = value;
}

template <class TInstance>
const DbusVariantType
    DBusQuery<TInstance>::processFormatters(const PropertyName& property,
                                            const DbusVariantType& value,
                                            DBusInstancePtr target) const

{
    if (getFormatters().empty())
    {
        BMC_LOG_DEBUG << "No formatters";
        return std::move(value);
    }

    auto findFormattersIt = getFormatters().find(property);
    if (getFormatters().end() == findFormattersIt)
    {
        return std::move(value);
    }

    BMC_LOG_DEBUG << "Found formatter for Property=" << property;
    DbusVariantType formattedValue(value);

    for (auto& formatter : findFormattersIt->second)
    {
        formattedValue =
            std::invoke(formatter, property, formattedValue, target);
    }

    return formattedValue;
}

template <class TInstance>
void DBusQuery<TInstance>::registerObjectCreationObserver(
    sdbusplus::bus::bus& connection, entity::EntityPtr entity)
{
    using namespace sdbusplus::bus::match;
    auto handler = [&connection, dbusQueryShr = this->getSharedPtr(),
                    actualProperties = std::ref(this->getSearchPropertiesMap()),
                    entity](sdbusplus::message::message& message) {
        DBusInterfacesMap interfacesAdded;
        sdbusplus::message::object_path objectPath;
        std::vector<InterfaceName> interfacesList;

        try
        {
            message.read(objectPath, interfacesAdded);
        }
        catch (sdbusplus::exception_t&)
        {
            BMC_LOG_ERROR << "Can't read added interfaces signal";
            return;
        }

        const std::string& objectPathStr = objectPath.str;
        const ServiceName serviceName =
            getWellKnownServiceName(message.get_sender());

        for (auto& [interfaceName, _] : interfacesAdded)
        {
            interfacesList.push_back(interfaceName);
        }

        if (!dbusQueryShr->checkCriteria(objectPath, interfacesList,
                                          serviceName))
        {
            return;
        }
        // fill actual properties from statically defined map.
        // this is must have because the interfacesAdded might missed a required
        // interfeses.
        interfacesList.clear();
        for (auto& [interfaceName, _] : actualProperties.get())
        {
            interfacesList.push_back(interfaceName);
        }

        auto entityInstance = dbusQueryShr->createInstance(
            connection, serviceName, objectPathStr, interfacesList);

        auto newInstance = entity->mergeInstance(entityInstance);
        auto dbusInstance =
            std::dynamic_pointer_cast<dbus::DBusInstance>(newInstance);
        if (!dbusInstance)
        {
            throw std::logic_error("At the DBusBroker the entity query "
                                   "accepted instance which is "
                                   "not DBusInstance.");
        }
        dbusInstance->bindListeners(connection);
    };

    match observer(connection, rules::interfacesAdded(), std::move(handler));

    BMC_LOG_DEBUG << "registerObjectCreationObserver";
    addObserver(std::forward<match>(observer));
}

template <class TInstance>
void DBusQuery<TInstance>::registerObjectRemovingObserver(
    sdbusplus::bus::bus& connection, entity::EntityPtr entity)
{
    using namespace sdbusplus::bus::match;

    auto handler = [entity, dbusQueryWeak = this->getWeakPtr()](
                       sdbusplus::message::message& message) {
        sdbusplus::message::object_path objectPath;
        std::vector<InterfaceName> removedInterfaces;

        try
        {
            message.read(objectPath, removedInterfaces);
        }
        catch (sdbusplus::exception_t& ex)
        {
            BMC_LOG_ERROR << "Can't read removed interfaces signal: " << ex.what();
            return;
        }

        const std::string& objectPathStr = objectPath.str;
        const ServiceName serviceName =
            getWellKnownServiceName(message.get_sender());
        auto instanceHash = DBusInstance::getHash(serviceName, objectPathStr);

        BMC_LOG_DEBUG << "Interface removed: " << serviceName << "|"
                  << objectPathStr;
        if (!dbusQueryWeak.lock()->checkCriteria(objectPath, removedInterfaces,
                                                 serviceName))
        {
            return;
        }

        entity->removeInstance(instanceHash);
    };

    BMC_LOG_DEBUG << "registerObjectRemovingObserver";
    match observer(connection, rules::interfacesRemoved(), std::move(handler));
    addObserver(std::forward<match>(observer));
}

template <class TInstance>
const DBusQuery<TInstance>::FieldsFormattingMap&
    DBusQuery<TInstance>::getFormatters() const
{
    static const FieldsFormattingMap noFormattes;
    return noFormattes;
}

template class DBusQuery<IEntity::InstancePtr>;

} // namespace dbus
} // namespace query
} // namespace app
