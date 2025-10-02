//----------------------------------------------------------------------------------------------------
// ModuleResolver.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <string>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
/// @brief Resolves ES6 module import specifiers to absolute file paths
///
/// @remark Handles three types of module resolution:
///         1. Relative imports: './player.js', '../config.js'
///         2. Named modules: '@engine/core', 'game/systems'
///         3. Absolute paths: 'Data/Scripts/main.js'
///
/// @remark Thread-safe for read operations after initialization
//----------------------------------------------------------------------------------------------------
class ModuleResolver
{
public:
    //------------------------------------------------------------------------------------------------
    /// @brief Construct resolver with base script directory path
    ///
    /// @param basePath Base directory for script files (e.g., "Data/Scripts/")
    //------------------------------------------------------------------------------------------------
    explicit ModuleResolver(std::string const& basePath);

    //------------------------------------------------------------------------------------------------
    /// @brief Resolve module specifier to absolute file path
    ///
    /// @param specifier Import specifier from JavaScript code (e.g., "./player.js")
    /// @param referrerPath Absolute path of the module doing the import
    ///
    /// @return Absolute file path to the requested module
    ///
    /// @remark Resolution order:
    ///         1. Check if named module exists in registry
    ///         2. If starts with './' or '../' → resolve relative to referrer
    ///         3. Otherwise → resolve relative to base path
    //------------------------------------------------------------------------------------------------
    std::string Resolve(std::string const& specifier, std::string const& referrerPath) const;

    //------------------------------------------------------------------------------------------------
    /// @brief Resolve relative import specifier (starts with './' or '../')
    ///
    /// @param specifier Relative import path
    /// @param referrerPath Absolute path of the importing module
    ///
    /// @return Absolute file path
    //------------------------------------------------------------------------------------------------
    std::string ResolveRelative(std::string const& specifier, std::string const& referrerPath) const;

    //------------------------------------------------------------------------------------------------
    /// @brief Resolve named module (e.g., '@engine/core')
    ///
    /// @param name Named module identifier
    ///
    /// @return Absolute file path, or empty string if not found
    //------------------------------------------------------------------------------------------------
    std::string ResolveNamed(std::string const& name) const;

    //------------------------------------------------------------------------------------------------
    /// @brief Register a named module for easier imports
    ///
    /// @param name Named identifier (e.g., "@engine/core")
    /// @param path Absolute or relative path to module file
    ///
    /// @remark Allows imports like: import {foo} from '@engine/core';
    //------------------------------------------------------------------------------------------------
    void RegisterNamedModule(std::string const& name, std::string const& path);

    //------------------------------------------------------------------------------------------------
    /// @brief Set base path for module resolution
    ///
    /// @param basePath New base directory path
    //------------------------------------------------------------------------------------------------
    void SetBasePath(std::string const& basePath);

    //------------------------------------------------------------------------------------------------
    /// @brief Get current base path
    ///
    /// @return Base directory path for scripts
    //------------------------------------------------------------------------------------------------
    std::string GetBasePath() const { return m_basePath; }

    //------------------------------------------------------------------------------------------------
    /// @brief Normalize file path (convert backslashes, remove redundant separators)
    ///
    /// @param path Input file path
    ///
    /// @return Normalized path with forward slashes
    //------------------------------------------------------------------------------------------------
    static std::string NormalizePath(std::string const& path);

    //------------------------------------------------------------------------------------------------
    /// @brief Ensure file path has .js extension
    ///
    /// @param path Input file path
    ///
    /// @return Path with .js extension added if missing
    //------------------------------------------------------------------------------------------------
    static std::string EnsureJsExtension(std::string const& path);

    //------------------------------------------------------------------------------------------------
    /// @brief Extract directory path from full file path
    ///
    /// @param filePath Full file path
    ///
    /// @return Directory portion of path
    //------------------------------------------------------------------------------------------------
    static std::string GetDirectoryPath(std::string const& filePath);

private:
    //------------------------------------------------------------------------------------------------
    // Member variables
    //------------------------------------------------------------------------------------------------

    /// @brief Base directory for script files (e.g., "C:/p4/Personal/SD/ProtogameJS3D/Run/Data/Scripts/")
    std::string m_basePath;

    /// @brief Named module registry (name → absolute path)
    std::unordered_map<std::string, std::string> m_namedModules;
};
