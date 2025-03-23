//----------------------------------------------------------------------------------------------------
// NamedStrings.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/NamedStrings.hpp"

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
void NamedStrings::PopulateFromXmlElementAttributes(XmlElement const& element)
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
        SetValue(childElement->Name(), childElement->GetText());
        childElement = childElement->NextSiblingElement();
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

    if (it == m_keyValuePairs.end())
    {
        printf("%s not found\n", keyName.c_str());
        
        return defaultValue;
    }
    
    return it->second;
}

//----------------------------------------------------------------------------------------------------
bool NamedStrings::GetValue(String const& keyName, bool const defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it == m_keyValuePairs.end())
    {
        printf("( %s ) is not in the game config!\n", keyName.c_str());
        
        return defaultValue;
    }

    return it->second == "true" || it->second == "1";
}

//----------------------------------------------------------------------------------------------------
int NamedStrings::GetValue(String const& keyName, int const defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it == m_keyValuePairs.end())
    {
        printf("( %s ) is not in the game config!\n", keyName.c_str());
        
        return defaultValue;
    }
    
    return atoi(it->second.c_str());
}

//----------------------------------------------------------------------------------------------------
float NamedStrings::GetValue(String const& keyName, float const defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it == m_keyValuePairs.end())
    {
        printf("( %s ) is not in the game config!\n", keyName.c_str());
        
        return defaultValue;
    }

    return static_cast<float>(atof(it->second.c_str()));
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

    if (it == m_keyValuePairs.end())
    {
        printf("( %s ) is not in the game config!\n", keyName.c_str());
        
        return defaultValue;
    }
    
    Rgba8 result = defaultValue;
    result.SetFromText(it->second.c_str());

    return result;

}

//----------------------------------------------------------------------------------------------------
Vec2 NamedStrings::GetValue(String const& keyName, Vec2 const& defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it == m_keyValuePairs.end())
    {
        printf("( %s ) is not in the game config!\n", keyName.c_str());
        
        return defaultValue;
    }
    
    Vec2 result = defaultValue;
    result.SetFromText(it->second.c_str());

    return result;
}

//----------------------------------------------------------------------------------------------------
IntVec2 NamedStrings::GetValue(String const& keyName, IntVec2 const& defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it == m_keyValuePairs.end())
    {
        printf("( %s ) is not in the game config!\n", keyName.c_str());
        
        return defaultValue;
    }
    
    IntVec2 result = defaultValue;
    result.SetFromText(it->second.c_str());

    return result;
}
