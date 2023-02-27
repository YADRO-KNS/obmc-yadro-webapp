// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <common_fields.hpp>
#include <core/application.hpp>
#include <core/connect/dbus_connect.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>
#include <sdbusplus/exception.hpp>

#include <chrono>
#include <thread>

namespace app
{
namespace query
{
namespace dbus
{
using namespace app::core::exceptions;
using namespace app::connect;

DBusQuery::InstanceCreateHandlers DBusQuery::instanceCreateHandlers;
DBusQuery::InstanceRemoveHandlers DBusQuery::instanceRemoveHandlers;

DBusInstance::WatchersMap DBusInstance::listeners;
std::vector<DBusInstance::InstanceHash> DBusInstance::toCleanupInstances;
std::atomic<std::thread::id> DBusInstance::instanceAccessGuard =
    std::thread::id(0);

std::size_t FindObjectDBusQuery::DBusObjectEndpoint::getHash() const
{
    std::size_t hashPath = std::hash<std::string>{}(path);
    std::size_t hashService =
        std::hash<std::string>{}(service.has_value() ? *service : "");
    std::size_t hashDepth = std::hash<int32_t>{}(depth);
    std::size_t hashInterfaces;

    for (const auto& interface : interfaces)
    {
        hashInterfaces ^= std::hash<std::string>{}(interface);
    }

    return (hashPath ^ (hashInterfaces << 1) ^ (hashDepth << 2) ^
            (hashService << 3));
}

const entity::IEntity::InstanceCollection FindObjectDBusQuery::process()
{
    using DBusSubTreeOut =
        std::vector<std::pair<std::string, DBusServiceInterfaces>>;
    std::vector<DBusInstancePtr> dbusInstances;
    DBusSubTreeOut mapperResponse;
    try
    {
        mapperResponse = getConnect()->callMethodAndRead<DBusSubTreeOut>(
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            getQueryCriteria().path, getQueryCriteria().depth,
            getQueryCriteria().interfaces);
    }
    catch (const sdbusplus::exception_t& ex)
    {
        log<level::DEBUG>(
            "Fail to process DBus query (global searching objects by criteria)",
            entry("DBUS_PATH=%s", getQueryCriteria().path.c_str()),
            entry("DBUS_DEPTH=%d", getQueryCriteria().depth),
            entry("IFACES=%ld", getQueryCriteria().interfaces.size()),
            entry("ERROR=%s", ex.what()));
        this->raiseError();
    }

    for (const auto& [objectPath, serviceInfoList] : mapperResponse)
    {
        for (const auto& [serviceName, interfaces] : serviceInfoList)
        {
            if (getQueryCriteria().service.has_value() &&
                getQueryCriteria().service.value() != serviceName)
            {
                continue;
            }
            auto instance =
                this->createInstance(serviceName, objectPath, interfaces);
            dbusInstances.push_back(instance);
            instance->bindListeners(getConnect());
        }
    }

    log<level::DEBUG>(
        "Acquire dbus-objects is complete.",
        entry("COUNT=%ld", dbusInstances.size()),
        entry("DBUS_PATH=%s", getQueryCriteria().path.c_str()),
        entry("DBUS_DEPTH=%d", getQueryCriteria().depth),
        entry("IFACES=%ld", getQueryCriteria().interfaces.size()));

    std::vector<IEntity::InstancePtr> result(dbusInstances.begin(),
                                             dbusInstances.end());
    return result;
}

bool FindObjectDBusQuery::checkCriteria(
    const ObjectPath& objectPath,
    std::optional<ServiceName> optionalServiceName) const
{
    if (optionalServiceName.has_value() &&
        getQueryCriteria().service.has_value() &&
        *optionalServiceName != *getQueryCriteria().service)
    {
        return false;
    }

    if (!objectPath.starts_with(getQueryCriteria().path))
    {
        return false;
    }

    return true;
}

bool FindObjectDBusQuery::checkCriteria(
    const ObjectPath& objectPath, const InterfaceList& interfacesToCheck,
    std::optional<ServiceName> optionalServiceName) const
{
    const auto& criteriaInterfaces = getQueryCriteria().interfaces;

    if (!this->checkCriteria(objectPath, optionalServiceName))
    {
        return false;
    }

    if (getQueryCriteria().depth > 0 &&
        app::helpers::utils::countExtraSegmentsOfPath(
            getQueryCriteria().path, objectPath) > getQueryCriteria().depth)
    {
        return false;
    }

    auto hasValidInterfaceForCriteria = std::any_of(
        interfacesToCheck.begin(), interfacesToCheck.end(),
        [&criteriaInterfaces](const InterfaceName& interfaceToCheck) {
            return std::any_of(
                criteriaInterfaces.begin(), criteriaInterfaces.end(),
                [&interfaceToCheck](const InterfaceName& criteriaInterface) {
                    return interfaceToCheck == criteriaInterface;
                });
        });
    if (!hasValidInterfaceForCriteria)
    {
        return false;
    }

    return true;
}

DBusQueryConstWeakPtr FindObjectDBusQuery::getWeakPtr() const
{
    return weak_from_this();
}

DBusQueryPtr FindObjectDBusQuery::getSharedPtr()
{
    return shared_from_this();
}

const entity::IEntity::InstanceCollection GetObjectDBusQuery::process()
{
    IEntity::InstanceCollection result;
    auto instance = createInstance(serviceName, objectPath, searchInterfaces());
    result.emplace_back(instance);
    instance->bindListeners(getConnect());
    return std::forward<IEntity::InstanceCollection>(result);
}

bool GetObjectDBusQuery::checkCriteria(
    const ObjectPath& objectPath,
    std::optional<ServiceName> optionalServiceName) const
{
    if (!optionalServiceName.has_value() || *optionalServiceName != serviceName)
    {
        return false;
    }

    if (getObjectPathNamespace() != objectPath)
    {
        return false;
    }
    return true;
}

bool GetObjectDBusQuery::checkCriteria(
    const ObjectPath& objectPath, const InterfaceList& inputInterfaces,
    std::optional<ServiceName> optionalServiceName) const
{
    if (!this->checkCriteria(objectPath, optionalServiceName))
    {
        return false;
    }

    InterfaceList notMatchedInterfaces;
    std::set_difference(
        searchInterfaces().begin(), searchInterfaces().end(),
        inputInterfaces.begin(), inputInterfaces.end(),
        std::inserter(notMatchedInterfaces, notMatchedInterfaces.begin()));
    if (notMatchedInterfaces.size() == searchInterfaces().size())
    {
        // No one interface matched
        return false;
    }

    return true;
}

const ObjectPath& GetObjectDBusQuery::getObjectPathNamespace() const
{
    return this->objectPath;
}

DBusQueryConstWeakPtr GetObjectDBusQuery::getWeakPtr() const
{
    return weak_from_this();
}

DBusQueryPtr GetObjectDBusQuery::getSharedPtr()
{
    return shared_from_this();
}

const ObjectPath& FindObjectDBusQuery::getObjectPathNamespace() const
{
    return this->getQueryCriteria().path;
}

const IEntity::InstanceCollection IntrospectServiceDBusQuery::process()
{
    using ObjectValueTree =
        std::map<sdbusplus::message::object_path, DBusInterfacesMap>;
    IEntity::InstanceCollection dbusInstances;
    ObjectValueTree interfacesResponse;

    try
    {
        interfacesResponse = getConnect()->callMethodAndRead<ObjectValueTree>(
            serviceName.c_str(), "/", "org.freedesktop.DBus.ObjectManager",
            "GetManagedObjects");
    }
    catch (const sdbusplus::exception_t& e)
    {
        log<level::DEBUG>("Fail to process DBus query (service introspect)",
                          entry("DBUS_SVC=%s", serviceName.c_str()),
                          entry("ERROR=%s", e.what()));
        this->raiseError();
    }
    for (const auto& [objectPath, interfaces] : interfacesResponse)
    {
        InterfaceList actualInterfaces;
        for (const auto& [interface, _] : interfaces)
        {
            actualInterfaces.emplace_back(interface);
        }
        auto instance = std::make_shared<DBusInstance>(
            serviceName, objectPath.str, actualInterfaces, getWeakPtr(),
            true);

        for (const auto& [interfaceName, propertiesMap] : interfaces)
        {
            instance->fillMembers(interfaceName, propertiesMap);
        }
        supplementByStaticFields(instance);
        dbusInstances.push_back(instance);
        instance->bindListeners(getConnect());
    }

    log<level::DEBUG>("Service introspect is successfull",
                      entry("DBUS_SVC=%s", serviceName.c_str()));
    return std::forward<IEntity::InstanceCollection>(dbusInstances);
}

bool IntrospectServiceDBusQuery::checkCriteria(
    const ObjectPath&, std::optional<ServiceName> optionalServiceName) const
{
    if (!optionalServiceName.has_value() || *optionalServiceName != serviceName)
    {
        return false;
    }

    return true;
}

bool IntrospectServiceDBusQuery::checkCriteria(
    const ObjectPath& op, const InterfaceList& inputInterfaces,
    std::optional<ServiceName> optionalServiceName) const
{
    InterfaceList notMatchedInterfaces;
    InterfaceList activeInterfaces;
    for (const auto& seachPropIt : getSearchPropertiesMap())
    {
        activeInterfaces.push_back(seachPropIt.first);
    }

    if (!this->checkCriteria(op, optionalServiceName))
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

DBusQueryConstWeakPtr IntrospectServiceDBusQuery::getWeakPtr() const
{
    return weak_from_this();
}

DBusQueryPtr IntrospectServiceDBusQuery::getSharedPtr()
{
    return shared_from_this();
}

const ObjectPath& IntrospectServiceDBusQuery::getObjectPathNamespace() const
{
    static const ObjectPath globalNamespace("/");
    return globalNamespace;
}

void DBusInstance::initialize()
{
    bool hasError = false;
    setUninitialized();
    for (const auto& interface : actualInterfaces)
    {
        try
        {
            auto properties =
                queryProperties(dbusQuery.lock()->getConnect(), interface);
            fillMembers(interface, properties);
        }
        catch (std::runtime_error&)
        {
            // if acquiring properties fails then mark IInstance as
            // uninitialized.
            hasError = true;
            continue;
        }
    }

    this->dbusQuery.lock()->supplementByStaticFields(this->shared_from_this());
    if (!hasError)
    {
        setInitialized();
    }
}

void DBusInstance::verifyState()
{
    if (!state)
    {
        initialize();
    }
}

DBusInstancePtr DBusQuery::createInstance(const ServiceName& serviceName,
                                          const ObjectPath& objectPath,
                                          const InterfaceList& interfaces)
{
    InterfaceList actualInterfaces;
    for (const auto& interface : interfaces)
    {
        auto epIt = getSearchPropertiesMap().find(interface);
        if (epIt == getSearchPropertiesMap().end())
        {
            // specified interface not declared in EP configuration
            log<level::DEBUG>("createInstance(): specified interface not "
                              "found in the endpoint configuration",
                              entry("DBUS_IFACE=%s", interface.c_str()));
            continue;
        }
        actualInterfaces.emplace_back(interface);
    }

    auto instance = std::make_shared<DBusInstance>(
        serviceName, objectPath, actualInterfaces, getWeakPtr());

    instance->initialize();
    return std::forward<DBusInstancePtr>(instance);
}

void DBusQuery::addObserver(sdbusplus::bus::match::match&& observer)
{
    observers.push_back(std::move(observer));
}

const DBusConnectUni& DBusQuery::getConnect()
{
    return app::core::application.getDBusConnect();
}

const DBusConnectUni& DBusQuery::getConnect() const
{
    return app::core::application.getDBusConnect();
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
    const auto queryShr = dbusQuery.lock();
    auto propertySetters = dbusPropertySetters(interfaceName);
    if (propertySetters.empty() || !queryShr)
    {
        return false;
    }

    for (const auto& [propertyReflection, setters] : propertySetters)
    {
        auto findProperty = properties.find(propertyReflection.first);
        if (findProperty == properties.end())
        {
            continue;
        }
        auto formattedValue = queryShr->processFormatters(
            propertyReflection.first, findProperty->second, self);

        this->resolveDBusVariant(propertyReflection.second, formattedValue);
    }
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
    DBusInstance::queryProperties(const connect::DBusConnectUni& connect,
                                  const InterfaceName& interface)
{
    log<level::DEBUG>("Query properties of DBus instance cache",
                      entry("DBUS_SVC=%s", serviceName.c_str()),
                      entry("DBUS_OBJ=%s", objectPath.c_str()),
                      entry("DBUS_IFACE=%s", interface.c_str()));

    try
    {
        return std::forward<const DBusPropertiesMap>(
            connect->callMethodAndRead<DBusPropertiesMap>(
                serviceName, objectPath, "org.freedesktop.DBus.Properties",
                "GetAll", interface));
    }
    catch (const sdbusplus::exception_t& e)
    {
        // Don't rase CRITICAL log level because the GetAll might processing for
        // object which already deleted by service.
        log<level::DEBUG>("Failed to GetAll properties",
                          entry("DBUS_SVC=%s", serviceName.c_str()),
                          entry("DBUS_OBJ=%s", objectPath.c_str()),
                          entry("DBUS_IFACE=%s", interface.c_str()),
                          entry("ERROR=%s", e.what()));
        throw std::runtime_error("Failed to query properties");
    }

    return DBusPropertiesMap();
}

void DBusInstance::bindListeners(const connect::DBusConnectUni& connection)
{
    using namespace sdbusplus::bus::match;

    auto selfWeak = weak_from_this();

    for (const auto& interface : actualInterfaces)
    {
        auto matcher = connection->createWatcher(
            rules::propertiesChanged(this->getObjectPath(), interface),
            [selfWeak,
             queryPtr = dbusQuery](sdbusplus::message::message& message) {
                std::map<PropertyName, DbusVariantType> changedValues;
                InterfaceName interfaceName;
                auto self = selfWeak.lock();
                if (!self)
                {
                    return;
                }
                try
                {
                    message.read(interfaceName, changedValues);
                }
                catch (const std::exception& e)
                {
                    log<level::ERR>("Failed to process DBus signal handler",
                                    entry("SIGNAL=PropertyChanged"),
                                    entry("DBUS_OBJ=%s", message.get_path()),
                                    entry("ERROR=%s", e.what()));
                }
                self->fillMembers(interfaceName, changedValues);
                auto query = queryPtr.lock();
                if (query)
                {
                    query->supplementByStaticFields(self);
                }
            });

        log<level::DEBUG>("The DBus signal watcher successfully registered",
                          entry("SIGNAL=PropertyChanged"),
                          entry("DBUS_OBJ=%s", getObjectPath().c_str()),
                          entry("DBUS_IFACE=%s", interface.c_str()));

        auto isLocked = instanceAccessGuardLock();
        listeners[getHash()].emplace_back(
            std::forward<sdbusplus::bus::match::match>(matcher));
        if (isLocked)
        {
            instanceAccessGuardUnlock();
        }
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

void DBusInstance::supplement(
    const MemberName& memberName,
    const IEntity::IEntityMember::IInstance::FieldType& value)
{
    Entity::StaticInstance::supplement(memberName, value);
}

void DBusInstance::supplementOrUpdate(
    const MemberName& memberName,
    const IEntity::IEntityMember::IInstance::FieldType& value)
{
    Entity::StaticInstance::supplementOrUpdate(memberName, value);
}

void DBusInstance::mergeInternalMetadata(
    const IEntity::InstancePtr& destination)
{
    InterfaceList mergedList;
    auto dbusInstanceDest =
        std::dynamic_pointer_cast<DBusInstance>(destination);
    if (!dbusInstanceDest)
    {
        return;
    }

    std::merge(actualInterfaces.begin(), actualInterfaces.end(),
               dbusInstanceDest->actualInterfaces.begin(),
               dbusInstanceDest->actualInterfaces.end(),
               std::back_inserter(mergedList));

    // asuming, a mutex required
    actualInterfaces.swap(mergedList);
}

void DBusInstance::supplementOrUpdate(const IEntity::InstancePtr& destination)
{
    for (const auto& memberName : destination->getMemberNames())
    {
        if (memberName.starts_with("__meta_"))
        {
            continue;
        }
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

void DBusInstance::captureDBusAssociations(
    const InterfaceName& interfaceName,
    const DBusAssociationsType& associations)
{
    using namespace app::obmc::entity::relations;
    log<level::DEBUG>("Disclosing an DBUs association",
                      entry("ASSOC_IFACE=%s", interfaceName.c_str()));

    const auto queryShr = dbusQuery.lock();
    if (!queryShr)
    {
        return;
    }
    this->complexInstances.clear();
    // Since the complex association is a disclose of shadow one DBus Property,
    // we should clear outdated instances with the same specified interface
    for (const auto& [source, destination, destObjectPath] : associations)
    {
        log<level::DEBUG>(
            "Strip an association down into members of child instance",
            entry("SOURCE=%s", source.c_str()),
            entry("DESTINATION=%s", destination.c_str()),
            entry("DEST_OBJ=%s", destObjectPath.c_str()));
        // The object path is used to calc the instance unique CRC, and it
        // should be unique. Build complex (valid) dbus path by concatenation
        // `dest-object + dest-name + source`.
        // Adding postfix `__meta` to be sure that calculated path never match
        // with real object path.
        auto complexAssocDummyPath =
            destObjectPath + "__meta/" + destination + "/" + source;

        auto childInstance =
            std::make_shared<DBusInstance>(serviceName, complexAssocDummyPath,
                                           actualInterfaces, dbusQuery, true);
        DBusPropertiesMap properties{
            {fieldSource, source},
            {fieldDestination, destination},
            {fieldEndpoint, destObjectPath},
        };
        // Don't care about outdated instances. The 'fillMemeber' method now be
        // able to update stored instances.
        childInstance->fillMembers(interfaceName, properties);
        queryShr->supplementByStaticFields(childInstance);

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

        if constexpr (std::is_constructible_v<
                          Entity::EntityMember::StaticInstance::FieldType,
                          TProperty>)
        {
            instance->supplementOrUpdate(
                memberName,
                Entity::EntityMember::StaticInstance::FieldType(value));
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
    const auto queryShr = dbusQuery.lock();
    if (!queryShr)
    {
        return;
    }
    const auto& defaultFields = queryShr->getDefaultFieldsValue();

    for (const auto& [memberName, memberValueSetter] : defaultFields)
    {
        this->supplementOrUpdate(
            memberName, std::invoke(memberValueSetter, shared_from_this()));
    }
}

void DBusInstance::cleanupInstacesWatchers()
{
    auto isLocked = instanceAccessGuardLock();

    for (auto hash : toCleanupInstances)
    {
        auto it = listeners.find(hash);
        if (it != listeners.end())
        {
            listeners.erase(it);
        }
    }
    toCleanupInstances.clear();

    if (isLocked)
    {
        instanceAccessGuardUnlock();
    }
}

void DBusInstance::markInstanceIsUnavailable(const DBusInstance& instance)
{
    auto isLocked = instanceAccessGuardLock();
    toCleanupInstances.emplace_back(instance.getHash());
    if (isLocked)
    {
        instanceAccessGuardUnlock();
    }
}

const DBusPropertySetters&
    DBusInstance::dbusPropertySetters(const InterfaceName& interface) const
{
    static const DBusPropertySetters noSetters;
    const auto query = this->dbusQuery.lock();
    auto findInterface = std::find_if(
        actualInterfaces.begin(), actualInterfaces.end(),
        [interface](const auto& value) { return value == interface; });

    if (findInterface == actualInterfaces.end())
    {
        return noSetters;
    }

    if (query)
    {
        try
        {
            return query->dbusPropertySetters(*findInterface);
        }
        catch (const std::invalid_argument& iaEx)
        {
            log<level::DEBUG>("Specified interface not found at the endpoint "
                              "criteria definition",
                              entry("ERROR=%s", iaEx.what()));
        }
    }

    return noSetters;
}

const DbusVariantType DBusQuery::processFormatters(const PropertyName& property,
                                                   const DbusVariantType& value,
                                                   DBusInstancePtr) const

{
    const auto& formatters = getFormatters(property);
    if (formatters.empty())
    {
        return std::move(value);
    }

    DbusVariantType formattedValue(value);
    for (auto formatter : formatters)
    {
        formattedValue = formatter->format(property, formattedValue);
    }

    return formattedValue;
}

const DBusPropertySetters&
    DBusQuery::dbusPropertySetters(const InterfaceName& interface) const
{
    auto findSetters = getSearchPropertiesMap().find(interface);
    if (findSetters != getSearchPropertiesMap().end())
    {
        return findSetters->second;
    }

    throw std::invalid_argument("Interface not found: " + interface);
}

void DBusQuery::registerObjectCreationObserver(
    std::reference_wrapper<entity::IEntity> entity)
{
    using namespace sdbusplus::bus::match;
    auto handler = [dbusQueryShr = this->getSharedPtr(),
                    entity](const sdbusplus::message::object_path& objectPath,
                            const DBusInterfacesMap& interfacesAdded,
                            const std::string& sender) -> bool {
        InterfaceList interfacesList;
        const std::string& objectPathStr = objectPath.str;

        try
        {
            const ServiceName serviceName =
                dbusQueryShr->getConnect()->getWellKnownServiceName(sender);

            for (const auto& [interfaceName, _] : interfacesAdded)
            {
                interfacesList.push_back(interfaceName);
            }

            if (!dbusQueryShr->checkCriteria(objectPath, serviceName))
            {
                return true;
            }

            interfacesList = dbusQueryShr->introspectDBusObjectInterfaces(
                serviceName, objectPath, interfacesList);

            // secondary verify the defiend interface list for incoming signal.
            if (!dbusQueryShr->checkCriteria(objectPath, interfacesList,
                                             serviceName))
            {
                return true;
            }

            log<level::DEBUG>(
                "Create instance from 'InterfaceAdded' signal handler",
                entry("SIGNAL=InterfaceAdded"),
                entry("DBUS_OBJ=%s", objectPathStr.c_str()));

            DBusInstancePtr newInstance = dbusQueryShr->createInstance(
                serviceName, objectPathStr, interfacesList);
            auto entityInstance =
                entity.get().getInstance(newInstance->getHash());

            if (!entityInstance)
            {
                newInstance->bindListeners(dbusQueryShr->getConnect());
                entity.get().mergeInstance(newInstance);
            }
            else
            {
                entityInstance->mergeInternalMetadata(newInstance);
                entityInstance->setUninitialized();
            }
        }
        catch (const std::runtime_error& e)
        {
            log<level::DEBUG>("Can't handle interfacesAdded signal",
                              entry("ERROR=%s", e.what()),
                              entry("SIGNAL=InterfaceAdded"),
                              entry("DBUS_OBJ=%s", objectPathStr.c_str()));
            return false;
        }

        return true;
    };

    DBusQuery::instanceCreateHandlers.push_back(std::move(handler));
}

void DBusQuery::registerObjectRemovingObserver(
    std::reference_wrapper<entity::IEntity> entity)
{
    using namespace sdbusplus::bus::match;

    auto handler = [entity, dbusQueryWeak = this->getWeakPtr()](
                       const sdbusplus::message::object_path& objectPath,
                       const InterfaceList& removedInterfaces,
                       const std::string& sender) -> bool {
        const std::string& objectPathStr = objectPath.str;
        const auto dbusQuery = dbusQueryWeak.lock();
        if (!dbusQuery)
        {
            return true;
        }

        try
        {
            const ServiceName serviceName =
                dbusQuery->getConnect()->getWellKnownServiceName(sender);
            auto instanceHash =
                DBusInstance::getHash(serviceName, objectPathStr);

            if (!dbusQuery->checkCriteria(objectPath, removedInterfaces,
                                          serviceName))
            {
                return true;
            }

            log<level::DEBUG>(
                "Remove instance from 'InterfaceRemoved' signal handler",
                entry("SIGNAL=InterfaceRemoved"),
                entry("DBUS_OBJ=%s", objectPathStr.c_str()));

            entity.get().removeInstance(instanceHash);
        }
        catch (const std::runtime_error& e)
        {
            log<level::DEBUG>("Can't handle interfaceRemoved signal",
                              entry("ERROR=%s", e.what()),
                              entry("SIGNAL=InterfaceRemoved"),
                              entry("DBUS_OBJ=%s", objectPathStr.c_str()));
            return false;
        }

        return true;
    };

    DBusQuery::instanceRemoveHandlers.push_back(std::move(handler));
}

template <typename THandlerCb>
static void enqueueSignalHandler(size_t signalHash, THandlerCb handler)
{
    using namespace std::literals;

    struct LockGuard
    {
        explicit LockGuard(std::atomic_bool& locked) : locked(locked)
        {
            bool expected = false;
            while (!locked.compare_exchange_strong(expected, true,
                                                   std::memory_order_seq_cst))
            {
                std::this_thread::sleep_for(1ms);
                expected = false;
            }
        }
        ~LockGuard()
        {
            locked.store(false);
        }

      private:
        std::atomic_bool& locked;
    };
    static std::map<size_t, std::pair<std::time_t, THandlerCb>> handlers;
    static std::atomic_bool locked;
    static std::thread signalWorker([]() {
        while (true)
        {
            std::this_thread::sleep_for(5s);
            LockGuard guard(locked);
            std::time_t now = std::time(nullptr);
            std::vector<std::size_t> handled;
            for (const auto& [hash, handlerPair] : handlers)
            {
                if (handlerPair.first < now)
                {
                    handlerPair.second();
                    handled.emplace_back(hash);
                }
            }
            // cleanup
            for (const auto key : handled)
            {
                handlers.erase(key);
            }
        }
    });

    LockGuard guard(locked);
    if (handlers.find(signalHash) == handlers.end())
    {
        // avoid debounce for 5 seconds
        std::time_t now = std::time(nullptr) + 5;
        handlers.emplace(signalHash, std::make_pair(now, handler));
    }
}

static std::size_t getSignalHash(const ServiceName& serviceName,
                                 const ObjectPath& objectPath)
{
    std::size_t hashPath = std::hash<std::string>{}(objectPath);
    std::size_t hashService = std::hash<std::string>{}(serviceName);

    return (hashService ^ (hashPath << 2));
}

static std::size_t getSignalHash(const ServiceName& serviceName,
                                 const ObjectPath& objectPath,
                                 const InterfaceList& interfaces)
{
    std::size_t hashPath = std::hash<std::string>{}(objectPath);
    std::size_t hashService = std::hash<std::string>{}(serviceName);
    std::size_t hashInterfaces;

    for (const auto& interface : interfaces)
    {
        hashInterfaces ^= std::hash<std::string>{}(interface);
    }

    return (hashService ^ (hashInterfaces << 1) ^ (hashPath << 2)) ^ 2;
}

template <typename TInterfacesDict, typename TCacheUpdatingDict>
bool DBusQuery::processGlobalSignal(sdbusplus::message::message& message,
                                    const TCacheUpdatingDict& handlers)
{
    sdbusplus::message::object_path objectPath;
    TInterfacesDict interfacesDict;
    bool status = false;

    try
    {
        message.read(objectPath, interfacesDict);
    }
    catch (sdbusplus::exception_t& ex)
    {
        log<level::ERR>("Failed to process DBus signal handler",
                        entry("DBUS_OBJ=%s", message.get_path()),
                        entry("ERROR=%s", ex.what()));
        return status;
    }

    const std::string sender = message.get_sender();
    for (const auto& handler : handlers)
    {
        if constexpr (std::is_same_v<decltype(handlers),
                                     InstanceCreateHandlers>)
        {
            auto signalHash = getSignalHash(sender, objectPath);
            enqueueSignalHandler(signalHash, std::bind(handler, objectPath,
                                                       interfacesDict, sender));
            status = true;
        }
        else
        {
            if (handler(objectPath, interfacesDict, message.get_sender()))
            {
                status = true;
            }
        }
    }
    return status;
}

InterfaceList DBusQuery::introspectDBusObjectInterfaces(
    const ServiceName& sn, const ObjectPath& op,
    const InterfaceList& searchInterfaces) const
{
    using DBusGetObjectOut = std::vector<std::pair<ServiceName, InterfaceList>>;
    std::vector<DBusGetObjectOut> objectInfo;
    DBusGetObjectOut mapperResponse;
    try
    {
        mapperResponse = getConnect()->callMethodAndRead<DBusGetObjectOut>(
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject", op.c_str(),
            searchInterfaces);
    }
    catch (const sdbusplus::exception_t& ex)
    {
        log<level::DEBUG>("Fail to process DBus query (introspect dbus-object)",
                          entry("DBUS_PATH=%s", op.c_str()),
                          entry("DBUS_SVC=%s", sn.c_str()),
                          entry("SEARC_IFACES=%ld", searchInterfaces.size()),
                          entry("ERROR=%s", ex.what()));
    }

    for (const auto& [svc, ifaces] : mapperResponse)
    {
        if (svc == sn)
        {
            return ifaces;
        }
    }

    log<level::DEBUG>("Introspect dbus-object: No dbus-metadata found for "
                      "specified criteria.",
                      entry("DBUS_PATH=%s", op.c_str()),
                      entry("DBUS_SVC=%s", sn.c_str()));
    for (const auto& i : searchInterfaces)
    {
        log<level::DEBUG>(
            ("Introspect dbus-object: Interface to search:" + i).c_str());
    }
    return {};
}

void DBusQuery::processObjectCreate(sdbusplus::message::message& message)
{
    processGlobalSignal<DBusInterfacesMap>(message,
                                           DBusQuery::instanceCreateHandlers);
}

void DBusQuery::processObjectRemove(sdbusplus::message::message& message)
{
    processGlobalSignal<InterfaceList>(message,
                                       DBusQuery::instanceRemoveHandlers);
}

const DBusPropertyFormatters&
    DBusQuery::getFormatters(const PropertyName& property) const
{
    const auto& eps = getSearchPropertiesMap();
    for (const auto& [_, setters] : eps)
    {
        for (const auto& [reflection, casters] : setters)
        {
            if (reflection.first != property)
            {
                continue;
            }
            return casters.first;
        }
    }
    throw QueryException("casters should be initialized an each setter");
}

const DBusPropertyValidators&
    DBusQuery::getValidators(const PropertyName& property) const
{
    static const DBusPropertyValidators noValidators;

    const auto& eps = getSearchPropertiesMap();
    for (const auto& [_, setters] : eps)
    {
        for (const auto& [reflection, casters] : setters)
        {
            if (reflection.first != property)
            {
                continue;
            }
            return casters.second;
        }
    }
    return noValidators;
}

} // namespace dbus
} // namespace query
} // namespace app
