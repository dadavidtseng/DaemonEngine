//----------------------------------------------------------------------------------------------------
// StringUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>
#include <format>

//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
using String     = std::string;
using StringList = std::vector<std::string>;
// typedef std::string String;
// typedef std::vector<std::string> StringList;

//----------------------------------------------------------------------------------------------------
/// @brief Printf-style string formatting utility with automatic memory management
///
/// @param format C-style format string with printf specifiers (e.g., "%d", "%s", "%f")
/// @param ... Variadic arguments matching the format specifiers in the format string
///
/// @return std::string containing the formatted result (truncated at 2047 chars, null-terminated)
///
/// @remark Uses 2048-byte stack buffer with vsnprintf_s for memory safety. Thread-safe operation.
///
/// @see https://en.cppreference.com/w/cpp/utility/variadic/va_list.html
/// @see https://en.cppreference.com/w/c/io/vfprintf
/// @see https://en.cppreference.com/w/cpp/utility/variadic/va_start.html
//----------------------------------------------------------------------------------------------------
String const Stringf(char const* format, ...);

//----------------------------------------------------------------------------------------------------
/// @brief Printf-style string formatting with configurable buffer size
///
/// @param maxLength Maximum length of output string (heap allocated if >2048, otherwise stack buffer)
/// @param format C-style format string with printf specifiers
/// @param ... Variadic arguments matching the format specifiers in the format string
///
/// @return std::string containing formatted result (truncated at maxLength-1 chars if needed)
///
/// @remark Automatically manages heap allocation/deallocation for large strings. Use standard Stringf() for typical cases.
///
/// @warning Performance overhead for heap allocation when maxLength exceeds 2048 characters.
//----------------------------------------------------------------------------------------------------
String const Stringf(int maxLength, char const* format, ...);

//----------------------------------------------------------------------------------------------------
/// @brief Modern C++20 format-style string formatting with type safety and performance
///
/// @tparam Args Template parameter pack for format arguments (automatically deduced)
/// @param stringToFormat Format string using C++20 format syntax (e.g., "Player {} has {} health")
/// @param args Perfect-forwarded arguments matching format string placeholders
///
/// @return String containing the formatted result (no length limitations, exception-safe)
///
/// @remark Uses C++20 std::format for compile-time format string validation and optimal performance.
/// @remark Preferred over printf-style functions for new code - provides type safety and better error handling.
///
/// @warning Requires C++20 compiler support. Format string syntax differs from printf.
/// @see https://en.cppreference.com/w/cpp/utility/format/format
/// @see https://fmt.dev/latest/syntax.html
//----------------------------------------------------------------------------------------------------
template <typename... Args>
String StringFormat(std::format_string<Args...> stringToFormat, Args&&... args)
{
    return std::format(stringToFormat, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------
/// @brief Split string into substrings using a single character delimiter
///
/// @param originalString Source string to split (const reference, not modified)
/// @param delimiterToSplitOn Single character used as delimiter (e.g., ',', ';', '|')
///
/// @return out_resultStringList containing all substrings (empty string if input is empty)
///
/// @remark Returns vector of strings split on delimiter boundaries. Preserves empty substrings.
/// @remark If input is empty, returns vector containing one empty string element.
///
/// @warning Does not remove whitespace or trim substrings. Use overloaded version for space handling.
/// @see SplitStringOnDelimiter(StringList&, std::string const&, char, bool) for output parameter version
//----------------------------------------------------------------------------------------------------
StringList const SplitStringOnDelimiter(String const& originalString, char delimiterToSplitOn);

//----------------------------------------------------------------------------------------------------
/// @brief Split string into substrings with delimiter and optional whitespace removal
///
/// @param out_resultStringList Output std::vector to store split substrings (cleared before use)
/// @param originalString Source string to split
/// @param delimiterToSplitOn Character delimiter for splitting (default: comma ',')
/// @param shouldRemoveExtraSpace If true, removes spaces from substrings and skips spaces after delimiters
///
/// @return int Number of substrings found and stored in out_resultStringList
///
/// @remark Advanced splitting with whitespace handling for clean parsing of delimited data.
/// @remark When removeExtraSpace=true, both leading/trailing spaces in tokens and spaces after delimiters are removed.
///
/// @warning Clears out_resultStringList before processing. Previous contents are lost.
/// @warning removeExtraSpace affects both substring content and delimiter positioning logic.
//----------------------------------------------------------------------------------------------------
int SplitStringOnDelimiter(StringList& out_resultStringList, String const& originalString, char delimiterToSplitOn = ',', bool shouldRemoveExtraSpace = false);

//----------------------------------------------------------------------------------------------------
/// @brief Split string into lines by newline characters with carriage return handling
///
/// @param out_resultStringList Output vector to store line strings (cleared before use)
/// @param originalString Source string containing newline-separated text
///
/// @return int Number of lines found and stored in out_resultStringList
///
/// @remark Splits on '\n' characters and automatically removes '\r' characters from each line.
/// @remark Handles both Unix (\n) and Windows (\r\n) line endings correctly.
///
/// @warning Clears out_resultStringList before processing. Previous contents are lost.
/// @see https://en.cppreference.com/w/cpp/string/basic_string/find_first_of
//----------------------------------------------------------------------------------------------------
int SplitStringIntoLines(StringList& out_resultStringList, String const& originalString);


