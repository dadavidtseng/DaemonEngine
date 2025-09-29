//----------------------------------------------------------------------------------------------------
// IScriptableObject.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Scripting/IScriptableObject.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
bool IScriptableObject::HasMethod(String const& methodName) const
{
    std::vector<ScriptMethodInfo> methods = GetAvailableMethods();

    bool const found = std::ranges::find(methods, methodName, &ScriptMethodInfo::name) != methods.end();

    if (!found)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("(IScriptableObject::HasMethod) Method '{}' not found", methodName));
    }

    return found;
}

//----------------------------------------------------------------------------------------------------
bool IScriptableObject::HasProperty(String const& propertyName) const
{
    StringList properties = GetAvailableProperties();

    bool const found = std::ranges::find(properties, propertyName) != properties.end();

    if (!found)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("(IScriptableObject::HasProperty) Property '{}' not found", propertyName));
    }

    return found;
}
