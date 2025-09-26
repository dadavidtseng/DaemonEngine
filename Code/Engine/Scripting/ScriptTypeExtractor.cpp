//----------------------------------------------------------------------------------------------------
// ScriptTypeExtractor.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Scripting/ScriptTypeExtractor.hpp"

#include <sstream>
#include <stdexcept>

#include "Engine/Math/Vec3.hpp"
#include "Engine/Scripting/IScriptableObject.hpp"

//----------------------------------------------------------------------------------------------------
Vec3 ScriptTypeExtractor::ExtractVec3(std::vector<std::any> const& args,
                                      size_t const                 startIndex)
{
    if (startIndex + 2 >= args.size())
    {
        throw std::invalid_argument("Vec3 needs 3 parameters (x, y, z)");
    }

    float x = ExtractFloat(args[startIndex]);
    float y = ExtractFloat(args[startIndex + 1]);
    float z = ExtractFloat(args[startIndex + 2]);

    return Vec3(x, y, z);
}

//----------------------------------------------------------------------------------------------------
float ScriptTypeExtractor::ExtractFloat(std::any const& arg)
{
    // Try multiple numeric type conversions
    try
    {
        return std::any_cast<float>(arg);
    }
    catch (const std::bad_any_cast&)
    {
        try
        {
            return static_cast<float>(std::any_cast<double>(arg));
        }
        catch (const std::bad_any_cast&)
        {
            try
            {
                return static_cast<float>(std::any_cast<int>(arg));
            }
            catch (const std::bad_any_cast&)
            {
                throw std::invalid_argument("Unable to convert to float type");
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
int ScriptTypeExtractor::ExtractInt(std::any const& arg)
{
    try
    {
        return std::any_cast<int>(arg);
    }
    catch (const std::bad_any_cast&)
    {
        try
        {
            return static_cast<int>(std::any_cast<float>(arg));
        }
        catch (const std::bad_any_cast&)
        {
            try
            {
                return static_cast<int>(std::any_cast<double>(arg));
            }
            catch (const std::bad_any_cast&)
            {
                throw std::invalid_argument("Unable to convert to int type");
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
std::string ScriptTypeExtractor::ExtractString(std::any const& arg)
{
    try
    {
        return std::any_cast<std::string>(arg);
    }
    catch (const std::bad_any_cast&)
    {
        try
        {
            const char* cstr = std::any_cast<const char*>(arg);
            return std::string(cstr);
        }
        catch (const std::bad_any_cast&)
        {
            throw std::invalid_argument("Unable to convert to string type");
        }
    }
}

//----------------------------------------------------------------------------------------------------
bool ScriptTypeExtractor::ExtractBool(std::any const& arg)
{
    try
    {
        return std::any_cast<bool>(arg);
    }
    catch (const std::bad_any_cast&)
    {
        try
        {
            // Try to convert from numeric value
            int val = std::any_cast<int>(arg);
            return val != 0;
        }
        catch (const std::bad_any_cast&)
        {
            throw std::invalid_argument("Unable to convert to bool type");
        }
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ScriptTypeExtractor::ValidateArgCount(std::vector<std::any> const& args,
                                                         size_t const                 expectedCount,
                                                         String const&                methodName)
{
    if (args.size() != expectedCount)
    {
        std::ostringstream oss;
        oss << methodName << " needs " << expectedCount << " variables, but receives " << args.size();
        return ScriptMethodResult::Error(oss.str());
    }
    return ScriptMethodResult::Success();
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ScriptTypeExtractor::ValidateArgCountRange(std::vector<std::any> const& args,
                                                              size_t const                 minCount,
                                                              size_t const                 maxCount,
                                                              String const&                methodName)
{
    if (args.size() < minCount || args.size() > maxCount)
    {
        std::ostringstream oss;
        oss << methodName << " needs " << minCount << "-" << maxCount << " variables, but receives " << args.size();
        return ScriptMethodResult::Error(oss.str());
    }
    return ScriptMethodResult::Success();
}
