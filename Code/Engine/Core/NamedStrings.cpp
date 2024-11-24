//----------------------------------------------------------------------------------------------------
// NamedStrings.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/NamedStrings.hpp"

//----------------------------------------------------------------------------------------------------
void NamedStrings::PopulateFromXmlElementAttributes(XmlElement const& element)
{
    XmlAttribute const* attribute = element.FirstAttribute();

    while (attribute)
    {
        m_keyValuePairs[attribute->Name()] = attribute->Value();
        attribute                          = attribute->Next();
    }
}

//----------------------------------------------------------------------------------------------------
void NamedStrings::SetValue(String const& keyName, String const& newValue)
{
    m_keyValuePairs[keyName] = newValue;
}

//----------------------------------------------------------------------------------------------------
String NamedStrings::GetValue(String const& keyName, String const& defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    return it != m_keyValuePairs.end() ? it->second : defaultValue;
}

//----------------------------------------------------------------------------------------------------
bool NamedStrings::GetValue(String const& keyName, bool const defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    return it != m_keyValuePairs.end() ? it->second == "true" || it->second == "1" : defaultValue;
}

//----------------------------------------------------------------------------------------------------
int NamedStrings::GetValue(String const& keyName, int const defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    return it != m_keyValuePairs.end() ? atoi(it->second.c_str()) : defaultValue;
}

//----------------------------------------------------------------------------------------------------
float NamedStrings::GetValue(String const& keyName, float const defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    return it != m_keyValuePairs.end() ? static_cast<float>(atof(it->second.c_str())) : defaultValue;
}

//----------------------------------------------------------------------------------------------------
String NamedStrings::GetValue(String const& keyName, char const* defaultValue) const
{
    return GetValue(keyName, String(defaultValue));
}

//----------------------------------------------------------------------------------------------------
Rgba8 NamedStrings::GetValue(String const& keyName, Rgba8 const& defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it != m_keyValuePairs.end())
    {
        Rgba8 result = defaultValue;
        result.SetFromText(it->second.c_str());

        return result;
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
Vec2 NamedStrings::GetValue(String const& keyName, Vec2 const& defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it != m_keyValuePairs.end())
    {
        Vec2 result = defaultValue;
        result.SetFromText(it->second.c_str());
        
        return result;
    }
    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
IntVec2 NamedStrings::GetValue(String const& keyName, IntVec2 const& defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it != m_keyValuePairs.end())
    {
        IntVec2 result = defaultValue;
        result.SetFromText(it->second.c_str());
        
        return result;
    }
    return defaultValue;
}
