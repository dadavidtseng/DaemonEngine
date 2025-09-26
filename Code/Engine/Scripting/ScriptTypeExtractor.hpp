//----------------------------------------------------------------------------------------------------
// ScriptTypeExtractor.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <any>
#include <string>
#include <vector>

#include "Engine/Scripting/IScriptableObject.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
struct Vec3;
struct ScriptMethodResult;

//----------------------------------------------------------------------------------------------------
// Static utility class for type extraction and validation in script interfaces
// Provides reusable methods for converting std::any arguments to specific types
// with comprehensive error handling and type safety
//----------------------------------------------------------------------------------------------------
class ScriptTypeExtractor
{
public:
    // Static utility class - no instantiation allowed
    ScriptTypeExtractor()                                      = delete;
    ~ScriptTypeExtractor()                                     = delete;
    ScriptTypeExtractor(ScriptTypeExtractor const&)            = delete;
    ScriptTypeExtractor& operator=(ScriptTypeExtractor const&) = delete;

    // Type extraction methods
    static Vec3   ExtractVec3(std::vector<std::any> const& args, size_t startIndex);
    static float  ExtractFloat(std::any const& arg);
    static int    ExtractInt(std::any const& arg);
    static String ExtractString(std::any const& arg);
    static bool   ExtractBool(std::any const& arg);

    // Argument validation utilities
    static ScriptMethodResult ValidateArgCount(std::vector<std::any> const& args,
                                               size_t                       expectedCount,
                                               String const&                methodName);

    static ScriptMethodResult ValidateArgCountRange(std::vector<std::any> const& args,
                                                    size_t                       minCount,
                                                    size_t                       maxCount,
                                                    String const&                methodName);
};
