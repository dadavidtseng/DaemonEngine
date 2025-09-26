//----------------------------------------------------------------------------------------------------
// ScriptCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <any>

#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
// 腳本方法呼叫的結果類型
//----------------------------------------------------------------------------------------------------
struct ScriptMethodResult
{
    bool     success = false;
    std::any result;
    String   errorMessage;

    static ScriptMethodResult Success(std::any const& value = std::any{});
    static ScriptMethodResult Error(String const& message);
};

//----------------------------------------------------------------------------------------------------
// 腳本方法的元資料
//----------------------------------------------------------------------------------------------------
struct ScriptMethodInfo
{
    String              name;
    String              description;
    std::vector<String> parameterTypes;
    String              returnType;

    explicit ScriptMethodInfo(String                     methodName,
                              String                     description = "",
                              std::vector<String> const& parameterTypes      = {},
                              String                     returnType  = "void");
};
