// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __QUERY_DBUS_H__
#define __QUERY_DBUS_H__

#include <core/broker/dbus_broker.hpp>
#include <core/entity/entity.hpp>
#include <core/entity/query.hpp>
#include <definitions.hpp>
#include <logger/logger.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>

#include <functional>
#include <map>
#include <utility>
#include <variant>
#include <vector>

namespace app
{
namespace query
{
namespace dbus
{

using namespace app::entity;

using InterfaceName = std::string;
using ServiceName = std::string;
using PropertyName = std::string;
using ObjectPath = std::string;

using InterfaceList = std::vector<InterfaceName>;
using DBusServiceInterfaces = std::map<ServiceName, InterfaceList>;
using DBusPropertyMemberDict = std::map<PropertyName, app::entity::MemberName>;
using DBusPropertyEndpointMap = std::map<InterfaceName, DBusPropertyMemberDict>;

using DBusAssociationsType =
    std::vector<std::tuple<std::string, std::string, std::string>>;
using DbusVariantType =
    std::variant<DBusAssociationsType, std::vector<std::string>,
                 std::vector<double>, std::string, int64_t, uint64_t, double,
                 int32_t, uint32_t, int16_t, uint16_t, uint8_t, bool>;
using DBusPropertiesMap = std::map<PropertyName, DbusVariantType>;
using DBusInterfacesMap = std::map<InterfaceName, DBusPropertiesMap>;

using DBusServiceObjects = std::vector<std::pair<ObjectPath, ServiceName>>;

template <class TInstance>
class DBusQuery;

class DBusInstance;
class DBusQueryBuilder;
class FindObjectDBusQuery;

using DBusQueryBuilderUni = std::unique_ptr<DBusQueryBuilder>;
using DBusQueryBuilderPtr = std::shared_ptr<DBusQueryBuilder>;
using FindObjectDBusQueryPtr = std::shared_ptr<FindObjectDBusQuery>;
using DBusInstancePtr = std::shared_ptr<DBusInstance>;

using EntityDBusQuery = DBusQuery<IEntity::InstancePtr>;
using EntityDBusQueryConstWeakPtr = std::weak_ptr<const EntityDBusQuery>;
using EntityDBusQueryPtr = std::shared_ptr<EntityDBusQuery>;

class DBusInstance final :
    public Entity::StaticInstance,
    public std::enable_shared_from_this<DBusInstance>
{
    using InstanceHash = std::size_t;

    const std::string serviceName;
    const std::string objectPath;
    const DBusPropertyEndpointMap& targetProperties;
    EntityDBusQueryConstWeakPtr dbusQuery;

    std::map<InstanceHash, DBusInstancePtr> complexInstances;
    std::vector<sdbusplus::bus::match::match> listeners;

  public:
    DBusInstance(const DBusInstance&) = delete;
    DBusInstance& operator=(const DBusInstance&) = delete;
    DBusInstance(DBusInstance&&) = delete;
    DBusInstance& operator=(DBusInstance&&) = delete;

    explicit DBusInstance(
        const std::string& inServiceName, const std::string& inObjectPath,
        const DBusPropertyEndpointMap& targetPropertiesDict,
        const EntityDBusQueryConstWeakPtr& queryObject) noexcept :
        Entity::StaticInstance(Entity::EntityMember::fieldValueNotAvailable),
        serviceName(inServiceName), objectPath(inObjectPath),
        targetProperties(targetPropertiesDict), dbusQuery(queryObject)
    {
        using namespace app::entity::obmc::definitions;
        try
        {
            this->supplement(metaObjectPath, objectPath);
            this->supplement(metaObjectService, serviceName);
        }
        catch (std::logic_error& ex)
        {
            BMC_LOG_ERROR << "Fail to supplement DBusInstance by a default "
                         "metadata fields. Error="
                      << ex.what();
        }
    }

    virtual ~DBusInstance() = default;

    const std::vector<DBusInstancePtr> getComplexInstances() const;

    bool fillMembers(const InterfaceName&, const DBusPropertiesMap&);

    const IEntity::IEntityMember::InstancePtr&
        getField(const IEntity::EntityMemberPtr&) const override;
    const IEntity::IEntityMember::InstancePtr&
        getField(const MemberName&) const override;
    const std::vector<MemberName>
        getMemberNames() const override;

    bool hasField(const MemberName&) const override;
    // TODO(IK) Move to the IFormatter abstractions instead the
    // FindObjectDBusQuery weak pointer.
    const DBusPropertiesMap queryProperties(const connect::DBusConnectUni&,
                                            const InterfaceName&);

    void bindListeners(const connect::DBusConnectUni&);
    const ObjectPath& getObjectPath() const;
    const ServiceName& getService() const;

    void supplement(
        const MemberName&,
        const IEntity::IEntityMember::IInstance::FieldType&) override;

    void supplementOrUpdate(
        const MemberName&,
        const IEntity::IEntityMember::IInstance::FieldType&) override;

    void supplementOrUpdate(const IEntity::InstancePtr&) override;

    bool checkCondition(const IEntity::ConditionPtr) const override;

    std::size_t getHash() const override;
    static std::size_t getHash(const ServiceName&, const ObjectPath&);

    void captureDBusAssociations(const InterfaceName&,
                                 const DBusAssociationsType&);

    void resolveDBusVariant(const MemberName&, const DbusVariantType&);

    const std::map<std::size_t, IEntity::InstancePtr> getComplex() const override;
    bool isComplex() const override;

    void initDefaultFieldsValue() override;
  protected:
    const DBusPropertyMemberDict
        getPropertyMemberDict(const InterfaceName&) const;
};

class DBusMemberInstance final : public IEntity::IEntityMember::IInstance
{
    FieldType value;

  public:
    DBusMemberInstance(const DBusMemberInstance&) = delete;
    DBusMemberInstance& operator=(const DBusMemberInstance&) = delete;
    DBusMemberInstance(DBusMemberInstance&&) = delete;
    DBusMemberInstance& operator=(DBusMemberInstance&&) = delete;

    explicit DBusMemberInstance(FieldType initValue) noexcept : value(initValue)
    {}
    virtual ~DBusMemberInstance() = default;

    const FieldType& getValue() const noexcept override;
    const std::string& getStringValue() const override;
    int getIntValue() const override;
    double getFloatValue() const override;
    bool getBoolValue() const override;
    void setValue(const FieldType&) override;
};

template <class TInstance>
class DBusQuery : public IQuery<TInstance, connect::DBusConnectUni>
{
    std::vector<sdbusplus::bus::match::match> observers;
    static std::map<std::string, std::string> serviceNamesDict;

  public:
    using DefaultValueSetter =
        std::function<IEntity::IEntityMember::IInstance::FieldType()>;
    using DefaultFieldsValueDict = std::map<MemberName, DefaultValueSetter>;

    DBusQuery(const DBusQuery&) = delete;
    DBusQuery& operator=(const DBusQuery&) = delete;
    DBusQuery(DBusQuery&&) = delete;
    DBusQuery& operator=(DBusQuery&&) = delete;

    explicit DBusQuery() = default;
    ~DBusQuery() override = default;

    const DbusVariantType processFormatters(const PropertyName&,
                                            const DbusVariantType&,
                                            DBusInstancePtr) const;

    virtual const DefaultFieldsValueDict& getDefaultFieldsValue() const
    {
        static const DefaultFieldsValueDict emptyDefaultFieldsDict;
        return emptyDefaultFieldsDict;
    }

    void registerObjectCreationObserver(const connect::DBusConnectionPoolUni&,
                                        entity::EntityPtr);
    void registerObjectRemovingObserver(const connect::DBusConnectionPoolUni&,
                                        entity::EntityPtr);

    virtual void supplementByStaticFields(DBusInstancePtr&) const
    {}

    virtual bool checkCriteria(const ObjectPath&,
                               const InterfaceList&,
                               std::optional<ServiceName> = std::nullopt) const = 0;

    static void setServiceName(const std::string, const std::string);
    static const ServiceName& getWellKnownServiceName(const std::string);
  protected:
    using FormatterFn = std::function<const DbusVariantType(
        const PropertyName&, const DbusVariantType&, DBusInstancePtr)>;
    using FieldsFormattingMap =
        std::map<PropertyName, std::vector<FormatterFn>>;

    virtual const FieldsFormattingMap& getFormatters() const;
    virtual const DBusPropertyEndpointMap& getSearchPropertiesMap() const = 0;

    virtual EntityDBusQueryConstWeakPtr getWeakPtr() const = 0;
    virtual EntityDBusQueryPtr getSharedPtr() = 0;

    virtual DBusInstancePtr createInstance(const connect::DBusConnectUni&,
                                           const ServiceName&,
                                           const ObjectPath&,
                                           const InterfaceList&);

    void addObserver(sdbusplus::bus::match::match&&);

    virtual const ObjectPath& getObjectPathNamespace() const = 0;
};

class DBusQueryBuilder final
{
    EntityManager::EntityBuilderPtr entityBuilder;
    EntityPtr entity;
    app::broker::DBusBrokerManager& manager;

  public:
    explicit DBusQueryBuilder(EntityManager::EntityBuilderPtr entityBuilderPtr,
                              EntityPtr targetEntity,
                              app::broker::DBusBrokerManager& dbusManager) :
        entityBuilder(entityBuilderPtr),
        entity(targetEntity), manager(dbusManager)
    {
    }

    virtual ~DBusQueryBuilder() = default;

    template <class TDBusQuery, typename... TArgs>
    DBusQueryBuilder& addObject(TArgs&&... args)
    {
        using namespace app::entity::obmc::definitions;
        static_assert(
            std::is_base_of_v<EntityDBusQuery, TDBusQuery>,
            "This is not a query");
        auto dbusQuery = std::make_shared<TDBusQuery>();
        auto broker = std::make_shared<app::broker::EntityDbusBroker>(
            entity, dbusQuery, args...);
        manager.bind(std::move(broker));
        // default metadata fields
        entityBuilder->addMembers({metaObjectPath, metaObjectService});

        return *this;
    }

    EntityManager::EntityBuilderPtr complete();
};

class FindObjectDBusQuery :
    public EntityDBusQuery,
    public std::enable_shared_from_this<FindObjectDBusQuery>
{
  public:
    struct DBusObjectEndpoint
    {
        const std::string path;
        const std::vector<std::string> interfaces;
        const int32_t depth;
        const std::optional<std::string> service;

        std::size_t getHash() const;
    };

    explicit FindObjectDBusQuery() noexcept : DBusQuery()
    {}
    ~FindObjectDBusQuery() override = default;

    std::vector<IEntity::InstancePtr> process(const connect::DBusConnectUni&) override;

    bool checkCriteria(const ObjectPath&, const InterfaceList&,
                       std::optional<ServiceName> = std::nullopt) const override;

  protected:
    static constexpr int32_t noDepth = 0U;
    static constexpr int32_t nextOneDepth = 1U;

    virtual constexpr const DBusObjectEndpoint& getQueryCriteria() const = 0;

    const ObjectPath& getObjectPathNamespace() const override;
    EntityDBusQueryConstWeakPtr getWeakPtr() const override;
    EntityDBusQueryPtr getSharedPtr() override;
};

class GetObjectDBusQuery :
    public EntityDBusQuery,
    public std::enable_shared_from_this<GetObjectDBusQuery>
{
    const ServiceName serviceName;
    const ObjectPath objectPath;

  public:
    explicit GetObjectDBusQuery(const ServiceName& service,
                                const ObjectPath& path) noexcept :
        EntityDBusQuery(), serviceName(service), objectPath(path)
    {}
    ~GetObjectDBusQuery() override = default;

    std::vector<IEntity::InstancePtr> process(const connect::DBusConnectUni&) override;

    bool checkCriteria(const ObjectPath&, const InterfaceList&,
                       std::optional<ServiceName> = std::nullopt) const override;

  protected:
    const ObjectPath& getObjectPathNamespace() const override;

    virtual const InterfaceList& searchInterfaces() const = 0;

    EntityDBusQueryConstWeakPtr getWeakPtr() const override;
    EntityDBusQueryPtr getSharedPtr() override;
};

class IntrospectServiceDBusQuery :
    public EntityDBusQuery,
    public std::enable_shared_from_this<IntrospectServiceDBusQuery>
{
    const std::string serviceName;

  public:
    explicit IntrospectServiceDBusQuery(
        const ServiceName& serviceNameInput) noexcept :
        DBusQuery(),
        serviceName(serviceNameInput)
    {}
    ~IntrospectServiceDBusQuery() override = default;

    std::vector<IEntity::InstancePtr> process(const connect::DBusConnectUni&) override;

    bool checkCriteria(const ObjectPath&, const InterfaceList&,
                       std::optional<ServiceName> = std::nullopt) const override;

  protected:
    EntityDBusQueryConstWeakPtr getWeakPtr() const override;
    EntityDBusQueryPtr getSharedPtr() override;
    const ObjectPath& getObjectPathNamespace() const override;
};

} // namespace dbus
} // namespace query
} // namespace app
#endif // __QUERY_DBUS_H__
