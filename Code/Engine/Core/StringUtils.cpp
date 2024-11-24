//----------------------------------------------------------------------------------------------------
// StringUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"

#include <cstdarg>

//----------------------------------------------------------------------------------------------------
constexpr int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;

//----------------------------------------------------------------------------------------------------
String const Stringf(char const* format, ...)
{
    char    textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH];
    va_list variableArgumentList;
    va_start(variableArgumentList, format);
    (void) vsnprintf_s(textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList);
    va_end(variableArgumentList);
    textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

    return String(textLiteral);
}

//----------------------------------------------------------------------------------------------------
String const Stringf(int const maxLength, char const* format, ...)
{
    char  textLiteralSmall[STRINGF_STACK_LOCAL_TEMP_LENGTH];
    char* textLiteral = textLiteralSmall;
    if (maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
        textLiteral = new char[maxLength];

    va_list variableArgumentList;
    va_start(variableArgumentList, format);
    (void) vsnprintf_s(textLiteral, maxLength, _TRUNCATE, format, variableArgumentList);
    va_end(variableArgumentList);

    textLiteral[maxLength - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

    String returnValue(textLiteral);

    if (maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
        delete[] textLiteral;

    return returnValue;
}

//----------------------------------------------------------------------------------------------------
Strings const SplitStringOnDelimiter(String const& originalString, char const delimiterToSplitOn)
{
    Strings result;
    size_t  start = 0;
    size_t  end   = originalString.find(delimiterToSplitOn);

    if (originalString.empty())
    {
        result.push_back(String(""));
        return result;
    }

    while (end != String::npos)
    {
        result.push_back(String(originalString.substr(start, end - start)));
        start = end + 1; // Move past the delimiter
        end   = originalString.find(delimiterToSplitOn, start);
    }

    // Add the last substring (or the full string if no delimiter was found)
    result.push_back(String(originalString.substr(start)));

    return result;
}
