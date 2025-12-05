//----------------------------------------------------------------------------------------------------
// WidgetSubsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Widget/WidgetSubsystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Widget/IWidget.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
//----------------------------------------------------------------------------------------------------
#include <algorithm>

//----------------------------------------------------------------------------------------------------
WidgetSubsystem::WidgetSubsystem(sWidgetSubsystemConfig const& config)
    : m_config(config)
{
    m_widgets.reserve(m_config.m_initialWidgetCapacity);
    m_ownerWidgetsMapping.reserve(m_config.m_initialOwnerCapacity);
}

//----------------------------------------------------------------------------------------------------
WidgetSubsystem::~WidgetSubsystem()
{
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::StartUp()
{
    // Clear all containers
    m_widgets.clear();
    m_ownerWidgetsMapping.clear();
    m_viewportWidget = nullptr;
    m_bNeedsSorting  = false;
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::BeginFrame()
{
    // Call BeginFrame on all widgets
    for (auto& widget : m_widgets)
    {
        if (widget && !widget->IsGarbage())
        {
            widget->BeginFrame();
        }
    }
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::Update()
{
    // Cleanup garbage widgets
    CleanupGarbageWidgets();

    // Re-sort if necessary
    if (m_bNeedsSorting)
    {
        SortWidgetsByZOrder();
        m_bNeedsSorting = false;
    }

    // Update all widgets that need ticking
    for (auto& widget : m_widgets)
    {
        if (widget && widget->IsTick() && !widget->IsGarbage())
        {
            widget->Update();
        }
    }
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::Render() const
{
    // DEBUG: Log widget count
    DebuggerPrintf("[WIDGET_SUBSYSTEM] Render() called - Widget count: %d\n", static_cast<int>(m_widgets.size()));

    // Render all widgets in z-order
    for (auto& widget : m_widgets)
    {
        if (widget && widget->IsVisible() && !widget->IsGarbage())
        {
            DebuggerPrintf("[WIDGET_SUBSYSTEM] Rendering widget: %s (visible=%d, garbage=%d)\n",
                widget->GetName().c_str(), widget->IsVisible(), widget->IsGarbage());
            widget->Render();
        }
    }
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::EndFrame()
{
    // Call EndFrame on all widgets
    for (auto& widget : m_widgets)
    {
        if (widget && !widget->IsGarbage())
        {
            widget->EndFrame();
        }
    }
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::ShutDown()
{
    RemoveAllWidgets();
    m_viewportWidget = nullptr;
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::AddWidget(WidgetPtr const& widget,
                                int const        zOrder)
{
    if (!widget) return;

    widget->SetZOrder(zOrder);
    m_widgets.push_back(widget);
    m_bNeedsSorting = true;

    // DEBUG: Confirm widget addition
    DebuggerPrintf("[WIDGET_SUBSYSTEM] AddWidget() - Added widget '%s' with zOrder=%d, Total widgets=%d\n",
        widget->GetName().c_str(), zOrder, static_cast<int>(m_widgets.size()));
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::AddWidgetToOwner(WidgetPtr const& widget,
                                       uint64_t const   ownerID,
                                       int const        zOrder)
{
    if (!widget || ownerID == 0) return;

    widget->SetOwner(ownerID);
    widget->SetZOrder(zOrder);

    m_widgets.push_back(widget);
    m_ownerWidgetsMapping[ownerID].push_back(widget);
    m_bNeedsSorting = true;
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::RemoveWidget(WidgetPtr const& widget)
{
    if (!widget) return;

    // Remove from main list
    auto it = std::find(m_widgets.begin(), m_widgets.end(), widget);
    if (it != m_widgets.end())
    {
        m_widgets.erase(it);
    }

    // Remove from owner mapping
    uint64_t ownerID = widget->GetOwner();
    if (ownerID != 0 && m_ownerWidgetsMapping.find(ownerID) != m_ownerWidgetsMapping.end())
    {
        auto& ownerWidgets = m_ownerWidgetsMapping[ownerID];
        auto  ownerIt      = std::find(ownerWidgets.begin(), ownerWidgets.end(), widget);
        if (ownerIt != ownerWidgets.end())
        {
            ownerWidgets.erase(ownerIt);
        }

        // Remove the owner entry if no widgets remain
        if (ownerWidgets.empty())
        {
            m_ownerWidgetsMapping.erase(ownerID);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::RemoveAllWidgetsFromOwner(uint64_t ownerID)
{
    if (ownerID == 0) return;

    auto it = m_ownerWidgetsMapping.find(ownerID);
    if (it != m_ownerWidgetsMapping.end())
    {
        // Remove all widgets belonging to this owner from the main list
        for (auto& widget : it->second)
        {
            auto mainIt = std::find(m_widgets.begin(), m_widgets.end(), widget);
            if (mainIt != m_widgets.end())
            {
                m_widgets.erase(mainIt);
            }
        }

        // Remove from owner mapping
        m_ownerWidgetsMapping.erase(it);
    }
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::RemoveAllWidgets()
{
    m_widgets.clear();
    m_ownerWidgetsMapping.clear();
}

//----------------------------------------------------------------------------------------------------
WidgetPtr WidgetSubsystem::FindWidgetByName(String const& name) const
{
    for (auto& widget : m_widgets)
    {
        if (widget && widget->GetName() == name)
        {
            return widget;
        }
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------------------
std::vector<WidgetPtr> WidgetSubsystem::GetWidgetsByOwner(uint64_t ownerID) const
{
    auto const it = m_ownerWidgetsMapping.find(ownerID);

    if (it != m_ownerWidgetsMapping.end())
    {
        return it->second;
    }

    return {};
}

//----------------------------------------------------------------------------------------------------
std::vector<WidgetPtr> WidgetSubsystem::GetAllWidgets() const
{
    return m_widgets;
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::SetViewportWidget(WidgetPtr const& widget)
{
    m_viewportWidget = widget;
}

//----------------------------------------------------------------------------------------------------
WidgetPtr WidgetSubsystem::GetViewportWidget() const
{
    return m_viewportWidget;
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::SortWidgetsByZOrder()
{
    if (m_widgets.empty()) return;
    std::sort(m_widgets.begin(), m_widgets.end(),
              [](const WidgetPtr& a, const WidgetPtr& b)
              {
                  return a->GetZOrder() < b->GetZOrder();
              });
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::CleanupGarbageWidgets()
{
    // Remove widgets marked as garbage
    m_widgets.erase(
        std::remove_if(m_widgets.begin(), m_widgets.end(),
                       [](const WidgetPtr& widget)
                       {
                           return !widget || widget->IsGarbage();
                       }),
        m_widgets.end());

    // Clean garbage widgets from owner mapping
    for (auto& pair : m_ownerWidgetsMapping)
    {
        auto& widgets = pair.second;
        widgets.erase(
            std::remove_if(widgets.begin(), widgets.end(),
                           [](const WidgetPtr& widget)
                           {
                               return !widget || widget->IsGarbage();
                           }),
            widgets.end());
    }

    // Remove empty owner entries
    for (auto it = m_ownerWidgetsMapping.begin(); it != m_ownerWidgetsMapping.end();)
    {
        if (it->second.empty())
        {
            it = m_ownerWidgetsMapping.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
