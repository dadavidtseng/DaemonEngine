//----------------------------------------------------------------------------------------------------
// FrameEventQueueScriptInterface.hpp
// JavaScript Interface for FrameEventQueue (Event Draining)
//
// Purpose:
//   Exposes FrameEventQueue to JavaScript runtime for event consumption on worker thread.
//   Follows the CallbackQueueScriptInterface pattern.
//
// JavaScript API:
//   frameEvents.drainAll()  // Returns JSON array of frame events
//
// Thread Safety:
//   - FrameEventQueue is lock-free SPSC (safe for worker thread consumption)
//   - Called from JavaScript worker thread (same thread as JSEngine.update())
//
// Author: FrameEventQueue - Input Refactoring
// Date: 2026-02-17
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"

//----------------------------------------------------------------------------------------------------
class FrameEventQueue;

//----------------------------------------------------------------------------------------------------
class FrameEventQueueScriptInterface : public IScriptableObject
{
public:
    explicit FrameEventQueueScriptInterface(FrameEventQueue* frameEventQueue);
    ~FrameEventQueueScriptInterface() override = default;

    void                          InitializeMethodRegistry() override;
    ScriptMethodResult            CallMethod(String const& methodName, ScriptArgs const& args) override;
    std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
    StringList                    GetAvailableProperties() const override;
    std::any                      GetProperty(String const& propertyName) const override;
    bool                          SetProperty(String const& propertyName, std::any const& value) override;

private:
    ScriptMethodResult ExecuteDrainAll(ScriptArgs const& args);

    FrameEventQueue* m_frameEventQueue;
};
