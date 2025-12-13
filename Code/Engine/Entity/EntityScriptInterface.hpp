//----------------------------------------------------------------------------------------------------
// EntityScriptInterface.hpp
// M4-T8: JavaScript Interface for Entity API (camera methods removed)
//
// Purpose:
//   Exposes EntityAPI to JavaScript runtime through IScriptableObject interface.
//   Provides user-friendly JavaScript APIs for entity management only.
//
// Design Philosophy:
//   - Single Responsibility: Entity operations only (camera moved to CameraScriptInterface)
//   - Clean separation from rendering and camera concerns
//   - Error-resilient (JavaScript errors don't crash C++ rendering)
//   - Async callbacks for creation operations
//
// JavaScript API (exposed methods):
//   Entity Management:
//     - entity.createMesh(type, properties, callback)
//     - entity.updatePosition(entityId, posX, posY, posZ)
//     - entity.moveBy(entityId, dx, dy, dz)
//     - entity.updateOrientation(entityId, yaw, pitch, roll)
//     - entity.updateColor(entityId, r, g, b, a)
//     - entity.destroy(entityId)
//
// Usage Example (from JavaScript):
//   // Create a cube entity
//   entity.createMesh('cube', 5, 0, 0, 1.0, 255, 0, 0, 255, (entityId) => {
//       console.log('Entity created:', entityId);
//       // Update entity position
//       entity.updatePosition(entityId, 10, 0, 0);
//   });
//
// Thread Safety:
//   - All methods submit commands to RenderCommandQueue (lock-free)
//   - Callbacks executed on JavaScript worker thread
//   - V8 locking handled internally by EntityAPI::ExecutePendingCallbacks()
//
// Author: Phase 2 - High-Level API Implementation
// M4-T8: Refactored to use EntityAPI, camera methods moved to CameraScriptInterface
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Entity/EntityID.hpp"

#include <memory>
#include <optional>
#include <any>

//----------------------------------------------------------------------------------------------------
// Type Aliases (shared with EntityAPI)
using ScriptCallback = std::any;

//----------------------------------------------------------------------------------------------------
// Forward Declarations
class EntityAPI;

//----------------------------------------------------------------------------------------------------
// EntityScriptInterface
//
// JavaScript interface for entity management only.
// Wraps EntityAPI and exposes methods to V8 JavaScript runtime.
//
// M4-T8 Changes:
//   - Changed from HighLevelEntityAPI to EntityAPI
//   - Removed all camera methods (moved to CameraScriptInterface)
//   - Maintains backward compatibility for entity operations
//
// Registration:
//   - Registered in ScriptSubsystem as "entity" global object
//   - Camera methods exposed via separate "camera" global (CameraScriptInterface)
//
// Method Naming Convention:
//   - JavaScript methods use camelCase (e.g., createMesh, moveBy)
//   - C++ methods map to EntityAPI (e.g., CreateMesh, MoveBy)
//
// Error Handling:
//   - Invalid parameters return ScriptMethodResult::Error()
//   - Errors logged to console, don't crash C++ rendering
//   - Callbacks with error status notify JavaScript of failures
//----------------------------------------------------------------------------------------------------
class EntityScriptInterface : public IScriptableObject
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit EntityScriptInterface(EntityAPI* entityAPI);
	~EntityScriptInterface() override = default;

	//------------------------------------------------------------------------------------------------
	// IScriptableObject Interface
	//------------------------------------------------------------------------------------------------
	void                          InitializeMethodRegistry() override;
	ScriptMethodResult            CallMethod(String const& methodName, ScriptArgs const& args) override;
	std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
	std::vector<String>           GetAvailableProperties() const override;
	std::any                      GetProperty(String const& propertyName) const override;
	bool                          SetProperty(String const& propertyName, std::any const& value) override;

private:
	//------------------------------------------------------------------------------------------------
	// Entity Management Methods (exposed to JavaScript)
	//------------------------------------------------------------------------------------------------

	// Create a mesh entity (async with callback)
	// JavaScript signature: createMesh(type, properties, callback)
	// Parameters:
	//   - type: string ("cube", "sphere", "grid", "plane")
	//   - properties: object {position: {x, y, z}, scale: number, color: {r, g, b, a}}
	//   - callback: function (entityId) => {...}
	ScriptMethodResult ExecuteCreateMesh(ScriptArgs const& args) const;

	// Update entity position (absolute, world-space)
	// JavaScript signature: updatePosition(entityId, position)
	// Parameters:
	//   - entityId: number
	//   - position: object {x, y, z}
	ScriptMethodResult ExecuteUpdatePosition(ScriptArgs const& args);

	// Move entity by delta (relative movement)
	// JavaScript signature: moveBy(entityId, delta)
	// Parameters:
	//   - entityId: number
	//   - delta: object {dx, dy, dz}
	ScriptMethodResult ExecuteMoveBy(ScriptArgs const& args);

	// Update entity orientation (Euler angles in degrees)
	// JavaScript signature: updateOrientation(entityId, orientation)
	// Parameters:
	//   - entityId: number
	//   - orientation: object {yaw, pitch, roll}
	ScriptMethodResult ExecuteUpdateOrientation(ScriptArgs const& args);

	// Update entity color
	// JavaScript signature: updateColor(entityId, color)
	// Parameters:
	//   - entityId: number
	//   - color: object {r, g, b, a}
	ScriptMethodResult ExecuteUpdateColor(ScriptArgs const& args);

	// Destroy entity
	// JavaScript signature: destroy(entityId)
	// Parameters:
	//   - entityId: number
	ScriptMethodResult ExecuteDestroyEntity(ScriptArgs const& args);

	//------------------------------------------------------------------------------------------------
	// Helper Methods
	//------------------------------------------------------------------------------------------------

	// Extract Vec3 from JavaScript object {x, y, z}
	// Returns std::nullopt if extraction fails
	std::optional<Vec3> ExtractVec3(std::any const& value) const;

	// Extract Rgba8 from JavaScript object {r, g, b, a}
	// Returns std::nullopt if extraction fails
	std::optional<Rgba8> ExtractRgba8(std::any const& value) const;

	// Extract EulerAngles from JavaScript object {yaw, pitch, roll}
	// Returns std::nullopt if extraction fails
	std::optional<EulerAngles> ExtractEulerAngles(std::any const& value) const;

	// Extract EntityID (uint64_t) from JavaScript number
	// Returns std::nullopt if extraction fails
	std::optional<EntityID> ExtractEntityID(std::any const& value) const;

	// Extract callback function from std::any
	// Returns std::nullopt if extraction fails
	std::optional<ScriptCallback> ExtractCallback(std::any const& value) const;

private:
	//------------------------------------------------------------------------------------------------
	// Internal State
	//------------------------------------------------------------------------------------------------
	EntityAPI* m_entityAPI;  // Pointer to entity API (owned by HighLevelEntityAPI)
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Method Naming Convention:
//   - JavaScript uses camelCase: createMesh, updatePosition, moveBy
//   - C++ uses PascalCase: ExecuteCreateMesh, ExecuteUpdatePosition, ExecuteMoveBy
//
// Parameter Extraction Strategy:
//   - All JavaScript objects passed as std::any (type-erased)
//   - Helper methods (ExtractVec3, ExtractRgba8, etc.) validate and extract types
//   - Extraction failures return ScriptMethodResult::Error() with descriptive message
//   - No crashes on invalid JavaScript input (defensive programming)
//
// Callback Handling:
//   - Callbacks stored in HighLevelEntityAPI::m_pendingCallbacks
//   - Executed by HighLevelEntityAPI::ExecutePendingCallbacks() on worker thread
//   - V8 locking handled by HighLevelEntityAPI (not this interface)
//
// Error Resilience:
//   - All parameter extraction wrapped in try-catch (std::bad_any_cast protection)
//   - Invalid parameters → ScriptMethodResult::Error() → JavaScript receives error
//   - C++ rendering continues regardless of JavaScript errors
//
// Future Extensions (Phase 2b):
//   - Add light management methods (createLight, updateLight, destroyLight)
//   - Add batch entity creation (createMeshBatch for multiple entities)
//   - Add entity query methods (getEntityPosition, getEntityCount)
//----------------------------------------------------------------------------------------------------
