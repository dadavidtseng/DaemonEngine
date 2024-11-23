//----------------------------------------------------------------------------------------------------
// StringUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"

#include <cstdarg>

//-----------------------------------------------------------------------------------------------
constexpr int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;

//-----------------------------------------------------------------------------------------------
const std::string Stringf(char const* format, ...)
{
    char    textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH];
    va_list variableArgumentList;
    va_start(variableArgumentList, format);
    vsnprintf_s(textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList);
    va_end(variableArgumentList);
    textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

    return std::string(textLiteral);
}

//-----------------------------------------------------------------------------------------------
const std::string Stringf(int const maxLength, char const* format, ...)
{
    char  textLiteralSmall[STRINGF_STACK_LOCAL_TEMP_LENGTH];
    char* textLiteral = textLiteralSmall;
    if (maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
        textLiteral = new char[maxLength];

    va_list variableArgumentList;
    va_start(variableArgumentList, format);
    vsnprintf_s(textLiteral, maxLength, _TRUNCATE, format, variableArgumentList);
    va_end(variableArgumentList);
    textLiteral[maxLength - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

    std::string returnValue(textLiteral);
    if (maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
        delete[] textLiteral;

    return returnValue;
}

//----------------------------------------------------------------------------------------------------
Strings SplitStringOnDelimiter(std::string const& originalString, char const delimiterToSplitOn)
{
    Strings result;
    size_t  start = 0;
    size_t  end   = originalString.find(delimiterToSplitOn);

    while (end != std::string::npos)
    {
        result.emplace_back(originalString.substr(start, end - start));
        start = end + 1; // Move past the delimiter
        end   = originalString.find(delimiterToSplitOn, start);
    }

    // Add the last substring (or the full string if no delimiter was found)
    result.emplace_back(originalString.substr(start));

    return result;
}
