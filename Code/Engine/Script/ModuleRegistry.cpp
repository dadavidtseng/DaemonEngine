//----------------------------------------------------------------------------------------------------
// ModuleRegistry.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/ModuleRegistry.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
ModuleRegistry::ModuleRegistry(v8::Isolate* isolate)
    : m_isolate(isolate)
{
    GUARANTEE_OR_DIE(isolate != nullptr, "ModuleRegistry: V8 isolate cannot be null")
}

//----------------------------------------------------------------------------------------------------
ModuleRegistry::~ModuleRegistry()
{
    Clear();
}

//----------------------------------------------------------------------------------------------------
void ModuleRegistry::RegisterModule(std::string const& url, v8::Local<v8::Module> module, std::string const& sourceCode)
{
    // Create persistent handle to prevent garbage collection
    auto persistentModule = std::make_unique<v8::Persistent<v8::Module>>();
    persistentModule->Reset(m_isolate, module);
    m_modules[url] = std::move(persistentModule);

    // Store module metadata
    ModuleInfo info;
    info.url            = url;
    info.sourceCode     = sourceCode;
    info.sourceHash     = std::hash<std::string>{}(sourceCode);
    info.isInstantiated = false;
    info.isEvaluated    = false;

    m_moduleInfo[url] = info;
}

//----------------------------------------------------------------------------------------------------
v8::Local<v8::Module> ModuleRegistry::GetModule(std::string const& url) const
{
    auto it = m_modules.find(url);
    if (it != m_modules.end())
    {
        return it->second->Get(m_isolate);
    }
    return v8::Local<v8::Module>();
}

//----------------------------------------------------------------------------------------------------
bool ModuleRegistry::HasModule(std::string const& url) const
{
    return m_modules.find(url) != m_modules.end();
}

//----------------------------------------------------------------------------------------------------
std::string ModuleRegistry::FindModulePath(v8::Local<v8::Module> module) const
{
    // Search through all registered modules to find matching instance
    for (auto const& pair : m_modules)
    {
        if (pair.second)
        {
            v8::Local<v8::Module> cachedModule = pair.second->Get(m_isolate);
            if (cachedModule == module)
            {
                return pair.first; // Return the URL/path
            }
        }
    }
    return ""; // Not found
}

//----------------------------------------------------------------------------------------------------
ModuleInfo const* ModuleRegistry::GetModuleInfo(std::string const& url) const
{
    auto it = m_moduleInfo.find(url);
    if (it != m_moduleInfo.end())
    {
        return &it->second;
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------------------
void ModuleRegistry::AddDependency(std::string const& importerUrl, std::string const& importeeUrl)
{
    // Forward graph: importer → importee
    m_dependencies[importerUrl].insert(importeeUrl);

    // Reverse graph: importee → importer
    m_dependents[importeeUrl].insert(importerUrl);
}

//----------------------------------------------------------------------------------------------------
std::vector<std::string> ModuleRegistry::GetDependents(std::string const& moduleUrl) const
{
    auto it = m_dependents.find(moduleUrl);
    if (it != m_dependents.end())
    {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
}

//----------------------------------------------------------------------------------------------------
std::vector<std::string> ModuleRegistry::GetDependencies(std::string const& moduleUrl) const
{
    auto it = m_dependencies.find(moduleUrl);
    if (it != m_dependencies.end())
    {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
}

//----------------------------------------------------------------------------------------------------
std::vector<std::string> ModuleRegistry::GetModulesInLoadOrder() const
{
    std::vector<std::string>        result;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursionStack;

    // Perform topological sort on all modules
    for (auto const& pair : m_modules)
    {
        if (visited.find(pair.first) == visited.end())
        {
            TopologicalSortHelper(pair.first, visited, recursionStack, result);
        }
    }

    return result;
}

//----------------------------------------------------------------------------------------------------
void ModuleRegistry::TopologicalSortHelper(std::string const&               moduleUrl,
                                           std::unordered_set<std::string>& visited,
                                           std::unordered_set<std::string>& recursionStack,
                                           std::vector<std::string>&        result) const
{
    visited.insert(moduleUrl);
    recursionStack.insert(moduleUrl);

    // Visit all dependencies first
    auto depIt = m_dependencies.find(moduleUrl);
    if (depIt != m_dependencies.end())
    {
        for (std::string const& dependency : depIt->second)
        {
            if (visited.find(dependency) == visited.end())
            {
                TopologicalSortHelper(dependency, visited, recursionStack, result);
            }
        }
    }

    recursionStack.erase(moduleUrl);
    result.push_back(moduleUrl);
}

//----------------------------------------------------------------------------------------------------
void ModuleRegistry::MarkInstantiated(std::string const& url)
{
    auto it = m_moduleInfo.find(url);
    if (it != m_moduleInfo.end())
    {
        it->second.isInstantiated = true;
    }
}

//----------------------------------------------------------------------------------------------------
void ModuleRegistry::MarkEvaluated(std::string const& url)
{
    auto it = m_moduleInfo.find(url);
    if (it != m_moduleInfo.end())
    {
        it->second.isEvaluated = true;
    }
}

//----------------------------------------------------------------------------------------------------
void ModuleRegistry::InvalidateModule(std::string const& url)
{
    // PHASE 5 FIX: Properly reset V8 persistent handles before removal
    auto it = m_modules.find(url);
    if (it != m_modules.end())
    {
        // Explicitly reset the persistent handle to release V8's reference
        if (it->second)
        {
            it->second->Reset();
        }
    }

    // Remove from module cache (preserves dependency graph)
    m_modules.erase(url);

    // Reset module info status
    auto infoIt = m_moduleInfo.find(url);
    if (infoIt != m_moduleInfo.end())
    {
        infoIt->second.isInstantiated = false;
        infoIt->second.isEvaluated    = false;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("ModuleRegistry: Invalidated module '{}'", url));
}

//----------------------------------------------------------------------------------------------------
std::vector<std::string> ModuleRegistry::InvalidateModuleTree(std::string const& url)
{
    std::vector<std::string>        invalidated;
    std::unordered_set<std::string> toInvalidate;

    // Collect all dependents recursively
    toInvalidate.insert(url);

    // BFS to collect all dependents
    std::vector<std::string> queue = {url};
    while (!queue.empty())
    {
        std::string current = queue.back();
        queue.pop_back();

        auto dependents = GetDependents(current);
        for (auto const& dependent : dependents)
        {
            if (toInvalidate.find(dependent) == toInvalidate.end())
            {
                toInvalidate.insert(dependent);
                queue.push_back(dependent);
            }
        }
    }

    // Invalidate all collected modules
    for (auto const& moduleUrl : toInvalidate)
    {
        InvalidateModule(moduleUrl);
        invalidated.push_back(moduleUrl);
    }

    return invalidated;
}

//----------------------------------------------------------------------------------------------------
void ModuleRegistry::Clear()
{
    m_modules.clear();
    m_moduleInfo.clear();
    m_dependencies.clear();
    m_dependents.clear();
}

//----------------------------------------------------------------------------------------------------
std::vector<std::string> ModuleRegistry::GetAllModuleUrls() const
{
    StringList urls;

    urls.reserve(m_modules.size());

    for (auto const& pair : m_modules)
    {
        urls.push_back(pair.first);
    }

    return urls;
}

//----------------------------------------------------------------------------------------------------
bool ModuleRegistry::HasCircularDependency(std::string const& moduleUrl) const
{
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursionStack;

    return HasCircularDependencyHelper(moduleUrl, visited, recursionStack);
}

//----------------------------------------------------------------------------------------------------
bool ModuleRegistry::HasCircularDependencyHelper(std::string const&               moduleUrl,
                                                 std::unordered_set<std::string>& visited,
                                                 std::unordered_set<std::string>& recursionStack) const
{
    visited.insert(moduleUrl);
    recursionStack.insert(moduleUrl);

    // Check all dependencies
    auto depIt = m_dependencies.find(moduleUrl);
    if (depIt != m_dependencies.end())
    {
        for (std::string const& dependency : depIt->second)
        {
            // If dependency is in recursion stack, we have a cycle
            if (recursionStack.contains(dependency))
            {
                return true;
            }

            // Recursively check dependency
            if (!visited.contains(dependency))
            {
                if (HasCircularDependencyHelper(dependency, visited, recursionStack))
                {
                    return true;
                }
            }
        }
    }

    recursionStack.erase(moduleUrl);
    return false;
}
