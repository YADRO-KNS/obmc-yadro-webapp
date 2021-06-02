// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <graphqlparser/AstVisitor.h>

#include <core/application.hpp>
#include <core/route/handlers/graphql_handler.hpp>
#include <logger/logger.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <type_traits>

namespace app
{
namespace core
{
namespace route
{
namespace handlers
{

using namespace nlohmann;

using namespace facebook::graphql;
using namespace facebook::graphql::ast;

GqlObjectBuild::GqlObjectBuild(const std::string& objectName,
                               const entity::IEntity::RelationPtr inputRelation,
                               GqlBuildPtr parentBuilder) :
    name(objectName),
    relation(inputRelation), parent(parentBuilder), fragment(json::object({}))
{
    BMC_LOG_DEBUG << "Build Objects " << objectName;

    if (!relation)
    {
        BMC_LOG_WARNING << "Attempt to create builder without Entity object";
        return;
    }

    for (auto instance : getEntity()->getInstances())
    {
        // init each one json object for each specified entity instance
        fragment[std::to_string(instance->getHash())] = json::object({});
    }
}

GqlObjectBuild::GqlObjectBuild(const std::string& objectName,
                               const entity::EntityPtr inputEntity,
                               GqlBuildPtr parentBuilder) :
    name(objectName),
    entityObject(inputEntity), parent(parentBuilder), fragment(json::object({}))
{
    BMC_LOG_DEBUG << "Build Objects " << objectName;

    if (!entityObject)
    {
        BMC_LOG_WARNING << "Attempt to create builder without Entity object";
        return;
    }

    for (auto instance : entityObject->getInstances())
    {
        // init each one json object for each specified entity instance
        fragment[std::to_string(instance->getHash())] = json::object({});
    }
}

bool ObmcGqlVisitor::visitOperationDefinition(
    const OperationDefinition& operationDefinition)
{
    BMC_LOG_DEBUG << "visitOperationDefinition: "
              << operationDefinition.getOperation();

    result.push_back({operationDefinition.getOperation(), json::object({})});

    BMC_LOG_DEBUG << "Make visitor";
    decltype(auto) visitor = VisitorFactory::build(
        operationDefinition.getOperation(), result.back());

    BMC_LOG_DEBUG << "visitor created";
    if (!visitor)
    {
        throw exceptions::NotSupported(operationDefinition.getOperation());
    }

    operationDefinition.accept(visitor.get());
    return true;
}

void ObmcGqlVisitor::endVisitOperationDefinition(
    const OperationDefinition& operationDefinition)
{
    BMC_LOG_DEBUG << "endVisitOperationDefinition: "
              << operationDefinition.getOperation();
}

const nlohmann::json& ObmcGqlVisitor::getResult() const
{
    return result;
}

// QUERY VISITOR
bool GqlQueryVisitor::visitVariableDefinition(
    const VariableDefinition&)
{
    throw exceptions::NotSupported(
        "GQL Variable definition temporarry not supported");
}

bool GqlQueryVisitor::visitArgument(const Argument& argument)
{
    BMC_LOG_DEBUG << "visitArgument: "
              << argument.getName().getValue() << " with value ";
    return true;
}

void GqlQueryVisitor::endVisitArgument(const Argument& argument)
{
    BMC_LOG_DEBUG << "endVisitArgument: "
              << argument.getName().getValue() << " with value ";
}

bool GqlQueryVisitor::visitStringValue(const StringValue& stringValue)
{
    BMC_LOG_DEBUG << "visitStringValue: " << stringValue.getValue();
    return true;
}

void GqlQueryVisitor::endVisitStringValue(const StringValue& stringValue)
{
    BMC_LOG_DEBUG << "endVisitStringValue: " << stringValue.getValue();
}

bool GqlQueryVisitor::visitField(const Field& field)
{
    BMC_LOG_DEBUG << "visitField: " << field.getName().getValue();

    const std::string fieldName = field.getName().getValue();
    auto fieldAlias =
        field.getAlias()
            ? std::optional<std::string>(field.getAlias()->getValue())
            : std::nullopt;

    if (field.getSelectionSet())
    {
        // This is object
        BMC_LOG_DEBUG << "This is object";

        if (!fragmentBuilder)
        {
            BMC_LOG_ERROR << "Not found builder for the " << fieldName
                      << " field (aliased)";
            throw exceptions::GqlInternalError("The builder was not found");
        }

        try
        {
            GqlBuildPtr childObjectBuilder;
            entity::EntityPtr entity = this->fragmentBuilder->getEntity();
            if (!entity)
            {
                entity = application.getEntityManager().getEntity(fieldName);
                childObjectBuilder = std::make_shared<GqlObjectBuild>(
                    fieldName, entity, fragmentBuilder);
                BMC_LOG_DEBUG << "Root Entity Object: " << entity->getName();
            }
            else
            {
                BMC_LOG_DEBUG << "Attempt to get a Relation to the " << fieldName
                              << " from " << entity->getName();
                auto relation = entity->getRelation(fieldName);
                if (!relation)
                {
                    BMC_LOG_DEBUG << "Relation not found. Destination: " << fieldName;
                    throw exceptions::GqlInvalidArgument(
                        fieldName,
                        "Relation to the " + fieldName + " was not found");
                }
                childObjectBuilder = std::make_shared<GqlObjectBuild>(
                    fieldName, relation, fragmentBuilder);
                BMC_LOG_DEBUG << "Relation Entity: " << childObjectBuilder->getEntity()->getName();
            }

            if (fieldAlias.has_value())
            {
                childObjectBuilder->setAlias(*fieldAlias);
            }

            fragmentBuilder = std::move(childObjectBuilder);
        }
        catch (entity::exceptions::EntityException& ex)
        {
            BMC_LOG_ERROR << "Not found object " << fieldName;
            throw exceptions::GqlAstError(ex.what());
        }
        catch (nlohmann::detail::type_error& ex)
        {
            BMC_LOG_ERROR << "JSON type error: " << ex.what();
            throw exceptions::GqlAstError(
                "Internal GQL implementation error or requested AST structure "
                "is not supported");
        }
    }
    else
    {
        // Scalar field
        BMC_LOG_DEBUG << "Scalar field";
        if (!fragmentBuilder)
        {
            BMC_LOG_ERROR << "Not found builder for the " << fieldName
                      << " field (aliased)";
            throw exceptions::GqlInternalError("Not builder found");
        }

        BMC_LOG_DEBUG << "Set field value";
        fragmentBuilder->supplement(fieldAlias.has_value() ? *fieldAlias
                                                           : fieldName);
    }

    return true;
}

void GqlQueryVisitor::endVisitField(const Field& field)
{
    BMC_LOG_DEBUG << "endVisitField: " << field.getName().getValue();
    if (field.getSelectionSet())
    {
        // this is object
        BMC_LOG_DEBUG << "Reset builder to the parent";
        fragmentBuilder->pushFragmentToParent();
        if (fragmentBuilder->getParent())
        {
            fragmentBuilder = std::move(fragmentBuilder->getParent());
        }
    }
}

GqlQueryVisitor::~GqlQueryVisitor()
{
    this->document = fragmentBuilder->getFragment();
}

const GqlBuildPtr GqlQueryVisitor::getBuilder() const
{
    return this->fragmentBuilder;
}

// ROUTER

bool GraphqlRouter::preHandlers(const RequestPtr& request)
{
    // Hmm, here something bad is happening without a critical section...
    // I have assume the GraphQL AST parser is not thread-safe
    static std::mutex parseLockMutex;
    std::lock_guard<std::mutex> lock(parseLockMutex);

    const char* error;

    const auto postBuffer = request->environment().postBuffer();

    if (postBuffer.empty())
    {
        BMC_LOG_DEBUG << "No data in POST. Skip parse body";
        return true;
    }
    BMC_LOG_DEBUG << "Buffer length=" << postBuffer.size();
    BMC_LOG_DEBUG << "Content length=" << request->environment().contentLength;

    std::string data(postBuffer.begin(), postBuffer.end());

    auto jsonData = json::parse(data.c_str(), nullptr, false);
    if (jsonData.is_discarded() || !jsonData["query"].is_string())
    {
        BMC_LOG_ERROR << "Error parsing GQL request in json file.";
        BMC_LOG_DEBUG << "Input Buffer: " << postBuffer.data();
        return true;
    }

    BMC_LOG_DEBUG << "GraphQL Schema: " << jsonData.dump(4);
    try
    {
        auto astData = jsonData["query"].get<const std::string>();
        gqlNode = facebook::graphql::parseString(astData.c_str(), &error);

        if (!gqlNode)
        {
            BMC_LOG_ERROR << "Can't parse AST GQL: " << error;
            free(const_cast<char*>(error));
        }
    }
    catch (std::exception& ex)
    {
        BMC_LOG_ERROR << "Error parsing GQL request. Reason:" << ex.what();
    }
    return true;
}

void GraphqlRouter::run(const RequestPtr& request, ResponseUni& response)
{
    ObmcGqlVisitor visitor;
    json result = json::object({});

    BMC_LOG_DEBUG << "Run route: " << request->environment().requestUri;
    try
    {
        if (!gqlNode)
        {
            throw exceptions::GqlAstError(
                "Invalid Grapqh AST. Can't parse comming request");
        }
        gqlNode->accept(&visitor);
        graphql_ast_to_json(
            reinterpret_cast<const struct GraphQLAstNode*>(gqlNode.get()));

        result.push_back({fields::respFieldData, visitor.getResult()});
    }
    catch (exceptions::GqlException& gqlException)
    {
        BMC_LOG_ERROR << "Error handle GQL request:";
        BMC_LOG_ERROR << gqlException.what();
        result.push_back({fields::respFieldError, gqlException.whatJson()});
    }

    response->push(result.dump(2));
    response->setStatus(statuses::Code::OK);
}

// BUILDERS
void GqlObjectBuild::supplement(const std::string& fieldName)
{
    if (!fragment.type_name() || fragment.is_null() || !getEntity())
    {
        BMC_LOG_ERROR << "GQL Invalid Structure";
        throw exceptions::GqlAstError("Invalid Structure");
    }

    try
    {
        auto member = getEntity()->getMember(fieldName);

        for (auto instance : getEntity()->getInstances())
        {
            auto& jsonObject = fragment[std::to_string(instance->getHash())];

            auto valVisitor = [&jsonObject, fieldName](auto&& value) {
                jsonObject.push_back({fieldName, value});
            };
            std::visit(std::move(valVisitor),
                       instance->getField(member)->getValue());
        }
    }
    catch (entity::exceptions::EntityException& ex)
    {
        BMC_LOG_ERROR << "GQL Invalid Argument: " << ex.what();
        throw exceptions::GqlInvalidArgument(fieldName, "Field not found");
    }
    catch (std::out_of_range& ex)
    {
        BMC_LOG_ERROR << "Out of range: " << ex.what();
        throw exceptions::GqlInternalError("Internal GQL logic error");
    }
}

void GqlObjectBuild::supplement(const std::string& fieldName,
                                GqlBuildPtr child)
{
    if (this->fragment.is_object())
    {
        for (auto& [hashStr, item] : this->fragment.items())
        {
            BMC_LOG_DEBUG << "hashStr=" << hashStr;
            std::optional<std::size_t> hash;
            try {
                hash.emplace(std::stoull(hashStr));
            } catch (std::invalid_argument& ex)
            {
                // The root node might haven't the valid instance hash. Hence,
                // suppress the raized exception
                BMC_LOG_DEBUG
                    << "Can't cast instance hash string to numeric hash view: "
                    << ex.what();
            }
            auto childJsonNode =
                std::forward<const json>(child->getFragment(hash));
            item.push_back({fieldName, childJsonNode});
        }
    }
    else
    {
        BMC_LOG_ERROR << "The internal JSON fragment type is not supproted. Type="
                  << this->fragment.type_name();
        throw exceptions::GqlInternalError("A GQL parser implementation error.");
    }
}

const json GqlObjectBuild::getFragment(std::optional<std::size_t> parentInstanceHash) const
{
    json result(json::object({}));

    try
    {
        if (!getEntity())
        {
            return std::forward<const json>(fragment.back());
        }
        else if (getEntity())
        {
            std::vector<entity::IEntity::ConditionPtr> conditions;
            if (this->relation && parentInstanceHash.has_value())
            {
                BMC_LOG_DEBUG << "HASH:" << parentInstanceHash.value();
                conditions = std::move(this->relation->getConditions(*parentInstanceHash));
            }
            BMC_LOG_DEBUG << "Count conditions: " << conditions.size();
            auto& instances = getEntity()->getInstances(conditions);
            if (instances.size() == 1)
            {
                BMC_LOG_DEBUG << "GQL: Fill a singale instanced object";
                if (std::to_string(instances.back()->getHash()) ==
                    fragment.begin().key())
                {
                    result = fragment.back();
                }
            }
            else if (instances.size() > 1)
            {
                BMC_LOG_DEBUG << "GQL: Fill a list of the instanced objects";
                result = json::array({});
                for (auto instance: instances)
                {
                    result.push_back(
                        fragment.at(std::to_string(instance->getHash())));
                }
            }
        }
    }
    catch (std::out_of_range& ex)
    {
        throw exceptions::GqlInternalError(
            "Internal GQL logic error. Can't setup the Object data: " +
            this->name);
    }
    return std::forward<json>(result);
}

void GqlObjectBuild::setAlias(const std::string& alias)
{
    this->alias = alias;
}

GqlBuildPtr GqlObjectBuild::getParent() const
{
    return parent;
}

const std::string GqlObjectBuild::getFieldName() const
{
    return std::forward<const std::string>(alias.has_value() ? *alias : name);
}

void GqlObjectBuild::pushFragmentToParent()
{
    if (!this->parent)
    {
        BMC_LOG_DEBUG << "The parent builder not specified. Skip pushing the JSON "
                     "framgent to the parent builder";
        return;
    }
    this->parent->supplement(getFieldName(), shared_from_this());
}

const entity::IEntity::InstancePtr GqlObjectBuild::getCurrentInstance() const
{
    return this->currentInstance;
}

const entity::EntityPtr GqlObjectBuild::getEntity() const
{
    if (!entityObject &&
        (!relation || (relation && !relation->getDestinationTarget())))
    {
        BMC_LOG_DEBUG << "Attempt to access the empty Entity";
        BMC_LOG_DEBUG << "PTR: " << entityObject << "|" << relation;
        return nullptr;
    }
    return entityObject ? this->entityObject : relation->getDestinationTarget();
}

// FACTORY
VisitorFactory::VisitorDict VisitorFactory::visitorBuildersDict;

template <class TVisitor>
void VisitorFactory::registerVisitor(const std::string& visitorName)
{
    static_assert(std::is_base_of_v<visitor::AstVisitor, TVisitor>,
                  "This is not a GQL visitor");

    visitorBuildersDict.emplace(
        visitorName, [visitorName](nlohmann::json& fragment) -> AstVisitorUni {
            return std::make_unique<TVisitor>(fragment);
        });
}

AstVisitorUni VisitorFactory::build(const std::string visitorName,
                                    nlohmann::json& fragment)
{
    auto builder = visitorBuildersDict.find(visitorName);
    if (builder == visitorBuildersDict.end())
    {
        return AstVisitorUni();
    }

    return builder->second(fragment);
}

void VisitorFactory::registerGqlVisitors() noexcept
{
    registerVisitor<GqlQueryVisitor>(GqlQueryVisitor::visitorName.data());
}

} // namespace handlers
} // namespace route
} // namespace core
} // namespace app
