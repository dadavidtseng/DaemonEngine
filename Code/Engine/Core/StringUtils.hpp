//----------------------------------------------------------------------------------------------------
// StringUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>

//----------------------------------------------------------------------------------------------------
typedef std::string         String;
typedef std::vector<String> Strings;

//----------------------------------------------------------------------------------------------------
String const  Stringf(char const* format, ...);
String const  Stringf(int maxLength, char const* format, ...);
Strings const SplitStringOnDelimiter(String const& originalString, char delimiterToSplitOn);
