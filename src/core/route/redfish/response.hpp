// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022 YADRO

#pragma once

#include <core/application.hpp>
#include <core/entity/entity_interface.hpp>
#include <core/request.hpp>
#include <core/response.hpp>
#include <nlohmann/json.hpp>

namespace app
{
namespace core
{
namespace redfish
{
using namespace phosphor::logging;
using namespace app::entity;

class RedfishResponse;
using RedfishResponsePtr = std::shared_ptr<RedfishResponse>;

class RedfishResponse : public Response
{
    static constexpr const char* errorField = "error";

  public:
    explicit RedfishResponse(bool prettyOutput) :
        Response(), payload(nlohmann::json({})), prettyOutput(prettyOutput)
    {}
    RedfishResponse(const RedfishResponse&) = delete;
    RedfishResponse(const RedfishResponse&&) = delete;

    RedfishResponse& operator=(const RedfishResponse&) = delete;
    RedfishResponse& operator=(const RedfishResponse&&) = delete;
    ~RedfishResponse() override = default;

    /**
     * @brief The redfish error might be provided via `error` field according to
     *        the http://redfish.dmtf.org/schemas/v1/redfish-error.v1_0_1.json
     *
     * @param[in] message   - The message of error
     */
    void addError(const nlohmann::json&& message);
    /**
     * @brief Each redfish property might having extended message annotation
     *        according to the
     *        http://redfish.dmtf.org/schemas/v1/redfish-payload-annotations.v1_1_1.json
     *
     * @note  The payload annotation is contained at same path of the annotated
     *        property.
     *
     * @param[in] property  - The property to add annotation
     * @param[in] message   - The message of annotation
     */
    void propertyError(const std::string& property,
                       const nlohmann::json&& message);
    /**
     * @brief Add significant REDFISH payload pice.
     *        Topically the root chank of REDFISH payload is json object.
     * @param key - The key of relevant payload chank
     * @param json - The chank of payload
     */
    void add(const std::string& key, const nlohmann::json& json);
    /**
     * @brief Get json of REDFISH payload
     *
     * @return const nlohmann::json - REDFISH payload
     */
    const nlohmann::json getJson() const;
    /**
     * @brief Flash all buffered REDFISH payload to the base response buffer to
     * obtain by next-depending consumers.
     *
     */
    void flash();

  protected:
    void prettyPrintRedfish();

  private:
    /**
     * @brief Internal REDFISH payload buffer
     */
    nlohmann::json payload;
    bool prettyOutput;
};

class ICacheEntity
{
  public:
    virtual ~ICacheEntity() = default;
};

class RedfishContext
{
    /**
     * @class ParameterCtx
     * @brief The REDFISH resource might be parameterized; the class provides
     *        last instances that are obtained by condition over step-by-step of
     *        expanding each URI parameter.
     */
    class ParameterCtx
    {
        IEntity::InstancePtr entityInstance;
        entity::EntityPtr sourceEntity;

      public:
        explicit ParameterCtx() = default;
        ParameterCtx(const IEntity::InstancePtr entityInstance,
                     const entity::EntityPtr sourceEntity) :
            entityInstance(entityInstance),
            sourceEntity(sourceEntity)
        {}
        // ParameterCtx& operator=(ParameterCtx&&) = default;
        ~ParameterCtx() = default;

        /** @brief Parameter instance that is acquired earlier.*/
        const IEntity::InstancePtr getInstance() const;
        /** @brief Entity of parameter instance that is acquired earlier.*/
        const entity::EntityPtr getEntity() const;

        /**
         * @brief Checks whether the parameter context initialized or resource
         *        hasn't condition yet.
         *
         * @return true   - Resource parameterized
         * @return false  - Resource is unconditional
         */
        explicit operator bool() const
        {
            return !!entityInstance && !!sourceEntity;
        }
    };

  public:
    /**
     * @brief Construct a new Redfish Context object
     *
     * @param request
     */
    explicit RedfishContext(const RequestPtr& request) :
        request(request),
        response(std::make_shared<RedfishResponse>(request->isBrowserRequest()))
    {
        initAnchor(request->getUriPath());
    }
    explicit RedfishContext(const RedfishContext& other) :
        request(other.request), response(std::make_shared<RedfishResponse>(
                                    request->isBrowserRequest())),
        parameterCtx(other.parameterCtx), parameters(other.parameters),
        anchor(other.anchor)
    {}
    /**
     * @brief Destroy the Redfish Context object
     *
     */
    ~RedfishContext();
    /**
     * @brief Get the Request object
     *
     * @return const RequestPtr&
     */
    const RequestPtr& getRequest() const;
    /**
     * @brief Get the Response object
     *
     * @return const RedfishResponsePtr&
     */
    const RedfishResponsePtr& getResponse() const;

    /**
     * @brief The REDFISH resource might depend on a specific Entity, the
     *        condition to find the relevant instance must be specified by
     *        REDFISH URI-parameter.
     *
     * @param condition     - condition to obtain corresponding Entity Instance
     * @param paramValue    - The value of the parameter from request
     * @tparam TEntity      - Entity type to search for matching
     * @tparam TParamValue  - The type of parameter value
     * @return true         - Resource valid for specified condition of defined
     *                        Entity
     * @return false        - Instance not found
     */
    template <class TEntity, typename TParamValue = std::string>
    bool verifyParameter(const IEntity::ConditionsList& conditions,
                         const TParamValue& paramValue)
    {
        using namespace app::entity;
        const auto entity = application.getEntityManager().getEntity<TEntity>();
        const auto instances = this->getInstances<TEntity>(conditions);
        if (instances.empty())
        {
            log<level::DEBUG>(
                "RedifshRouter parameter verify: instances not found");
            return false;
        }
        if (instances.size() > 1)
        {
            log<level::WARNING>(
                "Malformed lookup condition to search entity instance, "
                "too many instances found. Will be handled the first entry "
                "only.",
                entry("INSTANCE_COUNT=%lu", instances.size()));
        }
        parameterCtx = ParameterCtx(instances.front(), entity);
        parameters.insert_or_assign(paramValue, parameterCtx);
        return true;
    }

    template <class TEntity>
    const IEntity::InstanceCollection
        getInstances(const auto conditions = IEntity::ConditionsList()) const
    {
        EntityPtr entity = application.getEntityManager().getEntity<TEntity>();
        return this->getInstances(entity, conditions);
    }

    const IEntity::InstanceCollection
        getInstances(const EntityPtr entity,
                     const auto conditions = IEntity::ConditionsList()) const
    {
        if (parameterCtx)
        {
            auto relation =
                parameterCtx.getEntity()->getRelation(entity->getName());
            auto isSameEntity =
                parameterCtx.getEntity()->getName() == entity->getName();
            if (isSameEntity && !relation)
            {
                for (const auto condition : conditions)
                {
                    if (!parameterCtx.getInstance()->checkCondition(condition))
                    {
                        return {};
                    }
                }
                return {parameterCtx.getInstance()};
            }
            if (relation)
            {
                auto instances =
                    parameterCtx.getInstance()->getRelatedInstances(relation,
                                                                    conditions);
                if (instances.empty() && isSameEntity)
                {
                    // Workaround for cyclomatic dependency of single entity
                    // relation by closure.
                    return {parameterCtx.getInstance()};
                }
                return instances;
            }
        }

        return entity->getInstances(conditions);
    }

    inline const std::string encodeUriSegment(std::string value) const
    {
        std::replace(value.begin(), value.end(), ' ', '_');
        return value;
    }

    inline const std::string decodeUriSegment(std::string value) const
    {
        std::replace(value.begin(), value.end(), '_', ' ');
        return value;
    }

    /**
     * @brief Get the Parameter Instance object
     *
     * @param name                - The parameter value
     * @throw std::logic_error    - Thrown if the parameter is not found
     * @return const InstancePtr& - associated with parameter InstancePtr
     */
    const entity::IEntity::InstancePtr
        getParameterInstance(const std::string& name)
    {
        return parameters.at(name).getInstance();
    }

    const std::string getAnchorPath() const
    {
        return anchor;
    }

    void addAnchorSegment(const std::string& segment)
    {
        anchor = anchor + "/" + segment;
    }

  protected:
    inline void initAnchor(const std::string& uri)
    {
        if (uri.ends_with('/'))
        {
            this->anchor = uri.substr(0, uri.length() - 1) + "#";
            return;
        }

        anchor = uri + "#";
    }

  private:
    const RequestPtr request;
    const RedfishResponsePtr response;
    ParameterCtx parameterCtx;
    std::map<std::string, ParameterCtx> parameters;
    std::string anchor;
};

using RedfishContextPtr = std::shared_ptr<RedfishContext>;

} // namespace redfish
} // namespace core
} // namespace app
