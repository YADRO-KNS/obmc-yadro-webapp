## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2022, KNS Group LLC (YADRO)
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022, KNS Group LLC (YADRO)

/** @generated */
#pragma once

#include <core/route/redfish/node.hpp>

#include <nlohmann/json.hpp>

#include <includes.hpp>
% for ch in instance.childs():
#include <redfish/generated/${ch}.hpp>
% endfor

namespace app
{
namespace core
{
namespace redfish
{
using namespace phosphor::logging;
using namespace app::obmc::entity;
/**
 * @class ${instance.classname()}
 * @brief ${instance.node_annotation()}
 * @link ${instance.node_specifiaction()}
 */
class ${instance.classname()} : ${instance.base_inherit_node_classname()}
{
    static constexpr const char* fieldName = "${instance.name()}";
    static constexpr const char* fieldId = "${instance.schema_id()}";
    static constexpr const char* fieldDescription = "${instance.description()}";
    static constexpr const char* fieldODataType = "${instance.odata_type()}";
    % if instance.odata_context() is not None:
    static constexpr const char* fieldODataContext = "${instance.odata_context()}";
    % endif
  public:
    % if not instance.is_dynamic():
    static constexpr const char* segment  = "${instance.segment()}";
    % endif
    % if instance.has_prefix():
    static constexpr const std::array uriPrefix = {${instance.uri_prefix_segments()}};
    % endif
    ${instance.classname()}(const RedfishContextPtr ctx) : ${instance.base_node_classname()}
    {}

    ${instance.classname()}() = delete;
    ~${instance.classname()}() override = default;
% if instance.is_dynamic():
    <%include file="${instance.parameter_template()}" args="parameter=instance.node_parameter()"/>
% endif
  protected:
% for enum in instance.enums():
    <%include file="/enum.mako" args="enum=enum"/>
% endfor
    <%include file="/entity.source.mako" args="entities=instance.entities(), is_dynamic=instance.is_dynamic()"/>
% for fragment in instance.fragments():
    <%include file="/fragment.mako" args="fragment=fragment, is_dynamic=instance.is_dynamic()"/>
% endfor
% for oem in instance.oem():
    <%include file="/oem.property.mako" args="oem=oem, is_dynamic=instance.is_dynamic()"/>
% endfor
% for link in instance.links():
    <%include file="/link.property.mako" args="link=link, is_dynamic=instance.is_dynamic()"/>
% endfor
% for related_items in instance.related_items():
    <%include file="/related.items.property.mako" args="related_items=related_items"/>
% endfor
% for collection in instance.collections():
    <%include file="/collection.property.mako" args="collection=collection, is_dynamic=instance.is_dynamic()"/>
% endfor

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
            createAction<CallableGetter>(nameFieldODataID, std::bind(&${instance.classname()}::getOdataId, this)),
            /** The type of a resource */
            createAction<StringGetter>(nameFieldODataType, fieldODataType),
            /** The unique identifier for this resource within the collection of similar resources */
            ${instance.fieldIdGetterDefinition()}
            /** The name of the resource or array member */
            createAction<StringGetter>(nameFieldName, fieldName),
        % if instance.odata_context() is not None:
            /** The OData description of a payload */
            createAction<StringGetter>(nameFieldODataContext, fieldODataContext),
        % endif
            /** The description of this resource. Used for commonality in the schema definitions */
            createAction<StringGetter>(nameFieldDescription, fieldDescription),
            /** The reference to next related nodes */
        % for ref in instance.reference():
            createAction<${ref['Classname']}>("${ref['Field']}"),
            % if ref['Type'] == 'List':
                createAction<CollectionSizeAnnotation>("${ref['Field']}"),
            % endif
        % endfor
            /** The links to associated nodes */
            ${instance.links_class()}
            /** The RelatedItem to associated nodes */
        % for related_items in instance.related_items():
            createAction<${related_items.classname()}>(),
        % endfor
        % if len(instance.related_items()) > 0:
            createAction<CollectionSizeAnnotation>("RelatedItem"),
        % endif
            /** The node static-initialized fields */
        % for prop in instance.static_properties():
            /** ${prop.description()} */
            createAction<${prop.getter_name()}>("${prop.field()}", ${prop.value()}),
            % if prop.is_enum_type():
                // property '${prop.field()}' is Enum
            % endif
        % endfor
        % for entity in instance.entities():
            createAction<${entity.name()}EntityGetter>(${instance.parent_instance_definition()}),
        % endfor
        % for collection_action in instance.collections_actions():
            ${collection_action},
        % endfor
        % for fragment in instance.fragments():
            createAction<${fragment.name()}FragmentGetter>(${instance.parent_instance_definition()}),
        % endfor
        ${instance.oem_classes()}
        % for annotation in instance.annotations():
            ${annotation.instance().getter_definition()}
        % endfor
        };
        return getters;
    }

    /**
     * @brief Override base methodGet to provide self-logic of ${instance.classname()} node handler
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
