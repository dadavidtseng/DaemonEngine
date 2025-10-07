//----------------------------------------------------------------------------------------------------
// IScriptableObject.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include <functional>
//----------------------------------------------------------------------------------------------------
#include "Engine/Script/ScriptCommon.hpp"

//----------------------------------------------------------------------------------------------------
/// @brief Type alias for script method arguments using type-erased std::any vector
using ScriptArgs     = std::vector<std::any>;
using MethodFunction = std::function<ScriptMethodResult(ScriptArgs const&)>;

//----------------------------------------------------------------------------------------------------
/// @brief Abstract interface for C++ objects that can be exposed to JavaScript via V8 scripting system
///
/// @remark This interface enables seamless integration between C++ objects and JavaScript code through
///         the V8 engine. Any C++ class implementing this interface can be registered with the 
///         ScriptSubsystem and accessed from JavaScript with full type safety and error handling.
///
/// @remark The interface supports both method-based access (CallMethod) and property-based access 
///         (GetProperty/SetProperty) for flexible JavaScript API design.
///
/// @see ScriptSubsystem::RegisterScriptableObject for registration process
/// @see GameScriptInterface for a comprehensive implementation example
//----------------------------------------------------------------------------------------------------
class IScriptableObject
{
public:
    //------------------------------------------------------------------------------------------------
    /// @brief Virtual destructor ensuring proper cleanup of derived classes
    //------------------------------------------------------------------------------------------------
    virtual ~IScriptableObject() = default;

    //------------------------------------------------------------------------------------------------
    /// @brief Get comprehensive information about all methods available for JavaScript calls
    ///
    /// @return Vector of ScriptMethodInfo structures containing method signatures and descriptions
    ///
    /// @remark Each ScriptMethodInfo should include method name, description, parameter types,
    ///         and return type for proper JavaScript integration and error handling.
    ///
    /// @see ScriptMethodInfo structure definition in ScriptCommon.hpp
    //------------------------------------------------------------------------------------------------
    virtual std::vector<ScriptMethodInfo> GetAvailableMethods() const = 0;

    virtual void InitializeMethodRegistry() =0;

    //------------------------------------------------------------------------------------------------
    /// @brief Get list of all properties available for JavaScript property access
    ///
    /// @return Vector of property names that can be used with GetProperty/SetProperty
    ///
    /// @remark Used by ScriptSubsystem to automatically register V8 property accessors.
    ///         Default implementation returns empty vector - override to enable property access.
    ///
    /// @remark Property names should follow JavaScript conventions (camelCase) and be descriptive.
    //------------------------------------------------------------------------------------------------
    virtual StringList GetAvailableProperties() const = 0;

    //------------------------------------------------------------------------------------------------
    /// @brief Execute a method call from JavaScript with type-safe parameter handling
    ///
    /// @param methodName Name of the method to invoke (must match entry in GetAvailableMethods)
    /// @param args Vector of std::any containing method arguments from JavaScript
    ///
    /// @return ScriptMethodResult containing either success value or detailed error information
    ///
    /// @remark This is the core method-based JavaScript interop mechanism. Implementation should:
    ///         - Validate method name against available methods
    ///         - Perform type checking and conversion of arguments
    ///         - Execute the requested functionality
    ///         - Return properly typed results or comprehensive error messages
    ///
    /// @warning Arguments must be carefully validated and converted from std::any to prevent
    ///          runtime errors and maintain type safety across the C++/JavaScript boundary.
    ///
    /// @see ScriptMethodResult for return value specifications
    /// @see ScriptTypeExtractor for safe type conversion utilities
    //------------------------------------------------------------------------------------------------
    virtual ScriptMethodResult CallMethod(String const& methodName, ScriptArgs const& args) = 0;

    //------------------------------------------------------------------------------------------------
    /// @brief Get property value for JavaScript property access (object.propertyName)
    ///
    /// @param propertyName Name of the property to retrieve
    ///
    /// @return std::any containing the property value, or empty std::any if property doesn't exist
    ///
    /// @remark Enables JavaScript code to use natural property syntax instead of method calls.
    ///         Default implementation returns empty std::any - override to provide property support.
    ///
    /// @remark Property values are automatically converted by V8 property accessors to appropriate
    ///         JavaScript types (string, number, boolean, etc.).
    ///
    /// @warning Type conversion must be handled carefully to ensure JavaScript receives expected types.
    ///
    /// @see SetProperty for the corresponding property setter functionality
    //------------------------------------------------------------------------------------------------
    virtual std::any GetProperty(String const& propertyName) const = 0;

    //------------------------------------------------------------------------------------------------
    /// @brief Set property value from JavaScript property assignment (object.propertyName = value)
    ///
    /// @param propertyName Name of the property to modify
    /// @param value New value for the property (type-erased from JavaScript)
    ///
    /// @return true if property was successfully set, false if property doesn't exist or assignment failed
    ///
    /// @remark Enables JavaScript code to use natural assignment syntax for modifying object state.
    ///         Default implementation returns false - override to provide property assignment support.
    ///
    /// @remark V8 property accessors automatically convert JavaScript values to std::any before
    ///         calling this method. Implementation should extract and validate the actual type.
    ///
    /// @warning Always validate the value type and range before applying changes to prevent
    ///          undefined behavior from malformed JavaScript input.
    ///
    /// @see GetProperty for the corresponding property getter functionality
    /// @see ScriptTypeExtractor for safe type extraction from std::any
    //------------------------------------------------------------------------------------------------
    virtual bool SetProperty(String const& propertyName, std::any const& value) = 0;

    //------------------------------------------------------------------------------------------------
    /// @brief Check if a specific method is available for JavaScript calls
    ///
    /// @param methodName Name of the method to check
    ///
    /// @return true if method exists in GetAvailableMethods(), false otherwise
    ///
    /// @remark Utility method for validation and introspection. Implementation searches through
    ///         the results of GetAvailableMethods() for matching method name.
    //------------------------------------------------------------------------------------------------
    bool HasMethod(String const& methodName) const;

    //------------------------------------------------------------------------------------------------
    /// @brief Check if a specific property is available for JavaScript property access
    ///
    /// @param propertyName Name of the property to check
    ///
    /// @return true if property exists in GetAvailableProperties(), false otherwise
    ///
    /// @remark Utility method for validation and introspection. Implementation searches through
    ///         the results of GetAvailableProperties() for matching property name.
    //------------------------------------------------------------------------------------------------
    virtual bool HasProperty(String const& propertyName) const;

    std::unordered_map<String, MethodFunction> m_methodRegistry;
};
