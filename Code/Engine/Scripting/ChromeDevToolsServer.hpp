//----------------------------------------------------------------------------------------------------
// ChromeDevToolsServer.hpp
// Chrome DevTools WebSocket/HTTP Server for V8 Inspector Integration
//----------------------------------------------------------------------------------------------------

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>

#include "Engine/Core/StringUtils.hpp"

// Windows socket types (forward declarations to avoid header conflicts)
typedef uintptr_t SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;

//----------------------------------------------------------------------------------------------------
// Forward declarations
namespace v8_inspector
{
class V8Inspector;
class V8InspectorSession;
class V8InspectorClient;
} // namespace v8_inspector

class V8Subsystem;

//----------------------------------------------------------------------------------------------------
// Chrome DevTools Server Configuration
struct sChromeDevToolsConfig
{
    bool        enabled     = false;
    std::string host        = "127.0.0.1";
    int         port        = 9229;
    std::string contextName = "FirstV8 JavaScript Context";
};

//----------------------------------------------------------------------------------------------------
// WebSocket frame types
enum class eWebSocketOpcode : uint8_t
{
    CONTINUATION = 0x0,
    TEXT_FRAME   = 0x1,
    BINARY_FRAME = 0x2,
    CLOSE        = 0x8,
    PING         = 0x9,
    PONG         = 0xA
};

//----------------------------------------------------------------------------------------------------
// WebSocket connection state
struct sWebSocketConnection
{
    SOCKET      socket       = 0;
    bool        isUpgraded   = false;
    std::string receivedData;
    bool        isActive     = false;
};

//----------------------------------------------------------------------------------------------------
// Chrome DevTools Server
// Provides HTTP discovery endpoint and WebSocket server for Chrome DevTools Protocol
//----------------------------------------------------------------------------------------------------
class ChromeDevToolsServer
{
public:
    explicit ChromeDevToolsServer(sChromeDevToolsConfig config, V8Subsystem* v8Subsystem);
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
    void SetInspector(v8_inspector::V8Inspector* inspector, 
                     v8_inspector::V8InspectorSession* session);
    
    // Send Chrome DevTools Protocol message to connected clients
    void SendToDevTools(const std::string& message);

private:
    // HTTP Request Handling
    void ProcessHttpRequest(SOCKET clientSocket, const std::string& request);
    std::string HandleDiscoveryRequest();
    std::string CreateHttpResponse(const std::string& content, 
                                  const std::string& contentType = "application/json");

    // WebSocket Protocol Handling
    bool ProcessWebSocketUpgrade(SOCKET clientSocket, const std::string& request);
    void ProcessWebSocketMessage(SOCKET clientSocket, const std::string& data);
    bool HandleCustomCommand(const std::string& message); // Custom Chrome DevTools Protocol command handling
    std::string CreateWebSocketAcceptKey(const std::string& clientKey);
    std::string DecodeWebSocketFrame(const std::string& frame);
    std::string EncodeWebSocketFrame(const std::string& payload, 
                                    eWebSocketOpcode opcode = eWebSocketOpcode::TEXT_FRAME);

    // Socket Management
    void OnClientConnected(SOCKET clientSocket);
    void OnClientDisconnected(SOCKET clientSocket);
    void OnClientMessage(SOCKET clientSocket, const std::string& message);
    
    // Server Thread Functions
    void ServerThreadMain();
    void ClientHandlerThread(SOCKET clientSocket);
    
    // Socket Utilities
    bool SendRawDataToSocket(SOCKET socket, const std::string& data);
    std::string ReceiveDataFromSocket(SOCKET socket);
    void CloseSocket(SOCKET socket);

    // Utility
    std::string GenerateUUID();
    std::string Base64Encode(const std::string& input);
    std::string EscapeJsonString(const std::string& input); // JSON string escaping for responses

private:
    sChromeDevToolsConfig m_config;
    V8Subsystem*          m_v8Subsystem;

    // Socket server components
    SOCKET                                m_serverSocket;
    std::thread                           m_serverThread;
    std::vector<std::thread>              m_clientThreads;
    bool                                  m_isRunning = false;
    bool                                  m_shouldStop = false;

    // V8 Inspector integration
    v8_inspector::V8Inspector*        m_inspector        = nullptr;
    v8_inspector::V8InspectorSession* m_inspectorSession = nullptr;

    // WebSocket connections
    std::unordered_map<SOCKET, sWebSocketConnection> m_connections;
    std::vector<SOCKET>                              m_activeConnections;

    // Session management
    std::string m_sessionId;
    int         m_contextGroupId = 1;
};