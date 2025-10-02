//----------------------------------------------------------------------------------------------------
// ModuleLoader.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Scripting/ModuleLoader.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Scripting/ScriptSubsystem.hpp"
//----------------------------------------------------------------------------------------------------
#include <fstream>
#include <sstream>

//----------------------------------------------------------------------------------------------------
ModuleLoader::ModuleLoader(ScriptSubsystem* scriptSystem, std::string const& basePath)
    : m_scriptSystem(scriptSystem)
    , m_basePath(basePath)
{
    GUARANTEE_OR_DIE(scriptSystem != nullptr, "ModuleLoader: ScriptSubsystem cannot be null");

    // Get V8 isolate from ScriptSubsystem (we'll need to add accessor method)
    // For now, we'll initialize registry and resolver with the isolate we get later
    m_resolver = std::make_unique<ModuleResolver>(basePath);

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ModuleLoader: Created with base path: {}", basePath));
}

//----------------------------------------------------------------------------------------------------
ModuleLoader::~ModuleLoader()
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ModuleLoader: Destroyed"));
}

//----------------------------------------------------------------------------------------------------
bool ModuleLoader::LoadModule(std::string const& modulePath)
{
    ClearError();

    // Read module file
    std::string code;
    if (!ReadModuleFile(modulePath, code))
    {
        return false;
    }

    // Load from source with resolved path
    std::string resolvedPath = m_resolver->Resolve(modulePath, m_basePath);
    return LoadModuleFromSource(code, resolvedPath);
}

//----------------------------------------------------------------------------------------------------
bool ModuleLoader::LoadModuleFromSource(std::string const& moduleCode, std::string const& moduleName)
{
    ClearError();

    // Get V8 isolate and context from ScriptSubsystem
    void* isolatePtr = m_scriptSystem->GetV8Isolate();
    void* contextPtr = m_scriptSystem->GetV8Context();

    if (!isolatePtr || !contextPtr)
    {
        SetError("ModuleLoader: V8 isolate or context not available");
        return false;
    }

    v8::Isolate* isolate = static_cast<v8::Isolate*>(isolatePtr);
    v8::Local<v8::Context> context = *static_cast<v8::Local<v8::Context>*>(contextPtr);

    // Ensure registry is initialized
    if (!m_registry)
    {
        m_registry = std::make_unique<ModuleRegistry>(isolate);
        DAEMON_LOG(LogScript, eLogVerbosity::Log,
            StringFormat("ModuleLoader: Initialized ModuleRegistry"));
    }

    // PHASE 2: Store ModuleLoader instance in context embedder data for callback access
    // This allows the static ResolveModuleCallback to access this ModuleLoader instance
    context->SetAlignedPointerInEmbedderData(1, this);

    // Phase 1: Compile the module
    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ModuleLoader: Compiling module: {}", moduleName));

    v8::MaybeLocal<v8::Module> maybeModule = CompileModule(isolate, context, moduleCode, moduleName);

    if (maybeModule.IsEmpty())
    {
        // Error already set by CompileModule
        return false;
    }

    v8::Local<v8::Module> module = maybeModule.ToLocalChecked();

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ModuleLoader: Module compiled successfully: {}", moduleName));

    // Phase 2: Instantiate the module (resolve imports)
    // Note: ResolveModuleCallback will now load and compile imported modules
    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ModuleLoader: Instantiating module: {}", moduleName));

    if (!InstantiateModule(isolate, context, module))
    {
        // Error already set by InstantiateModule
        return false;
    }

    m_registry->MarkInstantiated(moduleName);

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ModuleLoader: Module instantiated successfully: {}", moduleName));

    // Phase 3: Evaluate the module (execute code)
    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ModuleLoader: Evaluating module: {}", moduleName));

    v8::MaybeLocal<v8::Value> result = EvaluateModule(isolate, context, module);

    if (result.IsEmpty())
    {
        // Error already set by EvaluateModule
        return false;
    }

    m_registry->MarkEvaluated(moduleName);

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
        StringFormat("ModuleLoader: Module loaded and executed successfully: {}", moduleName));

    return true;
}

//----------------------------------------------------------------------------------------------------
bool ModuleLoader::ReloadModule(std::string const& modulePath)
{
    ClearError();

    // Resolve module path
    std::string resolvedPath = m_resolver->Resolve(modulePath, m_basePath);

    // Check if module registry exists
    if (!m_registry)
    {
        SetError("ModuleLoader: Registry not initialized");
        return false;
    }

    // Invalidate the module
    m_registry->InvalidateModule(resolvedPath);

    // Reload from disk
    return LoadModule(modulePath);
}

//----------------------------------------------------------------------------------------------------
v8::MaybeLocal<v8::Module> ModuleLoader::CompileModule(v8::Isolate*           isolate,
                                                       v8::Local<v8::Context> context,
                                                       std::string const&     code,
                                                       std::string const&     name)
{
    v8::EscapableHandleScope handleScope(isolate);
    v8::TryCatch tryCatch(isolate);

    // Create source code string
    v8::Local<v8::String> sourceCode = v8::String::NewFromUtf8(isolate, code.c_str()).ToLocalChecked();

    // Create script origin for better error messages and debugging
    v8::ScriptOrigin origin(
        v8::String::NewFromUtf8(isolate, name.c_str()).ToLocalChecked(),  // resource_name
        0,                                                                  // line_offset
        0,                                                                  // column_offset
        false,                                                              // is_shared_cross_origin
        -1,                                                                 // script_id
        v8::Local<v8::Value>(),                                            // source_map_url
        false,                                                              // is_opaque
        false,                                                              // is_wasm
        true                                                                // is_module
    );

    // Compile module
    v8::ScriptCompiler::Source source(sourceCode, origin);
    v8::MaybeLocal<v8::Module> maybeModule = v8::ScriptCompiler::CompileModule(isolate, &source);

    // Check for compilation errors
    if (maybeModule.IsEmpty())
    {
        std::string error = GetV8ExceptionMessage(isolate, context, tryCatch);
        SetError(StringFormat("Module compilation failed for '{}': {}", name, error));
        return v8::MaybeLocal<v8::Module>();
    }

    v8::Local<v8::Module> module = maybeModule.ToLocalChecked();

    // Register module in registry if available
    if (m_registry)
    {
        m_registry->RegisterModule(name, module, code);
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ModuleLoader: Successfully compiled module: {}", name));

    return handleScope.Escape(module);
}

//----------------------------------------------------------------------------------------------------
bool ModuleLoader::InstantiateModule(v8::Isolate*           isolate,
                                     v8::Local<v8::Context> context,
                                     v8::Local<v8::Module>  module)
{
    v8::TryCatch tryCatch(isolate);

    // Instantiate module (resolve imports)
    // V8 13.0 requires two callbacks: module resolver and source resolver
    v8::Maybe<bool> result = module->InstantiateModule(context, ResolveModuleCallback, nullptr);

    if (result.IsNothing() || !result.FromJust())
    {
        std::string error = GetV8ExceptionMessage(isolate, context, tryCatch);
        SetError(StringFormat("Module instantiation failed: {}", error));
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
v8::MaybeLocal<v8::Value> ModuleLoader::EvaluateModule(v8::Isolate*           isolate,
                                                       v8::Local<v8::Context> context,
                                                       v8::Local<v8::Module>  module)
{
    v8::EscapableHandleScope handleScope(isolate);
    v8::TryCatch tryCatch(isolate);

    // Evaluate module (execute code)
    v8::MaybeLocal<v8::Value> result = module->Evaluate(context);

    if (result.IsEmpty())
    {
        std::string error = GetV8ExceptionMessage(isolate, context, tryCatch);
        SetError(StringFormat("Module evaluation failed: {}", error));
        return v8::MaybeLocal<v8::Value>();
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ModuleLoader: Module evaluated successfully"));

    return handleScope.Escape(result.ToLocalChecked());
}

//----------------------------------------------------------------------------------------------------
v8::MaybeLocal<v8::Module> ModuleLoader::ResolveModuleCallback(v8::Local<v8::Context>    context,
                                                               v8::Local<v8::String>     specifier,
                                                               v8::Local<v8::FixedArray> import_attributes,
                                                               v8::Local<v8::Module>     referrer)
{
    UNUSED(import_attributes);  // Not used for now

    v8::Isolate* isolate = context->GetIsolate();
    v8::EscapableHandleScope handleScope(isolate);

    // PHASE 2: Get ModuleLoader instance from context embedder data
    void* loaderPtr = context->GetAlignedPointerFromEmbedderData(1);
    if (!loaderPtr)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            "ResolveModuleCallback: ModuleLoader not found in context embedder data");
        return v8::MaybeLocal<v8::Module>();
    }

    ModuleLoader* loader = static_cast<ModuleLoader*>(loaderPtr);

    // Convert specifier to std::string
    v8::String::Utf8Value specifierUtf8(isolate, specifier);
    std::string specifierStr(*specifierUtf8);

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ResolveModuleCallback: Resolving import '{}'", specifierStr));

    // Get the referrer module's URL for relative path resolution
    // For now, use a simplified approach: treat all paths as relative to base path
    // In future, we can use referrer->GetIdentityHash() to track referrer module
    UNUSED(referrer);

    // Resolve the specifier to an absolute path
    std::string resolvedPath = loader->m_resolver->Resolve(specifierStr, loader->m_basePath);

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ResolveModuleCallback: Resolved '{}' to '{}'", specifierStr, resolvedPath));

    // Check if module is already in registry
    if (loader->m_registry->HasModule(resolvedPath))
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log,
            StringFormat("ResolveModuleCallback: Module '{}' found in cache", resolvedPath));

        v8::Local<v8::Module> cachedModule = loader->m_registry->GetModule(resolvedPath);
        return handleScope.Escape(cachedModule);
    }

    // Module not cached - need to load and compile it
    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ResolveModuleCallback: Loading module '{}' from disk", resolvedPath));

    // Read the module file
    std::string code;
    if (!loader->ReadModuleFile(resolvedPath, code))
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            StringFormat("ResolveModuleCallback: Failed to read module file: {}", resolvedPath));
        return v8::MaybeLocal<v8::Module>();
    }

    // Compile the imported module
    v8::MaybeLocal<v8::Module> maybeModule = loader->CompileModule(isolate, context, code, resolvedPath);

    if (maybeModule.IsEmpty())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            StringFormat("ResolveModuleCallback: Failed to compile module: {}", resolvedPath));
        return v8::MaybeLocal<v8::Module>();
    }

    v8::Local<v8::Module> module = maybeModule.ToLocalChecked();

    // Add dependency relationship
    // Get referrer URL - for now, we'll skip this as it requires more complex tracking
    // loader->m_registry->AddDependency(referrerUrl, resolvedPath);

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
        StringFormat("ResolveModuleCallback: Successfully resolved and compiled module: {}", resolvedPath));

    return handleScope.Escape(module);
}

//----------------------------------------------------------------------------------------------------
void ModuleLoader::InitializeImportMetaCallback(v8::Local<v8::Context> context,
                                                v8::Local<v8::Module>  module,
                                                v8::Local<v8::Object>  meta)
{
    v8::Isolate* isolate = context->GetIsolate();
    v8::HandleScope handleScope(isolate);

    // Set import.meta.url (use identity hash as placeholder URL)
    int identityHash = module->GetIdentityHash();
    std::string moduleUrl = StringFormat("module://{}",  identityHash);

    v8::Local<v8::String> urlKey = v8::String::NewFromUtf8Literal(isolate, "url");
    v8::Local<v8::String> urlValue = v8::String::NewFromUtf8(isolate, moduleUrl.c_str()).ToLocalChecked();
    meta->Set(context, urlKey, urlValue).Check();
}

//----------------------------------------------------------------------------------------------------
// Phase 3: Dynamic Import Support
//----------------------------------------------------------------------------------------------------
v8::MaybeLocal<v8::Promise> ModuleLoader::HostImportModuleDynamicallyCallback(
    v8::Local<v8::Context>    context,
    v8::Local<v8::Data>       host_defined_options,
    v8::Local<v8::Value>      resource_name,
    v8::Local<v8::String>     specifier,
    v8::Local<v8::FixedArray> import_attributes)
{
    UNUSED(host_defined_options);
    UNUSED(resource_name);
    UNUSED(import_attributes);

    v8::Isolate* isolate = context->GetIsolate();
    v8::EscapableHandleScope handleScope(isolate);

    DAEMON_LOG(LogScript, eLogVerbosity::Log, "HostImportModuleDynamicallyCallback: Dynamic import() called");

    // Get ModuleLoader instance from context embedder data
    void* loaderPtr = context->GetAlignedPointerFromEmbedderData(1);
    if (!loaderPtr)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            "HostImportModuleDynamicallyCallback: ModuleLoader not found in context embedder data");

        // Create rejected promise
        v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
        v8::Local<v8::String> errorMsg = v8::String::NewFromUtf8Literal(isolate,
            "Dynamic import failed: ModuleLoader not available");
        resolver->Reject(context, errorMsg).Check();
        return handleScope.Escape(resolver->GetPromise());
    }

    ModuleLoader* loader = static_cast<ModuleLoader*>(loaderPtr);

    // Convert specifier to std::string
    v8::String::Utf8Value specifierUtf8(isolate, specifier);
    std::string specifierStr(*specifierUtf8);

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("HostImportModuleDynamicallyCallback: Importing '{}'", specifierStr));

    // Create promise resolver
    v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
    v8::Local<v8::Promise> promise = resolver->GetPromise();

    // Resolve the module path
    std::string resolvedPath = loader->m_resolver->Resolve(specifierStr, loader->m_basePath);

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("HostImportModuleDynamicallyCallback: Resolved '{}' to '{}'", specifierStr, resolvedPath));

    // Check if module is already loaded
    v8::Local<v8::Module> module;
    if (loader->m_registry->HasModule(resolvedPath))
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log,
            StringFormat("HostImportModuleDynamicallyCallback: Module '{}' found in cache", resolvedPath));
        module = loader->m_registry->GetModule(resolvedPath);
    }
    else
    {
        // Load module from disk
        DAEMON_LOG(LogScript, eLogVerbosity::Log,
            StringFormat("HostImportModuleDynamicallyCallback: Loading module '{}' from disk", resolvedPath));

        std::string code;
        if (!loader->ReadModuleFile(resolvedPath, code))
        {
            v8::Local<v8::String> errorMsg = v8::String::NewFromUtf8(isolate,
                StringFormat("Dynamic import failed: Cannot read file '{}'", resolvedPath).c_str()).ToLocalChecked();
            resolver->Reject(context, errorMsg).Check();
            return handleScope.Escape(promise);
        }

        // Compile the module
        v8::MaybeLocal<v8::Module> maybeModule = loader->CompileModule(isolate, context, code, resolvedPath);
        if (maybeModule.IsEmpty())
        {
            v8::Local<v8::String> errorMsg = v8::String::NewFromUtf8(isolate,
                StringFormat("Dynamic import failed: Compilation error for '{}'", resolvedPath).c_str()).ToLocalChecked();
            resolver->Reject(context, errorMsg).Check();
            return handleScope.Escape(promise);
        }

        module = maybeModule.ToLocalChecked();

        // Instantiate the module
        if (!loader->InstantiateModule(isolate, context, module))
        {
            v8::Local<v8::String> errorMsg = v8::String::NewFromUtf8(isolate,
                StringFormat("Dynamic import failed: Instantiation error for '{}'", resolvedPath).c_str()).ToLocalChecked();
            resolver->Reject(context, errorMsg).Check();
            return handleScope.Escape(promise);
        }

        // Evaluate the module
        v8::MaybeLocal<v8::Value> maybeResult = loader->EvaluateModule(isolate, context, module);
        if (maybeResult.IsEmpty())
        {
            v8::Local<v8::String> errorMsg = v8::String::NewFromUtf8(isolate,
                StringFormat("Dynamic import failed: Evaluation error for '{}'", resolvedPath).c_str()).ToLocalChecked();
            resolver->Reject(context, errorMsg).Check();
            return handleScope.Escape(promise);
        }
    }

    // Get module namespace
    v8::Local<v8::Object> moduleNamespace = module->GetModuleNamespace().As<v8::Object>();

    // Resolve promise with module namespace
    resolver->Resolve(context, moduleNamespace).Check();

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
        StringFormat("HostImportModuleDynamicallyCallback: Successfully imported '{}'", resolvedPath));

    return handleScope.Escape(promise);
}

//----------------------------------------------------------------------------------------------------
bool ModuleLoader::ReadModuleFile(std::string const& filePath, std::string& outCode)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        SetError(StringFormat("Failed to open module file: {}", filePath));
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    outCode = buffer.str();

    return true;
}

//----------------------------------------------------------------------------------------------------
void ModuleLoader::SetError(std::string const& error)
{
    m_lastError = error;
    DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("ModuleLoader Error: {}", error));
}

//----------------------------------------------------------------------------------------------------
std::string ModuleLoader::GetV8ExceptionMessage(v8::Isolate*           isolate,
                                                v8::Local<v8::Context> context,
                                                v8::TryCatch&          tryCatch)
{
    v8::HandleScope handleScope(isolate);

    if (!tryCatch.HasCaught())
    {
        return "Unknown error";
    }

    v8::Local<v8::Message> message = tryCatch.Message();
    v8::Local<v8::Value> exception = tryCatch.Exception();

    std::string exceptionStr;
    if (!exception.IsEmpty())
    {
        v8::String::Utf8Value exceptionUtf8(isolate, exception);
        exceptionStr = *exceptionUtf8;
    }

    if (message.IsEmpty())
    {
        return exceptionStr;
    }

    // Get source line
    v8::String::Utf8Value filename(isolate, message->GetScriptResourceName());
    int linenum = message->GetLineNumber(context).FromMaybe(-1);
    int colnum = message->GetStartColumn(context).FromMaybe(-1);

    std::string result = StringFormat("{}:{}:{}: {}", *filename, linenum, colnum, exceptionStr);

    // Get source line content if available
    v8::MaybeLocal<v8::String> sourceLine = message->GetSourceLine(context);
    if (!sourceLine.IsEmpty())
    {
        v8::String::Utf8Value sourceLineUtf8(isolate, sourceLine.ToLocalChecked());
        result += StringFormat("\n{}", *sourceLineUtf8);
    }

    return result;
}
