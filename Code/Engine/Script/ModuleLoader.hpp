//----------------------------------------------------------------------------------------------------
// ModuleLoader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Script/ModuleRegistry.hpp"
#include "Engine/Script/ModuleResolver.hpp"
//----------------------------------------------------------------------------------------------------
#include <memory>
#include <string>

// Forward declaration
class ScriptSubsystem;

//----------------------------------------------------------------------------------------------------
/// @brief Loads and manages ES6 JavaScript modules using V8 Module API
///
/// @remark Handles complete module lifecycle:
///         1. Compilation: JavaScript source → v8::Module
///         2. Instantiation: Resolve imports, link modules
///         3. Evaluation: Execute module code
///         4. Caching: Prevent duplicate loads
///
/// @remark Integrates with ScriptSubsystem for V8 isolate and context access
//----------------------------------------------------------------------------------------------------
class ModuleLoader
{
public:
    //------------------------------------------------------------------------------------------------
    /// @brief Construct module loader with script subsystem integration
    ///
    /// @param scriptSystem Pointer to ScriptSubsystem for V8 access
    /// @param basePath Base directory for module resolution (e.g., "Data/Scripts/")
    //------------------------------------------------------------------------------------------------
    ModuleLoader(ScriptSubsystem* scriptSystem, std::string const& basePath);

    //------------------------------------------------------------------------------------------------
    /// @brief Destructor
    //------------------------------------------------------------------------------------------------
    ~ModuleLoader();

    //------------------------------------------------------------------------------------------------
    // Core Module Loading
    //------------------------------------------------------------------------------------------------

    /// @brief Load and execute ES6 module from file
    ///
    /// @param modulePath Absolute or relative path to module file
    ///
    /// @return true if module loaded and executed successfully
    ///
    /// @remark Full lifecycle: Read file → Compile → Instantiate → Evaluate
    bool LoadModule(std::string const& modulePath);

    /// @brief Load and execute ES6 module from source code
    ///
    /// @param moduleCode JavaScript source code with import/export
    /// @param moduleName Name/URL for the module (for error messages and cache)
    ///
    /// @return true if module loaded and executed successfully
    bool LoadModuleFromSource(std::string const& moduleCode, std::string const& moduleName);

    /// @brief Reload existing module (for hot-reload scenarios)
    ///
    /// @param modulePath Module path to reload
    ///
    /// @return true if module reloaded successfully
    ///
    /// @remark Invalidates cached module and re-executes full lifecycle
    bool ReloadModule(std::string const& modulePath);

    //------------------------------------------------------------------------------------------------
    // V8 Module API Integration
    //------------------------------------------------------------------------------------------------

    /// @brief Compile JavaScript source code as ES6 module
    ///
    /// @param isolate V8 isolate
    /// @param context V8 context
    /// @param code JavaScript source code
    /// @param name Module name/URL for debugging
    ///
    /// @return Compiled v8::Module, or empty handle on error
    v8::MaybeLocal<v8::Module> CompileModule(v8::Isolate*              isolate,
                                             v8::Local<v8::Context>    context,
                                             std::string const&        code,
                                             std::string const&        name);

    /// @brief Instantiate module (resolve imports and link dependencies)
    ///
    /// @param isolate V8 isolate
    /// @param context V8 context
    /// @param module Module to instantiate
    ///
    /// @return true if instantiation succeeded
    ///
    /// @remark Calls ResolveModuleCallback for each import statement
    bool InstantiateModule(v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Module> module);

    /// @brief Evaluate module (execute module code)
    ///
    /// @param isolate V8 isolate
    /// @param context V8 context
    /// @param module Module to evaluate
    ///
    /// @return Module evaluation result, or empty handle on error
    v8::MaybeLocal<v8::Value> EvaluateModule(v8::Isolate*           isolate,
                                             v8::Local<v8::Context> context,
                                             v8::Local<v8::Module>  module);

    //------------------------------------------------------------------------------------------------
    // Module Resolution Callback (for V8)
    //------------------------------------------------------------------------------------------------

    /// @brief V8 module resolution callback
    ///
    /// @remark Called by V8 when module encounters import statement
    ///         Must return the imported module (already compiled and cached)
    static v8::MaybeLocal<v8::Module> ResolveModuleCallback(v8::Local<v8::Context>     context,
                                                            v8::Local<v8::String>      specifier,
                                                            v8::Local<v8::FixedArray>  import_attributes,
                                                            v8::Local<v8::Module>      referrer);

    /// @brief import.meta initialization callback
    ///
    /// @remark Called by V8 to set up import.meta object for module
    static void InitializeImportMetaCallback(v8::Local<v8::Context> context,
                                            v8::Local<v8::Module>  module,
                                            v8::Local<v8::Object>  meta);

    /// @brief Dynamic import callback (Phase 3)
    ///
    /// @remark Called by V8 when import() function is used
    ///         Returns a Promise that resolves to the imported module's namespace
    ///
    /// @param context V8 context
    /// @param host_defined_options Additional options (unused)
    /// @param resource_name Module resource name
    /// @param specifier Import specifier (module path)
    /// @param import_attributes Import attributes (unused)
    ///
    /// @return Promise that resolves to module namespace
    static v8::MaybeLocal<v8::Promise> HostImportModuleDynamicallyCallback(
        v8::Local<v8::Context>    context,
        v8::Local<v8::Data>       host_defined_options,
        v8::Local<v8::Value>      resource_name,
        v8::Local<v8::String>     specifier,
        v8::Local<v8::FixedArray> import_attributes);

    //------------------------------------------------------------------------------------------------
    // Registry and Resolver Access
    //------------------------------------------------------------------------------------------------

    /// @brief Get module registry
    ModuleRegistry* GetRegistry() { return m_registry.get(); }

    /// @brief Get module resolver
    ModuleResolver* GetResolver() { return m_resolver.get(); }

    //------------------------------------------------------------------------------------------------
    // Error Handling
    //------------------------------------------------------------------------------------------------

    /// @brief Get last error message
    std::string GetLastError() const { return m_lastError; }

    /// @brief Check if last operation succeeded
    bool HasError() const { return !m_lastError.empty(); }

    /// @brief Clear error state
    void ClearError() { m_lastError.clear(); }

private:
    //------------------------------------------------------------------------------------------------
    // Internal helper methods
    //------------------------------------------------------------------------------------------------

    /// @brief Check if JSEngine/JSGame instances exist in globalThis
    /// @return true if instances exist and should be preserved during hot-reload
    bool CheckForExistingInstances();

    /// @brief Read module file from disk
    bool ReadModuleFile(std::string const& filePath, std::string& outCode);

    /// @brief Set error message
    void SetError(std::string const& error);

    /// @brief Extract V8 exception message
    std::string GetV8ExceptionMessage(v8::Isolate* isolate, v8::Local<v8::Context> context, v8::TryCatch& tryCatch);

    //------------------------------------------------------------------------------------------------
    // Member variables
    //------------------------------------------------------------------------------------------------

    /// @brief Pointer to script subsystem for V8 access
    ScriptSubsystem* m_scriptSystem;

    /// @brief Module registry (cache and dependency tracking)
    std::unique_ptr<ModuleRegistry> m_registry;

    /// @brief Module path resolver
    std::unique_ptr<ModuleResolver> m_resolver;

    /// @brief Last error message
    std::string m_lastError;

    /// @brief Base path for module resolution
    std::string m_basePath;
};
