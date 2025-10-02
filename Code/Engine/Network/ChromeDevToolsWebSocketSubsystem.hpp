//----------------------------------------------------------------------------------------------------
// ChromeDevToolsWebSocketSubsystem.hpp
// Chrome DevTools WebSocket/HTTP Server for V8 Inspector Integration
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Network/BaseWebSocketSubsystem.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
namespace v8_inspector
{
    class V8Inspector;
    class V8InspectorSession;
    class V8InspectorClient;
}

class ScriptSubsystem;

//----------------------------------------------------------------------------------------------------
// Chrome DevTools Server Configuration (extends base WebSocket config)
//----------------------------------------------------------------------------------------------------
struct sChromeDevToolsConfig : sBaseWebSocketConfig
{
    String contextName = "ProtogameJS3D JavaScript Context";

    // JSON Parsing (extends base class FromJSON)
    static sChromeDevToolsConfig FromJSON(nlohmann::json const& j)
    {
        sChromeDevToolsConfig config;
        // Parse base class fields
        sBaseWebSocketConfig base = sBaseWebSocketConfig::FromJSON(j);
        config.enabled            = base.enabled;
        config.host               = base.host;
        config.port               = base.port;
        config.maxConnections     = base.maxConnections;
        config.enableLogging      = base.enableLogging;

        // Parse derived class fields
        if (j.contains("contextName")) config.contextName = j["contextName"].get<std::string>();

        return config;
    }
};

//----------------------------------------------------------------------------------------------------
// Chrome DevTools WebSocket Subsystem
// Provides HTTP discovery endpoint and WebSocket server for Chrome DevTools Protocol
// Inherits WebSocket protocol implementation from BaseWebSocketSubsystem
//----------------------------------------------------------------------------------------------------
class ChromeDevToolsWebSocketSubsystem : public BaseWebSocketSubsystem
{
public:
    explicit ChromeDevToolsWebSocketSubsystem(sChromeDevToolsConfig config, ScriptSubsystem* scriptSubsystem);
    ~ChromeDevToolsWebSocketSubsystem();

    // V8 Inspector Integration
    void SetInspector(v8_inspector::V8Inspector*        inspector,
                      v8_inspector::V8InspectorSession* session);

    // Send Chrome DevTools Protocol message to connected clients
    void SendToDevTools(const String& message);

    // Process queued V8 Inspector messages (call from main thread - must be public for ScriptSubsystem)
    void ProcessQueuedMessages() override;

protected:
    //----------------------------------------------------------------------------------------------------
    // BaseWebSocketSubsystem Pure Virtual Implementations
    //----------------------------------------------------------------------------------------------------

    /// @brief Called when a client connects (before WebSocket upgrade)
    void OnClientConnected(SOCKET clientSocket) override;

    /// @brief Called when a client disconnects
    void OnClientDisconnected(SOCKET clientSocket) override;

    /// @brief Called when a WebSocket message is received
    void OnClientMessage(SOCKET clientSocket, String const& message) override;

    /// @brief Generate HTTP discovery endpoint response (/json/list)
    String HandleDiscoveryRequest() override;

    /// @brief Called after successful WebSocket upgrade
    void OnWebSocketUpgraded(SOCKET clientSocket) override;

private:
    // Chrome DevTools Protocol Handling
    bool   HandleCustomCommand(String const& message);
    void   EnableDevToolsDomains(SOCKET clientSocket);
    String EscapeJsonString(const String& input);

    // Thread-safe V8 Inspector message queue
    void QueueInspectorMessage(const String& message);

    sChromeDevToolsConfig m_config;
    ScriptSubsystem*      m_scriptSubsystem;

    // V8 Inspector integration
    v8_inspector::V8Inspector*        m_inspector        = nullptr;
    v8_inspector::V8InspectorSession* m_inspectorSession = nullptr;

    // Session management
    String m_sessionId;
    int    m_contextGroupId = 1;

    // Thread-safe message queue for V8 Inspector communication
    std::queue<String> m_inspectorMessageQueue;
    std::mutex         m_messageQueueMutex;
};
