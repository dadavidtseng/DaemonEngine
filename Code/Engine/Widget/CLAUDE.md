[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Widget**

# Widget Module Documentation

## Module Responsibilities

The Widget module provides a DirectX 11-based UI rendering system for game user interfaces, HUD elements, and interactive widgets. It manages widget lifecycle, z-order rendering, owner-based widget groups, and viewport management for both 2D screen-space and 3D world-space UI elements.

⚠️ **Current Status**: The WidgetSubsystem is functional but still in early development ("very rough" per user feedback). It requires refinement and enhancement for production-quality UI systems like Minecraft-style inventory screens and crafting interfaces.

## Entry and Startup

### Primary Entry Point
- `WidgetSubsystem.hpp` - Main widget management system interface
- `IWidget.hpp` - Base widget interface for all UI elements

### Initialization Pattern
```cpp
// Widget subsystem setup
sWidgetSubsystemConfig config;
config.m_initialWidgetCapacity = 64;
config.m_initialOwnerCapacity = 32;

WidgetSubsystem* widgetSystem = new WidgetSubsystem(config);
widgetSystem->StartUp();

// Create and add widgets
auto myWidget = widgetSystem->CreateWidget<MyCustomWidget>(args...);
widgetSystem->AddWidget(myWidget, zOrder = 100); // Higher z-order renders on top

// Owner-based widgets (e.g., player UI)
uint64_t playerEntityID = player->GetEntityID();
auto playerHUD = widgetSystem->CreateWidget<HUDWidget>(player);
widgetSystem->AddWidgetToOwner(playerHUD, playerEntityID, zOrder = 100);

// Frame lifecycle
widgetSystem->BeginFrame();
widgetSystem->Update();
widgetSystem->Render();
widgetSystem->EndFrame();
```

## External Interfaces

### Core Widget Management API
```cpp
class WidgetSubsystem {
    // Lifecycle
    void StartUp();
    void BeginFrame();
    void Update();
    void Render() const;
    void EndFrame();
    void ShutDown();

    // Widget Management
    void AddWidget(WidgetPtr const& widget, int zOrder = 0);
    void AddWidgetToOwner(WidgetPtr const& widget, uint64_t ownerID, int zOrder = 0);
    void RemoveWidget(WidgetPtr const& widget);
    void RemoveAllWidgetsFromOwner(uint64_t ownerID);
    void RemoveAllWidgets();

    // Widget Queries
    WidgetPtr              FindWidgetByName(String const& name) const;
    std::vector<WidgetPtr> GetWidgetsByOwner(uint64_t ownerID) const;
    std::vector<WidgetPtr> GetAllWidgets() const;

    // Viewport Management
    void      SetViewportWidget(WidgetPtr const& widget);
    WidgetPtr GetViewportWidget() const;

    // Template widget creation
    template <typename T, typename... Args>
    std::shared_ptr<T> CreateWidget(Args&&... args);
};
```

### IWidget Base Interface
```cpp
class IWidget {
    // Lifecycle (override in derived classes)
    virtual void Initialize() = 0;
    virtual void Update(float deltaSeconds) = 0;
    virtual void Render() const = 0;
    virtual void Shutdown() = 0;

    // Widget properties
    virtual String GetName() const = 0;
    virtual bool IsVisible() const = 0;
    virtual void SetVisible(bool visible) = 0;

    // Z-order sorting
    int GetZOrder() const;
    void SetZOrder(int zOrder);

    // Owner tracking
    uint64_t GetOwnerID() const;
    void SetOwnerID(uint64_t ownerID);
};
```

## Key Dependencies and Configuration

### External Dependencies
- **DirectX 11**: All widget rendering uses DX11 immediate mode or retained buffers
- **Renderer Module**: Vertex/index buffers, textures, shaders for UI rendering
- **Input System**: Mouse and keyboard events for widget interaction

### Internal Dependencies
- Core module for string utilities, timing, and basic data structures
- Math module for 2D/3D transformations and screen-space calculations
- Resource system for texture loading (UI sprite sheets)

### Configuration Structure
```cpp
struct sWidgetSubsystemConfig {
    size_t m_initialWidgetCapacity = 64;  // Reserve capacity for widget storage
    size_t m_initialOwnerCapacity  = 32;  // Reserve capacity for owner groupings
};
```

### Widget Storage and Organization
```cpp
class WidgetSubsystem {
private:
    sWidgetSubsystemConfig                               m_config;
    std::vector<WidgetPtr>                               m_widgets;           // All active widgets
    std::unordered_map<uint64_t, std::vector<WidgetPtr>> m_ownerWidgetsMapping; // Owner-based groups
    WidgetPtr                                            m_viewportWidget;    // Active viewport widget
    bool                                                 m_bNeedsSorting;     // Z-order dirty flag
};
```

## Data Models

### Widget Pointer Type
```cpp
using WidgetPtr = std::shared_ptr<IWidget>;
```

### Z-Order Rendering System
- **Lower z-order values render first** (background layers)
- **Higher z-order values render last** (foreground layers)
- Default z-order = 0
- Typical ranges:
  - -100 to -1: Background UI elements
  - 0 to 50: Standard UI elements (HUD, health bars)
  - 51 to 100: Modal dialogs, inventory screens
  - 101+: Tooltips, context menus, debug overlays

### Owner-Based Widget Grouping
- **Owner ID = 0**: Global widgets with no specific owner
- **Owner ID > 0**: Typically entity IDs (player, agents)
- Use cases:
  - Player HUD widgets owned by player entity
  - Agent UI widgets owned by AI agent entities
  - Quick cleanup: `RemoveAllWidgetsFromOwner(entityID)` when entity dies

## Common Widget Implementations

### Typical Custom Widget Structure
```cpp
class MyCustomWidget : public IWidget {
public:
    MyCustomWidget(/* constructor args */);
    ~MyCustomWidget() override;

    // IWidget interface
    void Initialize() override;
    void Update(float deltaSeconds) override;
    void Render() const override;
    void Shutdown() override;

    String GetName() const override { return "MyCustomWidget"; }
    bool IsVisible() const override { return m_isVisible; }
    void SetVisible(bool visible) override { m_isVisible = visible; }

private:
    bool m_isVisible = true;
    // Custom widget data...
};
```

### Example Widget Types
- **HUDWidget**: Hotbar, crosshair, health bars
- **InventoryWidget**: Full inventory screen with mouse interaction
- **CraftingWidget**: Crafting grid with recipe matching
- **DebugWidget**: F3 debug overlay with system stats
- **MenuWidget**: Main menu, pause menu, settings
- **TooltipWidget**: Hover tooltips for items and UI elements

## Testing and Quality

### Current Status
⚠️ **Alpha Quality**: The WidgetSubsystem is functional for basic use cases but requires significant refinement:
- Z-order sorting works
- Owner-based grouping works
- Basic lifecycle management works
- **Needs Work**: Modal dialog support, input capture, event routing, layout systems

### Recommended Enhancements for Production Use
1. **Input Handling**: Widget-level mouse/keyboard event routing
2. **Layout System**: Automatic positioning, anchoring, flex layouts
3. **Event System**: Widget event bubbling and propagation
4. **Animation System**: Smooth transitions, tweening, easing
5. **Focus Management**: Tab order, keyboard navigation
6. **Rendering Optimization**: Batch rendering, sprite atlases, texture packing
7. **Theming System**: Consistent visual styles across widgets
8. **Accessibility**: Screen reader support, high-contrast modes

### Quality Assurance Features
- Automatic z-order sorting when widgets added/removed
- Safe widget removal during iteration (garbage collection)
- Owner-based cleanup prevents memory leaks
- Widget name-based lookup for debugging

### Recommended Testing Additions
- Unit tests for z-order sorting edge cases
- Performance benchmarks with 100+ widgets
- Memory leak detection with rapid widget add/remove cycles
- Input event routing validation

## Usage Examples for Assignment 7

### Creating Minecraft-Style HUD (FR-4.1)
```cpp
// Create hotbar widget (z-order 100, player-owned)
auto hotbarWidget = widgetSystem->CreateWidget<HotbarWidget>(player->GetInventory());
widgetSystem->AddWidgetToOwner(hotbarWidget, player->GetEntityID(), 100);

// Create crosshair widget (z-order 101, global)
auto crosshairWidget = widgetSystem->CreateWidget<CrosshairWidget>();
widgetSystem->AddWidget(crosshairWidget, 101);

// Create F3 debug overlay (z-order 200, global, hidden by default)
auto debugWidget = widgetSystem->CreateWidget<DebugOverlayWidget>();
debugWidget->SetVisible(false);
widgetSystem->AddWidget(debugWidget, 200);
```

### Creating Inventory Screen (FR-4.2)
```cpp
// Create modal inventory widget (z-order 150, pauses game)
auto inventoryWidget = widgetSystem->CreateWidget<InventoryWidget>(player->GetInventory());
inventoryWidget->SetVisible(false); // Hidden initially
widgetSystem->AddWidgetToOwner(inventoryWidget, player->GetEntityID(), 150);

// Toggle with 'E' key
if (input->WasKeyJustPressed('E')) {
    bool isVisible = inventoryWidget->IsVisible();
    inventoryWidget->SetVisible(!isVisible);
    game->SetPaused(!isVisible); // Pause gameplay when inventory open
}
```

### Creating Agent UI (FR-5.1)
```cpp
// Create name tag widget for each agent (z-order 90)
for (AIAgent* agent : world->GetAgents()) {
    auto nameTagWidget = widgetSystem->CreateWidget<NameTagWidget>(agent);
    widgetSystem->AddWidgetToOwner(nameTagWidget, agent->GetEntityID(), 90);
}

// When agent is destroyed, cleanup all its widgets
widgetSystem->RemoveAllWidgetsFromOwner(agent->GetEntityID());
```

## FAQ

### Q: Why is the WidgetSubsystem considered "very rough"?
A: The core infrastructure (lifecycle, z-order, owner grouping) is functional, but many production features are missing: modal dialogs, input event routing, layout systems, and rendering optimizations. It works for basic HUDs but needs refinement for complex UIs like Minecraft inventory screens.

### Q: How do I make a widget capture mouse input (modal dialog)?
A: Currently not fully supported. You'll need to implement custom input capture logic in your widget's Update() method and check mouse position against widget bounds. This is a planned enhancement.

### Q: Can I use the WidgetSubsystem for 3D world-space UI (name tags above agents)?
A: Yes, but you'll need to implement custom projection from world-space to screen-space in your widget's Render() method. The system primarily focuses on 2D screen-space UI currently.

### Q: How do I render Minecraft-style pixel-perfect UI?
A: Use orthographic projection with 1:1 pixel mapping, load Minecraft UI sprite sheets as textures, and render with nearest-neighbor filtering to preserve pixel art crispness.

### Q: What's the performance impact of many widgets?
A: Z-order sorting is O(n log n) when widgets are added/removed, O(1) during stable rendering. For 100+ widgets, consider spatial partitioning or dirty-flag optimizations.

### Q: Can I create custom widget base classes?
A: Yes! Create intermediate base classes between IWidget and your concrete widgets to share common functionality (e.g., `ModalWidget`, `InteractiveWidget`, `AnimatedWidget`).

## Related Files

### Core Implementation
- `WidgetSubsystem.cpp` - Main widget system implementation
- `IWidget.hpp` - Widget base interface (header only)

### Integration Points
- **Renderer Module**: DirectX 11 rendering for widget graphics
- **Input System**: Mouse/keyboard events for widget interaction
- **Resource System**: Texture loading for UI sprite sheets
- **Game Logic**: Widget creation and management from gameplay code

### Planned Extensions for A7 (Minecraft UI)
- **Modal Dialog System**: Input capture, dimmed background, focus management
- **Layout System**: Anchoring, grid layouts, flex containers
- **Event Routing**: Mouse hover, click, drag-and-drop for inventory
- **Rendering Optimization**: Batch rendering for Minecraft-style UI sprites
- **Texture Atlas System**: Efficient sprite sheet management
- **Tooltip System**: Hover tooltips for items and UI elements

### Reference Implementation
For A7 implementation patterns, reference:
- **SimpleMiner ProtogameJS3D**: Existing UI implementation examples
- **Minecraft UI Screenshots**: For pixel-perfect visual fidelity
- **Minecraft Wiki - User Interface**: For layout specifications and behavior

## Changelog

- 2025-11-24: **Initial Widget module documentation created** for Assignment 7
  - Documented WidgetSubsystem API (lifecycle, widget management, owner grouping)
  - Noted "very rough" alpha status requiring refinement for production use
  - Added A7-specific usage examples (HUD, inventory, agent UI)
  - Identified missing features (modal dialogs, input routing, layouts)
  - Provided Minecraft UI integration guidance
