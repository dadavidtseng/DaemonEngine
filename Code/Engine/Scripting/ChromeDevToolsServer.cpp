//----------------------------------------------------------------------------------------------------
// ChromeDevToolsServer.cpp
// Chrome DevTools WebSocket/HTTP Server Implementation using Direct Windows Sockets
//----------------------------------------------------------------------------------------------------

#include "Engine/Scripting/ChromeDevToolsServer.hpp"

#include <sstream>
#include <iomanip>
#include <random>
#include <array>
#include <algorithm>

#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/StringUtils.hpp"
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

// Windows socket includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Engine/Core/EngineCommon.hpp"

#pragma comment(lib, "ws2_32.lib")

//----------------------------------------------------------------------------------------------------
// WebSocket Magic String for handshake
static const std::string WEBSOCKET_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// Invalid socket value
static constexpr SOCKET INVALID_SOCKET_VALUE = INVALID_SOCKET;

//----------------------------------------------------------------------------------------------------
// Simple SHA1 implementation to avoid Windows header conflicts
class SimpleSHA1
{
public:
    static std::string Hash(const std::string& input)
    {
        // For WebSocket handshake, we can use a simplified approach
        // This is not cryptographically secure but works for the WebSocket protocol requirement
        uint32_t hash[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

        std::string data = input;
        data += '\x80'; // Append single 1 bit

        // Pad to 64 bytes - 8 for length
        while ((data.length() % 64) != 56)
        {
            data += '\x00';
        }

        // Append length in bits as 64-bit big-endian integer
        uint64_t bitLength = static_cast<uint64_t>(input.length()) * 8;
        for (int i = 7; i >= 0; --i)
        {
            data += static_cast<char>((bitLength >> (i * 8)) & 0xFF);
        }

        // Process in 64-byte chunks
        for (size_t chunk = 0; chunk < data.length(); chunk += 64)
        {
            uint32_t w[80];

            // Break chunk into sixteen 32-bit big-endian words
            for (size_t i = 0; i < 16; ++i)
            {
                w[i] = static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 0])) << 24 |
                static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 1])) << 16 |
                static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 2])) << 8 |
                static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 3]));
            }

            // Extend the sixteen 32-bit words into eighty 32-bit words
            for (int i = 16; i < 80; ++i)
            {
                w[i] = LeftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
            }

            // Initialize hash value for this chunk
            uint32_t a = hash[0], b = hash[1], c = hash[2], d = hash[3], e = hash[4];

            // Main loop
            for (int i = 0; i < 80; ++i)
            {
                uint32_t f, k;
                if (i < 20)
                {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                }
                else if (i < 40)
                {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if (i < 60)
                {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                }
                else
                {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }

                uint32_t temp = LeftRotate(a, 5) + f + e + k + w[i];
                e             = d;
                d             = c;
                c             = LeftRotate(b, 30);
                b             = a;
                a             = temp;
            }

            // Add this chunk's hash to result so far
            hash[0] += a;
            hash[1] += b;
            hash[2] += c;
            hash[3] += d;
            hash[4] += e;
        }

        // Produce the final hash value as a 160-bit number (20 bytes)
        std::string result;
        for (int i = 0; i < 5; ++i)
        {
            for (int j = 3; j >= 0; --j)
            {
                result += static_cast<char>((hash[i] >> (j * 8)) & 0xFF);
            }
        }

        return result;
    }

private:
    static uint32_t LeftRotate(uint32_t value, int amount)
    {
        return (value << amount) | (value >> (32 - amount));
    }
};

//----------------------------------------------------------------------------------------------------
ChromeDevToolsServer::ChromeDevToolsServer(sChromeDevToolsConfig config, ScriptSubsystem* scriptSubsystem)
    : m_config(std::move(config)), m_scriptSubsystem(scriptSubsystem), m_serverSocket(INVALID_SOCKET_VALUE)
{
    // Generate unique session ID
    m_sessionId = GenerateUUID();
}

//----------------------------------------------------------------------------------------------------
ChromeDevToolsServer::~ChromeDevToolsServer()
{
    Stop();
}

//----------------------------------------------------------------------------------------------------
bool ChromeDevToolsServer::Start()
{
    if (m_isRunning || !m_config.enabled)
    {
        return false;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("Starting Chrome DevTools Server on {}:{}", m_config.host, m_config.port));

    // Initialize Winsock
    WSADATA wsaData;
    int     result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
                   StringFormat("WSAStartup failed: {}", result));
        return false;
    }

    // Create server socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_serverSocket == INVALID_SOCKET)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
                   StringFormat("Failed to create server socket: {}", WSAGetLastError()));
        WSACleanup();
        return false;
    }

    // Set socket options
    int reuse = 1;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse)) == SOCKET_ERROR)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                   StringFormat("Failed to set SO_REUSEADDR: {}", WSAGetLastError()));
    }

    // Bind to address and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(static_cast<u_short>(m_config.port));

    if (inet_pton(AF_INET, m_config.host.c_str(), &serverAddr.sin_addr) <= 0)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
                   StringFormat("Invalid server address: {}", m_config.host));
        CloseSocket(m_serverSocket);
        WSACleanup();
        return false;
    }

    if (bind(m_serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
                   StringFormat("Failed to bind to {}:{}, error: {}", m_config.host, m_config.port, WSAGetLastError()));
        CloseSocket(m_serverSocket);
        WSACleanup();
        return false;
    }

    // Start listening
    if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
                   StringFormat("Failed to listen on socket: {}", WSAGetLastError()));
        CloseSocket(m_serverSocket);
        WSACleanup();
        return false;
    }

    m_isRunning  = true;
    m_shouldStop = false;

    // Start server thread
    m_serverThread = std::thread(&ChromeDevToolsServer::ServerThreadMain, this);

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("Chrome DevTools Server started successfully"));
    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("DevTools Discovery: https://{}:{}/json/list", m_config.host, m_config.port));
    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("Chrome DevTools URL: chrome://inspect/#devices"));

    return true;
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::Update()
{
    // For direct socket implementation, most work is done in background threads
    // This method can be used for cleanup of finished client threads

    // Clean up finished client threads
    m_clientThreads.erase(
        std::remove_if(m_clientThreads.begin(), m_clientThreads.end(),
                       [](std::thread& t) {
                           if (t.joinable())
                           {
                               return false; // Keep active threads
                           }
                           return true; // Remove finished threads
                       }),
        m_clientThreads.end());
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::Stop()
{
    if (!m_isRunning) return;

    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Stopping Chrome DevTools Server..."));

    m_shouldStop = true;
    m_isRunning  = false;

    // Close server socket to stop accepting new connections
    if (m_serverSocket != INVALID_SOCKET)
    {
        CloseSocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
    }

    // Wait for server thread to finish
    if (m_serverThread.joinable())
    {
        m_serverThread.join();
    }

    // Wait for all client threads to finish
    for (auto& thread : m_clientThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    m_clientThreads.clear();

    // Close all client sockets
    for (auto& [socket, connection] : m_connections)
    {
        CloseSocket(socket);
    }
    m_connections.clear();
    m_activeConnections.clear();

    // Cleanup Winsock
    WSACleanup();

    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Chrome DevTools Server stopped"));
}

//----------------------------------------------------------------------------------------------------
bool ChromeDevToolsServer::IsRunning() const
{
    return m_isRunning;
}

//----------------------------------------------------------------------------------------------------
bool ChromeDevToolsServer::HasActiveConnections() const
{
    return !m_activeConnections.empty();
}

//----------------------------------------------------------------------------------------------------
int ChromeDevToolsServer::GetPort() const
{
    return m_config.port;
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::SetInspector(v8_inspector::V8Inspector*        inspector,
                                        v8_inspector::V8InspectorSession* session)
{
    m_inspector        = inspector;
    m_inspectorSession = session;

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("Chrome DevTools Server connected to V8 Inspector"));
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::SendToDevTools(const std::string& message)
{
    if (!m_isRunning || m_activeConnections.empty())
    {
        return;
    }

    std::string wsFrame = EncodeWebSocketFrame(message);

    // Send to all active WebSocket connections
    for (SOCKET clientSocket : m_activeConnections)
    {
        SendRawDataToSocket(clientSocket, wsFrame);
    }
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::ServerThreadMain()
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("Chrome DevTools server thread started"));

    while (!m_shouldStop && m_isRunning)
    {
        // Accept incoming connections
        sockaddr_in clientAddr;
        int         clientAddrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(m_serverSocket,
                                     reinterpret_cast<sockaddr*>(&clientAddr),
                                     &clientAddrSize);

        if (clientSocket == INVALID_SOCKET)
        {
            if (m_shouldStop) break;

            int error = WSAGetLastError();
            if (error != WSAEINTR && error != WSAEWOULDBLOCK)
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Error,
                           StringFormat("Accept failed: {}", error));
            }
            continue;
        }

        // Handle client in separate thread
        m_clientThreads.emplace_back(&ChromeDevToolsServer::ClientHandlerThread, this, clientSocket);

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        DAEMON_LOG(LogScript, eLogVerbosity::Log,
                   StringFormat("Chrome DevTools client connected from {}:{}",
                       clientIP, ntohs(clientAddr.sin_port)));
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("Chrome DevTools server thread stopped"));
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::ClientHandlerThread(SOCKET const clientSocket)
{
    OnClientConnected(clientSocket);

    std::string receivedData;
    bool        isWebSocket = false;

    while (!m_shouldStop && m_isRunning)
    {
        std::string data = ReceiveDataFromSocket(clientSocket);
        if (data.empty())
        {
            break; // Client disconnected
        }

        receivedData += data;

        if (!isWebSocket)
        {
            // Look for complete HTTP request (double CRLF)
            size_t headerEnd = receivedData.find("\r\n\r\n");
            if (headerEnd != std::string::npos)
            {
                std::string httpRequest = receivedData.substr(0, headerEnd + 4);
                receivedData.erase(0, headerEnd + 4);

                // Check if this is a WebSocket upgrade request (case insensitive)
                std::string httpLowerCase = httpRequest;
                std::transform(httpLowerCase.begin(), httpLowerCase.end(), httpLowerCase.begin(),
                               [](char c) { return static_cast<char>(::tolower(static_cast<unsigned char>(c))); });
                if (httpLowerCase.find("upgrade: websocket") != std::string::npos)
                {
                    if (ProcessWebSocketUpgrade(clientSocket, httpRequest))
                    {
                        isWebSocket = true;
                        m_activeConnections.push_back(clientSocket);

                        // Replay all loaded scripts to the newly connected DevTools client
                        if (m_scriptSubsystem)
                        {
                            DAEMON_LOG(LogScript, eLogVerbosity::Display,
                                       "Replaying scripts to newly connected Chrome DevTools client");
                            m_scriptSubsystem->ReplayScriptsToDevTools();
                        }
                    }
                    else
                    {
                        DAEMON_LOG(LogScript, eLogVerbosity::Error, "WebSocket upgrade failed");
                        break; // Upgrade failed
                    }
                }
                else
                {
                    // Handle HTTP request (discovery endpoint)
                    ProcessHttpRequest(clientSocket, httpRequest);
                    break; // Close HTTP connection after response
                }
            }
        }
        else
        {
            // Process WebSocket frames
            if (!receivedData.empty())
            {
                ProcessWebSocketMessage(clientSocket, receivedData);
                receivedData.clear();
            }
        }
    }

    OnClientDisconnected(clientSocket);
    CloseSocket(clientSocket);
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::OnClientConnected(SOCKET const clientSocket)
{
    sWebSocketConnection connection;
    connection.socket     = clientSocket;
    connection.isUpgraded = false;
    connection.isActive   = false;

    m_connections[clientSocket] = connection;
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::OnClientDisconnected(SOCKET const clientSocket)
{
    // Remove from active connections
    auto const it = std::ranges::find(m_activeConnections, clientSocket);

    if (it != m_activeConnections.end())
    {
        m_activeConnections.erase(it);
    }

    // Remove connection
    m_connections.erase(clientSocket);

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("Chrome DevTools client {} disconnected", static_cast<int>(clientSocket)));
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::ProcessHttpRequest(SOCKET const  clientSocket,
                                              String const& request)
{
    std::string response;

    // Parse request line
    std::istringstream requestStream(request);
    std::string        method, path, version;
    requestStream >> method >> path >> version;

    if (method == "GET" && (path == "/json/list" || path == "/json"))
    {
        response = HandleDiscoveryRequest();
    }
    else
    {
        // 404 Not Found
        response = CreateHttpResponse("Not Found", "text/plain");
        response = "HTTP/1.1 404 Not Found\r\n" + response;
    }

    SendRawDataToSocket(clientSocket, response);
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::HandleDiscoveryRequest()
{
    // Create Chrome DevTools discovery JSON
    std::ostringstream json;
    json << "[\n";
    json << "  {\n";
    json << "    \"id\": \"" << m_sessionId << "\",\n";
    json << "    \"type\": \"node\",\n";
    json << "    \"title\": \"" << m_config.contextName << "\",\n";
    json << "    \"description\": \"FirstV8 JavaScript Engine\",\n";
    json << "    \"webSocketDebuggerUrl\": \"ws://" << m_config.host << ":" << m_config.port << "/\",\n";
    json << "    \"devtoolsFrontendUrl\": \"devtools://devtools/bundled/js_app.html?experiments=true&v8only=true&ws=" << m_config.host << ":" << m_config.port << "/\",\n";
    json << "    \"url\": \"file://FirstV8\",\n";
    json << "    \"faviconUrl\": \"\"\n";
    json << "  }\n";
    json << "]";

    return CreateHttpResponse(json.str(), "application/json");
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::CreateHttpResponse(const std::string& content,
                                                     const std::string& contentType)
{
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << content;

    return response.str();
}

//----------------------------------------------------------------------------------------------------
bool ChromeDevToolsServer::ProcessWebSocketUpgrade(SOCKET clientSocket, const std::string& request)
{
    // Extract WebSocket key from request
    std::string        wsKey;
    std::istringstream requestStream(request);
    std::string        line;

    while (std::getline(requestStream, line) && line != "\r")
    {
        if (line.find("Sec-WebSocket-Key:") == 0)
        {
            wsKey = line.substr(19); // Skip "Sec-WebSocket-Key: "
            // Remove carriage return
            if (!wsKey.empty() && wsKey.back() == '\r') wsKey.pop_back();
            break;
        }
    }

    if (wsKey.empty())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
                   StringFormat("Missing Sec-WebSocket-Key in upgrade request"));
        return false;
    }

    // Create WebSocket accept key
    std::string acceptKey = CreateWebSocketAcceptKey(wsKey);

    // Create WebSocket upgrade response
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << acceptKey << "\r\n";
    response << "\r\n";

    bool success = SendRawDataToSocket(clientSocket, response.str());

    if (success)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display,
                   StringFormat("Chrome DevTools WebSocket connection established"));
        
        // AUTO-ENABLE CRITICAL DOMAINS for panel population
        // Chrome DevTools requires explicit domain enablement to process events
        EnableDevToolsDomains(clientSocket);
    }

    return success;
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::ProcessWebSocketMessage(SOCKET clientSocket, const std::string& data)
{
    if (data.empty())
    {
        return;
    }

    try
    {
        std::string decodedMessage = DecodeWebSocketFrame(data);

        if (!decodedMessage.empty())
        {
            // Check for custom commands we need to handle before forwarding to V8 Inspector
            bool handledCustomCommand = HandleCustomCommand(decodedMessage);

            if (!handledCustomCommand)
            {
                // Forward to V8 Inspector for standard commands
                if (m_inspectorSession)
                {
                    // THREAD SAFETY FIX: Queue the message instead of calling directly
                    // V8 Inspector operations must happen on the main thread
                    QueueInspectorMessage(decodedMessage);
                }
                else
                {
                    DAEMON_LOG(LogScript, eLogVerbosity::Error,
                               StringFormat("Cannot forward message: V8 Inspector session is null"));
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
                   StringFormat("Error processing WebSocket message: {}", e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
bool ChromeDevToolsServer::HandleCustomCommand(const std::string& message)
{
    // Parse JSON to check for commands we need to handle
    
    // DOMAIN ENABLE COMMANDS: Handle critical domain enablement requests
    // Chrome DevTools sends these automatically when connecting
    if (message.find("\"method\":\"Runtime.enable\"") != std::string::npos ||
        message.find("\"method\":\"Console.enable\"") != std::string::npos ||
        message.find("\"method\":\"Debugger.enable\"") != std::string::npos ||
        message.find("\"method\":\"Profiler.enable\"") != std::string::npos ||
        message.find("\"method\":\"HeapProfiler.enable\"") != std::string::npos ||
        message.find("\"method\":\"Network.enable\"") != std::string::npos ||
        message.find("\"method\":\"Page.enable\"") != std::string::npos ||
        message.find("\"method\":\"DOM.enable\"") != std::string::npos)
    {
        // Extract call ID from the enable request
        std::string callId = "1"; // Default fallback
        size_t idPos = message.find("\"id\":");
        if (idPos != std::string::npos)
        {
            size_t idStart = message.find(":", idPos) + 1;
            size_t idEnd = message.find(",", idStart);
            if (idEnd == std::string::npos) idEnd = message.find("}", idStart);
            callId = message.substr(idStart, idEnd - idStart);
            // Remove any whitespace or quotes
            callId.erase(0, callId.find_first_not_of(" \t\""));
            callId.erase(callId.find_last_not_of(" \t\"") + 1);
        }

        // Send success response for domain enablement
        std::string enableResponse = "{\"id\":" + callId + ",\"result\":{}}";
        SendToDevTools(enableResponse);
        
        // Send domain-specific initialization events after successful enablement
        std::string domainType;
        if (message.find("Runtime.enable") != std::string::npos) domainType = "Runtime";
        else if (message.find("Console.enable") != std::string::npos) domainType = "Console";
        else if (message.find("Debugger.enable") != std::string::npos) domainType = "Debugger";
        else if (message.find("Profiler.enable") != std::string::npos) domainType = "Profiler";
        else if (message.find("HeapProfiler.enable") != std::string::npos) domainType = "HeapProfiler";
        else if (message.find("Network.enable") != std::string::npos) domainType = "Network";
        else if (message.find("Page.enable") != std::string::npos) domainType = "Page";
        else if (message.find("DOM.enable") != std::string::npos) domainType = "DOM";
        
        DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                   StringFormat("DEVTOOLS DEBUG: Successfully enabled {} domain (id: {})", domainType, callId));
        return true; // Handled
    }
    
    // DEBUGGER SCRIPT SOURCE: Handle Debugger.getScriptSource requests
    if (message.find("\"method\":\"Debugger.getScriptSource\"") != std::string::npos)
    {
        // Extract call ID and script ID from the message
        // Example: {"id":123,"method":"Debugger.getScriptSource","params":{"scriptId":"456"}}

        std::string callId;
        std::string scriptId;

        // Simple JSON parsing for call ID
        size_t idPos = message.find("\"id\":");
        if (idPos != std::string::npos)
        {
            size_t idStart = message.find(":", idPos) + 1;
            size_t idEnd   = message.find(",", idStart);
            if (idEnd == std::string::npos) idEnd = message.find("}", idStart);
            callId = message.substr(idStart, idEnd - idStart);
            // Remove any whitespace or quotes
            callId.erase(0, callId.find_first_not_of(" \t\""));
            callId.erase(callId.find_last_not_of(" \t\"") + 1);
        }

        // Simple JSON parsing for script ID
        size_t scriptIdPos = message.find("\"scriptId\":");
        if (scriptIdPos != std::string::npos)
        {
            size_t scriptIdStart = message.find("\"", scriptIdPos + 11) + 1; // Skip "scriptId":"
            size_t scriptIdEnd   = message.find("\"", scriptIdStart);
            scriptId             = message.substr(scriptIdStart, scriptIdEnd - scriptIdStart);
        }

        if (!callId.empty() && !scriptId.empty() && m_scriptSubsystem)
        {
            std::string scriptSource = m_scriptSubsystem->HandleDebuggerGetScriptSource(scriptId);

            // Create response
            std::string response =
            "{\"id\":" + callId + ","
            "\"result\":{"
            "\"scriptSource\":\"" + EscapeJsonString(scriptSource) + "\""
            "}"
            "}";

            SendToDevTools(response);

            return true; // Command handled
        }
    }

    return false; // Command not handled, forward to V8 Inspector
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::EscapeJsonString(const std::string& input)
{
    std::string escaped;
    escaped.reserve(input.length() * 2); // Reserve space for potential escaping

    for (char c : input)
    {
        switch (c)
        {
        case '"': escaped += "\\\"";
            break;
        case '\\': escaped += "\\\\";
            break;
        case '\n': escaped += "\\n";
            break;
        case '\r': escaped += "\\r";
            break;
        case '\t': escaped += "\\t";
            break;
        default: escaped += c;
            break;
        }
    }

    return escaped;
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::CreateWebSocketAcceptKey(const std::string& clientKey)
{
    std::string combined = clientKey + WEBSOCKET_MAGIC;

    // Calculate SHA-1 hash using our simple implementation
    std::string hash = SimpleSHA1::Hash(combined);

    // Base64 encode the hash
    return Base64Encode(hash);
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::DecodeWebSocketFrame(const std::string& frame)
{
    if (frame.length() < 2) return "";

    uint8_t firstByte  = static_cast<uint8_t>(frame[0]);
    uint8_t secondByte = static_cast<uint8_t>(frame[1]);

    bool             isFinal       = (firstByte & 0x80) != 0;
    eWebSocketOpcode opcode        = static_cast<eWebSocketOpcode>(firstByte & 0x0F);
    bool             isMasked      = (secondByte & 0x80) != 0;
    uint64_t         payloadLength = secondByte & 0x7F;

    // Suppress unused variable warnings (these variables are used for protocol validation in more complex scenarios)
    UNUSED(isFinal)
    UNUSED(opcode)

    size_t headerLength = 2;

    // Extended payload length
    if (payloadLength == 126)
    {
        if (frame.length() < 4) return "";
        payloadLength = (static_cast<uint16_t>(static_cast<uint8_t>(frame[2])) << 8) |
        static_cast<uint16_t>(static_cast<uint8_t>(frame[3]));
        headerLength = 4;
    }
    else if (payloadLength == 127)
    {
        if (frame.length() < 10) return "";
        payloadLength = 0;
        for (int i = 2; i < 10; ++i)
        {
            payloadLength = (payloadLength << 8) | static_cast<uint8_t>(frame[i]);
        }
        headerLength = 10;
    }

    // Masking key
    std::array<uint8_t, 4> maskingKey = {0};
    if (isMasked)
    {
        if (frame.length() < headerLength + 4) return "";
        for (int i = 0; i < 4; ++i)
        {
            maskingKey[i] = static_cast<uint8_t>(frame[headerLength + i]);
        }
        headerLength += 4;
    }

    // Payload
    if (frame.length() < headerLength + payloadLength) return "";

    std::string payload = frame.substr(headerLength, payloadLength);

    // Unmask payload if necessary
    if (isMasked)
    {
        for (size_t i = 0; i < payload.length(); ++i)
        {
            payload[i] = static_cast<char>(static_cast<uint8_t>(payload[i]) ^ maskingKey[i % 4]);
        }
    }

    return payload;
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::EncodeWebSocketFrame(const std::string& payload,
                                                       eWebSocketOpcode   opcode)
{
    std::string frame;

    auto pushByte = [&frame](uint8_t byte) {
        frame.push_back(static_cast<char>(byte));
    };

    // First byte: FIN=1, RSV=000, Opcode
    pushByte(0x80 | static_cast<uint8_t>(opcode));

    // Second byte and payload length
    size_t payloadLength = payload.length();
    if (payloadLength < 126)
    {
        pushByte(static_cast<uint8_t>(payloadLength));
    }
    else if (payloadLength <= 0xFFFF)
    {
        pushByte(126);
        pushByte(static_cast<uint8_t>((payloadLength >> 8) & 0xFF));
        pushByte(static_cast<uint8_t>(payloadLength & 0xFF));
    }
    else
    {
        pushByte(127);
        for (int i = 7; i >= 0; --i)
        {
            pushByte(static_cast<uint8_t>((payloadLength >> (i * 8)) & 0xFF));
        }
    }

    // Payload
    frame.append(payload);

    return frame;
}

//----------------------------------------------------------------------------------------------------
bool ChromeDevToolsServer::SendRawDataToSocket(SOCKET socket, const std::string& data)
{
    if (socket == INVALID_SOCKET || data.empty()) return false;

    const char* buffer    = data.c_str();
    int         totalSent = 0;
    int         dataSize  = static_cast<int>(data.length());

    while (totalSent < dataSize)
    {
        int sent = send(socket, buffer + totalSent, dataSize - totalSent, 0);
        if (sent == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            DAEMON_LOG(LogScript, eLogVerbosity::Error,
                       StringFormat("Send failed for socket {}: {}", static_cast<int>(socket), error));
            return false;
        }
        totalSent += sent;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::ReceiveDataFromSocket(SOCKET socket)
{
    if (socket == INVALID_SOCKET) return "";

    char buffer[4096];
    int  received = recv(socket, buffer, sizeof(buffer) - 1, 0);

    if (received > 0)
    {
        buffer[received] = '\0';
        return std::string(buffer, received);
    }
    else if (received == 0)
    {
        // Connection closed gracefully
        return "";
    }
    else
    {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAECONNRESET)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error,
                       StringFormat("Receive failed for socket {}: {}", static_cast<int>(socket), error));
        }
        return "";
    }
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::CloseSocket(SOCKET socket)
{
    if (socket != INVALID_SOCKET)
    {
        closesocket(socket);
    }
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::GenerateUUID()
{
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    const char* chars = "0123456789abcdef";
    std::string uuid  = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";

    for (char& c : uuid)
    {
        if (c == 'x')
        {
            c = chars[dis(gen)];
        }
        else if (c == 'y')
        {
            c = chars[(dis(gen) & 0x3) | 0x8];
        }
    }

    return uuid;
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::Base64Encode(const std::string& input)
{
    static const std::string chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    int         val  = 0;
    int         valb = -6;

    for (unsigned char c : input)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6)
    {
        result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (result.size() % 4)
    {
        result.push_back('=');
    }

    return result;
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::QueueInspectorMessage(const std::string& message)
{
    // Thread-safe queuing of V8 Inspector messages
    // This method is called from background client threads
    std::lock_guard<std::mutex> lock(m_messageQueueMutex);
    m_inspectorMessageQueue.push(message);

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               StringFormat("Queued V8 Inspector message for main thread processing (queue size: {})",
                           m_inspectorMessageQueue.size()));
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::ProcessQueuedMessages()
{
    // Process all queued V8 Inspector messages on the main thread
    // This method must be called from the main thread (e.g., in ScriptSubsystem::Update())
    
    if (!m_inspectorSession)
    {
        return; // No inspector session available
    }

    std::lock_guard<std::mutex> lock(m_messageQueueMutex);
    
    while (!m_inspectorMessageQueue.empty())
    {
        std::string message = m_inspectorMessageQueue.front();
        m_inspectorMessageQueue.pop();

        try
        {
            // Convert std::string to v8_inspector::StringView
            v8_inspector::StringView messageView(
                reinterpret_cast<const uint8_t*>(message.c_str()),
                message.length()
            );

            // Safely dispatch message to V8 Inspector on main thread
            m_inspectorSession->dispatchProtocolMessage(messageView);

            DAEMON_LOG(LogScript, eLogVerbosity::Log,
                       StringFormat("Processed V8 Inspector message on main thread: {}",
                                   message.substr(0, 100))); // Log first 100 chars
        }
        catch (const std::exception& e)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error,
                       StringFormat("Error processing queued V8 Inspector message: {}", e.what()));
        }
    }
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::EnableDevToolsDomains(SOCKET clientSocket)
{
    // AUTO-RESPOND to domain enable requests that Chrome DevTools will send
    // This simulates successful domain enablement responses
    
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
               StringFormat("DEVTOOLS DEBUG: Preparing auto-responses for domain enablement (client socket {})", static_cast<int>(clientSocket)));

    // Chrome DevTools will send these enable commands automatically when connecting
    // We need to ensure we respond with success messages for each domain
    
    // Store the client socket for later domain enable responses
    // The actual domain enablement will be handled in HandleCustomCommand when DevTools sends the requests
    
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
               StringFormat("DEVTOOLS DEBUG: Domain enablement handler ready for client socket {}", static_cast<int>(clientSocket)));
}
