// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/entity/dbus_query.hpp>

#include <core/connect/dbus_connect.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>
#include <core/application.hpp>

#include <sdbusplus/exception.hpp>
#include <common_fields.hpp>

namespace app
{
namespace query
{
namespace dbus
{
using namespace app::core::exceptions;
using namespace app::connect;

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
    }

    for (auto& [objectPath, serviceInfoList] : mapperResponse)
    {
        for (auto& [serviceName, interfaces] : serviceInfoList)
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
    const ObjectPath& objectPath, const InterfaceList& interfacesToCheck,
    std::optional<ServiceName> optionalServiceName) const
{
    auto& criteriaInterfaces = getQueryCriteria().interfaces;

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
    const InterfaceList& inputInterfaces,
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
    }
    for (auto& [objectPath, interfaces] : interfacesResponse)
    {
        auto instance = std::make_shared<DBusInstance>(
            serviceName, objectPath.str, getSearchPropertiesMap(),
            getWeakPtr());

        for (auto& [interfaceName, propertiesMap] : interfaces)
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
    const ObjectPath&, const InterfaceList& inputInterfaces,
    std::optional<ServiceName> optionalServiceName) const
{
    InterfaceList notMatchedInterfaces;
    InterfaceList activeInterfaces;
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

DBusInstancePtr DBusQuery::createInstance(const ServiceName& serviceName,
                                          const ObjectPath& objectPath,
                                          const InterfaceList& interfaces)
{
    auto instance = std::make_shared<DBusInstance>(
        serviceName, objectPath, getSearchPropertiesMap(), getWeakPtr());
    for (auto& interface : interfaces)
    {
        auto properties = instance->queryProperties(getConnect(), interface);
        instance->fillMembers(interface, properties);
    }

    supplementByStaticFields(instance);
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

    for (auto& [propertyReflection, setters] : propertySetters)
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
   DBusPropertiesMap properties;

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
    }

    return DBusPropertiesMap();
}

void DBusInstance::bindListeners(const connect::DBusConnectUni& connection)
{
    using namespace sdbusplus::bus::match;

    auto selfWeak = weak_from_this();

    for (auto [interface, _] : targetProperties)
    {
        auto matcher = connection->createWatcher(
            rules::propertiesChanged(this->getObjectPath(), interface),
            [selfWeak, queryPtr = dbusQuery](sdbusplus::message::message& message) {
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
        listeners.emplace_back(
            std::forward<sdbusplus::bus::match::match>(matcher));
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
    for (auto& memberName : destination->getMemberNames())
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
    for (auto& [source, destination, destObjectPath] : associations)
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

        auto childInstance = std::make_shared<DBusInstance>(
            serviceName, complexAssocDummyPath, targetProperties,
            dbusQuery);
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

const DBusPropertySetters&
    DBusInstance::dbusPropertySetters(const InterfaceName& interface) const
{
    auto findInterface = this->targetProperties.find(interface);
    if (findInterface == this->targetProperties.end())
    {
        static const DBusPropertySetters noSetters;
        return noSetters;
    }

    return std::move(findInterface->second);
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
    for (auto formatter: formatters)
    {
        formattedValue = formatter->format(property, formattedValue);
    }

    return formattedValue;
}

void DBusQuery::registerObjectCreationObserver(
    std::reference_wrapper<entity::IEntity> entity)
{
    using namespace sdbusplus::bus::match;
    auto handler = [dbusQueryShr = this->getSharedPtr(),
                    actualProperties = std::ref(this->getSearchPropertiesMap()),
                    entity](sdbusplus::message::message& message) {
        DBusInterfacesMap interfacesAdded;
        sdbusplus::message::object_path objectPath;
        InterfaceList interfacesList;

        try
        {
            message.read(objectPath, interfacesAdded);
        }
        catch (sdbusplus::exception_t& ex)
        {
            log<level::ERR>("Failed to process DBus signal handler",
                                    entry("SIGNAL=InterfaceAdded"),
                                    entry("DBUS_OBJ=%s", message.get_path()),
                                    entry("ERROR=%s", ex.what()));
            return;
        }

        const std::string& objectPathStr = objectPath.str;
        try
        {
            const ServiceName serviceName =
                dbusQueryShr->getConnect()->getWellKnownServiceName(
                    message.get_sender());

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
            // this is must have because the interfacesAdded might missing a
            // required interfeses.
            interfacesList.clear();
            for (auto& [interfaceName, _] : actualProperties.get())
            {
                interfacesList.push_back(interfaceName);
            }

            log<level::DEBUG>(
                "Create instance from 'InterfaceAdded' signal handler",
                entry("SIGNAL=InterfaceAdded"),
                entry("DBUS_OBJ=%s", message.get_path()));

            auto entityInstance = dbusQueryShr->createInstance(
                serviceName, objectPathStr, interfacesList);

            auto newInstance = entity.get().mergeInstance(entityInstance);
            auto dbusInstance =
                std::dynamic_pointer_cast<dbus::DBusInstance>(newInstance);
            if (!dbusInstance)
            {
                throw std::logic_error("At the DBusBroker the entity query "
                                       "accepted instance which is "
                                       "not DBusInstance.");
            }
            dbusInstance->bindListeners(dbusQueryShr->getConnect());
        }
        catch (const std::runtime_error& e)
        {
            log<level::DEBUG>("Can't handle interfacesAdded signal",
                              entry("ERROR=%s", e.what()),
                              entry("SIGNAL=InterfaceAdded"),
                              entry("DBUS_OBJ=%s", message.get_path()));
            return;
        }
    };

    addObserver(getConnect()->createWatcher(rules::interfacesAdded(),
                                            std::move(handler)));
}

void DBusQuery::registerObjectRemovingObserver(
    std::reference_wrapper<entity::IEntity> entity)
{
    using namespace sdbusplus::bus::match;

    auto handler = [entity, dbusQueryWeak = this->getWeakPtr()](
                       sdbusplus::message::message& message) {
        sdbusplus::message::object_path objectPath;
        InterfaceList removedInterfaces;

        try
        {
            message.read(objectPath, removedInterfaces);
        }
        catch (sdbusplus::exception_t& ex)
        {
            log<level::ERR>("Failed to process DBus signal handler",
                            entry("SIGNAL=InterfaceRemoved"),
                            entry("DBUS_OBJ=%s", message.get_path()),
                            entry("ERROR=%s", ex.what()));
            return;
        }

        const std::string& objectPathStr = objectPath.str;
        try
        {
            const ServiceName serviceName =
                dbusQueryWeak.lock()->getConnect()->getWellKnownServiceName(
                    message.get_sender());
            auto instanceHash =
                DBusInstance::getHash(serviceName, objectPathStr);

            log<level::DEBUG>(
                "Remove instance from 'InterfaceRemoved' signal handler",
                entry("SIGNAL=InterfaceRemoved"),
                entry("DBUS_OBJ=%s", message.get_path()));

            if (!dbusQueryWeak.lock()->checkCriteria(
                    objectPath, removedInterfaces, serviceName))
            {
                return;
            }

            entity.get().removeInstance(instanceHash);
        }
        catch (const std::runtime_error& e)
        {
            log<level::DEBUG>("Can't handle interfaceRemoved signal",
                              entry("ERROR=%s", e.what()),
                              entry("SIGNAL=InterfaceRemoved"),
                              entry("DBUS_OBJ=%s", message.get_path()));
            return;
        }
    };

    addObserver(this->getConnect()->createWatcher(rules::interfacesRemoved(),
                                            std::move(handler)));
}

const DBusPropertyFormatters&
    DBusQuery::getFormatters(const PropertyName& property) const
{
    const auto& eps = getSearchPropertiesMap();
    for (auto& [_, setters] : eps)
    {
        for (auto& [reflection, casters]: setters)
        {
            if(reflection.first != property)
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
    for(auto& [_, setters] : eps)
    {
        for (auto& [reflection, casters]: setters)
        {
            if(reflection.first != property)
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
