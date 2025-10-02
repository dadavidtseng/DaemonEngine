//----------------------------------------------------------------------------------------------------
// ChromeDevToolsWebSocketSubsystem.cpp
// Chrome DevTools WebSocket Subsystem - inherits WebSocket protocol from BaseWebSocketSubsystem
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Network/ChromeDevToolsWebSocketSubsystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Scripting/ScriptSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
// Any changes that you made to the warning state between push and pop are undone.
//----------------------------------------------------------------------------------------------------
#pragma warning(push)           // stores the current warning state for every warning

#pragma warning(disable: 4100)  // 'identifier' : unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4324)  // 'structname': structure was padded due to alignment specifier

// V8 Inspector includes
#include "v8-inspector.h"

#pragma warning(pop)            // pops the last warning state pushed onto the stack
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
ChromeDevToolsWebSocketSubsystem::ChromeDevToolsWebSocketSubsystem(sChromeDevToolsConfig config,
                                                                   ScriptSubsystem*      scriptSubsystem)
    : BaseWebSocketSubsystem(config),  // Call base class constructor with config
      m_config(std::move(config)),
      m_scriptSubsystem(scriptSubsystem)
{
    // Generate unique session ID (using base class GenerateUUID through inheritance)
    m_sessionId = GenerateUUID();
}

//----------------------------------------------------------------------------------------------------
ChromeDevToolsWebSocketSubsystem::~ChromeDevToolsWebSocketSubsystem()
{
    Stop();  // Base class Stop() method
}

//----------------------------------------------------------------------------------------------------
// V8 Inspector Integration
//----------------------------------------------------------------------------------------------------

void ChromeDevToolsWebSocketSubsystem::SetInspector(v8_inspector::V8Inspector*        inspector,
                                                    v8_inspector::V8InspectorSession* session)
{
    m_inspector        = inspector;
    m_inspectorSession = session;

    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               "Chrome DevTools Inspector connected");
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsWebSocketSubsystem::SendToDevTools(String const& message)
{
    // Use base class BroadcastToAllClients method
    BroadcastToAllClients(message);
}

//----------------------------------------------------------------------------------------------------
// BaseWebSocketSubsystem Pure Virtual Implementations
//----------------------------------------------------------------------------------------------------

void ChromeDevToolsWebSocketSubsystem::OnClientConnected(SOCKET const clientSocket)
{
    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("Chrome DevTools client connected from socket %llu",
                   static_cast<unsigned long long>(clientSocket)));
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsWebSocketSubsystem::OnClientDisconnected(SOCKET const clientSocket)
{
    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("Chrome DevTools client disconnected: socket %llu",
                   static_cast<unsigned long long>(clientSocket)));
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsWebSocketSubsystem::OnClientMessage(SOCKET const  clientSocket,
                                                       String const& message)
{
    UNUSED(clientSocket)  // Parameter not currently used in message handling

    // Handle custom Chrome DevTools commands first
    if (HandleCustomCommand(message))
    {
        return; // Custom command handled
    }

    // Queue the message for main-thread V8 Inspector processing
    QueueInspectorMessage(message);
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsWebSocketSubsystem::OnWebSocketUpgraded(SOCKET const clientSocket)
{
    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("Chrome DevTools WebSocket upgraded for socket %llu",
                   static_cast<unsigned long long>(clientSocket)));

    // Auto-enable Chrome DevTools domains for proper panel population
    EnableDevToolsDomains(clientSocket);
}

//----------------------------------------------------------------------------------------------------
String ChromeDevToolsWebSocketSubsystem::HandleDiscoveryRequest()
{
    // Generate Chrome DevTools Protocol discovery response (/json/list endpoint)
    std::ostringstream json;

    json << "[\n";
    json << "  {\n";
    json << "    \"id\": \"" << m_sessionId << "\",\n";
    json << "    \"type\": \"node\",\n";
    json << "    \"title\": \"" << EscapeJsonString(m_config.contextName) << "\",\n";
    json << "    \"description\": \"" << EscapeJsonString(m_config.contextName) << "\",\n";
    json << "    \"webSocketDebuggerUrl\": \"ws://" << m_config.host << ":" << m_config.port << "/\",\n";
    json << "    \"devtoolsFrontendUrl\": \"devtools://devtools/bundled/js_app.html?experiments=true&v8only=true&ws="
        << m_config.host << ":" << m_config.port << "/\",\n";
    json << "    \"url\": \"file://\",\n";
    json << "    \"faviconUrl\": \"https://v8.dev/_img/v8.svg\"\n";
    json << "  }\n";
    json << "]";

    return json.str();
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsWebSocketSubsystem::ProcessQueuedMessages()
{
    std::lock_guard lock(m_messageQueueMutex);

    while (!m_inspectorMessageQueue.empty())
    {
        String message = m_inspectorMessageQueue.front();
        m_inspectorMessageQueue.pop();

        // THREAD SAFETY: Only process messages if ScriptSubsystem is still initialized
        // Prevents race condition during shutdown where V8 isolate may be destroyed
        // while inspector messages are still queued
        if (m_inspectorSession && m_scriptSubsystem && m_scriptSubsystem->IsInitialized())
        {
            v8_inspector::StringView messageView(
                reinterpret_cast<const uint8_t*>(message.c_str()), message.length());
            m_inspectorSession->dispatchProtocolMessage(messageView);
        }
        else
        {
            // Log dropped message during shutdown
            DAEMON_LOG(LogNetwork, eLogVerbosity::Verbose,
                       "Dropped inspector message during shutdown");
        }
    }
}

//----------------------------------------------------------------------------------------------------
// Chrome DevTools Protocol Handling
//----------------------------------------------------------------------------------------------------

bool ChromeDevToolsWebSocketSubsystem::HandleCustomCommand(String const& message)
{
    // Parse for custom Chrome DevTools Protocol commands
    // Example: Runtime.evaluate, Debugger.pause, etc.

    // Check for common commands that might need special handling
    if (message.find("\"method\":\"Runtime.enable\"") != String::npos ||
        message.find("\"method\":\"Debugger.enable\"") != String::npos ||
        message.find("\"method\":\"Profiler.enable\"") != String::npos)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Verbose,
                   StringFormat("Chrome DevTools domain enabled: %s", message.c_str()));
        return false; // Let V8 Inspector handle it
    }

    return false; // No custom handling needed, forward to V8 Inspector
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsWebSocketSubsystem::EnableDevToolsDomains(SOCKET const clientSocket)
{
    // Auto-enable essential Chrome DevTools domains to populate inspector panels

    // Enable Runtime domain (Console panel)
    String enableRuntime = R"({"id":1,"method":"Runtime.enable"})";
    SendToClient(clientSocket, enableRuntime);

    // Enable Debugger domain (Sources panel)
    String enableDebugger = R"({"id":2,"method":"Debugger.enable"})";
    SendToClient(clientSocket, enableDebugger);

    // Enable Profiler domain (Profiler panel)
    String enableProfiler = R"({"id":3,"method":"Profiler.enable"})";
    SendToClient(clientSocket, enableProfiler);

    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               "Chrome DevTools domains auto-enabled (Runtime, Debugger, Profiler)");
}

//----------------------------------------------------------------------------------------------------
String ChromeDevToolsWebSocketSubsystem::EscapeJsonString(String const& input)
{
    std::ostringstream escaped;

    for (char c : input)
    {
        switch (c)
        {
        case '\"': escaped << "\\\"";
            break;
        case '\\': escaped << "\\\\";
            break;
        case '\b': escaped << "\\b";
            break;
        case '\f': escaped << "\\f";
            break;
        case '\n': escaped << "\\n";
            break;
        case '\r': escaped << "\\r";
            break;
        case '\t': escaped << "\\t";
            break;
        default:
            if ('\x00' <= c && c <= '\x1f')
            {
                escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(c);
            }
            else
            {
                escaped << c;
            }
        }
    }

    return escaped.str();
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsWebSocketSubsystem::QueueInspectorMessage(String const& message)
{
    std::lock_guard<std::mutex> lock(m_messageQueueMutex);
    m_inspectorMessageQueue.push(message);
}
