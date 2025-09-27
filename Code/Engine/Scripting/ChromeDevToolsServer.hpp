//----------------------------------------------------------------------------------------------------
// ChromeDevToolsServer.hpp
// Chrome DevTools WebSocket/HTTP Server for V8 Inspector Integration
//----------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>

#include "Engine/Core/StringUtils.hpp"

// Windows socket types (forward declarations to avoid header conflicts)
typedef uintptr_t          SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;

//----------------------------------------------------------------------------------------------------
// Forward declarations
namespace v8_inspector
{
    class V8Inspector;
    class V8InspectorSession;
    class V8InspectorClient;
} // namespace v8_inspector

class ScriptSubsystem;

//----------------------------------------------------------------------------------------------------
// Chrome DevTools Server Configuration
struct sChromeDevToolsConfig
{
    bool   enabled     = false;
    String host        = "127.0.0.1";
    int    port        = 9229;
    String contextName = "ProtogameJS3D JavaScript Context";
};

//----------------------------------------------------------------------------------------------------
// WebSocket frame types
enum class eWebSocketOpcode : uint8_t
{
    CONTINUATION = 0x0,
    TEXT_FRAME = 0x1,
    BINARY_FRAME = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

//----------------------------------------------------------------------------------------------------
// WebSocket connection state
struct sWebSocketConnection
{
    SOCKET socket     = 0;
    bool   isUpgraded = false;
    String receivedData;
    bool   isActive = false;
};

//----------------------------------------------------------------------------------------------------
// Chrome DevTools Server
// Provides HTTP discovery endpoint and WebSocket server for Chrome DevTools Protocol
//----------------------------------------------------------------------------------------------------
class ChromeDevToolsServer
{
public:
    explicit ChromeDevToolsServer(sChromeDevToolsConfig config, ScriptSubsystem* scriptSubsystem);
    ~ChromeDevToolsServer();

    // Lifecycle
    bool Start();
    void Update();
    void Stop();

    // Status
    bool IsRunning() const;
    bool HasActiveConnections() const;
    int  GetPort() const;

    // V8 Inspector Integration
    void SetInspector(v8_inspector::V8Inspector*        inspector,
                      v8_inspector::V8InspectorSession* session);

    // Send Chrome DevTools Protocol message to connected clients
    void SendToDevTools(const String& message);

    // Process queued V8 Inspector messages (call from main thread)
    void ProcessQueuedMessages();

private:
    // HTTP Request Handling
    void   ProcessHttpRequest(SOCKET clientSocket, const String& request);
    String HandleDiscoveryRequest();
    String CreateHttpResponse(String const& content,
                              String const& contentType = "application/json");

    // WebSocket Protocol Handling
    bool   ProcessWebSocketUpgrade(SOCKET clientSocket, const String& request);
    void   ProcessWebSocketMessage(SOCKET clientSocket, const String& data);
    bool   HandleCustomCommand(String const& message); // Custom Chrome DevTools Protocol command handling
    String CreateWebSocketAcceptKey(String const& clientKey);
    String DecodeWebSocketFrame(String const& frame);
    String EncodeWebSocketFrame(String const& payload, eWebSocketOpcode opcode = eWebSocketOpcode::TEXT_FRAME);

    // Socket Management
    void OnClientConnected(SOCKET clientSocket);
    void OnClientDisconnected(SOCKET clientSocket);
    void OnClientMessage(SOCKET clientSocket, const String& message);
    void EnableDevToolsDomains(SOCKET clientSocket); // Auto-enable domains for panel population

    // Server Thread Functions
    void ServerThreadMain();
    void ClientHandlerThread(SOCKET clientSocket);

    // Socket Utilities
    bool   SendRawDataToSocket(SOCKET socket, const String& data);
    String ReceiveDataFromSocket(SOCKET socket);
    void   CloseSocket(SOCKET socket);

    // Utility
    String GenerateUUID();
    String Base64Encode(const String& input);
    String EscapeJsonString(const String& input); // JSON string escaping for responses

    // Thread-safe V8 Inspector message queue
    void QueueInspectorMessage(const String& message);

    sChromeDevToolsConfig m_config;
    ScriptSubsystem*      m_scriptSubsystem;

    // Socket server components
    SOCKET                   m_serverSocket;
    std::thread              m_serverThread;
    std::vector<std::thread> m_clientThreads;
    bool                     m_isRunning  = false;
    bool                     m_shouldStop = false;

    // V8 Inspector integration
    v8_inspector::V8Inspector*        m_inspector        = nullptr;
    v8_inspector::V8InspectorSession* m_inspectorSession = nullptr;

    // WebSocket connections
    std::unordered_map<SOCKET, sWebSocketConnection> m_connections;
    std::vector<SOCKET>                              m_activeConnections;

    // Session management
    String m_sessionId;
    int    m_contextGroupId = 1;

    // Thread-safe message queue for V8 Inspector communication
    std::queue<String> m_inspectorMessageQueue;
    std::mutex         m_messageQueueMutex;
};
