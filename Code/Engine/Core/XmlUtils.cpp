//----------------------------------------------------------------------------------------------------
// XmlUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/XmlUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
int ParseXmlAttribute(XmlElement const& element,
                      char const*       attributeName,
                      int const         defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        return atoi(attributeValue);
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
char ParseXmlAttribute(XmlElement const& element,
                       char const*       attributeName,
                       char const        defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue && attributeValue[0] != '\0')
    {
        return attributeValue[0];
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
bool ParseXmlAttribute(XmlElement const& element,
                       char const*       attributeName,
                       bool const        defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        return
            _stricmp(attributeValue, "true") == 0 ||
            strcmp(attributeValue, "1") == 0;
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
float ParseXmlAttribute(XmlElement const& element,
                        char const*       attributeName,
                        float const       defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        return static_cast<float>(atof(attributeValue));
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
Rgba8 ParseXmlAttribute(XmlElement const& element,
                        char const*       attributeName,
                        Rgba8 const&      defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        Rgba8       result        = defaultValue;
        Rgba8 const previousValue = result;

        result.SetFromText(attributeValue);

        if (result.r != previousValue.r ||
            result.g != previousValue.g ||
            result.b != previousValue.b ||
            result.a != previousValue.a)
        {
            return result;
        }
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
Vec2 ParseXmlAttribute(XmlElement const& element,
                       char const*       attributeName,
                       Vec2 const&       defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        Vec2       result        = defaultValue;
        Vec2 const previousValue = result;

        result.SetFromText(attributeValue);

        if (result.x != previousValue.x ||
            result.y != previousValue.y)
        {
            return result;
        }
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
IntVec2 ParseXmlAttribute(XmlElement const& element,
                          char const*       attributeName,
                          IntVec2 const&    defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        IntVec2       result        = defaultValue;
        IntVec2 const previousValue = result;

        result.SetFromText(attributeValue);

        if (result.x != previousValue.x ||
            result.y != previousValue.y)
        {
            return result;
        }
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
Vec3 ParseXmlAttribute(XmlElement const& element,
                       char const*       attributeName,
                       Vec3 const&       defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        Vec3       result        = defaultValue;
        Vec3 const previousValue = result;

        result.SetFromText(attributeValue);

        if (result.x != previousValue.x ||
            result.y != previousValue.y ||
            result.z != previousValue.z)
        {
            return result;
        }
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
EulerAngles ParseXmlAttribute(XmlElement const&  element,
                              char const*        attributeName,
                              EulerAngles const& defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        EulerAngles       result        = defaultValue;
        EulerAngles const previousValue = result;

        result.SetFromText(attributeValue);

        if (result.m_yawDegrees != previousValue.m_yawDegrees ||
            result.m_pitchDegrees != previousValue.m_pitchDegrees ||
            result.m_rollDegrees != previousValue.m_rollDegrees)
        {
            return result;
        }
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
FloatRange ParseXmlAttribute(XmlElement const& element,
                             char const*       attributeName,
                             FloatRange const& defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        FloatRange       result        = defaultValue;
        FloatRange const previousValue = result;

        result.SetFromText(attributeValue);

        if (result.m_min != previousValue.m_min ||
            result.m_max != previousValue.m_max)
        {
            return result;
        }
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
String ParseXmlAttribute(XmlElement const& element,
                         char const*       attributeName,
                         String const&     defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        return attributeValue;
    }

    return defaultValue;
}

//----------------------------------------------------------------------------------------------------
StringList ParseXmlAttribute(XmlElement const& element,
                             char const*       attributeName,
                             StringList const& defaultValues)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        StringList result = SplitStringOnDelimiter(attributeValue, ',');

        return result;
    }

    return defaultValues;
}

//----------------------------------------------------------------------------------------------------
String ParseXmlAttribute(XmlElement const& element,
                         char const*       attributeName,
                         char const*       defaultValue)
{
    char const* attributeValue = element.Attribute(attributeName);

    if (attributeValue)
    {
        return attributeValue;
    }

    return defaultValue;
}
