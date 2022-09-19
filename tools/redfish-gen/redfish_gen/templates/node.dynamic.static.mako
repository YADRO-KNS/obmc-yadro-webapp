<%page args="parameter"/>

/** @brief The node parameter value */
static constexpr const char* parameterValue = "${instance.node_parameter().value()}";

/** 
 * @brief Do verify captured by redfish-request parameter value to the defined static value
 * @param ctx   - the REDFISH context
 * @param value - the value from REDFISH-request
 * @tparam TValue - type of value to verify
 * @return true if specified value match to defined static value
 * @return false otherwise
 */ 
template<typename TValue = std::string>
static bool matchParameter(const RedfishContextPtr ctx, const TValue& segment)
{
    return segment == parameterValue;
}
