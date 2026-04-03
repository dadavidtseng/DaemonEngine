//----------------------------------------------------------------------------------------------------
// HashedCaseInsensitiveString.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/HashedCaseInsensitiveString.hpp"

#include <cctype>
#include <cstring>

//----------------------------------------------------------------------------------------------------
HashedCaseInsensitiveString::HashedCaseInsensitiveString(HashedCaseInsensitiveString const& copyFrom)
    : m_caseIntactText(copyFrom.m_caseIntactText)
    , m_lowerCaseHash(copyFrom.m_lowerCaseHash)
{
}

//----------------------------------------------------------------------------------------------------
HashedCaseInsensitiveString::HashedCaseInsensitiveString(char const* text)
    : m_caseIntactText(text)
    , m_lowerCaseHash(CalcHashForText(text))
{
}

//----------------------------------------------------------------------------------------------------
HashedCaseInsensitiveString::HashedCaseInsensitiveString(std::string const& text)
    : m_caseIntactText(text)
    , m_lowerCaseHash(CalcHashForText(text.c_str()))
{
}

//----------------------------------------------------------------------------------------------------
unsigned int HashedCaseInsensitiveString::GetHash() const
{
    return m_lowerCaseHash;
}

//----------------------------------------------------------------------------------------------------
std::string const& HashedCaseInsensitiveString::GetOriginalString() const
{
    return m_caseIntactText;
}

//----------------------------------------------------------------------------------------------------
char const* HashedCaseInsensitiveString::c_str() const
{
    return m_caseIntactText.c_str();
}

//----------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator<(HashedCaseInsensitiveString const& compare) const
{
    if (m_lowerCaseHash != compare.m_lowerCaseHash)
        return m_lowerCaseHash < compare.m_lowerCaseHash;

    return _stricmp(m_caseIntactText.c_str(), compare.m_caseIntactText.c_str()) < 0;
}

//----------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator==(HashedCaseInsensitiveString const& compare) const
{
    if (m_lowerCaseHash != compare.m_lowerCaseHash)
        return false;

    return _stricmp(m_caseIntactText.c_str(), compare.m_caseIntactText.c_str()) == 0;
}

//----------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator!=(HashedCaseInsensitiveString const& compare) const
{
    return !(*this == compare);
}

//----------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator==(char const* text) const
{
    return *this == HashedCaseInsensitiveString(text);
}

//----------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator!=(char const* text) const
{
    return !(*this == text);
}

//----------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator==(std::string const& text) const
{
    return *this == HashedCaseInsensitiveString(text);
}

//----------------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator!=(std::string const& text) const
{
    return !(*this == text);
}

//----------------------------------------------------------------------------------------------------
void HashedCaseInsensitiveString::operator=(HashedCaseInsensitiveString const& assignFrom)
{
    m_caseIntactText = assignFrom.m_caseIntactText;
    m_lowerCaseHash  = assignFrom.m_lowerCaseHash;
}

//----------------------------------------------------------------------------------------------------
void HashedCaseInsensitiveString::operator=(char const* text)
{
    m_caseIntactText = text;
    m_lowerCaseHash  = CalcHashForText(text);
}

//----------------------------------------------------------------------------------------------------
void HashedCaseInsensitiveString::operator=(std::string const& text)
{
    m_caseIntactText = text;
    m_lowerCaseHash  = CalcHashForText(text.c_str());
}

//----------------------------------------------------------------------------------------------------
unsigned int HashedCaseInsensitiveString::CalcHashForText(char const* text)
{
    unsigned int hash = 0;

    while (*text)
    {
        hash *= 31;
        hash += static_cast<unsigned int>(tolower(*text));
        ++text;
    }

    return hash;
}
