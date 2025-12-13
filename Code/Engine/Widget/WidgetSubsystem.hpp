//----------------------------------------------------------------------------------------------------
// WidgetSubsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Widget/IWidget.hpp"
//----------------------------------------------------------------------------------------------------
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
using WidgetPtr = std::shared_ptr<IWidget>;

//----------------------------------------------------------------------------------------------------
struct sWidgetSubsystemConfig
{
    size_t m_initialWidgetCapacity = 64;
    size_t m_initialOwnerCapacity  = 32;
};

//----------------------------------------------------------------------------------------------------
// WidgetSubsystem - Central UI management system
//
// Manages lifecycle of all widgets in the application, provides z-order sorting,
// owner-based widget grouping, and viewport management.
//
// Owner IDs are uint64_t values that typically correspond to entity IDs.
// Pass 0 as owner ID for global widgets with no specific owner.
//----------------------------------------------------------------------------------------------------
class WidgetSubsystem
{
public:
    explicit WidgetSubsystem(sWidgetSubsystemConfig const& config);
    ~WidgetSubsystem();

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
    bool                   HasModalWidget() const;  // Returns true if any modal widget is visible

    // Viewport Management
    void      SetViewportWidget(WidgetPtr const& widget);
    WidgetPtr GetViewportWidget() const;

    // Template function for creating widgets
    template <typename T, typename... Args>
    std::shared_ptr<T> CreateWidget(Args&&... args);

private:
    void SortWidgetsByZOrder();
    void CleanupGarbageWidgets();

    sWidgetSubsystemConfig                               m_config;
    std::vector<WidgetPtr>                               m_widgets;
    std::unordered_map<uint64_t, std::vector<WidgetPtr>> m_ownerWidgetsMapping;
    WidgetPtr                                            m_viewportWidget = nullptr;
    bool                                                 m_bNeedsSorting  = false;
};

//----------------------------------------------------------------------------------------------------
template <typename T, typename... Args>
std::shared_ptr<T> WidgetSubsystem::CreateWidget(Args&&... args)
{
    static_assert(std::is_base_of_v<IWidget, T>, "T must derive from IWidget");

    return std::make_shared<T>(std::forward<Args>(args)...);
}
