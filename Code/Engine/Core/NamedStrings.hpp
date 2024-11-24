//----------------------------------------------------------------------------------------------------
// NamedStrings.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <map>

#include "Engine/Core/XmlUtils.hpp"

//----------------------------------------------------------------------------------------------------
class NamedStrings
{
public:
    void    PopulateFromXmlElementAttributes(XmlElement const& element);
    void    SetValue(String const& keyName, String const& newValue);
    String  GetValue(String const& keyName, String const& defaultValue) const;
    bool    GetValue(String const& keyName, bool defaultValue) const;
    int     GetValue(String const& keyName, int defaultValue) const;
    float   GetValue(String const& keyName, float defaultValue) const;
    String  GetValue(String const& keyName, char const* defaultValue) const;
    Rgba8   GetValue(String const& keyName, Rgba8 const& defaultValue) const;
    Vec2    GetValue(String const& keyName, Vec2 const& defaultValue) const;
    IntVec2 GetValue(String const& keyName, IntVec2 const& defaultValue) const;

private:
    std::map<String, String> m_keyValuePairs;
};
