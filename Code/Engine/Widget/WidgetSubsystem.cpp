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
WidgetSubsystem::~WidgetSubsystem() = default;

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::StartUp()
{
    m_widgets.clear();
    m_ownerWidgetsMapping.clear();
    m_viewportWidget = nullptr;
    m_bNeedsSorting  = false;
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::BeginFrame()
{
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
    CleanupGarbageWidgets();

    if (m_bNeedsSorting)
    {
        SortWidgetsByZOrder();
        m_bNeedsSorting = false;
    }

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
    for (auto& widget : m_widgets)
    {
        if (widget && widget->IsVisible() && !widget->IsGarbage())
        {
            widget->Render();
        }
    }
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::EndFrame()
{
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

    auto it = std::find(m_widgets.begin(), m_widgets.end(), widget);
    if (it != m_widgets.end())
    {
        m_widgets.erase(it);
    }

    // Remove from owner mapping
    uint64_t ownerID = widget->GetOwner();
    auto     mapIt   = m_ownerWidgetsMapping.find(ownerID);
    if (ownerID != 0 && mapIt != m_ownerWidgetsMapping.end())
    {
        auto& ownerWidgets = mapIt->second;
        auto  ownerIt      = std::find(ownerWidgets.begin(), ownerWidgets.end(), widget);
        if (ownerIt != ownerWidgets.end())
        {
            ownerWidgets.erase(ownerIt);
        }

        if (ownerWidgets.empty())
        {
            m_ownerWidgetsMapping.erase(mapIt);
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
        for (auto& widget : it->second)
        {
            auto mainIt = std::find(m_widgets.begin(), m_widgets.end(), widget);
            if (mainIt != m_widgets.end())
            {
                m_widgets.erase(mainIt);
            }
        }

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
bool WidgetSubsystem::HasModalWidget() const
{
    for (auto const& widget : m_widgets)
    {
        if (widget && widget->IsModal() && widget->IsVisible() && !widget->IsGarbage())
        {
            return true;
        }
    }
    return false;
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
              [](WidgetPtr const& a, WidgetPtr const& b)
              {
                  return a->GetZOrder() < b->GetZOrder();
              });
}

//----------------------------------------------------------------------------------------------------
void WidgetSubsystem::CleanupGarbageWidgets()
{
    m_widgets.erase(
        std::remove_if(m_widgets.begin(), m_widgets.end(),
                       [](WidgetPtr const& widget)
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
                           [](WidgetPtr const& widget)
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
