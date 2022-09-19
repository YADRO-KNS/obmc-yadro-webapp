// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

/** @generated */
#pragma once

#include <core/route/redfish/node.hpp>
#include <includes.hpp>
#include <nlohmann/json.hpp>
#include <redfish/generated/RedfishAccountService.hpp>
#include <redfish/generated/RedfishCertificateService.hpp>
#include <redfish/generated/RedfishChassis.hpp>
#include <redfish/generated/RedfishEventService.hpp>
#include <redfish/generated/RedfishJsonSchemas.hpp>
#include <redfish/generated/RedfishManagers.hpp>
#include <redfish/generated/RedfishRegistries.hpp>
#include <redfish/generated/RedfishSessionService.hpp>
#include <redfish/generated/RedfishSystems.hpp>
#include <redfish/generated/RedfishTaskService.hpp>
#include <redfish/generated/RedfishUpdateService.hpp>

namespace app
{
namespace core
{
namespace redfish
{
using namespace phosphor::logging;
using namespace app::obmc::entity;
/**
 * @class RedfishV1
 * @brief This Resource represents the root Redfish Service.  All values that
 * this schema describes for Resources shall comply with the Redfish
 * Specification-described requirements.
 * @link http://redfish.dmtf.org/schemas/v1/ServiceRoot.json
 */
class RedfishV1 :
    public Node<RedfishV1, RedfishAccountService, RedfishRegistries,
                RedfishCertificateService, RedfishChassis, RedfishEventService,
                RedfishJsonSchemas, RedfishSessionService, RedfishSystems,
                RedfishTaskService, RedfishUpdateService, RedfishManagers>
{
    static constexpr const char* fieldName = "The BMC service root";
    static constexpr const char* fieldId = "ServiceRoot";
    static constexpr const char* fieldDescription =
        "The ServiceRoot schema describes the root of the Redfish Service, "
        "located at the '/redfish/v1' URI.  All other Resources accessible "
        "through the Redfish interface on this device are linked directly or "
        "indirectly from the Service Root.";
    static constexpr const char* fieldODataType =
        "#ServiceRoot.v1_14_0.ServiceRoot";
    static constexpr const char* fieldODataContext =
        "/redfish/v1/$metadata#ServiceRoot.ServiceRoot";

  public:
    static constexpr const char* segment = "v1";
    RedfishV1(const RedfishContextPtr ctx) :
        Node<RedfishV1, RedfishAccountService, RedfishRegistries,
             RedfishCertificateService, RedfishChassis, RedfishEventService,
             RedfishJsonSchemas, RedfishSessionService, RedfishSystems,
             RedfishTaskService, RedfishUpdateService, RedfishManagers>(ctx)
    {}

    RedfishV1() = delete;
    ~RedfishV1() override = default;

  protected:
    /**
     * @class ServerEntityGetter
     *
     * @brief Getter to provide data that is related to the `Server` entity
     */
    class ServerEntityGetter : public EntityGetter<Server>
    {
      public:
        ServerEntityGetter(const app::entity::IEntity::InstancePtr instance) :
            EntityGetter<Server>(instance)
        {}
        ServerEntityGetter() : EntityGetter<Server>()
        {}
        ~ServerEntityGetter() override = default;

        /**
         * @brief Obtaining child action to populate target part of response
         * @param ctx - Redfish context
         * @return List of actions
         */
        const FieldHandlers childs(const RedfishContextPtr& ctx) const override
        {
            FieldHandlers handlers;
            /** Unique identifier for a service instance.  When SSDP is used,
             * this value should be an exact match of the UUID value returned in
             * a 200 OK from an SSDP M-SEARCH request during discovery. */
            addGetter<ScalarFieldGetter>("UUID", Server::fieldUUID, ctx,
                                         handlers);
            /** The vendor or manufacturer associated with this Redfish Service.
             */
            addGetter<ScalarFieldGetter>(
                "Vendor", general::assets::manufacturer, ctx, handlers);
            return handlers;
        }
        /**
         * @brief Get conditions to obtain relevant Server
         *        instances that is depends on the parent instance or an
         *        enitty relation
         * @return List of entity conditions
         */
        static const entity::IEntity::ConditionsList getStaticConditions([
            [maybe_unused]] const IEntity::InstancePtr instance)
        {
            const entity::IEntity::ConditionsList conditions{};
            return conditions;
        }

        const entity::IEntity::ConditionsList getConditions() const override
        {
            return ServerEntityGetter::getStaticConditions(
                this->getParentInstance());
        }
    };

    /**
     * @class ProtocolFeaturesSupportedFragmentGetter
     *
     * @brief Getter to populate the `ProtocolFeaturesSupported` json object the
     * part of REDFISH response
     */
    class ProtocolFeaturesSupportedFragmentGetter : public ObjectGetter
    {
        const app::entity::IEntity::InstancePtr instance;

      public:
        ProtocolFeaturesSupportedFragmentGetter(
            const std::string& name,
            const app::entity::IEntity::InstancePtr instance) :
            ObjectGetter(name),
            instance(instance)
        {}
        ProtocolFeaturesSupportedFragmentGetter(
            const app::entity::IEntity::InstancePtr instance) :
            ObjectGetter("ProtocolFeaturesSupported"),
            instance(instance)
        {}
        ProtocolFeaturesSupportedFragmentGetter() :
            ObjectGetter("ProtocolFeaturesSupported"), instance()
        {}
        ~ProtocolFeaturesSupportedFragmentGetter() override = default;

      protected:
        const FieldHandlers childs([
            [maybe_unused]] const RedfishContextPtr& ctx) const override
        {
            const FieldHandlers getters{
                /** An indication of whether the service supports the excerpt
                   query parameter. */
                createAction<BoolGetter>("ExcerptQuery", false),
                /** An indication of whether the service supports the $filter
                   query parameter. */
                createAction<BoolGetter>("FilterQuery", false),
                /** An indication of whether the service supports multiple
                   outstanding HTTP requests. */
                createAction<BoolGetter>("MultipleHTTPRequests", true),
                /** An indication of whether the service supports the only query
                   parameter. */
                createAction<BoolGetter>("OnlyMemberQuery", false),
                /** An indication of whether the service supports the $select
                   query parameter. */
                createAction<BoolGetter>("SelectQuery", false),

            };
            return getters;
        }
    };

    /**
     * @class SessionsLinks
     *
     * @brief Getter to provide Link
     */
    class SessionsLinks : public LinkObjectGetter
    {
      public:
        SessionsLinks() :
            LinkObjectGetter("Sessions", "/redfish/v1/SessionService/Sessions")
        {}
        ~SessionsLinks() override = default;

      protected:
        const ParameterResolveDict getParameterConditions([
            [maybe_unused]] const RedfishContextPtr ctx) const override
        {
            const ParameterResolveDict resolvers{};
            return resolvers;
        }
    };

    /**
     * @brief Obtaining child action to populate REDFISH response root object
     * @param ctx - Redfish context
     * @return List of actions
     */
    const FieldHandlers getFieldsGetters() const override
    {
        const FieldHandlers getters{
            /** The general fieldset for each node that are required. */
            /** The unique identifier for a resource */
            createAction<CallableGetter>(
                nameFieldODataID, std::bind(&RedfishV1::getOdataId, this)),
            /** The type of a resource */
            createAction<StringGetter>(nameFieldODataType, fieldODataType),
            /** The unique identifier for this resource within the collection of
               similar resources */
            createAction<StringGetter>(nameFieldId, fieldId),
            /** The name of the resource or array member */
            createAction<StringGetter>(nameFieldName, fieldName),
            /** The OData description of a payload */
            createAction<StringGetter>(nameFieldODataContext,
                                       fieldODataContext),
            /** The description of this resource. Used for commonality in the
               schema definitions */
            createAction<StringGetter>(nameFieldDescription, fieldDescription),
            /** The reference to next related nodes */
            createAction<IdReference<RedfishAccountService>>("AccountService"),
            createAction<IdReference<RedfishRegistries>>("Registries"),
            createAction<IdReference<RedfishCertificateService>>(
                "CertificateService"),
            createAction<IdReference<RedfishChassis>>("Chassis"),
            createAction<IdReference<RedfishEventService>>("EventService"),
            createAction<IdReference<RedfishJsonSchemas>>("JsonSchemas"),
            createAction<IdReference<RedfishSessionService>>("SessionService"),
            createAction<IdReference<RedfishSystems>>("Systems"),
            createAction<IdReference<RedfishTaskService>>("Tasks"),
            createAction<IdReference<RedfishUpdateService>>("UpdateService"),
            createAction<IdReference<RedfishManagers>>("Managers"),
            /** The links to associated nodes */
            createAction<LinksGetter<SessionsLinks>>("Links"),
            /** The RelatedItem to associated nodes */
            /** The node static-initialized fields */
            /** The version of the Redfish Service. */
            createAction<StringGetter>("RedfishVersion", "1.14.0"),
            createAction<ServerEntityGetter>(),
            createAction<ProtocolFeaturesSupportedFragmentGetter>(),

        };
        return getters;
    }

    /**
     * @brief Override base methodGet to provide self-logic of RedfishV1 node
     * handler
     */
    void methodGet() const override
    {
        ctx->getResponse()->setStatus(http::statuses::Code::OK);
        Node::methodGet();
    }
};

} // namespace redfish
} // namespace core
} // namespace app
