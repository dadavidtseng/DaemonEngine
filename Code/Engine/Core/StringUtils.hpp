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
int SplitStringIntoLines( StringList& resultStringVector, String const& originalString );
int SplitStringOnDelimiter( StringList& resultStringVector, std::string const& originalString, char delimiterToSplitOn = ',', bool removeExtraSpace = false );
