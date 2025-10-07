//----------------------------------------------------------------------------------------------------
// ScriptCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <any>

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
    String     name;
    String     description;
    StringList parameterTypes;
    String     returnType;

    explicit ScriptMethodInfo(String            methodName,
                              String            description    = "",
                              StringList const& parameterTypes = {},
                              String            returnType     = "void");
};
