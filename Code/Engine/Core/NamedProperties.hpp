//----------------------------------------------------------------------------------------------------
// NamedProperties.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/HashedCaseInsensitiveString.hpp"
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <map>
#include <memory>
#include <type_traits>

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/XmlUtils.hpp"

//----------------------------------------------------------------------------------------------------
class NamedProperties
{
public:
    NamedProperties() = default;
    NamedProperties(NamedProperties const& copyFrom);
    NamedProperties& operator=(NamedProperties const& assignFrom);
    ~NamedProperties() = default;

    // Template SetValue / GetValue
    template <typename T>
    void SetValue(std::string const& keyName, T const& value);

    template <typename T>
    T GetValue(std::string const& keyName, T const& defaultValue) const;

    // Explicit char const* overloads (stores as std::string)
    void        SetValue(std::string const& keyName, char const* value);
    std::string GetValue(std::string const& keyName, char const* defaultValue) const;

    // Backward compatibility with NamedStrings
    void PopulateFromXmlElementAttributes(XmlElement const& element);

private:
    //----------------------------------------------------------------------------------------------------
    // Type-erased property base
    //----------------------------------------------------------------------------------------------------
    struct PropertyBase
    {
        virtual ~PropertyBase() = default;
        virtual std::unique_ptr<PropertyBase> Clone() const = 0;
    };

    //----------------------------------------------------------------------------------------------------
    // Typed property storage
    //----------------------------------------------------------------------------------------------------
    template <typename T>
    struct TypedProperty : public PropertyBase
    {
        T m_value;

        explicit TypedProperty(T const& value)
            : m_value(value)
        {
        }

        std::unique_ptr<PropertyBase> Clone() const override
        {
            return std::make_unique<TypedProperty<T>>(m_value);
        }
    };

    std::map<HashedCaseInsensitiveString, std::unique_ptr<PropertyBase>> m_properties;
};

//----------------------------------------------------------------------------------------------------
// SFINAE helper: detect if T has SetFromText(char const*)
//----------------------------------------------------------------------------------------------------
template <typename T, typename = void>
struct HasSetFromText : std::false_type {};

template <typename T>
struct HasSetFromText<T, std::void_t<decltype(std::declval<T>().SetFromText(std::declval<char const*>()))>> : std::true_type {};

//----------------------------------------------------------------------------------------------------
// ParseFromString: convert std::string to T
//----------------------------------------------------------------------------------------------------

// Default: types with SetFromText
template <typename T>
typename std::enable_if<HasSetFromText<T>::value, T>::type
ParseFromString(std::string const& str, T const& defaultValue)
{
    T result = defaultValue;
    result.SetFromText(str.c_str());
    return result;
}

// Fallback: types without SetFromText — return default
template <typename T>
typename std::enable_if<!HasSetFromText<T>::value, T>::type
ParseFromString(std::string const& /*str*/, T const& defaultValue)
{
    return defaultValue;
}

// Explicit specializations for primitives
template <> inline float         ParseFromString<float>(std::string const& str, float const& defaultValue)         { return str.empty() ? defaultValue : static_cast<float>(atof(str.c_str())); }
template <> inline int           ParseFromString<int>(std::string const& str, int const& defaultValue)             { return str.empty() ? defaultValue : atoi(str.c_str()); }
template <> inline bool          ParseFromString<bool>(std::string const& str, bool const& defaultValue)           { if (str.empty()) return defaultValue; return str == "true" || str == "1"; }
template <> inline unsigned short ParseFromString<unsigned short>(std::string const& str, unsigned short const& defaultValue) { if (str.empty()) return defaultValue; return static_cast<unsigned short>(std::stoul(str)); }
template <> inline std::string   ParseFromString<std::string>(std::string const& str, std::string const& /*defaultValue*/)   { return str; }

//----------------------------------------------------------------------------------------------------
// Template implementations
//----------------------------------------------------------------------------------------------------
template <typename T>
void NamedProperties::SetValue(std::string const& keyName, T const& value)
{
    HashedCaseInsensitiveString hcisKey(keyName);
    m_properties[hcisKey] = std::make_unique<TypedProperty<T>>(value);
}

//----------------------------------------------------------------------------------------------------
template <typename T>
T NamedProperties::GetValue(std::string const& keyName, T const& defaultValue) const
{
    static_assert(!std::is_pointer_v<T>,
        "NamedProperties::GetValue does not support pointer types. "
        "Use std::string instead of char const* for string retrieval.");

    HashedCaseInsensitiveString hcisKey(keyName);
    auto it = m_properties.find(hcisKey);

    if (it == m_properties.end())
    {
        return defaultValue;
    }

    // Try exact type match
    TypedProperty<T> const* typedProp = dynamic_cast<TypedProperty<T> const*>(it->second.get());

    if (typedProp)
    {
        return typedProp->m_value;
    }

    // Backward compat: if stored as string, try parsing via SetFromText / atof / atoi
    TypedProperty<std::string> const* stringProp =
        dynamic_cast<TypedProperty<std::string> const*>(it->second.get());

    if (stringProp)
    {
        return ParseFromString<T>(stringProp->m_value, defaultValue);
    }

    // Type mismatch — return default
    return defaultValue;
}
