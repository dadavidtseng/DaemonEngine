//----------------------------------------------------------------------------------------------------
// ModuleRegistry.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
//----------------------------------------------------------------------------------------------------
// Prevent Windows.h min/max macros from conflicting with V8 headers
#ifndef NOMINMAX
#define NOMINMAX
#endif

#pragma warning(push)
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4324)  // structure was padded due to alignment specifier

#include "v8.h"

#pragma warning(pop)
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
/// @brief Module information for tracking and caching
//----------------------------------------------------------------------------------------------------
struct ModuleInfo
{
    std::string url;                // Module URL/path
    std::string sourceCode;         // Original source code
    size_t      sourceHash;         // Hash for change detection
    bool        isInstantiated;     // Has module been instantiated?
    bool        isEvaluated;        // Has module been evaluated?

    ModuleInfo()
        : url(), sourceCode(), sourceHash(0), isInstantiated(false), isEvaluated(false)
    {
    }
};

//----------------------------------------------------------------------------------------------------
/// @brief Registry for ES6 modules with dependency tracking and caching
///
/// @remark Manages module lifecycle:
///         1. Registration: Cache compiled v8::Module instances
///         2. Dependency tracking: Build import/export graph
///         3. Invalidation: Support for hot-reload scenarios
///
/// @remark Thread-safe for read operations; write operations should be synchronized externally
//----------------------------------------------------------------------------------------------------
class ModuleRegistry
{
public:
    //------------------------------------------------------------------------------------------------
    /// @brief Construct module registry with V8 isolate
    ///
    /// @param isolate V8 isolate for persistent handle management
    //------------------------------------------------------------------------------------------------
    explicit ModuleRegistry(v8::Isolate* isolate);

    //------------------------------------------------------------------------------------------------
    /// @brief Destructor - cleans up persistent V8 handles
    //------------------------------------------------------------------------------------------------
    ~ModuleRegistry();

    //------------------------------------------------------------------------------------------------
    // Module Registration and Lookup
    //------------------------------------------------------------------------------------------------

    /// @brief Register a compiled module in the registry
    ///
    /// @param url Module URL/path (absolute)
    /// @param module V8 module instance
    /// @param sourceCode Original JavaScript source code
    ///
    /// @remark Creates persistent handle to prevent garbage collection
    void RegisterModule(std::string const& url, v8::Local<v8::Module> module, std::string const& sourceCode);

    /// @brief Get cached module by URL
    ///
    /// @param url Module URL/path
    ///
    /// @return V8 module instance, or empty handle if not found
    v8::Local<v8::Module> GetModule(std::string const& url) const;

    /// @brief Check if module is registered
    ///
    /// @param url Module URL/path
    ///
    /// @return true if module exists in registry
    bool HasModule(std::string const& url) const;

    /// @brief Get module information
    ///
    /// @param url Module URL/path
    ///
    /// @return Module info, or nullptr if not found
    ModuleInfo const* GetModuleInfo(std::string const& url) const;

    //------------------------------------------------------------------------------------------------
    // Dependency Graph Management
    //------------------------------------------------------------------------------------------------

    /// @brief Add dependency relationship between modules
    ///
    /// @param importerUrl URL of module doing the import
    /// @param importeeUrl URL of module being imported
    ///
    /// @remark Used to build dependency graph for cascade reloads
    void AddDependency(std::string const& importerUrl, std::string const& importeeUrl);

    /// @brief Get modules that depend on (import) the specified module
    ///
    /// @param moduleUrl Module URL to query
    ///
    /// @return List of dependent module URLs
    ///
    /// @remark Used for cascade reload: if module A changes, reload all modules that import A
    std::vector<std::string> GetDependents(std::string const& moduleUrl) const;

    /// @brief Get modules that the specified module depends on (imports)
    ///
    /// @param moduleUrl Module URL to query
    ///
    /// @return List of dependency module URLs
    std::vector<std::string> GetDependencies(std::string const& moduleUrl) const;

    /// @brief Get all modules in topological order (dependencies first)
    ///
    /// @return Ordered list of module URLs safe for sequential loading
    ///
    /// @remark Useful for reload scenarios where order matters
    std::vector<std::string> GetModulesInLoadOrder() const;

    //------------------------------------------------------------------------------------------------
    // Module Lifecycle Management
    //------------------------------------------------------------------------------------------------

    /// @brief Mark module as instantiated (imports resolved)
    ///
    /// @param url Module URL
    void MarkInstantiated(std::string const& url);

    /// @brief Mark module as evaluated (code executed)
    ///
    /// @param url Module URL
    void MarkEvaluated(std::string const& url);

    /// @brief Invalidate module for hot-reload
    ///
    /// @param url Module URL to invalidate
    ///
    /// @remark Removes module from cache but preserves dependency graph
    void InvalidateModule(std::string const& url);

    /// @brief Invalidate module and all its dependents
    ///
    /// @param url Root module URL
    ///
    /// @return List of invalidated module URLs
    std::vector<std::string> InvalidateModuleTree(std::string const& url);

    /// @brief Clear entire registry
    ///
    /// @remark Used for full reset scenarios
    void Clear();

    //------------------------------------------------------------------------------------------------
    // Statistics and Debugging
    //------------------------------------------------------------------------------------------------

    /// @brief Get number of registered modules
    size_t GetModuleCount() const { return m_modules.size(); }

    /// @brief Get all registered module URLs
    std::vector<std::string> GetAllModuleUrls() const;

    /// @brief Check for circular dependencies
    ///
    /// @param moduleUrl Starting module URL
    ///
    /// @return true if circular dependency detected
    bool HasCircularDependency(std::string const& moduleUrl) const;

private:
    //------------------------------------------------------------------------------------------------
    // Internal helper methods
    //------------------------------------------------------------------------------------------------

    /// @brief Recursive topological sort helper
    void TopologicalSortHelper(std::string const&              moduleUrl,
                               std::unordered_set<std::string>& visited,
                               std::unordered_set<std::string>& recursionStack,
                               std::vector<std::string>&        result) const;

    /// @brief Detect circular dependencies recursively
    bool HasCircularDependencyHelper(std::string const&              moduleUrl,
                                     std::unordered_set<std::string>& visited,
                                     std::unordered_set<std::string>& recursionStack) const;

    //------------------------------------------------------------------------------------------------
    // Member variables
    //------------------------------------------------------------------------------------------------

    /// @brief V8 isolate for persistent handle management
    v8::Isolate* m_isolate;

    /// @brief Module cache: URL → Persistent V8 Module handle
    std::unordered_map<std::string, std::unique_ptr<v8::Persistent<v8::Module>>> m_modules;

    /// @brief Module metadata: URL → ModuleInfo
    std::unordered_map<std::string, ModuleInfo> m_moduleInfo;

    /// @brief Dependency graph: Module URL → Set of modules it imports
    std::unordered_map<std::string, std::unordered_set<std::string>> m_dependencies;

    /// @brief Reverse dependency graph: Module URL → Set of modules that import it
    std::unordered_map<std::string, std::unordered_set<std::string>> m_dependents;
};
