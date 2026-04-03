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

//----------------------------------------------------------------------------------------------------
void NamedProperties::PopulateFromXmlElementAttributes(XmlElement const& element)
{
    XmlAttribute const* attribute = element.FirstAttribute();

    while (attribute)
    {
        SetValue(attribute->Name(), attribute->Value());
        attribute = attribute->Next();
    }

    XmlElement const* childElement = element.FirstChildElement();

    while (childElement)
    {
        char const* text = childElement->GetText();
        SetValue(childElement->Name(), text ? text : "");
        childElement = childElement->NextSiblingElement();
    }
}
