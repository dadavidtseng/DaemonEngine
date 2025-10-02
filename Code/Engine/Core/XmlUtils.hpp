//----------------------------------------------------------------------------------------------------
// XmlUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include "ThirdParty/TinyXML2/tinyxml2.h"

//----------------------------------------------------------------------------------------------------
using XmlDocument  = tinyxml2::XMLDocument;
using XmlElement   = tinyxml2::XMLElement;
using XmlAttribute = tinyxml2::XMLAttribute;
using XmlResult    = tinyxml2::XMLError;

//-Forward-Declaration--------------------------------------------------------------------------------
struct EulerAngles;
struct FloatRange;
struct IntVec2;
struct Rgba8;
struct Vec2;
struct Vec3;

//----------------------------------------------------------------------------------------------------
int         ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue);
char        ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue);
bool        ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue);
float       ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue);
Rgba8       ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue);
Vec2        ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue);
IntVec2     ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue);
Vec3        ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue);
EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue);
FloatRange  ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange const& defaultValue);
String      ParseXmlAttribute(XmlElement const& element, char const* attributeName, String const& defaultValue);
StringList  ParseXmlAttribute(XmlElement const& element, char const* attributeName, StringList const& defaultValues);

// a custom special-case function for getting an attribute as a std::string,
// even if you provide the “default” value as traditional / hardcoded C-style char const* text (as this will be important later)
String ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue);
