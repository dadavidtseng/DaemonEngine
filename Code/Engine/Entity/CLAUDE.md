[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Entity**

# Entity Module Documentation

## Module Responsibilities

The Entity module provides high-level entity management API for asynchronous JavaScript integration. It handles entity creation, updates, and destruction through a thread-safe command queue system with double-buffered state synchronization. This module was extracted from ProtogameJS3D during M4-T8 refactoring to provide reusable async entity management for any project.

## Entry and Startup

### Primary Entry Points
- `EntityAPI.hpp` - High-level entity management API
- `EntityScriptInterface.hpp` - JavaScript bindings for entity control
- `EntityID.hpp` - Entity identifier type definition
- `EntityState.hpp` - Entity state data structure
- `EntityStateBuffer.hpp` - Thread-safe entity state double-buffering

### Initialization Pattern
```cpp
#include "Engine/Entity/EntityAPI.hpp"
#include "Engine/Entity/EntityScriptInterface.hpp"

// Create EntityAPI (requires RenderCommandQueue and ScriptSubsystem)
RenderCommandQueue* commandQueue = new RenderCommandQueue();
ScriptSubsystem* scriptSubsystem = new ScriptSubsystem(config);

EntityAPI* entityAPI = new EntityAPI(commandQueue, scriptSubsystem);

// Create JavaScript interface and register with script system
EntityScriptInterface* entityInterface = new EntityScriptInterface(entityAPI);
scriptSubsystem->RegisterScriptableObject("entity", entityInterface);

// Execute JavaScript (entity API now available)
scriptSubsystem->ExecuteScript("JSEngine.initialize()");
```

## External Interfaces

### EntityAPI - High-Level Entity Management
```cpp
class EntityAPI {
public:
    // Entity Creation/Destruction (async with callbacks)
    CallbackID CreateMesh(std::string const& meshType, Vec3 const& position,
                         float scale, Rgba8 const& color,
                         ScriptCallback const& callback);
    void DestroyEntity(EntityID entityId);
    
    // Entity Updates (fire-and-forget)
    void UpdatePosition(EntityID entityId, Vec3 const& position);
    void MoveBy(EntityID entityId, Vec3 const& delta);
    void UpdateOrientation(EntityID entityId, EulerAngles const& orientation);
    void UpdateColor(EntityID entityId, Rgba8 const& color);
    
    // Callback Execution (main thread)
    void ExecutePendingCallbacks();
    void NotifyCallbackReady(CallbackID callbackId, EntityID resultId);
    
    // ID Generation
    EntityID GenerateEntityID();
    CallbackID GenerateCallbackID();
};
```

### EntityScriptInterface - JavaScript Bindings
```cpp
class EntityScriptInterface : public IScriptableObject {
    // Exposed JavaScript methods:
    // - createMesh(type, properties, callback)
    // - updatePosition(entityId, position)
    // - moveBy(entityId, delta)
    // - updateOrientation(entityId, orientation)
    // - updateColor(entityId, color)
    // - destroy(entityId)
};

// Usage from JavaScript:
// entity.createMesh('cube', {
//     position: {x: 5, y: 0, z: 0},
//     scale: 1.0,
//     color: {r: 255, g: 0, b: 0, a: 255}
// }, (entityId) => {
//     console.log('Entity created:', entityId);
//     entity.updatePosition(entityId, {x: 10, y: 0, z: 0});
// });
```

### EntityState - State Data Structure
```cpp
struct EntityState {
    Vec3        position;          // World-space position
    EulerAngles orientation;       // World-space rotation (degrees)
    Rgba8       color;             // RGBA color (4 bytes)
    float       radius;            // Uniform scale
    std::string meshType;          // "cube", "sphere", "grid", "plane"
    bool        isActive;          // Active flag (render if true)
    int         vertexBufferHandle; // Persistent VBO handle
    std::string cameraType;        // "world" or "screen"
};

using EntityStateMap = std::unordered_map<EntityID, EntityState>;
```

### EntityStateBuffer - Thread-Safe Synchronization
```cpp
// Type alias using Core's StateBuffer template
using EntityStateBuffer = StateBuffer<EntityStateMap>;

// Usage pattern:
EntityStateBuffer* buffer = new EntityStateBuffer();

// Worker thread (JavaScript logic updates)
EntityStateMap* backBuffer = buffer->GetBackBuffer();
(*backBuffer)[entityId] = EntityState(pos, orient, color, scale, "cube");

// Main thread (Rendering reads)
EntityStateMap const* frontBuffer = buffer->GetFrontBuffer();
for (auto const& [id, state] : *frontBuffer) {
    if (state.isActive) {
        RenderEntity(state);  // Lock-free read
    }
}

// Frame boundary (Main thread)
buffer->SwapBuffers();  // Brief locked copy and swap
```

## Key Dependencies and Configuration

### Internal Dependencies
- **Core Module**: StateBuffer template, Rgba8, EngineCommon
- **Math Module**: Vec3, EulerAngles
- **Renderer Module**: RenderCommand, RenderCommandQueue
- **Script Module**: IScriptableObject, ScriptSubsystem, ScriptCallback

### External Dependencies
- None (uses only Engine modules)

### Configuration Structures
```cpp
// EntityID Type Definition
using EntityID = uint64_t;  // 64-bit unique entity identifier

// CallbackID Type Definition
using CallbackID = uint64_t;  // 64-bit unique callback identifier

// ScriptCallback Type Definition
using ScriptCallback = std::any;  // Type-erased JavaScript callback
```

### Coordinate System Conventions
- **X-forward**: +X points forward in world space
- **Y-left**: +Y points left in world space
- **Z-up**: +Z points up in world space
- **Right-handed coordinate system**
- All positions/deltas use this convention

## Data Models

### EntityID Type
Simple 64-bit unsigned integer for uniquely identifying entities. JavaScript-safe up to 2^53 (9 quadrillion entities).

### EntityState Structure
Plain-Old-Data (POD) struct containing all data needed to render an entity:
- Position, orientation, color, scale
- Mesh type (string-based for Phase 1 flexibility)
- Active flag, vertex buffer handle, camera type

### Callback Management
```cpp
struct PendingCallback {
    ScriptCallback callback;  // JavaScript callback function
    EntityID       resultId;  // Entity ID to pass to callback
    bool           ready;     // True when C++ has processed command
};

std::unordered_map<CallbackID, PendingCallback> m_pendingCallbacks;
```

## Testing and Quality

### Current Testing Approach
- Manual integration testing through JavaScript console
- Visual testing of entity creation and updates
- Performance profiling of async callback execution
- Thread Safety Analyzer (TSan) validation

### Quality Assurance Features
- Error-resilient callback execution (JavaScript errors don't crash C++)
- Invalid entityId logging and graceful handling
- Queue overflow detection and logging
- Thread-safe state buffer access validation

### Recommended Testing Additions
- Unit tests for EntityAPI methods
- Stress tests for high entity counts (1000+ entities)
- Callback execution timing tests
- Thread safety race condition tests

## FAQ

### Q: How do I create an entity from JavaScript?
A: Use `entity.createMesh(type, properties, callback)` where type is "cube"/"sphere"/"grid"/"plane", properties include position/scale/color, and callback receives the entityId when creation completes.

### Q: Why are entity creation callbacks async?
A: Entity creation involves render command submission and processing on the main thread. Callbacks notify JavaScript when the entity is actually created and assigned an ID.

### Q: Can I update entity state from multiple threads?
A: No. EntityAPI methods should only be called from the JavaScript worker thread. EntityStateBuffer ensures thread-safe synchronization between worker and main threads.

### Q: What's the difference between UpdatePosition and MoveBy?
A: UpdatePosition sets absolute world-space position. MoveBy applies relative delta movement (+X = forward, +Y = left, +Z = up).

### Q: How are entity IDs generated?
A: EntityAPI maintains an auto-incremented counter starting at 1. IDs are unique within a session but not persistent across restarts.

### Q: Can I create entities from C++ instead of JavaScript?
A: Yes, EntityAPI can be called from C++ code. However, callbacks must be thread-safe if executed on the worker thread.

### Q: How does EntityStateBuffer prevent data races?
A: It uses double-buffering: worker thread writes to back buffer (lock-free), main thread reads from front buffer (lock-free), and SwapBuffers() briefly locks to copy and swap pointers.

### Q: What happens if I destroy an entity that doesn't exist?
A: EntityAPI logs a warning and ignores the command. Invalid entityIds are handled gracefully without crashes.

## Related Files

### Core Entity API
- `EntityAPI.cpp` - High-level entity management implementation
- `EntityAPI.hpp` - Entity API interface declarations
- `EntityID.hpp` - Entity identifier type definition
- `EntityState.hpp` - Entity state data structure
- `EntityStateBuffer.hpp` - Thread-safe state buffer using StateBuffer template

### JavaScript Integration
- `EntityScriptInterface.cpp` - JavaScript bindings implementation
- `EntityScriptInterface.hpp` - Script interface declarations

## Changelog

- 2025-10-27: **M4-T8 Engine Refactoring - Initial Entity Module Creation**
  - Extracted EntityAPI from ProtogameJS3D::HighLevelEntityAPI
  - Separated entity management from camera management (Single Responsibility Principle)
  - Introduced async callback system for entity creation
  - Implemented EntityStateBuffer using Core's StateBuffer template
  - Created EntityScriptInterface for JavaScript bindings
  - Applied SOLID principles: SRP, Dependency Inversion
  - Enabled reusability across multiple projects
  - Thread-safe double-buffered state synchronization
  - Lock-free command queue integration
  - Comprehensive error resilience for JavaScript integration

## Design Philosophy

### Single Responsibility Principle
EntityAPI handles only entity operations. Camera operations moved to separate CameraAPI. Each API has a clear, focused responsibility.

### Async Callback Pattern
Entity creation is async to handle:
- Command queue submission delays
- Render command processing time
- ID generation on main thread
- Error recovery without blocking JavaScript

Callbacks notify JavaScript when operations complete, enabling responsive UI and game logic.

### Double-Buffering Strategy
EntityStateBuffer uses Core's StateBuffer template:
- Worker thread writes entity state to back buffer (lock-free)
- Main thread reads from front buffer for rendering (lock-free)
- SwapBuffers() at frame boundaries (brief lock, < 1ms)
- Full copy strategy for Phase 1 simplicity

### Error Resilience
- JavaScript callback errors caught with V8 TryCatch blocks
- C++ rendering continues with last valid state
- Invalid entityIds logged as warnings, commands ignored
- Queue overflow logged, creation requests dropped

### Thread Safety Model
- EntityAPI methods called from JavaScript worker thread
- NotifyCallbackReady() called from main thread (command processor)
- ExecutePendingCallbacks() called from worker thread (requires mutex)
- V8 locking required for callback execution (v8::Locker)

### Reusability Across Projects
Entity module is fully decoupled from ProtogameJS3D:
- No game-specific code or dependencies
- Generic entity state structure
- Flexible mesh type system (string-based)
- Extensible for future features (lights, particles, etc.)

## Integration Guidelines

### Basic Integration
1. Create RenderCommandQueue and ScriptSubsystem
2. Instantiate EntityAPI with command queue and script subsystem
3. Create EntityScriptInterface and register with script system
4. Execute JavaScript that uses `entity` global object
5. Call EntityAPI::ExecutePendingCallbacks() each frame

### Advanced Integration
- Implement custom mesh types by extending EntityState
- Add custom render commands for entity-specific rendering
- Integrate with physics system for entity collisions
- Extend EntityScriptInterface for game-specific methods

### Performance Considerations
- Limit entity count to avoid swap overhead (< 1000 for Phase 1)
- Use batch updates for multiple entities
- Profile SwapBuffers() timing to ensure < 1ms
- Consider dirty tracking for Phase 2 optimization

### Thread Safety Checklist
- EntityAPI methods called only from worker thread
- V8 locking (v8::Locker) acquired before JavaScript execution
- NotifyCallbackReady() called only from main thread
- ExecutePendingCallbacks() synchronized with mutex
- SwapBuffers() called only from main thread
- No concurrent reads/writes to same buffer

## Future Extensions (Phase 2+)

- **Batch Entity Creation**: createMeshBatch() for multiple entities
- **Entity Queries**: getEntityPosition(), getEntityCount()
- **Light Management**: createLight(), updateLight(), destroyLight()
- **Particle Systems**: createParticleEmitter() with entity-like API
- **Dirty Bit Tracking**: Optimize SwapBuffers() to copy only changed entities
- **Non-Uniform Scaling**: Vec3 scale instead of single float radius
- **Mesh Type Enum**: Replace string with enum for performance
- **Persistent IDs**: Save/load entity IDs across sessions
