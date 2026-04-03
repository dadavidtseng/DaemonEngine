//----------------------------------------------------------------------------------------------------
// NamedProperties.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/NamedProperties.hpp"

//----------------------------------------------------------------------------------------------------
NamedProperties::NamedProperties(NamedProperties const& copyFrom)
{
    for (auto const& pair : copyFrom.m_properties)
    {
        m_properties[pair.first] = pair.second->Clone();
    }
}

//----------------------------------------------------------------------------------------------------
NamedProperties& NamedProperties::operator=(NamedProperties const& assignFrom)
{
    if (this != &assignFrom)
    {
        m_properties.clear();

        for (auto const& pair : assignFrom.m_properties)
        {
            m_properties[pair.first] = pair.second->Clone();
        }
    }

    return *this;
}

//----------------------------------------------------------------------------------------------------
void NamedProperties::SetValue(std::string const& keyName, char const* value)
{
    SetValue<std::string>(keyName, std::string(value));
}

//----------------------------------------------------------------------------------------------------
std::string NamedProperties::GetValue(std::string const& keyName, char const* defaultValue) const
{
    return GetValue<std::string>(keyName, std::string(defaultValue));
}
