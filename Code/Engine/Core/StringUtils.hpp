//----------------------------------------------------------------------------------------------------
// StringUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>

//----------------------------------------------------------------------------------------------------
typedef std::string         String;
typedef std::vector<String> StringList;

//----------------------------------------------------------------------------------------------------
String const     Stringf(char const* format, ...);
String const     Stringf(int maxLength, char const* format, ...);
StringList const SplitStringOnDelimiter(String const& originalString, char delimiterToSplitOn);
String           ToUpperCase(String const& text);
