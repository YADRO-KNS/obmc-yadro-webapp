<%page args="related_items"/>
/**
* @class ${related_items.name()}
* 
* @brief Getter to provide RelatedItems for defined resource.
*/
class ${related_items.classname()} : public ${related_items.get_type()}
{
    public:
    ${related_items.classname()}():
        ${related_items.get_type()}(
            "${related_items.field_name()}",
            "${related_items.template()}")
    {}
    ~${related_items.classname()}() override = default;

    protected:
    const ParameterResolveDict
        getParameterConditions([[maybe_unused]]const RedfishContextPtr ctx) const override
    {
        const ParameterResolveDict resolvers{
        % for p in related_items.parameters():
            {
                "${p.id()}",
                buildCondition<${p.entity()}>(
                    ${p.field()},
                % if p.is_from_context():
                    ctx->getParameterInstance("${p.id()}")
                % else:
                    IEntity::ConditionsList{
                    % for condition in p.conditions():
                        ${condition.definition()},
                    % endfor
                    }
                % endif                    
                ),
            },
        % endfor
        };
        return resolvers;
    }
};
