//----------------------------------------------------------------------------------------------------
// ModuleResolver.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/ModuleResolver.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <filesystem>

//----------------------------------------------------------------------------------------------------
ModuleResolver::ModuleResolver(std::string const& basePath)
    : m_basePath(NormalizePath(basePath))
{
    // Ensure base path ends with separator
    if (!m_basePath.empty() && m_basePath.back() != '/')
    {
        m_basePath += '/';
    }
}

//----------------------------------------------------------------------------------------------------
std::string ModuleResolver::Resolve(std::string const& specifier, std::string const& referrerPath) const
{
    // Check for named module first
    std::string namedPath = ResolveNamed(specifier);
    if (!namedPath.empty())
    {
        return namedPath;
    }

    // Check if relative import (starts with './' or '../')
    if (specifier.size() >= 2 &&
        ((specifier[0] == '.' && specifier[1] == '/') ||
         (specifier.size() >= 3 && specifier[0] == '.' && specifier[1] == '.' && specifier[2] == '/')))
    {
        return ResolveRelative(specifier, referrerPath);
    }

    // Otherwise, resolve relative to base path
    std::string resolved = m_basePath + specifier;
    resolved = NormalizePath(resolved);
    resolved = EnsureJsExtension(resolved);

    return resolved;
}

//----------------------------------------------------------------------------------------------------
std::string ModuleResolver::ResolveRelative(std::string const& specifier, std::string const& referrerPath) const
{
    // Get directory of referrer module
    std::string referrerDir = GetDirectoryPath(referrerPath);

    // Combine with specifier
    std::string resolved = referrerDir + specifier;

    // Normalize path (resolve '..' and '.')
    resolved = NormalizePath(resolved);

    // Ensure .js extension
    resolved = EnsureJsExtension(resolved);

    return resolved;
}

//----------------------------------------------------------------------------------------------------
std::string ModuleResolver::ResolveNamed(std::string const& name) const
{
    auto it = m_namedModules.find(name);
    if (it != m_namedModules.end())
    {
        return it->second;
    }
    return "";
}

//----------------------------------------------------------------------------------------------------
void ModuleResolver::RegisterNamedModule(std::string const& name, std::string const& path)
{
    std::string absolutePath = path;

    // If path is relative, make it absolute using base path
    if (!std::filesystem::path(path).is_absolute())
    {
        absolutePath = m_basePath + path;
    }

    absolutePath = NormalizePath(absolutePath);
    absolutePath = EnsureJsExtension(absolutePath);

    m_namedModules[name] = absolutePath;
}

//----------------------------------------------------------------------------------------------------
void ModuleResolver::SetBasePath(std::string const& basePath)
{
    m_basePath = NormalizePath(basePath);

    // Ensure base path ends with separator
    if (!m_basePath.empty() && m_basePath.back() != '/')
    {
        m_basePath += '/';
    }
}

//----------------------------------------------------------------------------------------------------
std::string ModuleResolver::NormalizePath(std::string const& path)
{
    if (path.empty())
    {
        return path;
    }

    // Use std::filesystem for robust path normalization
    try
    {
        std::filesystem::path p(path);
        p = p.lexically_normal();
        std::string normalized = p.string();

        // Convert backslashes to forward slashes for consistency
        for (char& c : normalized)
        {
            if (c == '\\')
            {
                c = '/';
            }
        }

        return normalized;
    }
    catch (...)
    {
        // Fallback: manual normalization
        std::string normalized = path;
        for (char& c : normalized)
        {
            if (c == '\\')
            {
                c = '/';
            }
        }
        return normalized;
    }
}

//----------------------------------------------------------------------------------------------------
std::string ModuleResolver::EnsureJsExtension(std::string const& path)
{
    // Check if already has .js extension
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js")
    {
        return path;
    }

    // Add .js extension
    return path + ".js";
}

//----------------------------------------------------------------------------------------------------
std::string ModuleResolver::GetDirectoryPath(std::string const& filePath)
{
    std::filesystem::path p(filePath);
    std::filesystem::path dir = p.parent_path();

    std::string dirStr = dir.string();

    // Convert backslashes to forward slashes
    for (char& c : dirStr)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }

    // Ensure ends with separator
    if (!dirStr.empty() && dirStr.back() != '/')
    {
        dirStr += '/';
    }

    return dirStr;
}
