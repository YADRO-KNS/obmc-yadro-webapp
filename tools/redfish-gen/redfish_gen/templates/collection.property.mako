## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2022, KNS Group LLC (YADRO)

<%page args="collection, is_dynamic, parent_entity=None"/>

<%include file="/fragment.mako" args="fragment=collection, is_dynamic=is_dynamic, parent_entity=collection.source()"/>

/**
 * @class ${collection.classname()}
 *
 * @brief Getter to populate the `${collection.name()}` json object the part of REDFISH response
 */
class ${collection.classname()} : public ${collection.base_class(is_dynamic, parent_entity)}
{
    public:
    ${collection.classname()}(const app::entity::IEntity::InstancePtr instance)
        : ${collection.base_class(is_dynamic, parent_entity)}("${collection.name()}", instance)
    {}
    ${collection.classname()}(): ${collection.base_class(is_dynamic, parent_entity)}("${collection.name()}")
    {}
    ~${collection.classname()}() override = default;
    protected:
    /**
     * @brief Get conditions to obtain relevant ${collection.source()}
     *        instances that is depends on the parent instance or an
     *        enitty relation
     * @return List of collection conditions
     */
    static const entity::IEntity::ConditionsList getStaticConditions()
    {
        static const entity::IEntity::ConditionsList conditions {
        % for condition in collection.conditions():
            ${condition.definition()},
        % endfor
        };
        return conditions;
    }

    const entity::IEntity::ConditionsList getConditions() const override
    {
        return ${collection.classname()}::getStaticConditions();
    }
};
