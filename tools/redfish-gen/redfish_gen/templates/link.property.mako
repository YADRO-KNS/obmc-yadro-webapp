<%page args="link, is_dynamic"/>
/**
* @class ${link.classname()}
* 
* @brief Getter to provide Link
*/
class ${link.classname()} : public ${link.get_type()}
{
    public:
    ${link.classname()}():
        ${link.get_type()}(
            "${link.name()}",
            "${link.template()}")
    {}
    ~${link.classname()}() override = default;

    protected:
    const ParameterResolveDict
        getParameterConditions([[maybe_unused]]const RedfishContextPtr ctx) const override
    {
        const ParameterResolveDict resolvers{
        % for p in link.parameters():
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
