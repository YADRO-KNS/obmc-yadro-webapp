## SPDX-License-Identifier: Apache-2.0
## Copyright (C) 2022, KNS Group LLC (YADRO)

<%page args="enum"/>

/**
 * @Class Redfish enum ${enum.name()}
 */
class ${enum.name()}Enum  : public EnumGetter<${enum.source()}>
{
  public:
    ${enum.name()}Enum(const std::string& field, int value) :
        EnumGetter(field, value)
    {}
    ~${enum.name()}Enum() override = default;

    /**
    * @brief Cast enum form ${enum.source()} entity to std::string
    * @throw std::runtime_error - Unrecognized destionation enum value: <enum-index>
    * @return const std::string - Redfish enum value
    */
    std::string castEnumToString() const override
    {
        static const std::map<${enum.source()}, std::string> mapping {
        % for enum_pair in enum.mapping():
        /** ${enum.description(enum_pair["source"])} */
            {${enum.source()}::${enum_pair["dest"]}, "${enum_pair["source"]}"},
        %endfor
        };
        auto it = mapping.find(getValue());
        if (it == mapping.end())
        {
                throw std::runtime_error("Unrecognized destionation enum value: " +
                                        std::to_string(static_cast<int>(getValue())));
        }
        return it->second;
    }
};

