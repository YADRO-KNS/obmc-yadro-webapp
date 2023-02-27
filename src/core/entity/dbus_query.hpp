// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#pragma once

#include <core/connect/dbus_connect.hpp>
#include <core/entity/entity.hpp>
#include <core/entity/event.hpp>
#include <sdbusplus/bus/match.hpp>

#include <atomic>
#include <functional>
#include <map>
#include <tuple>
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
using namespace app::connect;
using namespace phosphor::logging;

class IFormatter;
class IValidator;

using InterfaceName = std::string;
using ServiceName = std::string;
using PropertyName = std::string;
using ObjectPath = std::string;

using FormatterPtr = std::shared_ptr<IFormatter>;
using ValidatorPtr = std::shared_ptr<IValidator>;
using InterfaceList = std::vector<InterfaceName>;
using DBusServiceInterfaces = std::map<ServiceName, InterfaceList>;
using DBusPropertyFormatters = std::vector<FormatterPtr>;
using DBusPropertyValidators = std::vector<ValidatorPtr>;
using DBusPropertyReflection = std::pair<PropertyName, MemberName>;
using DBusPropertyCasters =
    std::pair<DBusPropertyFormatters, DBusPropertyValidators>;
using DBusPropertySetters =
    std::vector<std::pair<DBusPropertyReflection, DBusPropertyCasters>>;
using DBusPropertyEndpointMap = std::map<InterfaceName, DBusPropertySetters>;

using DBusAssociationsType =
    std::vector<std::tuple<std::string, std::string, std::string>>;
using DbusVariantType =
    std::variant<DBusAssociationsType, std::vector<std::string>,
                 std::vector<double>, std::vector<int>, std::string, int64_t,
                 uint64_t, double, int32_t, uint32_t, int16_t, uint16_t,
                 uint8_t, bool>;
using DBusPropertiesMap = std::map<PropertyName, DbusVariantType>;
using DBusInterfacesMap = std::map<InterfaceName, DBusPropertiesMap>;

using DBusServiceObjects = std::vector<std::pair<ObjectPath, ServiceName>>;

class DBusQuery;
class DBusInstance;
class DBusQueryBuilder;
class FindObjectDBusQuery;

using DBusQueryBuilderUni = std::unique_ptr<DBusQueryBuilder>;
using DBusQueryBuilderPtr = std::shared_ptr<DBusQueryBuilder>;
using FindObjectDBusQueryPtr = std::shared_ptr<FindObjectDBusQuery>;
using DBusInstancePtr = std::shared_ptr<DBusInstance>;
using DBusInstanceWeak = std::weak_ptr<DBusInstance>;

using DBusQueryConstWeakPtr = std::weak_ptr<const DBusQuery>;
using DBusQueryPtr = std::shared_ptr<DBusQuery>;

constexpr const char* metaObjectPath = "__meta_field__object_path";
constexpr const char* metaObjectService = "__meta_field__object_service";

/**
 * @class DBusInstance
 * @brief The DBusInstance provide DBus objects' functions to resolve all
 *        relevant properties and correct resolve/expand own values to IEntity
 *        definition
 */
class DBusInstance final :
    public Entity::StaticInstance,
    public std::enable_shared_from_this<DBusInstance>
{
    /** @brief Type of instance hash */
    using InstanceHash = std::size_t;
    /** @brief The dictionary type of dbus-match watchers */
    using WatchersMap =
        std::map<InstanceHash, std::vector<sdbusplus::bus::match::match>>;
    /** @brief The DBus service of instance */
    const std::string serviceName;
    /** @brief The DBus object path of instace*/
    const std::string objectPath;
    /**
     * @brief The state of instance.
     *        This flag determines whether each target property is obtained or
     *        not.
     */
    bool state;
    /** 
     * @brief Interfaces list that actual for this instance 
     */
    InterfaceList actualInterfaces;
    /**
     * @brief The weak-pointer to the query object that is obtained instace from
     *        dbus
     */
    DBusQueryConstWeakPtr dbusQuery;
    /**
     * @brief Child instances of complex types
     */
    std::map<InstanceHash, DBusInstancePtr> complexInstances;
    /**
     * @brief watchers to observe the object/properties signals to
     *        remove/update
     */
    static WatchersMap listeners;
    /**
     * @brief when instances are removed, it is requried to clean up the dictionary
     *        of registered dbus-match watchers. This dictionary contains those
     *        instances that were already removed but haven't yet been pruned.
     */
    static std::vector<InstanceHash> toCleanupInstances;

    /**
     * @brief The DBusInstances internal resources access guard.
     */
    static std::atomic<std::thread::id> instanceAccessGuard;

  public:
    DBusInstance(const DBusInstance&) = delete;
    DBusInstance& operator=(const DBusInstance&) = delete;
    DBusInstance(DBusInstance&&) = delete;
    DBusInstance& operator=(DBusInstance&&) = delete;

    /**
     * @brief Construct a new DBusInstance object
     *
     * @param inServiceName         - The DBus service name
     * @param inObjectPath          - The DBus object path
     * @param actualInterfaces      - Interfaces list that actual for this
     *                                instance
     * @param queryObject           - The weak-pointer to the query that is
     *                                obtained dbus-object
     */
    explicit DBusInstance(const std::string& inServiceName,
                          const std::string& inObjectPath,
                          const InterfaceList& actualInterfaces,
                          const DBusQueryConstWeakPtr& queryObject,
                          bool state = false) noexcept :
        Entity::StaticInstance(inServiceName + inObjectPath),
        serviceName(inServiceName), objectPath(inObjectPath), state(state),
        actualInterfaces(actualInterfaces), dbusQuery(queryObject)
    {
        log<level::DEBUG>("Create DBus instance cache",
                          entry("DBUS_SVC=%s", inServiceName.c_str()),
                          entry("DBUS_OBJ=%s", inObjectPath.c_str()));
        try
        {
            this->supplement(metaObjectPath, objectPath);
            this->supplement(metaObjectService, serviceName);
        }
        catch (std::logic_error& ex)
        {
            log<level::ERR>("Fail to create new DBus cache instance",
                            entry("DBUS_SVC=%s", inServiceName.c_str()),
                            entry("DBUS_OBJ=%s", inObjectPath.c_str()),
                            entry("ERROR=%s", ex.what()));
        }
    }
    /**
     * @brief Destroy the DBusInstance
     *
     */
    virtual ~DBusInstance()
    {
        markInstanceIsUnavailable(*this);
    }

    /**
     * @brief Get the object instances with expanded complex types(e.g.
     *        associations)
     *
     * @return const std::vector<DBusInstancePtr>
     */
    const std::vector<DBusInstancePtr> getComplexInstances() const;

    /**
     * @brief Fill the object instance by properties map
     *
     * @return true     - sucess
     * @return false    - fail
     */
    bool fillMembers(const InterfaceName&, const DBusPropertiesMap&);

    /**
     * @brief Get the field by entity-member instance
     *
     * @return const IEntity::IEntityMember::InstancePtr&
     */
    const IEntity::IEntityMember::InstancePtr&
        getField(const IEntity::EntityMemberPtr&) const override;

    /**
     * @brief Get the field by name
     *
     * @return const IEntity::IEntityMember::InstancePtr& object of query field
     *                                                    instance
     */
    const IEntity::IEntityMember::InstancePtr&
        getField(const MemberName&) const override;
    /**
     * @brief Get the list of well-known members of instance that is origins
     *        from an IEntity
     *
     * @note  The IEntity defines the set of fields. The each field should be
     *        initialized from predefined source. DBusInstance contains DBus
     *        endpoint configuration that is already having reflection property
     *        name to the entity member name. Thun, we might provide the clean
     *        list of end-IEntity-members from DBusInstance directly.
     *
     * @return const std::vector<MemberName>
     */
    const std::vector<MemberName> getMemberNames() const override;

    /**
     * @brief Check is psecified feild obtained from dbus
     *
     * @return true     - acquired
     * @return false    - not found
     */
    bool hasField(const MemberName&) const override;

    /**
     * @brief Obtain the properties from dbus of the specified interface
     *
     * @param DBusConnectUni             - connection to the DBus
     * @param InterfaceName              - interface to obtain properties
     *
     * @throw std::runtime_error         - if failed to receive properties
     * @return const DBusPropertiesMap   - The dictionary of properties
     */
    const DBusPropertiesMap queryProperties(const connect::DBusConnectUni&,
                                            const InterfaceName&);
    /**
     * @brief Attach an custom watcher of dbus signals
     */
    void bindListeners(const connect::DBusConnectUni&);
    /**
     * @brief Get the DBus object path
     *
     * @return const ObjectPath&  the DBus object path
     */
    const ObjectPath& getObjectPath() const;
    /**
     * @brief Get the DBus service name
     *
     * @return const ServiceName& the DBus service name
     */
    const ServiceName& getService() const;
    /**
     * @brief Suppliment Instance of specified MemberName by the value.
     *        The DBus instance should not having exists specified field
     *
     * @param MemberName    - The field to add to the instance
     * @param FieldType     - The valud to initialize field
     */
    void supplement(
        const MemberName&,
        const IEntity::IEntityMember::IInstance::FieldType&) override;
    /**
     * @brief Update Instance of specified MemberName by the value.
     *        If specified field is not initialized then it will be created.
     *
     * @param MemberName    - The field to add to the instance
     * @param FieldType     - The valud to initialize field
     */
    void supplementOrUpdate(
        const MemberName&,
        const IEntity::IEntityMember::IInstance::FieldType&) override;
    /**
     * @brief Update Instance by copy all members from specified destination
     *        Instance.
     *        If specified field is not initialized then it will be created.
     *
     * @param MemberName    - The field to add to the instance
     * @param FieldType     - The valud to initialize field
     */
    void supplementOrUpdate(const IEntity::InstancePtr&) override;
    /**
     * @brief Merge internal metadata of IInstance that is defined by
     *        implementation.
     *
     * @param InstancePtr    - Destination IInstance to copy metadata from.
     */
    void mergeInternalMetadata(const IEntity::InstancePtr&) override;
    /**
     * @brief Apply ICondition to the Instance to verify is the instance is
     *        relevant to some condition.
     *
     * @return true     - condition passed
     * @return false    - condition not passed
     */
    bool checkCondition(const IEntity::ConditionPtr) const override;
    /**
     * @brief Get the checksum of instance to uniquelly identity object
     *        according IEntity.
     *
     * @return InstanceHash - the checksum
     */
    InstanceHash getHash() const override;
    /**
     * @brief Calculate a checksum origins on the ServiceName and ObjectPath
     *
     * @param ServiceName   - the literal service name
     * @param ObjectPath    - the literal object path
     *
     * @return InstanceHash - the checksum
     */
    static std::size_t getHash(const ServiceName&, const ObjectPath&);
    /**
     * @brief Parse DBus complex type to expand to the DBus associtaion
     *
     * @note The captured accosiation will be stored into complex-instances
     *       dictionary and will be expanded to primitives by special
     *       condition that will defined in the IEntity end-class.
     *
     * @param InterfaceName         - interface name to extract primitives from
     *                                complex
     * @param DBusAssociationsType  - The complex type that is interpert as DBus
     *                                association
     */
    void captureDBusAssociations(const InterfaceName&,
                                 const DBusAssociationsType&);
    /**
     * @brief Passthrow the specified DBusInstance field type to the IEntity
     *        throw casting dbus-variant type to the IEntity general type.
     * @param PropertyName - the field name to resolve type
     * @param PropertyName - the value of specified dbus field
     */
    void resolveDBusVariant(const PropertyName&, const DbusVariantType&);

    /**
     * @brief Get the instances that is extracted from complex dbus-type
     *
     * @return const std::map<std::size_t,
     *                    IEntity::InstancePtr> - map of complex instances
     */
    const std::map<std::size_t, IEntity::InstancePtr>
        getComplex() const override;
    /**
     * @brief Checks is the DBusInstance origins from complex dbus-type
     *
     * @return true     - root instance
     * @return false    - child instance from complex dbus-type
     */
    bool isComplex() const override;
    /**
     * @brief Initialize members by default values
     *        Some fields might required to initailze by predefined value to
     *        avoid provide null-value if configured by endpoint map is not
     *        obtained
     */
    void initDefaultFieldsValue() override;

    /**
     * @brief Clean up dbus-match watchers
     *
     * @note thread-safe
     */
    static void cleanupInstacesWatchers();

    virtual void initialize();

    void verifyState() override;

    void setUninitialized() override
    {
        this->state = false;
    }

  protected:
    void setInitialized()
    {
        this->state = true;
    }

    /**
     * @brief Mark that dbus-match watchers must be down for the specified
     *        DBusInstance.
     *
     * @note thread-safe
     */
    static void markInstanceIsUnavailable(const DBusInstance&);

    /**
     * @brief Get setters for specified interfaces.
     *        The setters dictionary origin from endpoint dictionary
     *
     * @param interface                   - interface name
     * @return const DBusPropertySetters& - setters for the interface properties
     */
    const DBusPropertySetters&
        dbusPropertySetters(const InterfaceName& interface) const;

  private:
    /**
     * @brief Lock access to the DBusInstances internal resources.
     *
     * @note Thread safe, non-blocking (like spin-lock), global scope
     *
     * @return bool - true if locked, false if protection is not required.
     */
    inline static bool instanceAccessGuardLock()
    {
        using namespace std::chrono;
        using namespace std::chrono_literals;

        auto vocation = std::thread::id(0);
        auto currentThreadId = std::this_thread::get_id();

        /* non-blocking guard to prevent onetime access to the
         * `toCleanupInstances` from different threads
         */
        while (!instanceAccessGuard.compare_exchange_strong(
            vocation, currentThreadId, std::memory_order_release))
        {
            if (vocation == currentThreadId)
            {
                return false;
            }
            std::this_thread::sleep_for(10ms);
            vocation = std::thread::id(0);
        }

        return true;
    }

    /**
     * @brief Unlock access to the DBusInstances internal resources.
     *
     * @note Thread safe, non-blocking (like spin-lock), global scope
     *
     */
    inline static void instanceAccessGuardUnlock()
    {
        instanceAccessGuard.store(std::thread::id(0));
    }
};

class DBusQuery : public IQuery, public virtual Event<app::query::QueryEvent>
{
#define DBUS_QUERY_CRIT_IFACES(...) __VA_ARGS__
#define DBUS_QUERY_DECLARE_CRITERIA(p, ifaces, depth, svc)                     \
    const DBusObjectEndpoint& getQueryCriteria() const override                \
    {                                                                          \
        static const DBusObjectEndpoint criteria{                              \
            p,                                                                 \
            {ifaces},                                                          \
            depth,                                                             \
            svc,                                                               \
        };                                                                     \
                                                                               \
        return criteria;                                                       \
    }

#define DBUS_QUERY_EP_CSTR(cstr, ...)                                          \
    {                                                                          \
        std::make_shared<cstr>(__VA_ARGS__)                                    \
    }
#define DBUS_QUERY_EP_SET(p, m, f, v)                                          \
    {                                                                          \
        {p, m}, {f, v},                                                        \
    }
#define DBUS_QUERY_EP_SET_FORMATTERS(p, m, f) DBUS_QUERY_EP_SET(p, m, f, {})
#define DBUS_QUERY_EP_SET_VALIDATORS(p, m, v) DBUS_QUERY_EP_SET(p, m, {}, v)
#define DBUS_QUERY_EP_SET_FORMATTERS2(m, f)                                    \
    DBUS_QUERY_EP_SET_FORMATTERS(m, m, f)
#define DBUS_QUERY_EP_SET_VALIDATORS2(m, v)                                    \
    DBUS_QUERY_EP_SET_VALIDATORS(m, m, v)
#define DBUS_QUERY_EP_SET2(member, f, v) DBUS_QUERY_EP_SET(member, member, f, v)
#define DBUS_QUERY_EP_FIELDS_ONLY(prop, member)                                \
    DBUS_QUERY_EP_SET(prop, member, {}, {})
#define DBUS_QUERY_EP_FIELDS_ONLY2(member)                                     \
    DBUS_QUERY_EP_FIELDS_ONLY(member, member)
#define DBUS_QUERY_EP_IFACES(iface, ...)                                       \
    {                                                                          \
        iface,                                                                 \
        {                                                                      \
            __VA_ARGS__                                                        \
        }                                                                      \
    }

#define DBUS_QUERY_DECL_EP(...)                                                \
    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap()              \
        const override                                                         \
    {                                                                          \
        static const dbus::DBusPropertyEndpointMap dictionary{__VA_ARGS__};    \
        return dictionary;                                                     \
    }
    template <typename TInterfacesDict>
    using CacheUpdatingHandler =
        std::function<bool(const sdbusplus::message::object_path&,
                           const TInterfacesDict&, const std::string&)>;
    template <typename TInterfacesDict>
    using CacheUpdatingHandlers =
        std::vector<CacheUpdatingHandler<TInterfacesDict>>;
    using InstanceCreateHandlers = CacheUpdatingHandlers<DBusInterfacesMap>;
    using InstanceRemoveHandlers = CacheUpdatingHandlers<InterfaceList>;

  public:
    using DefaultValueSetter =
        std::function<IEntity::IEntityMember::IInstance::FieldType(
            const IEntity::InstancePtr&)>;
    using DefaultFieldsValueDict =
        std::unordered_map<MemberName, DefaultValueSetter>;

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

    virtual void supplementByStaticFields(const DBusInstancePtr&) const
    {}

    virtual bool
        checkCriteria(const ObjectPath&, const InterfaceList&,
                      std::optional<ServiceName> = std::nullopt) const = 0;

    virtual bool
        checkCriteria(const ObjectPath&,
                      std::optional<ServiceName> = std::nullopt) const = 0;

    const QueryFields getFields() const override
    {
        const auto& searchEp = getSearchPropertiesMap();
        QueryFields fields;

        for (auto [_, setters] : searchEp)
        {
            for (auto [reflectionIt, _2] : setters)
            {
                fields.emplace_back(reflectionIt.second);
            }
        }

        log<level::DEBUG>("Success to obtaining query fields",
                          entry("COUNT=%ld", fields.size()));
        return std::forward<const QueryFields>(fields);
    }

    /**
     * @brief Get setters for specified interfaces.
     *        The setters dictionary origin from endpoint dictionary
     *
     * @throw std::invalid_argument       - setters not found for the specified
     *                                      interface
     * @param interface                   - interface name
     * @return const DBusPropertySetters& - setters for the interface properties
     */
    const DBusPropertySetters&
        dbusPropertySetters(const InterfaceName& interface) const;

    void configure(std::reference_wrapper<IEntity> entity) override
    {
        registerObjectCreationObserver(entity);
        registerObjectRemovingObserver(entity);
    }

    static void processObjectCreate(sdbusplus::message::message& message);

    static void processObjectRemove(sdbusplus::message::message& message);

    inline void raiseError() const
    {
        this->emitEvent(app::query::QueryEvent::hasFailures);
    }

  protected:
    using FormatterFn = std::function<const DbusVariantType(
        const PropertyName&, const DbusVariantType&, DBusInstancePtr)>;

    const DBusPropertyFormatters&
        getFormatters(const PropertyName& property) const;
    const DBusPropertyValidators&
        getValidators(const PropertyName& property) const;
    virtual const DBusPropertyEndpointMap& getSearchPropertiesMap() const = 0;

    virtual DBusQueryConstWeakPtr getWeakPtr() const = 0;
    virtual DBusQueryPtr getSharedPtr() = 0;

    virtual DBusInstancePtr createInstance(const ServiceName&,
                                           const ObjectPath&,
                                           const InterfaceList&);

    void addObserver(sdbusplus::bus::match::match&&);

    virtual const ObjectPath& getObjectPathNamespace() const = 0;

    void
        registerObjectCreationObserver(std::reference_wrapper<entity::IEntity>);
    void
        registerObjectRemovingObserver(std::reference_wrapper<entity::IEntity>);

    template <typename TInterfacesDict, typename TCacheUpdatingDict>
    static bool processGlobalSignal(sdbusplus::message::message& message,
                                    const TCacheUpdatingDict& handlers);

    InterfaceList introspectDBusObjectInterfaces(
        const ServiceName& sn, const ObjectPath& op,
        const InterfaceList& searchInterfaces) const;

  public:
    const DBusConnectUni& getConnect();
    const DBusConnectUni& getConnect() const;

  private:
    std::vector<sdbusplus::bus::match::match> observers;
    static InstanceCreateHandlers instanceCreateHandlers;
    static InstanceRemoveHandlers instanceRemoveHandlers;
};

class IFormatter
{
  public:
    virtual ~IFormatter() = default;

    virtual const DbusVariantType format(const PropertyName& property,
                                         const DbusVariantType& value) = 0;
};

class IValidator
{
  public:
    virtual ~IValidator() = default;

    virtual const DbusVariantType validate(const PropertyName& property,
                                           const DbusVariantType& value) = 0;
};

class DBusAction : public IQuery
{
  public:
    DBusAction(const DBusAction&) = delete;
    DBusAction& operator=(const DBusAction&) = delete;
    DBusAction(DBusAction&&) = delete;
    DBusAction& operator=(DBusAction&&) = delete;

    explicit DBusAction() = default;
    ~DBusAction() override = default;

    const entity::IEntity::InstanceCollection process() override;
    const QueryFields getFields() const override
    {
        return {};
    }

  protected:
};

class FindObjectDBusQuery :
    public DBusQuery,
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

    const entity::IEntity::InstanceCollection process() override;

    bool
        checkCriteria(const ObjectPath&, const InterfaceList&,
                      std::optional<ServiceName> = std::nullopt) const override;

    bool
        checkCriteria(const ObjectPath&,
                      std::optional<ServiceName> = std::nullopt) const override;

  protected:
    static constexpr int32_t noDepth = 0U;
    static constexpr int32_t nextOneDepth = 1U;

    virtual constexpr const DBusObjectEndpoint& getQueryCriteria() const = 0;

    const ObjectPath& getObjectPathNamespace() const override;
    DBusQueryConstWeakPtr getWeakPtr() const override;
    DBusQueryPtr getSharedPtr() override;
};

class GetObjectDBusQuery :
    public DBusQuery,
    public std::enable_shared_from_this<GetObjectDBusQuery>
{
    const ServiceName serviceName;
    const ObjectPath objectPath;

  public:
    explicit GetObjectDBusQuery(const ServiceName& service,
                                const ObjectPath& path) noexcept :
        DBusQuery(),
        serviceName(service), objectPath(path)
    {}
    ~GetObjectDBusQuery() override = default;

    const entity::IEntity::InstanceCollection process() override;

    bool
        checkCriteria(const ObjectPath&, const InterfaceList&,
                      std::optional<ServiceName> = std::nullopt) const override;

    bool
        checkCriteria(const ObjectPath&,
                      std::optional<ServiceName> = std::nullopt) const override;

  protected:
    const ObjectPath& getObjectPathNamespace() const override;

    virtual const InterfaceList& searchInterfaces() const = 0;

    DBusQueryConstWeakPtr getWeakPtr() const override;
    DBusQueryPtr getSharedPtr() override;
};

class IntrospectServiceDBusQuery :
    public DBusQuery,
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

    const entity::IEntity::InstanceCollection process() override;

    bool
        checkCriteria(const ObjectPath&, const InterfaceList&,
                      std::optional<ServiceName> = std::nullopt) const override;
    bool
        checkCriteria(const ObjectPath&,
                      std::optional<ServiceName> = std::nullopt) const override;

  protected:
    DBusQueryConstWeakPtr getWeakPtr() const override;
    DBusQueryPtr getSharedPtr() override;
    const ObjectPath& getObjectPathNamespace() const override;
};

template <typename TCallResult, typename... Args>
class DBusQueryViaMethodCall :
    public DBusQuery,
    public std::enable_shared_from_this<
        DBusQueryViaMethodCall<TCallResult, Args...>>
{
    using MethodName = std::string;
    using MethodParams = std::tuple<Args...>;
    const ServiceName service;
    const ObjectPath object;
    const InterfaceName interface;
    const MethodName method;
    MethodParams methodParams;

  public:
    explicit DBusQueryViaMethodCall(const ServiceName& service,
                                    const ObjectPath& object,
                                    const InterfaceName& interface,
                                    const MethodName& method,
                                    Args&&... args) noexcept :
        DBusQuery(),
        service(service), object(object), interface(interface), method(method),
        methodParams(MethodParams(args...))
    {}
    ~DBusQueryViaMethodCall() override = default;

    const entity::IEntity::InstanceCollection process() override
    {
        try
        {
            const auto response = this->callDBusWithParams(
                methodParams, std::index_sequence_for<Args...>());
            return populateInstances(response);
        }
        catch (const sdbusplus::exception_t& ex)
        {
            log<level::DEBUG>(
                "Fail to process DBus query (DBusQueryViaMethodCall)",
                entry("DBUS_SVC=%s", this->service.c_str()),
                entry("DBUS_PATH=%s", this->object.c_str()),
                entry("DBUS_INTF=%s", this->interface.c_str()),
                entry("DBUS_MTD=%s", this->method.c_str()),
                entry("ERROR=%s", ex.what()));
            this->raiseError();
        }
        return {};
    }

    bool checkCriteria(
        const ObjectPath&, const InterfaceList&,
        std::optional<ServiceName> = std::nullopt) const override final
    {
        // A criteria checkup required to support cache instances only.
        return false;
    }
    bool checkCriteria(const ObjectPath&, std::optional<ServiceName> =
                                              std::nullopt) const override final
    {
        // A criteria checkup required to support cache instances only.
        return false;
    }
    const DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const DBusPropertyEndpointMap emptyMap{};
        return emptyMap;
    }

  protected:
    template <std::size_t... ISeq>
    const TCallResult callDBusWithParams(const MethodParams& params,
                                         std::index_sequence<ISeq...>)
    {
        return getConnect()->template callMethodAndRead<TCallResult, Args...>(
            this->service, this->object, this->interface, this->method,
            std::get<ISeq>(std::forward<decltype(params)>(params))...);
    }

    virtual IEntity::InstanceCollection
        populateInstances(const TCallResult& yield) = 0;

  protected:
    DBusQueryConstWeakPtr getWeakPtr() const override
    {
        return this->weak_from_this();
    }
    DBusQueryPtr getSharedPtr() override
    {
        return this->shared_from_this();
    }
    const ObjectPath& getObjectPathNamespace() const override
    {
        return this->object;
    }
};
} // namespace dbus
} // namespace query
} // namespace app
