// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

#pragma once

#include <core/application.hpp>
#include <core/route/redfish/error_messages.hpp>
#include <core/route/redfish/response.hpp>
#include <http/headers.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <type_traits>
#include <vector>

namespace app
{
namespace core
{
namespace redfish
{
class INode
{
  public:
    virtual ~INode() = default;
    virtual void process() = 0;
};

class IParameterizedNode
{
  public:
    virtual ~IParameterizedNode() = default;

    virtual const std::string getParameterValue() const = 0;
    virtual const entity::IEntity::ConditionsList
        getEntityCondition() const = 0;
    virtual const entity::IEntity::InstancePtr getTargetInstance() const = 0;
};

/**
 * @class IStaticSegments
 * @brief The 'Node' may have several static segments that might be found in the
 *        path that leads before main node semgnet. This workaround is necessary
 *        to provide a node URI that contains segments that do not accurate any
 *        defined node handler.
 *        For example, the path `/redfish/v1/FooNode/BarNode` may having Node
 *        handler for `BarNode`, and `FooNode` may avoid relevant handler in
 *        reason `FooNode` is not real REDFISH endpoint.
 */
class IStaticSegments
{
  public:
    virtual ~IStaticSegments() = default;

    template <typename TNode>
    static inline bool verifyStaticSegments(const RedfishContextPtr ctx,
                                            size_t& depth)
    {
        const auto& pathInfo = ctx->getRequest()->environment().pathInfo;
        if (pathInfo.size() <= depth)
        {
            return false;
        }

        size_t const depthToVerifyStaticSegments = depth;
        size_t segIndex = 0;
        for (const auto& prefix : TNode::uriPrefix)
        {
            auto index = depthToVerifyStaticSegments + segIndex;
            if (pathInfo.size() <= index)
            {
                return false;
            }
            const auto& segmentToVerify = pathInfo[index];
            if (segmentToVerify != prefix)
            {
                return false;
            }
            segIndex++;
        }
        depth += segIndex;
        return (pathInfo.size() > depth);
    }

    template <typename TNode>
    static inline const std::string prefix(const std::string& segment)
    {
        std::ostringstream prefix;
        std::copy(TNode::uriPrefix.begin(), TNode::uriPrefix.end(),
                  std::ostream_iterator<std::string>(prefix, "/"));
        return prefix.str() + segment;
    }
};

template <typename TSelf>
class ParameterizedNode : public IParameterizedNode
{
    const RedfishContextPtr pnCtx;
    const std::string value;

  public:
    ParameterizedNode() = delete;

    explicit ParameterizedNode(const RedfishContextPtr& ctx) :
        pnCtx(ctx), value(ctx->getRequest()->environment().pathInfo.back())
    {}
    ~ParameterizedNode() override = default;

    const std::string getParameterValue() const override
    {
        return pnCtx->decodeUriSegment(value);
    }

    const entity::IEntity::InstancePtr getTargetInstance() const override
    {
        const auto instances =
            pnCtx->getInstances<typename TSelf::TParameterEntity>(
                this->getEntityCondition());
        if (instances.empty())
        {
            throw std::runtime_error("No instance found for requested node");
        }
        if (instances.size() > 1)
        {
            log<level::WARNING>(
                "Malformed lookup condition to search entity instance,  too "
                "many "
                "instances found. Will be returned first entry only.",
                entry("INSTANCE_COUNT=%lu", instances.size()));
        }
        return instances.front();
    }

    const IEntity::ConditionsList getEntityCondition() const override
    {
        return std::forward<IEntity::ConditionsList>(
            TSelf::getConditions(this->pnCtx, this->getValue()));
    }

  protected:
    const std::string getValue() const
    {
        return value;
    }
};

class NodeNotFound : public INode
{
    RedfishContextPtr ctx;

  public:
    explicit NodeNotFound(const RedfishContextPtr& ctx) : ctx(ctx)
    {}
    ~NodeNotFound() override = default;
    void process() override
    {
        messages::uriNotFound(ctx);
    }
};

template <typename TSelf, typename... TChilds>
class Node : public INode
{
  public:
    Node() = delete;
    Node(const Node&) = delete;
    Node(const Node&&) = delete;

    Node& operator=(const Node&) = delete;
    Node& operator=(const Node&&) = delete;

    explicit Node(const RequestPtr& request) :
        ctx(std::make_shared<RedfishContext>(request))
    {}
    explicit Node(const RedfishContextPtr ctx) : ctx(ctx)
    {}
    ~Node() override = default;

    template <typename TChild, typename... TNextChilds,
              typename = std::enable_if_t<sizeof...(TNextChilds) != 0>>
    static std::shared_ptr<INode> nextChildResolver(const RedfishContextPtr ctx,
                                                    size_t depth)
    {
        if (TChild::validateSegment(ctx, depth))
        {
            if (TChild::matchEntirePattern(ctx, depth))
            {
                return std::make_shared<TChild>(ctx);
            }
            else
            {
                return TChild::uriResolver(ctx, depth);
            }
        }
        return nextChildResolver<TNextChilds...>(ctx, depth);
    }

    template <typename TChild>
    static std::shared_ptr<INode> nextChildResolver(const RedfishContextPtr ctx,
                                                    size_t depth)
    {
        if (TChild::validateSegment(ctx, depth))
        {
            if (TChild::matchEntirePattern(ctx, depth))
            {
                return std::make_shared<TChild>(ctx);
            }
            else
            {
                return TChild::uriResolver(ctx, depth);
            }
        }
        return std::make_shared<NodeNotFound>(ctx);
    }

    static std::shared_ptr<INode> uriResolver(const RedfishContextPtr ctx,
                                              size_t depth = 0)
    {
        constexpr std::size_t childsCount = sizeof...(TChilds);
        if (validateSegment(ctx, depth))
        {
            if (matchEntirePattern(ctx, depth))
            {
                return std::make_shared<TSelf>(ctx);
            }
        }
        if constexpr (childsCount > 0)
        {
            return nextChildResolver<TChilds...>(ctx, depth + 1);
        }
        return std::make_shared<NodeNotFound>(ctx);
    }

    void process() override
    {
        switch (ctx->getRequest()->environment().requestMethod)
        {
            case RequestMethod::HEAD:
                methodHead();
                break;
            case RequestMethod::GET:
                methodGet();
                break;
            case RequestMethod::POST:
                methodPost();
                break;
            case RequestMethod::PUT:
                methodPut();
                break;
            case RequestMethod::DELETE:
                methodDelete();
                break;
            case RequestMethod::OPTIONS:
                methodOptions();
                break;
            default:
                methodNotAllowed();
        }
    }

    static bool matchEntirePattern(const RedfishContextPtr ctx, size_t depth)
    {
        return depth == (ctx->getRequest()->environment().pathInfo.size() - 1);
    }

    static bool validateSegment(const RedfishContextPtr ctx, size_t& depth)
    {
        const auto& pathInfo = ctx->getRequest()->environment().pathInfo;
        if (pathInfo.size() <= depth)
        {
            return false;
        }
        if constexpr (std::is_base_of_v<IStaticSegments, TSelf>)
        {
            if (!IStaticSegments::verifyStaticSegments<TSelf>(ctx, depth))
            {
                return false;
            }
        }
        const auto segmentToVerify = pathInfo.at(depth);
        if constexpr (std::is_base_of_v<IParameterizedNode, TSelf>)
        {
            return TSelf::matchParameter(ctx, segmentToVerify);
        }
        else
        {
            return segmentToVerify == TSelf::segment;
        }
        return false;
    }

    static const std::vector<std::string>
        getAllSegmentValues(const RedfishContextPtr ctx)
    {
        std::vector<std::string> segments;
        if constexpr (std::is_base_of_v<IParameterizedNode, TSelf>)
        {
            const auto instances =
                ctx->getInstances<typename TSelf::TParameterEntity>(
                    TSelf::getStaticConditions());
            for (const auto& instance : instances)
            {
                const auto fieldValue =
                    instance->getField(TSelf::parameterField)->getStringValue();
                segments.emplace_back(ctx->encodeUriSegment(fieldValue));
            }
        }
        else
        {
            segments.emplace_back(TSelf::segment);
        }
        return segments;
    }

  protected:
    const std::string getOdataId() const
    {
        return ctx->getRequest()->getUriPath();
    }

  protected:
    class IAction;
    using ActionPtr = std::shared_ptr<IAction>;
    using FieldHandlerFn = std::function<void(const RedfishContextPtr& ctx)>;
    using FieldHandlers = std::vector<ActionPtr>;
    using Actions = std::map<const char*, ActionPtr>;

    /** Defined bellow fields are required by DMTF DSP0266_1.15.1: 9.1
     * Resources */
    static constexpr const char* nameFieldODataID = "@odata.id";
    static constexpr const char* nameFieldODataType = "@odata.type";
    static constexpr const char* nameFieldId = "Id";
    static constexpr const char* nameFieldName = "Name";
    /** The next two fields are not required, but we define rule that fields
     *  will be contained in each redfish node. */
    static constexpr const char* nameFieldODataContext = "@odata.context";
    static constexpr const char* nameFieldODataCount = "@odata.count";
    static constexpr const char* nameFieldDescription = "Description";

    class IAction
    {
      public:
        virtual ~IAction() = default;
        virtual void process(const RedfishContextPtr& ctx) = 0;
        virtual const std::string& getFieldName() const = 0;
    };

    class IComplexAction
    {
      public:
        virtual ~IComplexAction() = default;

      protected:
        virtual const FieldHandlers
            childs(const RedfishContextPtr& ctx) const = 0;
    };

    template <typename TField>
    class FieldGetter : public IAction
    {
        const std::string field;
        const TField value;

      public:
        using FieldType = TField;

        FieldGetter(const std::string&& field, const TField&& value) :
            field(field), value(value)
        {}
        FieldGetter(const std::string& field, const TField& value) :
            field(field), value(value)
        {}
        ~FieldGetter() override = default;
        void process(const RedfishContextPtr& ctx) override
        {
            if constexpr (std::is_invocable_v<TField>)
            {
                ctx->getResponse()->add(field, std::invoke(value));
            }
            else
            {
                ctx->getResponse()->add(field, value);
            }
        }

        const std::string& getFieldName() const override
        {
            return field;
        }
    };

    template <typename TEnumSource>
    class EnumGetter : public IAction
    {
        const std::string field;
        TEnumSource value;

      public:
        using FieldType = int;
        EnumGetter(const std::string& field, int value) :
            field(field), value(static_cast<TEnumSource>(value))
        {}
        ~EnumGetter() override = default;
        void process(const RedfishContextPtr& ctx) override
        {
            try
            {
                ctx->getResponse()->add(field, castEnumToString());
            }
            catch (const std::exception& e)
            {
                log<level::DEBUG>(
                    "Fail to set enum field into response redfish json",
                    entry("FIELD=%s", field.c_str()),
                    entry("ERROR=%s", e.what()));
                ctx->getResponse()->add(field, nullptr);
            }
        }

        const std::string& getFieldName() const override
        {
            return field;
        }

      protected:
        TEnumSource getValue() const
        {
            return value;
        }

        virtual std::string castEnumToString() const = 0;
    };

    using CallableGetter = FieldGetter<std::function<std::string()>>;
    using StringGetter = FieldGetter<std::string>;
    using NullGetter = FieldGetter<nullptr_t>;
    using BoolGetter = FieldGetter<bool>;
    using DecimalGetter = FieldGetter<int64_t>;
    using FloatGetter = FieldGetter<float_t>;

    class ObjectGetter : public IAction, public IComplexAction
    {
        static constexpr const char* dummyFieldName = "_";
        const std::string field;

      public:
        ObjectGetter() : field(dummyFieldName)
        {}
        explicit ObjectGetter(const std::string& field) : field(field)
        {}
        ~ObjectGetter() override = default;

        void process(const RedfishContextPtr& ctx) override
        {
            auto mockContext = std::make_shared<RedfishContext>(*ctx.get());
            mockContext->addAnchorSegment(field);
            const auto childs = this->childs(mockContext);
            for (const auto valueGetter : childs)
            {
                valueGetter->process(mockContext);
            }
            // Skip empty objects.
            const auto& result = mockContext->getResponse()->getJson();
            if (!result.empty())
            {
                ctx->getResponse()->add(field, result);
            }
            mockContext.reset();
        }

        const std::string& getFieldName() const override
        {
            return field;
        }
    };

    class StaticCollectionGetter : public IAction, public IComplexAction
    {
        static constexpr const char* dummyFieldName = "_";
        const std::string field;

      public:
        StaticCollectionGetter() : field(dummyFieldName)
        {}
        explicit StaticCollectionGetter(const std::string& field) : field(field)
        {}
        ~StaticCollectionGetter() override = default;

        void process(const RedfishContextPtr& ctx) override
        {
            nlohmann::json::array_t array({});
            auto mockComplexContext =
                std::make_shared<RedfishContext>(*ctx.get());
            mockComplexContext->addAnchorSegment(field);
            for (const auto valueGetter : this->childs(mockComplexContext))
            {
                auto mockContext =
                    std::make_shared<RedfishContext>(*mockComplexContext.get());
                valueGetter->process(mockContext);
                const auto payload =
                    mockContext->getResponse()
                        ->getJson()[valueGetter->getFieldName()];
                array.emplace_back(payload);
            }
            std::sort(array.begin(), array.end());
            ctx->getResponse()->add(field, array);
        }

        const std::string& getFieldName() const override
        {
            return field;
        }
    };

    class CollectionSizeAnnotation : public IAction
    {
        const std::string itemFieldName;
        const std::string annotationFieldName;

      public:
        explicit CollectionSizeAnnotation(const std::string& field) :
            itemFieldName(field),
            annotationFieldName(field + nameFieldODataCount)
        {}
        ~CollectionSizeAnnotation() override = default;

        void process(const RedfishContextPtr& ctx) override
        {
            const auto& payload = ctx->getResponse()->getJson();
            if (payload[itemFieldName].is_null() ||
                !payload[itemFieldName].is_array())
            {
                throw std::runtime_error(
                    "Can't get collection size of none array type");
            }
            auto size = payload[itemFieldName].size();
            ctx->getResponse()->add(getFieldName(), size);
        }
        const std::string& getFieldName() const override
        {
            return annotationFieldName;
        }
    };

    template <typename TEntity, typename TParentEntity = TEntity>
    class EntityGetter : public IAction, public IComplexAction
    {
        const app::entity::IEntity::InstancePtr parentInstance;

      public:
        using EntityType = TEntity;

        EntityGetter() = default;
        explicit EntityGetter(
            const app::entity::IEntity::InstancePtr& instance) :
            parentInstance(instance)
        {}
        ~EntityGetter() override = default;

        void process(const RedfishContextPtr& ctx) override
        {
            for (const auto valueGetter : this->childs(ctx))
            {
                valueGetter->process(ctx);
            }
        }

        const std::string& getFieldName() const override
        {
            throw std::runtime_error("Not implemented");
        }

        template <class TSource>
        static const entity::IEntity::InstanceCollection
            getInstances(const app::entity::IEntity::InstancePtr parentInstance,
                         const entity::IEntity::ConditionsList conditions =
                             entity::IEntity::ConditionsList())
        {
            if constexpr (!std::is_same_v<TEntity, TSource>)
            {
                if (parentInstance)
                {
                    const auto parentEntity =
                        application.getEntityManager().getEntity<TSource>();
                    const auto relation =
                        parentEntity->getRelation(entity()->getName());
                    if (relation)
                    {
                        return parentInstance->getRelatedInstances(relation,
                                                                   conditions);
                    }
                }
                return entity()->getInstances(conditions);
            }
            if (parentInstance)
            {
                return {parentInstance};
            }
            return entity()->getInstances(conditions);
        }

        static const entity::EntityPtr entity()
        {
            return application.getEntityManager().getEntity<TEntity>();
        }

      protected:
        template <typename T>
        struct CastFnTraits;

        template <typename R, typename TArgCastFrom>
        struct CastFnTraits<R(TArgCastFrom)>
        {
            using CastToType = typename std::remove_reference<R>::type;
            using CastFromType =
                typename std::remove_reference<TArgCastFrom>::type;
        };
        class ScalarFieldGetter : public IAction
        {
            const std::string field;
            const std::string sourceField;
            const app::entity::IEntity::InstancePtr instance;

          public:
            ScalarFieldGetter(
                const std::string& field, const std::string& sourceField,
                const app::entity::IEntity::InstancePtr instance) :
                field(field),
                sourceField(sourceField), instance(instance)
            {}
            ~ScalarFieldGetter() override = default;

            void process(const RedfishContextPtr& ctx) override
            {
                const auto valVisitor = [ctx, this](auto&& value) {
                    if constexpr (std::is_same_v<std::string,
                                                 std::decay_t<decltype(value)>>)
                    {
                        if (value.empty())
                        {
                            return;
                        }
                    }
                    ctx->getResponse()->add(field, value);
                };
                try
                {
                    const auto member =
                        EntityGetter::entity()->getMember(sourceField);
                    if (instance && !instance->getField(member)->isNull())
                    {
                        std::visit(std::move(valVisitor),
                                   instance->getField(member)->getValue());
                        return;
                    }
                }
                catch (const std::runtime_error& e)
                {
                    log<level::DEBUG>(
                        "Error while getting field value",
                        entry("ERROR=%s", e.what()),
                        entry("FIELD=%s", this->getFieldName().c_str()));
                    messages::internalError(ctx, this->getFieldName());
                }
                valVisitor(nullptr);
            }
            const std::string& getFieldName() const override
            {
                return field;
            }

          protected:
            const std::string& getSourceField() const
            {
                return sourceField;
            }

            const app::entity::IEntity::InstancePtr getInstance() const
            {
                return instance;
            }
        };

        template <typename TEnumGetter>
        class EntityEnumFieldGetter : public ScalarFieldGetter
        {
          public:
            EntityEnumFieldGetter(
                const std::string& field, const std::string& sourceField,
                const app::entity::IEntity::InstancePtr instance) :
                ScalarFieldGetter(field, sourceField, instance)

            {}
            ~EntityEnumFieldGetter() override = default;

            void process(const RedfishContextPtr& ctx) override
            {
                const auto valVisitor = [ctx, this](auto&& value) {
                    using TValue = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<TValue, int>)
                    {
                        TEnumGetter getter(this->getFieldName(), value);
                        getter.process(ctx);
                    }
                    else
                    {
                        if constexpr (!std::is_same_v<TValue, std::nullptr_t>)
                        {
                            throw std::runtime_error("Unknown field type: " +
                                                     this->getSourceField());
                        }
                        else
                        {
                            ctx->getResponse()->add(this->getFieldName(),
                                                    value);
                        }
                    }
                };
                try
                {
                    const auto member = EntityGetter::entity()->getMember(
                        this->getSourceField());
                    if (this->getInstance())
                    {
                        const auto field =
                            this->getInstance()->getField(member);
                        log<level::DEBUG>(
                            "Obtaining entity enum field.",
                            entry("FIELD=%s", this->getSourceField().c_str()),
                            entry("FTYPE=%s", field->getType().c_str()));
                        if (!field->isNull())
                        {
                            std::visit(std::move(valVisitor),
                                       this->getInstance()
                                           ->getField(member)
                                           ->getValue());
                            return;
                        }
                    }
                }
                catch (std::exception& e)
                {
                    log<level::DEBUG>(
                        "Error while getting enum field value",
                        entry("ERROR=%s", e.what()),
                        entry("FIELD=%s", this->getFieldName().c_str()));
                    messages::internalError(ctx, this->getFieldName());
                }
                valVisitor(nullptr);
            }
        };

        template <typename TSubTypeGetter>
        class ListFieldGetter : public StaticCollectionGetter
        {
            template <class TVal, class... TItOf>
            using IsIterable =
                std::disjunction<std::is_same<std::vector<TItOf>, TVal>...>;

            const std::string sourceField;
            const app::entity::IEntity::InstancePtr instance;

          public:
            ListFieldGetter(const std::string& field,
                            const std::string& sourceField,
                            const app::entity::IEntity::InstancePtr instance) :
                StaticCollectionGetter(field),
                sourceField(sourceField), instance(instance)
            {}
            ~ListFieldGetter() override = default;

          protected:
            const FieldHandlers
                childs(const RedfishContextPtr& ctx) const override
            {
                const auto valVisitor = [ ctx, this ](auto&& values) -> auto
                {
                    using TVal = std::decay_t<decltype(values)>;
                    using TFieldGetter = typename TSubTypeGetter::FieldType;
                    using TIterator = std::vector<TFieldGetter>;

                    FieldHandlers getters;

                    if constexpr (std::is_same_v<TIterator, TVal>)
                    {
                        for (auto value : values)
                        {
                            getters.emplace_back(
                                std::make_shared<TSubTypeGetter>(
                                    this->getFieldName(), value));
                        }
                    }
                    return getters;
                };
                try
                {
                    return std::visit(
                        std::move(valVisitor),
                        instance->getField(sourceField)->getValue());
                }
                catch (const std::exception& e)
                {
                    log<level::DEBUG>(
                        "Error while getting field value to list getter "
                        "populating",
                        entry("FIELD=%s", this->getFieldName().c_str()),
                        entry("ERROR=%s", e.what()),
                        entry("ENTITY_FIELD=%s", sourceField.c_str()));
                    messages::internalError(ctx, this->getFieldName());
                }
                return {};
            }
        };

        template <typename TGetter, typename... Args>
        inline void addGetter(const std::string& field,
                              const std::string& sourceField,
                              const RedfishContextPtr ctx,
                              FieldHandlers& handlers, Args... args) const
        {
            auto const instances = getInstances();
            if (instances.empty())
            {
                log<level::DEBUG>(
                    "Error while getting field value",
                    entry("FIELD=%s", field.c_str()),
                    entry("ERROR=No instance found to obtain field value"),
                    entry("ENTITY_FIELD=%s", sourceField.c_str()));
                messages::internalError(ctx, field);
                return;
            }
            handlers.emplace_back(std::make_shared<TGetter>(
                field, sourceField, instances.front(), args...));
        }

        virtual const entity::IEntity::ConditionsList getConditions() const
        {
            return {};
        }

        const entity::IEntity::InstanceCollection getInstances() const
        {
            return this->getInstances<TParentEntity>(this->parentInstance,
                                                     this->getConditions());
        }

        const entity::IEntity::InstancePtr getParentInstance() const
        {
            return this->parentInstance;
        }
    };

    template <typename TFragment, typename TSourceEntity,
              typename TParentEntity = TSourceEntity>
    class CollectionGetter : public StaticCollectionGetter
    {
        const app::entity::IEntity::InstancePtr parentInstance;

      public:
        CollectionGetter(const std::string& fieldName,
                         const app::entity::IEntity::InstancePtr& instance) :
            StaticCollectionGetter(fieldName),
            parentInstance(instance)
        {}
        explicit CollectionGetter(const std::string& fieldName) :
            StaticCollectionGetter(fieldName), parentInstance()
        {}
        ~CollectionGetter() override = default;

      protected:
        const FieldHandlers childs(const RedfishContextPtr&) const override
        {
            FieldHandlers getters;
            const auto instances =
                EntityGetter<TSourceEntity>::template getInstances<
                    TParentEntity>(parentInstance, this->getConditions());
            size_t index = 0;
            for (const auto targetInstance : instances)
            {
                getters.emplace_back(std::make_shared<TFragment>(
                    std::to_string(index), targetInstance));
                index++;
            }
            return getters;
        }

        virtual const entity::IEntity::ConditionsList getConditions() const
        {
            return {};
        }
    };

    template <typename TFragmentType = ObjectGetter>
    class LinkGetter : public TFragmentType
    {
      protected:
        using Parameter = std::string;
        using ParameterConditions =
            std::tuple<entity::EntityPtr, entity::MemberName,
                       entity::IEntity::ConditionsList,
                       entity::IEntity::InstancePtr>;
        using ParameterResolveDict = std::map<Parameter, ParameterConditions>;

      private:
        const std::string uriTmpl;

        class LinkRefObject : public ObjectGetter
        {
            const std::string uri;

          public:
            explicit LinkRefObject(const std::string& uri) :
                ObjectGetter(nameFieldODataID), uri(uri)
            {}
            ~LinkRefObject() override = default;

          protected:
            const FieldHandlers childs(const RedfishContextPtr&) const override
            {
                const FieldHandlers getters{
                    createAction<StringGetter>(nameFieldODataID, uri),
                };
                return getters;
            }
        };

      public:
        LinkGetter(const std::string& name, const std::string& uriTmpl) :
            TFragmentType(name), uriTmpl(uriTmpl)
        {
            static_assert(
                std::disjunction_v<
                    std::is_same<TFragmentType, ObjectGetter>,
                    std::is_same<TFragmentType, StaticCollectionGetter>>,
                "Invalid TFragmentType definition");
        }
        ~LinkGetter() override = default;

      protected:
        const FieldHandlers childs(const RedfishContextPtr& ctx) const override
        {
            FieldHandlers getters;
            const auto uris = this->resolveTemplate(ctx);
            if constexpr (std::is_base_of_v<ObjectGetter, TFragmentType>)
            {
                if (uris.empty())
                {
                    messages::internalError(ctx, this->getFieldName());
                    return getters;
                }
                else if (uris.size() > 1)
                {
                    log<level::WARNING>(
                        "Malformed condition to resolve uri link. Acquired "
                        "too "
                        "many uris, exactly one must be associated.",
                        entry("LINKTMPL=%s", uriTmpl.c_str()),
                        entry("COUNT=%d", uris.size()));
                }
                getters.emplace_back(
                    createAction<StringGetter>(nameFieldODataID, uris.front()));
            }
            else
            {
                for (const auto& uri : uris)
                {
                    getters.emplace_back(createAction<LinkRefObject>(uri));
                }
            }
            return getters;
        }

        virtual const ParameterResolveDict
            getParameterConditions(const RedfishContextPtr) const
        {
            // Hasn't parameters by default
            return {};
        }

        using ResolvedTmpls =
            std::map<std::string, std::pair<entity::IEntity::InstancePtr,
                                            entity::EntityPtr>>;
        /**
         * @brief Resolive parameters and format template-uri to full path to
         *        the corresponding resource
         * @note  The default implementation passthrough template for the
         *        unparameterized link
         * @param ctx - The REDFISH context
         * @return The list of valid REDFISH resource URI
         */
        virtual const std::vector<std::string>
            resolveTemplate(const RedfishContextPtr ctx) const
        {
            std::vector<std::string> uris;
            const auto resolvers = this->getParameterConditions(ctx);
            if (resolvers.empty())
            {
                uris.emplace_back(uriTmpl);
                return uris;
            }
            ResolvedTmpls partiallyResolvedTmpls;
            for (const auto& [param, resolver] : resolvers)
            {
                ResolvedTmpls currentTmpls;
                const auto& [entity, mn, cond, depInst] = resolver;
                if (partiallyResolvedTmpls.empty())
                {
                    entity::IEntity::InstanceCollection instances;
                    if (depInst)
                    {
                        instances.emplace_back(depInst);
                    }
                    else
                    {
                        instances = ctx->getInstances(entity, cond);
                    }
                    currentTmpls = resolveParameter(ctx, uriTmpl, param, mn,
                                                    entity, instances);
                    if (currentTmpls.empty())
                    {
                        return uris;
                    }
                }
                else
                {
                    for (const auto& [tmpl, context] : partiallyResolvedTmpls)
                    {
                        entity::IEntity::InstanceCollection instances;
                        if (depInst)
                        {
                            instances.emplace_back(depInst);
                        }
                        else
                        {
                            const auto [pInstance, pEntity] = context;
                            const auto relation =
                                pEntity->getRelation(entity->getName());
                            if (!relation)
                            {
                                instances = entity->getInstances(cond);
                            }
                            else
                            {
                                instances = pInstance->getRelatedInstances(
                                    relation, cond);
                            }
                        }
                        currentTmpls.merge(resolveParameter(
                            ctx, tmpl, param, mn, entity, instances));
                    }
                }
                partiallyResolvedTmpls.swap(currentTmpls);
            }
            for (const auto it : partiallyResolvedTmpls)
            {
                uris.emplace_back(it.first);
            }
            return uris;
        }

        template <typename TEntity>
        static const ParameterConditions
            buildCondition(const MemberName& mn,
                           const IEntity::ConditionsList&& conditions)
        {
            return std::make_tuple(entity<TEntity>(), mn, conditions,
                                   IEntity::InstancePtr());
        }

        template <typename TEntity>
        static const ParameterConditions
            buildCondition(const MemberName& mn,
                           const IEntity::InstancePtr instance)
        {
            return std::make_tuple(entity<TEntity>(), mn,
                                   IEntity::ConditionsList(), instance);
        }

      private:
        static ResolvedTmpls resolveParameter(
            const RedfishContextPtr ctx, const std::string& tmpl,
            const Parameter& paramName, const entity::MemberName& memberName,
            const entity::EntityPtr& entity,
            const entity::IEntity::InstanceCollection& instances)
        {
            ResolvedTmpls currentTmpls;
            const auto paramBraced = "{" + paramName + "}";
            if (instances.empty())
            {
                log<level::WARNING>("No corresponding instances found to "
                                    "resolve template by specified conditions",
                                    entry("TEMPLATE=%s", tmpl.c_str()));
                return currentTmpls;
            }
            for (const auto instance : instances)
            {
                const auto paramVal =
                    instance->getField(memberName)->getStringValue();
                const auto formatedTmpl = formatTemplate(
                    tmpl, paramBraced, ctx->encodeUriSegment(paramVal));
                currentTmpls.emplace(formatedTmpl,
                                     std::make_pair(instance, entity));
            }
            return currentTmpls;
        }

        static const std::string formatTemplate(const std::string& tmpl,
                                                const Parameter& paramName,
                                                const std::string& value)
        {
            auto pos = tmpl.find(paramName);
            if (pos == std::string::npos)
                throw std::runtime_error("Defined parameter not found in the "
                                         "specfifed template");
            return tmpl.substr(0, pos) + value +
                   tmpl.substr(pos + paramName.length());
        }

        template <typename TEntity>
        static const entity::EntityPtr entity()
        {
            return application.getEntityManager().getEntity<TEntity>();
        }
    };

    using LinkObjectGetter = LinkGetter<ObjectGetter>;
    using LinkCollectionGetter = LinkGetter<StaticCollectionGetter>;

    template <typename... TLinkGetter>
    class LinksGetter : public ObjectGetter
    {
      public:
        explicit LinksGetter(const std::string& fieldName) :
            ObjectGetter(fieldName)
        {
            static_assert(
                std::conjunction_v<std::disjunction<
                    std::is_base_of<LinkObjectGetter, TLinkGetter>,
                    std::is_base_of<LinkCollectionGetter, TLinkGetter>>...>,
                "Invalid TLinkGetter definition. The TLinkGetter must be "
                "derrived "
                "from LinkObjectGetter or LinkCollectionGetter");
        }
        ~LinksGetter() override = default;

      protected:
        const FieldHandlers childs(const RedfishContextPtr&) const override
        {
            FieldHandlers getters;
            (getters.emplace_back(createAction<TLinkGetter>()), ...);
            return getters;
        }
    };

    template <typename... TChildGetters>
    class OemGetter : public ObjectGetter
    {
        /** The OEM extension property */
        static constexpr const char* oemFieldName = "Oem";
        const app::entity::IEntity::InstancePtr instance;

      public:
        explicit OemGetter(const app::entity::IEntity::InstancePtr& instance) :
            ObjectGetter(oemFieldName), instance(instance)
        {}
        OemGetter() : ObjectGetter(oemFieldName)
        {}
        ~OemGetter() override = default;

      protected:
        const FieldHandlers childs(const RedfishContextPtr&) const override
        {
            FieldHandlers getters;
            (getters.emplace_back(createAction<TChildGetters>(instance)), ...);
            return getters;
        }
    };

    template <typename TNextNode>
    class IdReference : public ObjectGetter
    {
        const std::string segment;

      public:
        IdReference(const std::string& name, const std::string& segment) :
            ObjectGetter(name), segment(segment)
        {}
        explicit IdReference(const std::string& name) :
            ObjectGetter(name), segment(TNextNode::segment)
        {}
        ~IdReference() override = default;

      protected:
        const FieldHandlers childs(const RedfishContextPtr& ctx) const override
        {
            FieldHandlers getters;
            const auto odataId = (ctx->getRequest()->getUriPath() + segment);
            getters.emplace_back(
                createAction<StringGetter>(nameFieldODataID, odataId.c_str()));

            return getters;
        }
    };

    template <typename... TNextNode>
    class ListReferences : public StaticCollectionGetter
    {
      public:
        explicit ListReferences(const std::string& field) :
            StaticCollectionGetter(field)
        {}
        ~ListReferences() override = default;

      protected:
        const FieldHandlers childs(const RedfishContextPtr& ctx) const
        {
            /* clang-format off */
            FieldHandlers getters;
            (addReference<TNextNode>(ctx, getters), ...);
            return getters;
            /* clang-format on */
        }

      private:
        template <typename TRef>
        void addReference(const RedfishContextPtr& ctx,
                          FieldHandlers& getters) const
        {
            const auto segments = TRef::getAllSegmentValues(ctx);
            for (const auto& refSegment : segments)
            {
                std::string segment = refSegment;
                if constexpr (std::is_base_of_v<IStaticSegments, TRef>)
                {
                    segment = IStaticSegments::prefix<TRef>(refSegment);
                }
                getters.emplace_back(createAction<IdReference<TRef>>(
                    this->getFieldName(), segment));
            }
        }
    };

    template <typename TAction, typename... Args>
    static const ActionPtr createAction(Args... args)
    {
        return std::make_shared<TAction>(args...);
    }

    virtual const FieldHandlers getFieldsGetters() const
    {
        static const FieldHandlers getters{};
        return getters;
    }

    virtual const FieldHandlers& getFieldSetters() const
    {
        static const FieldHandlers setters{};
        return setters;
    };

    // virtual const Field getActionsSetters() const = 0;

    /**
     * @brief DMTF DSP0266, 6.2 HTTP methods: provides
     *        - Create resouce
     *        - Resource action
     *        - Eventing
     * @note  The method must be implemented
     */
    virtual void methodPost() const
    {
        messages::methodNotAllowed(ctx);
    }
    /**
     * @brief DMTF DSP0266, 6.2 HTTP methods: provides
     *        - Retrieve resource
     * @note  The method must be implemented
     */
    virtual void methodGet() const
    {
        const auto fields = getFieldsGetters();
        for (const auto valueGetter : fields)
        {
            valueGetter->process(ctx);
        }
    }
    /**
     * @brief DMTF DSP0266, 6.2HTTP methods: provides
     *        - Update resource
     * @note  The method must be implemented
     */
    virtual void methodPatch() const
    {
        messages::methodNotAllowed(ctx);
    }
    /**
     * @brief DMTF DSP0266, 6.2 HTTP methods: provides
     *        - Replace resource
     * @note  Optional method
     */
    virtual void methodPut() const
    {
        messages::methodNotAllowed(ctx);
    }
    /**
     * @brief DMTF DSP0266, 6.2 HTTP methods: provides
     *        - Delete resource
     * @note  The method must be implemented
     */
    virtual void methodDelete() const
    {
        messages::methodNotAllowed(ctx);
    }
    /**
     * @brief DMTF DSP0266, 6.2 HTTP methods: provides
     *        - Retrieve resource header
     * @note  Optional method
     */
    virtual void methodHead() const
    {
        messages::methodNotAllowed(ctx);
    }
    /**
     * @brief DMTF DSP0266, 6.2 HTTP methods: provides
     *        - Retrieve header
     *          Cross-origin resource sharing (CORS) pre-flight
     * @note  Optional method
     */
    virtual void methodOptions() const
    {
        messages::methodNotAllowed(ctx);
    }

  protected:
    const RedfishContextPtr ctx;

  private:
    void methodNotAllowed() const
    {
        messages::methodNotAllowed(ctx);
    }
};
} // namespace redfish
} // namespace core
} // namespace app
