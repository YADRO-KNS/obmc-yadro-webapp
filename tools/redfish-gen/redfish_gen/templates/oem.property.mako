<%page args="oem, is_dynamic, parent_entity=None"/>
/**
* @class ${oem.oem_name()}
* 
* @brief Getter to provide OEM-specific payload by defined OEM-schema
*/
class ${oem.oem_name()} : public ObjectGetter
{
    /** @brief The name of OEM-owner */
    static constexpr const char* owningEntity = "${oem.owning_entity()}";
    /** @brief The OEM schema type */
    static constexpr const char* odataType = "${oem.schema_type()}";
    /** @brief The instance to populate payload. Passthrough to the child actions */
    const app::entity::IEntity::InstancePtr instance;

    % for child_fragment in oem.fragment_properties():
        <%include file="/fragment.mako" args="fragment=child_fragment, is_dynamic=is_dynamic, parent_entity=parent_entity"/>
    % endfor 
    <%include file="/entity.source.mako" args="entities=oem.entity_properties(), is_dynamic=is_dynamic, parent_entity=parent_entity"/>
    % for enum in oem.enum_definitions():
        <%include file="/enum.mako" args="enum=enum"/>
    % endfor
    % for collection in oem.collection_properties():
        <%include file="/collection.property.mako" args="collection=collection, is_dynamic=is_dynamic, parent_entity=parent_entity"/>
    % endfor
    public:
    ${oem.oem_name()}(const app::entity::IEntity::InstancePtr instance) 
        : ObjectGetter(owningEntity), instance(instance)
    {}
    ${oem.oem_name()}() 
        : ObjectGetter(owningEntity), instance()
    {}
    ~${oem.oem_name()}() override = default;
    
    protected:
    /** 
     * @brief Obtaining child action to populate OEM-object
     * @param ctx - Redfish context
     * @return List of actions
     */
    const FieldHandlers childs([[maybe_unused]] const RedfishContextPtr& ctx) const override
    {
        const FieldHandlers handlers {
            % for annotation in oem.annotations():
                ${annotation.instance().getter_definition()},
            % endfor
            createAction<StringGetter>(nameFieldODataType, odataType),
             % for prop in oem.static_properties():
                /** ${prop.description()} */
                createAction<${prop.getter_name()}>("${prop.field()}", ${prop.value()}),
            % endfor
            % for entity in oem.entity_properties():
                createAction<${entity.name()}EntityGetter>(instance),
            % endfor 
            % for child_fragment in oem.fragment_properties():
                createAction<${child_fragment.name()}FragmentGetter>(instance),
            % endfor 
            % for collection in oem.collection_properties():
                ${collection.action_classname()},
            % endfor 
        };
        return handlers;
    }
};
