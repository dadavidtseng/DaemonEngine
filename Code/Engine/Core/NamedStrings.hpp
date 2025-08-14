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
    void                     PopulateFromXmlElementAttributes(XmlElement const& element);
    void                     SetValue(String const& keyName, String const& newValue);
    String                   GetValue(String const& keyName, String const& defaultValue) const;
    bool                     GetValue(String const& keyName, bool defaultValue) const;
    int                      GetValue(String const& keyName, int defaultValue) const;
    unsigned short           GetValue(String const& keyName, unsigned short defaultValue) const;
    float                    GetValue(String const& keyName, float defaultValue) const;
    String                   GetValue(String const& keyName, char const* defaultValue) const;
    Rgba8                    GetValue(String const& keyName, Rgba8 const& defaultValue) const;
    Vec2                     GetValue(String const& keyName, Vec2 const& defaultValue) const;
    Vec3                     GetValue(String const& keyName, Vec3 const& defaultValue) const;
    IntVec2                  GetValue(String const& keyName, IntVec2 const& defaultValue) const;
    EulerAngles              GetValue(String const& keyName, EulerAngles const& defaultValue) const;
    FloatRange               GetValue(String const& keyName, FloatRange const& defaultValue) const;
    std::map<String, String> GetAllKeyValuePairs();

private:
    std::map<String, String> m_keyValuePairs;
};
