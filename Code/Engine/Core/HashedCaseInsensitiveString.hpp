//----------------------------------------------------------------------------------------------------
// HashedCaseInsensitiveString.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <string>

//----------------------------------------------------------------------------------------------------
class HashedCaseInsensitiveString
{
public:
    HashedCaseInsensitiveString() = default;
    HashedCaseInsensitiveString(HashedCaseInsensitiveString const& copyFrom);
    HashedCaseInsensitiveString(char const* text);
    HashedCaseInsensitiveString(std::string const& text);

    // Accessors
    unsigned int        GetHash() const;
    std::string const&  GetOriginalString() const;
    char const*         c_str() const;

    // Comparison operators (hash-first, _stricmp fallback)
    bool operator<(HashedCaseInsensitiveString const& compare) const;
    bool operator==(HashedCaseInsensitiveString const& compare) const;
    bool operator!=(HashedCaseInsensitiveString const& compare) const;
    bool operator==(char const* text) const;
    bool operator!=(char const* text) const;
    bool operator==(std::string const& text) const;
    bool operator!=(std::string const& text) const;

    // Assignment operators
    void operator=(HashedCaseInsensitiveString const& assignFrom);
    void operator=(char const* text);
    void operator=(std::string const& text);

private:
    static unsigned int CalcHashForText(char const* text);

    std::string  m_caseIntactText;
    unsigned int m_lowerCaseHash = 0;
};
