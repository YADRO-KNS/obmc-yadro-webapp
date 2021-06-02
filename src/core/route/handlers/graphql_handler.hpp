// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __GRAPHQL_HANDLER_H__
#define __GRAPHQL_HANDLER_H__

#include <graphqlparser/AstNode.h>
#include <graphqlparser/AstVisitor.h>
#include <graphqlparser/GraphQLParser.h>
#include <graphqlparser/c/GraphQLAstToJSON.h>

#include <core/entity/entity.hpp>
#include <core/exceptions.hpp>
#include <core/router.hpp>
#include <logger/logger.hpp>
#include <nlohmann/json.hpp>

#include <exception>
#include <map>
#include <optional>
#include <variant>

namespace app
{
namespace core
{
namespace route
{

namespace fields
{

constexpr const char* respFieldData = "data";
constexpr const char* respFieldError = "error";

} // namespace fields
namespace handlers
{

using namespace nlohmann;

using namespace facebook::graphql;
using namespace facebook::graphql::ast;

using AstVisitorUni = std::unique_ptr<visitor::AstVisitor>;

class IGqlBuild;
using GqlBuildPtr = std::shared_ptr<IGqlBuild>;

class IGqlBuild
{
  public:
    virtual ~IGqlBuild() = default;

    virtual void setAlias(const std::string&) = 0;
    virtual void supplement(const std::string&) = 0;
    virtual void supplement(const std::string&, GqlBuildPtr) = 0;

    virtual void pushFragmentToParent() = 0;
    virtual const json getFragment(std::optional<std::size_t> = std::nullopt) const = 0;
    virtual GqlBuildPtr getParent() const = 0;
    virtual const std::string getFieldName() const = 0;
    virtual const entity::IEntity::InstancePtr getCurrentInstance() const = 0;
    virtual const entity::EntityPtr getEntity() const = 0;
};

class GqlObjectBuild :
    public IGqlBuild,
    public std::enable_shared_from_this<GqlObjectBuild>
{
    const std::string name;
    const entity::EntityPtr entityObject;
    entity::IEntity::RelationPtr relation;

    GqlBuildPtr parent;
    json fragment;
    std::optional<std::string> alias;
    entity::IEntity::InstancePtr currentInstance;
  public:
    GqlObjectBuild(const std::string& objectName) :
        name(objectName), entityObject(), parent(GqlBuildPtr()),
        fragment(json::object({{objectName, json::object({})}}))
    {}

    GqlObjectBuild(const std::string&, const entity::EntityPtr, GqlBuildPtr);
    GqlObjectBuild(const std::string&, const entity::IEntity::RelationPtr, GqlBuildPtr);

    GqlObjectBuild(const GqlObjectBuild&) = delete;
    GqlObjectBuild(const GqlObjectBuild&&) = delete;

    GqlObjectBuild& operator=(const GqlObjectBuild&) = delete;
    GqlObjectBuild& operator=(const GqlObjectBuild&&) = delete;

    ~GqlObjectBuild() override = default;

    void setAlias(const std::string&) override;

    void supplement(const std::string&) override;
    void supplement(const std::string&, GqlBuildPtr) override;

    const json getFragment(std::optional<std::size_t> = std::nullopt) const override;
    GqlBuildPtr getParent() const override;
    const std::string getFieldName() const override;
    void pushFragmentToParent() override;
    const entity::IEntity::InstancePtr getCurrentInstance() const;
    const entity::EntityPtr getEntity() const override;
};

namespace exceptions
{
class GqlException : public core::exceptions::ObmcAppException
{
    static constexpr const char* fieldTarget = "Target";
    static constexpr const char* fieldDescription = "Description";

  public:
    explicit GqlException(const std::string target,
                          const std::string description) noexcept :
        core::exceptions::ObmcAppException("GraphQL error"),
        error(json::object())
    {
        addField(fieldTarget, target);
        addField(fieldDescription, description);
    }

    const char* what() const noexcept override
    {
        whatBuffer = whatJson().dump();
        return whatBuffer.c_str();
    }

    virtual const json& whatJson() const noexcept
    {
        return error;
    }
    virtual ~GqlException() = default;

  protected:
    void addField(const std::string field, const std::string data) noexcept
    {
        error.push_back({field, data});
    }

  private:
    json error;
    mutable std::string whatBuffer;
};

class GqlInvalidArgument : public GqlException
{
  public:
    explicit GqlInvalidArgument(const std::string& arg,
                                const std::string& desc) noexcept :
        GqlException("Invalid Argument", desc)
    {
        addField("Argument", arg);
    }
    virtual ~GqlInvalidArgument() = default;
};

class GqlInternalError : public GqlException
{
  public:
    explicit GqlInternalError(const std::string error) noexcept :
        GqlException("Graphql internal impelenetation error", std::move(error))
    {}
    virtual ~GqlInternalError() = default;
};

class NotSupported : public GqlException
{
    static constexpr const char* fieldOperation = "Operation";

  public:
    explicit NotSupported(const std::string entity) noexcept :
        GqlException("Operation", "Not Supported")
    {
        addField(fieldOperation, entity);
    }
    virtual ~NotSupported() = default;
};

class GqlAstError : public GqlException
{
    static constexpr const char* fieldAst = "AST";

  public:
    explicit GqlAstError(const std::string error) noexcept :
        GqlException("GraphQL", "GraphQL AST error")
    {
        addField(fieldAst, error);
    }
    virtual ~GqlAstError() = default;
};

} // namespace exceptions
class GraphqlRouter : public IRouteHandler
{
  public:
    explicit GraphqlRouter(const std::string& iPath) : path(iPath)
    {}

    GraphqlRouter() = default;
    GraphqlRouter(const GraphqlRouter&) = delete;
    GraphqlRouter(const GraphqlRouter&&) = delete;

    GraphqlRouter& operator=(const GraphqlRouter&) = delete;
    GraphqlRouter& operator=(const GraphqlRouter&&) = delete;

    void run(const RequestPtr& request, ResponseUni& response) override;
    bool preHandlers(const RequestPtr& request) override;

    virtual ~GraphqlRouter() = default;

  private:
    std::string path;

    std::unique_ptr<ast::Node> gqlNode;
};

// VISITORS
class ObmcGqlVisitor : public visitor::AstVisitor
{
    nlohmann::json result;

  public:
    ObmcGqlVisitor() : result(json::object())
    {}
    ~ObmcGqlVisitor() override = default;

    bool visitOperationDefinition(
        const OperationDefinition& operationDefinition) override;
    void endVisitOperationDefinition(
        const OperationDefinition& operationDefinition) override;

    const nlohmann::json& getResult() const;
};

class GqlQueryVisitor : public visitor::AstVisitor
{
    GqlBuildPtr fragmentBuilder;

  public:
    static constexpr std::string_view visitorName = "query";

    GqlQueryVisitor(nlohmann::json& fragment) : document(fragment)
    {
        fragmentBuilder = std::make_shared<GqlObjectBuild>(visitorName.data());
    }

    GqlQueryVisitor(const GqlQueryVisitor&) = delete;
    GqlQueryVisitor(const GqlQueryVisitor&&) = delete;

    GqlQueryVisitor& operator=(const GqlQueryVisitor&) = delete;
    GqlQueryVisitor& operator=(const GqlQueryVisitor&&) = delete;

    bool visitVariableDefinition(
        const VariableDefinition&) override;

    bool visitArgument(const Argument&) override;
    void endVisitArgument(const Argument&) override;

    bool visitStringValue(const StringValue&) override;
    void endVisitStringValue(const StringValue&) override;

    bool visitField(const Field& field) override;
    void endVisitField(const Field& field) override;

    ~GqlQueryVisitor() override;

    const GqlBuildPtr getBuilder() const;

  private:
    nlohmann::json& document;
};

class VisitorFactory final
{
    using VisitorPurpose = std::string;
    using VisitorBuilderFn = std::function<AstVisitorUni(nlohmann::json&)>;
    using VisitorDict = std::map<VisitorPurpose, VisitorBuilderFn>;
    static VisitorDict visitorBuildersDict;

  public:
    VisitorFactory() = delete;
    VisitorFactory(const VisitorFactory&) = delete;
    VisitorFactory(const VisitorFactory&&) = delete;

    VisitorFactory& operator=(const VisitorFactory&) = delete;
    VisitorFactory& operator=(const VisitorFactory&&) = delete;
    ~VisitorFactory() = delete;

    static void registerGqlVisitors() noexcept;

    static AstVisitorUni build(const std::string visitorName,
                               nlohmann::json& fragment);

  private:
    template <class TVisitor>
    static void registerVisitor(const std::string& visitorName);
};

} // namespace handlers
} // namespace route
} // namespace core
} // namespace app

#endif // __GRAPHQL_HANDLER_H__
