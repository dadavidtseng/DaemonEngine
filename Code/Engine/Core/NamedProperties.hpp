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

    // Type mismatch — return default
    return defaultValue;
}
