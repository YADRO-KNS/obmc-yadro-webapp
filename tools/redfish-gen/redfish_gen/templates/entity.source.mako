<%page args="entities, is_dynamic, parent_entity=None"/>
% for entity in entities:
    /**
    * @class ${entity.classname()}
    * 
    * @brief Getter to provide data that is related to the `${entity.source()}` entity
    */
    class ${entity.classname()}: public ${entity.base_class(is_dynamic, parent_entity)}
    {
        public:
        ${entity.classname()}(const app::entity::IEntity::InstancePtr instance)
            : ${entity.base_class(is_dynamic, parent_entity)}(instance)
        {}
        ${entity.classname()}() : ${entity.base_class(is_dynamic, parent_entity)}()
        {}
        ~${entity.classname()}() override = default;
        
        /** 
         * @brief Obtaining child action to populate target part of response
         * @param ctx - Redfish context
         * @return List of actions
         */
        const FieldHandlers childs(const RedfishContextPtr& ctx) const override
        {
            FieldHandlers handlers;
            % for prop in entity.fields():
                /** ${prop.description()} */
                addGetter<${prop.getter_name()}>("${prop.field()}", ${prop.source_field()}, 
                    ctx, handlers ${prop.get_additional_getter_args()});
            %endfor
            return handlers;
        }
        /**
         * @brief Get conditions to obtain relevant ${entity.source()} 
         *        instances that is depends on the parent instance or an 
         *        enitty relation
         * @return List of entity conditions
         */
        static const entity::IEntity::ConditionsList getStaticConditions([[maybe_unused]]const IEntity::InstancePtr instance)
        {
            const entity::IEntity::ConditionsList conditions {
            % for condition in entity.conditions():
                ${condition.definition()},
            % endfor
            };
            return conditions;
        }

        const entity::IEntity::ConditionsList getConditions() const override
        {
            return ${entity.name()}EntityGetter::getStaticConditions(this->getParentInstance());
        }
    }; 
% endfor
