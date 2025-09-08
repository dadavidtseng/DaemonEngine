//----------------------------------------------------------------------------------------------------
// StringUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"

// #include <algorithm>
#include <cstdarg>

//----------------------------------------------------------------------------------------------------
int constexpr STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;

//----------------------------------------------------------------------------------------------------
String const Stringf(char const* format, ...)
{
    // Stack-allocated buffer for efficiency with small strings
    char    textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH];
    va_list variableArgumentList;

    // Initialize variable argument list starting after 'format' parameter
    va_start(variableArgumentList, format);

    // Format the string into our buffer with bounds checking
    // _TRUNCATE flag ensures safe truncation if format result exceeds buffer size
    (void)vsnprintf_s(textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList);

    // Clean up variable argument list
    va_end(variableArgumentList);

    // Ensure null termination as a safety measure
    // vsnprintf_s should handle this, but this provides extra protection
    textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH - 1] = '\0';    // In case vsnprintf overran (doesn't auto-terminate)

    // Construct and return String object from formatted text
    return String(textLiteral);
}

//----------------------------------------------------------------------------------------------------
String const Stringf(int const   maxLength,
                     char const* format, ...)
{
    // Default to small stack-allocated buffer for efficiency
    char  textLiteralSmall[STRINGF_STACK_LOCAL_TEMP_LENGTH];
    char* textLiteral = textLiteralSmall;

    // For large strings, allocate on heap to avoid stack overflow
    if (maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
    {
        textLiteral = new char[maxLength];
    }

    va_list variableArgumentList;


    va_start(variableArgumentList, format);     // Initialize variable argument list starting after 'format' parameter
    // Format the string into our buffer with bounds checking
    // _TRUNCATE flag ensures safe truncation if format result exceeds maxLength
    (void)vsnprintf_s(textLiteral, maxLength, _TRUNCATE, format, variableArgumentList);
    va_end(variableArgumentList);               // Clean up variable argument list

    // Ensure null termination as a safety measure
    // This protects against potential vsnprintf edge cases
    textLiteral[maxLength - 1] = '\0';      // In case vsnprintf overran (doesn't auto-terminate)

    // Create String object from formatted text before potential cleanup
    String returnValue(textLiteral);

    // Clean up dynamically allocated memory if we used heap allocation
    if (maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
    {
        delete[] textLiteral;
    }

    return returnValue;
}

//----------------------------------------------------------------------------------------------------
StringList const SplitStringOnDelimiter(String const& originalString,
                                        char const    delimiterToSplitOn)
{
    StringList result;
    size_t     start = 0;
    size_t     end   = originalString.find(delimiterToSplitOn);     // Position of the first character of the found substring or npos if no such substring is found.

    // 1. If originalString is empty, store an empty string, and return.
    if (originalString.empty())
    {
        result.emplace_back("");

        return result;
    }

    // 2. While the end position is not the end ( npos ), keep storing string in the result stringList.
    while (end != String::npos)
    {
        // Store string base on the start and end in the originalString.
        result.push_back(String(originalString.substr(start, end - start)));

        // Move start position past the delimiter, and move end position to the next delimiterToSplitOn.
        start = end + 1;
        end   = originalString.find(delimiterToSplitOn, start);
    }

    // 3. Store the last substring of originalString (or the full string if no delimiter was found).
    result.emplace_back(originalString.substr(start));

    return result;
}

//----------------------------------------------------------------------------------------------------
int SplitStringOnDelimiter(StringList&   out_resultStringList,
                           String const& originalString,
                           char const    delimiterToSplitOn,
                           bool const    shouldRemoveExtraSpace)
{
    out_resultStringList.clear();

    size_t findResultPosition;
    size_t firstPosition = 0;

    do
    {
        findResultPosition = originalString.find_first_of(delimiterToSplitOn, firstPosition);
        String splitString = originalString.substr(firstPosition, findResultPosition - firstPosition);

        if (shouldRemoveExtraSpace && findResultPosition != String::npos)
        {
            size_t const nextNoSpacePosition = originalString.find_first_not_of(' ', findResultPosition + 1);

            if (nextNoSpacePosition != 0 && nextNoSpacePosition != String::npos)
            {
                firstPosition = nextNoSpacePosition;
            }
            else if (nextNoSpacePosition == String::npos)
            {
                findResultPosition = String::npos;
            }
            else
            {
                firstPosition = findResultPosition + 1;
            }

            std::erase(splitString, ' ');
        }
        else
        {
            firstPosition = findResultPosition + 1;
        }

        out_resultStringList.push_back(splitString);
    }
    while (findResultPosition != String::npos);

    return static_cast<int>(out_resultStringList.size());
}

//----------------------------------------------------------------------------------------------------
int SplitStringIntoLines(StringList&   out_resultStringList,
                         String const& originalString)
{
    out_resultStringList.clear();

    size_t findResultPosition;      // Position of the found newline character (or npos if not found)
    size_t firstPosition = 0;       // Starting position for the current substring search

    do
    {
        // Find the position of the next newline character starting from firstPos
        findResultPosition = originalString.find_first_of('\n', firstPosition);
        // Extract substring from firstPos to the newline (or end of string if no newline found)
        String splitString = originalString.substr(firstPosition, findResultPosition - firstPosition);
        // Remove all carriage return characters ('\r') from the extracted line
        // This handles Windows-style line endings (\r\n) by removing the \r part
        std::erase(splitString, '\r');
        // Add the cleaned line to the result vector

        out_resultStringList.push_back(splitString);
        // Move the starting position past the current newline character for next iteration

        firstPosition = findResultPosition + 1;
    }
    while (findResultPosition != String::npos);

    return static_cast<int>(out_resultStringList.size());
}
