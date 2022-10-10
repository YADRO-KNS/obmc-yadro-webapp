// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <graphqlparser/AstVisitor.h>

#include <core/application.hpp>
#include <core/route/handlers/graphql_handler.hpp>
#include <http/headers.hpp>
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

using namespace phosphor::logging;

GqlObjectBuild::GqlObjectBuild(const std::string& objectName,
                               const entity::IEntity::RelationPtr inputRelation,
                               GqlBuildPtr parentBuilder) :
    name(objectName),
    relation(inputRelation), parent(parentBuilder), fragment(json::object({}))
{
    const auto targetEntity = getEntity();
    if (!relation)
    {
        log<level::DEBUG>("Attempt to create builder without Entity object");
        return;
    }

    for (auto instance : targetEntity->getInstances())
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
    if (!entityObject)
    {
        log<level::DEBUG>("Attempt to create builder without Entity object");
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
    result.push_back({operationDefinition.getOperation(), json::object({})});

    decltype(auto) visitor = VisitorFactory::build(
        operationDefinition.getOperation(), result.back());

    if (!visitor)
    {
        throw exceptions::NotSupported(operationDefinition.getOperation());
    }

    operationDefinition.accept(visitor.get());
    return true;
}

void ObmcGqlVisitor::endVisitOperationDefinition(
    const OperationDefinition& operationDefinition)
{}

const nlohmann::json& ObmcGqlVisitor::getResult() const
{
    return result;
}

// QUERY VISITOR
bool GqlQueryVisitor::visitVariableDefinition(const VariableDefinition&)
{
    throw exceptions::NotSupported(
        "GQL Variable definition temporarry not supported");
}

bool GqlQueryVisitor::visitArgument(const Argument& argument)
{
    return true;
}

void GqlQueryVisitor::endVisitArgument(const Argument& argument)
{}

bool GqlQueryVisitor::visitStringValue(const StringValue& stringValue)
{
    return true;
}

void GqlQueryVisitor::endVisitStringValue(const StringValue& stringValue)
{}

bool GqlQueryVisitor::visitField(const Field& field)
{
    const std::string fieldName = field.getName().getValue();
    auto fieldAlias =
        field.getAlias()
            ? std::optional<std::string>(field.getAlias()->getValue())
            : std::nullopt;

    if (field.getSelectionSet())
    {
        if (!fragmentBuilder)
        {
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
            }
            else
            {
                auto relation = entity->getRelation(fieldName);
                if (!relation)
                {
                    throw exceptions::GqlInvalidArgument(
                        fieldName,
                        "Relation to the " + fieldName + " was not found");
                }
                childObjectBuilder = std::make_shared<GqlObjectBuild>(
                    fieldName, relation, fragmentBuilder);

                auto entityOfChild = childObjectBuilder->getEntity();
            }

            if (fieldAlias.has_value())
            {
                childObjectBuilder->setAlias(*fieldAlias);
            }

            fragmentBuilder = std::move(childObjectBuilder);
        }
        catch (entity::exceptions::EntityException& ex)
        {
            throw exceptions::GqlAstError(ex.what());
        }
        catch (nlohmann::detail::type_error& ex)
        {
            throw exceptions::GqlAstError(
                "Internal GQL implementation error or requested AST structure "
                "is not supported");
        }
    }
    else
    {
        // Scalar field
        if (!fragmentBuilder)
        {
            throw exceptions::GqlInternalError("Not builder found");
        }

        fragmentBuilder->supplement(fieldAlias.has_value() ? *fieldAlias
                                                           : fieldName);
    }

    return true;
}

void GqlQueryVisitor::endVisitField(const Field& field)
{
    if (field.getSelectionSet())
    {
        // this is object
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
        log<level::DEBUG>("No data in POST. Skip parse body");
        return true;
    }

    std::string data(postBuffer.begin(), postBuffer.end());

    auto jsonData = json::parse(data.c_str(), nullptr, false);
    if (jsonData.is_discarded() || !jsonData["query"].is_string())
    {
        log<level::DEBUG>("Error parsing GQL request in json file.");
        return true;
    }

    try
    {
        auto astData = jsonData["query"].get<const std::string>();
        gqlNode = facebook::graphql::parseString(astData.c_str(), &error);

        if (!gqlNode)
        {
            free(const_cast<char*>(error));
        }
    }
    catch (std::exception& ex)
    {
        log<level::DEBUG>("Error parsing GQL request.",
                          entry("ERROR=%s", ex.what()));
    }
    return true;
}

const ResponsePtr GraphqlRouter::run(const RequestPtr& request)
{
    ObmcGqlVisitor visitor;
    json result = json::object({});

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
        log<level::DEBUG>("Error handle GQL request.",
                          entry("ERROR=%s", gqlException.what()));
        result.push_back({fields::respFieldError, gqlException.whatJson()});
    }
    auto response = std::make_shared<Response>();
    response->push(result.dump(2));
    response->setStatus(statuses::Code::OK);
    response->setContentType(http::content_types::applicationJson);
    return response;
}

// BUILDERS
void GqlObjectBuild::supplement(const std::string& fieldName)
{
    auto targetEntity = getEntity();
    if (!fragment.type_name() || fragment.is_null() || !targetEntity)
    {
        log<level::ERR>("GQL Invalid Structure");
        throw exceptions::GqlAstError("Invalid Structure");
    }

    try
    {
        auto member = targetEntity->getMember(fieldName);
        const auto& instances = targetEntity->getInstances();
        for (auto instance : instances)
        {
            auto& jsonObject = fragment[std::to_string(instance->getHash())];

            auto valVisitor = [&jsonObject, fieldName](auto&& value) {
                jsonObject.push_back({fieldName, value});
            };
            if (!instance->getField(member)->isNull())
            {
                std::visit(std::move(valVisitor),
                           instance->getField(member)->getValue());
                continue;
            }
            valVisitor(nullptr);
        }
    }
    catch (entity::exceptions::EntityException& ex)
    {
        log<level::DEBUG>("GQL Invalid Argument.",
                          entry("ERROR=%s", ex.what()));
        throw exceptions::GqlInvalidArgument(fieldName, "Field not found");
    }
    catch (std::out_of_range& ex)
    {
        log<level::DEBUG>("Out of range", entry("ERROR=%s", ex.what()));
        throw exceptions::GqlInternalError("Internal GQL logic error");
    }
}

void GqlObjectBuild::supplement(const std::string& fieldName, GqlBuildPtr child)
{
    if (this->fragment.is_object())
    {
        for (auto& [hashStr, item] : this->fragment.items())
        {
            std::optional<std::size_t> hash;
            try
            {
                hash.emplace(std::stoull(hashStr));
            }
            catch (std::invalid_argument& ex)
            {
                // The root node might haven't the valid instance hash. Hence,
                // suppress the raized exception
                log<level::DEBUG>(
                    "Can't cast instance hash string to numeric hash view",
                    entry("ERROR=%s", ex.what()));
            }
            auto childJsonNode =
                std::forward<const json>(child->getFragment(hash));
            item.push_back({fieldName, childJsonNode});
        }
    }
    else
    {
        log<level::DEBUG>("The internal JSON fragment type is not supproted.");
        throw exceptions::GqlInternalError(
            "A GQL parser implementation error.");
    }
}

const json GqlObjectBuild::getFragment(
    std::optional<std::size_t> parentInstanceHash) const
{
    json result(json::object({}));

    try
    {
        auto entity = getEntity();
        if (!entity)
        {
            return std::forward<const json>(fragment.back());
        }
        else
        {
            std::vector<entity::IEntity::ConditionPtr> conditions;
            if (this->relation && parentInstanceHash.has_value())
            {
                conditions = std::move(
                    this->relation->getConditions(*parentInstanceHash));
            }
            auto& instances = entity->getInstances(conditions);
            if (instances.size() == 1 &&
                entity->getType() != entity::IEntity::Type::array)
            {
                result =
                    fragment.at(std::to_string(instances.back()->getHash()));
            }
            else if (instances.size() > 1 ||
                     entity->getType() == entity::IEntity::Type::array)
            {
                result = json::array({});
                for (auto instance : instances)
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
        log<level::DEBUG>("The parent builder not specified. Skip pushing the "
                          "JSON framgent to the parent builder");
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
        log<level::DEBUG>("Attempt to access the empty Entity");
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
