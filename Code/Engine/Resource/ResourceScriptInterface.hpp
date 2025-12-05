//----------------------------------------------------------------------------------------------------
// ResourceScriptInterface.hpp
// Phase 3: JavaScript Interface for Resource Loading API
//----------------------------------------------------------------------------------------------------

#pragma once
#include <optional>
#include <unordered_map>
#include <atomic>
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Script/IScriptableObject.hpp"
#include "Game/EngineBuildPreferences.hpp"

//----------------------------------------------------------------------------------------------------
// Type Aliases (shared with script system)
//----------------------------------------------------------------------------------------------------
using ScriptCallback = std::any;

//----------------------------------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------------------------------
class ResourceSubsystem;

#ifdef ENGINE_SCRIPTING_ENABLED
class ResourceCommandQueue;
class CallbackQueue;
#endif

//----------------------------------------------------------------------------------------------------
// ResourceScriptInterface
//
// Exposes async resource loading methods to JavaScript via V8 runtime.
// Methods submit commands to ResourceCommandQueue for JobSystem execution.
// Results returned via CallbackQueue with unique callbackId.
//
// JavaScript API:
//   - loadTexture(path, callback) → callbackId
//   - loadModel(path, callback) → callbackId
//   - loadShader(path, callback) → callbackId
//
// Example JavaScript usage:
//   const callbackId = resource.loadTexture("Data/Images/test.png", (resourceId, error) => {
//       if (error) {
//           console.error("Failed to load texture:", error);
//       } else {
//           console.log("Texture loaded, resourceId:", resourceId);
//       }
//   });
//----------------------------------------------------------------------------------------------------
class ResourceScriptInterface : public IScriptableObject
{
public:
#ifdef ENGINE_SCRIPTING_ENABLED
	explicit ResourceScriptInterface(ResourceCommandQueue* commandQueue, CallbackQueue* callbackQueue);
#else
	explicit ResourceScriptInterface();
#endif
	~ResourceScriptInterface() override = default;

	// IScriptableObject interface
	void                           InitializeMethodRegistry() override;
	ScriptMethodResult             CallMethod(String const& methodName, ScriptArgs const& args) override;
	std::vector<ScriptMethodInfo>  GetAvailableMethods() const override;
	std::vector<String>            GetAvailableProperties() const override;
	std::any                       GetProperty(String const& propertyName) const override;
	bool                           SetProperty(String const& propertyName, std::any const& value) override;

private:
	//------------------------------------------------------------------------------------------------
	// JavaScript Method Implementations
	//------------------------------------------------------------------------------------------------

	// Load texture asynchronously
	// Arguments: [0] = path (string), [1] = callback (function)
	// Returns: callbackId (number) or error
	ScriptMethodResult ExecuteLoadTexture(ScriptArgs const& args);

	// Load 3D model asynchronously
	// Arguments: [0] = path (string), [1] = callback (function)
	// Returns: callbackId (number) or error
	ScriptMethodResult ExecuteLoadModel(ScriptArgs const& args);

	// Load shader asynchronously
	// Arguments: [0] = path (string), [1] = callback (function)
	// Returns: callbackId (number) or error
	ScriptMethodResult ExecuteLoadShader(ScriptArgs const& args);

	//------------------------------------------------------------------------------------------------
	// Helper Methods
	//------------------------------------------------------------------------------------------------

	// Extract callback from ScriptArgs
	std::optional<ScriptCallback> ExtractCallback(std::any const& value) const;

	// Validate file path (basic validation)
	bool IsValidPath(String const& path) const;

	// Generate unique callback ID (thread-safe)
	uint64_t GenerateCallbackID();

	//------------------------------------------------------------------------------------------------
	// Member Variables
	//------------------------------------------------------------------------------------------------

#ifdef ENGINE_SCRIPTING_ENABLED
	ResourceCommandQueue* m_commandQueue  = nullptr;  // Command queue to JobSystem
	CallbackQueue*        m_callbackQueue = nullptr;  // Callback queue from JobSystem
#endif

	// Callback ID generator (atomic for thread safety)
	std::atomic<uint64_t> m_nextCallbackId{1};
};
