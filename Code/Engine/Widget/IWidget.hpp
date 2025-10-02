//----------------------------------------------------------------------------------------------------
// IWidget.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <cstdint>

//----------------------------------------------------------------------------------------------------
// IWidget - Abstract base class for all UI widget components
//
// Provides a common interface for widget lifecycle management, rendering, and state control.
// Widgets can be owned by game entities (via uint64_t owner ID) and support z-ordering for
// layered rendering.
//----------------------------------------------------------------------------------------------------
class IWidget
{
    friend class WidgetSubsystem;

public:
    IWidget();
    virtual ~IWidget();

    // Lifecycle methods - can be overridden by derived classes
    virtual void BeginFrame();
    virtual void Render();
    virtual void Draw() const;
    virtual void Update();
    virtual void EndFrame();

    // Getters
    virtual uint64_t GetOwner() const;
    virtual int      GetZOrder() const;
    virtual String   GetName() const;
    virtual bool     IsVisible() const;
    virtual bool     IsTick() const;
    virtual bool     IsGarbage() const;

    // Setters
    virtual void SetOwner(uint64_t ownerID);
    virtual void SetZOrder(int zOrder);
    virtual void SetName(String const& name);
    virtual void SetVisible(bool visible);
    virtual void SetTick(bool tick);

    // Lifecycle control
    virtual void MarkForDestroy();

protected:
    uint64_t m_ownerID    = 0;          // Owner entity ID (0 = no owner)
    int      m_zOrder     = 0;          // Render order (higher values render on top)
    bool     m_bIsTick    = true;       // Whether Update() should be called
    String   m_name       = "DEFAULT";  // Widget name for debugging/queries
    bool     m_bIsVisible = true;       // Whether widget should be rendered
    bool     m_bIsGarbage = false;      // Marked for garbage collection
};
