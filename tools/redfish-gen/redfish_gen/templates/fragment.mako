<%page args="fragment, is_dynamic, parent_entity=None"/>

/**
 * @class ${fragment.name()}FragmentGetter
 * 
 * @brief Getter to populate the `${fragment.name()}` json object the part of REDFISH response
 */
class ${fragment.name()}FragmentGetter : public ObjectGetter
{
    % for child_fragment in fragment.fragment_properties():
        <%include file="/fragment.mako" args="fragment=child_fragment, is_dynamic=is_dynamic, parent_entity=parent_entity"/>
    % endfor 
    const app::entity::IEntity::InstancePtr instance;
    % for enum in fragment.enum_definitions():
        <%include file="/enum.mako" args="enum=enum"/>
    % endfor
    <%include file="/entity.source.mako" args="entities=fragment.entity_properties(), is_dynamic=is_dynamic, parent_entity=parent_entity"/>
    % for collection in fragment.collection_properties():
        <%include file="/collection.property.mako" args="collection=collection, is_dynamic=is_dynamic, parent_entity=parent_entity"/>
    % endfor
    % for oem in fragment.oem_properties():
        <%include file="/oem.property.mako" args="oem=oem, is_dynamic=is_dynamic, parent_entity=parent_entity"/>
    % endfor 
    % for related_items in fragment.related_items():
        <%include file="/related.items.property.mako" args="related_items=related_items"/>
    % endfor 
    public:
    ${fragment.name()}FragmentGetter(
        const std::string& name, 
        const app::entity::IEntity::InstancePtr instance) 
        : ObjectGetter(name), instance(instance)
    {}
    ${fragment.name()}FragmentGetter(const app::entity::IEntity::InstancePtr instance) 
        : ObjectGetter("${fragment.name()}"), instance(instance)
    {}
    ${fragment.name()}FragmentGetter(): ObjectGetter("${fragment.name()}"), instance()
    {}
    ~${fragment.name()}FragmentGetter() override = default;
    protected:
    const FieldHandlers childs([[maybe_unused]] const RedfishContextPtr& ctx) const override
    {
        const FieldHandlers getters{
        % for annotation in fragment.annotations():
            ${annotation.instance().getter_definition()},
        % endfor
        % for prop in fragment.static_properties():
            /** ${prop.description()} */
            createAction<${prop.getter_name()}>("${prop.field()}", ${prop.value()}),
        % endfor
         % for related_items in fragment.related_items():
            createAction<${related_items.classname()}>(),
        % endfor
        % for entity in fragment.entity_properties():
            createAction<${entity.name()}EntityGetter>(instance),
        % endfor 
        % for child_fragment in fragment.fragment_properties():
            createAction<${child_fragment.name()}FragmentGetter>(instance),
        % endfor 
        % for collection in fragment.collection_properties():
            ${collection.action_classname()},
        % endfor 
        ${fragment.oem_classes()}
        };
        return getters;
    }
};
