//----------------------------------------------------------------------------------------------------
// NamedStrings.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/NamedStrings.hpp"

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

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

    // XmlAttribute const* attribute = element.FirstAttribute();
    // while (attribute)
    // {
    //     SetValue(attribute->Name(), attribute->Value());
    //     attribute = attribute->Next();
    // }
    //
    // // 然後處理當前元素的 child elements（含屬性 + inner text）
    // XmlElement const* childElement = element.FirstChildElement();
    // while (childElement)
    // {
    //     // 先處理 child 的屬性
    //     XmlAttribute const* childAttr = childElement->FirstAttribute();
    //     while (childAttr)
    //     {
    //         // 格式化 key，避免重複，如 GamePachinkoMachine2D.BallRadius
    //         String key = Stringf("%s.%s", childElement->Name(), childAttr->Name());
    //         SetValue(key, childAttr->Value());
    //         childAttr = childAttr->Next();
    //     }
    //
    //     // 再處理 child 的文字內容
    //     if (childElement->GetText())
    //     {
    //         SetValue(childElement->Name(), childElement->GetText());
    //     }
    //
    //     childElement = childElement->NextSiblingElement();
    // }

    // std::string currentPath = element.Name();
    //
    // // 1. 處理當前 element 的屬性
    // XmlAttribute const* attribute = element.FirstAttribute();
    // while (attribute)
    // {
    //     std::string key = currentPath + "." + attribute->Name();
    //     SetValue(key, attribute->Value());
    //     attribute = attribute->Next();
    // }
    //
    // // 2. 處理當前 element 的文字（如果是 <tag>value</tag> 這種）
    // if (element.GetText())
    // {
    //     SetValue(currentPath, element.GetText());
    // }
    //
    // // 3. 遞迴處理子元素
    // XmlElement const* child = element.FirstChildElement();
    // while (child)
    // {
    //     PopulateFromXmlElementAttributes(*child);
    //     child = child->NextSiblingElement();
    // }


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
Vec3 NamedStrings::GetValue(String const& keyName, Vec3 const& defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it == m_keyValuePairs.end())
    {
        printf("( %s ) is not in the game config!\n", keyName.c_str());

        return defaultValue;
    }

    Vec3 result = defaultValue;
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

//----------------------------------------------------------------------------------------------------
EulerAngles NamedStrings::GetValue(String const& keyName, EulerAngles const& defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it == m_keyValuePairs.end())
    {
        printf("( %s ) is not in the game config!\n", keyName.c_str());

        return defaultValue;
    }

    EulerAngles result = defaultValue;
    result.SetFromText(it->second.c_str());

    return result;
}

FloatRange NamedStrings::GetValue(String const& keyName, FloatRange const& defaultValue) const
{
    auto const it = m_keyValuePairs.find(keyName);

    if (it == m_keyValuePairs.end())
    {
        printf("( %s ) is not in the game config!\n", keyName.c_str());

        return defaultValue;
    }

    FloatRange result = defaultValue;
    result.SetFromText(it->second.c_str());

    return result;
}
