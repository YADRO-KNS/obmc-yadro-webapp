## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2022, KNS Group LLC (YADRO)

<%page args="parameter"/>

/** @brief The name of field accepted with Node parameter */
static constexpr const char* parameterField = ${instance.node_parameter().value()};
/** @brief The node parameter */
static constexpr const char* parameterValue = "${instance.node_parameter().name()}";
/** @brief The type of IEntity to match the captured parameter value */
using TParameterEntity = app::obmc::entity::${parameter.source()};

/**
 * @brief Do verify captured by redfish-request parameter value to the defined IEntity type
 * @param ctx   - the REDFISH context
 * @param value - the value from REDFISH-request
 * @tparam TValue - type of value to verify
 * @return true if specified value found in the defined IEntity type
 * @return false otherwise
 */
template<typename TValue = std::string>
static bool matchParameter(const RedfishContextPtr ctx, const TValue& value)
{
    return ctx->verifyParameter<TParameterEntity>(getConditions(ctx, value), parameterValue);
}


template <typename TValue = std::string>
static auto getConditions(const RedfishContextPtr ctx, const TValue& value)
{
    return std::forward<IEntity::ConditionsList>({
        BaseEntity::Condition::buildEqual(${instance.classname()}::parameterField, ctx->decodeUriSegment(value)),
    % for condition in parameter.conditions():
        ${condition.definition()},
    % endfor
    });
}

static auto getStaticConditions()
{
    return std::forward<IEntity::ConditionsList>({
    % for condition in parameter.conditions():
        ${condition.definition()},
    % endfor
    });
}

