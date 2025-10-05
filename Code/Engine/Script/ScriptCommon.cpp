//----------------------------------------------------------------------------------------------------
// ScriptCommon.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/ScriptCommon.hpp"

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ScriptMethodResult::Success(std::any const& value)
{
    ScriptMethodResult result;
    result.success = true;
    result.result  = value;
    return result;
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ScriptMethodResult::Error(String const& message)
{
    ScriptMethodResult result;
    result.success      = false;
    result.errorMessage = message;
    return result;
}

//----------------------------------------------------------------------------------------------------
ScriptMethodInfo::ScriptMethodInfo(String                     methodName,
                                   String                     description,
                                   std::vector<String> const& parameterTypes,
                                   String                     returnType)
    : name(std::move(methodName)),
      description(std::move(description)),
      parameterTypes(parameterTypes),
      returnType(std::move(returnType))
{
}
