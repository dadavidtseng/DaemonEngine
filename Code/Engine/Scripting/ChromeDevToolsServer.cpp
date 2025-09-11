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

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Scripting/V8Subsystem.hpp"

// V8 Inspector includes
#include "v8-inspector.h"

// Windows socket includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

//----------------------------------------------------------------------------------------------------
// WebSocket Magic String for handshake
static const std::string WEBSOCKET_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// Invalid socket value
static const SOCKET INVALID_SOCKET_VALUE = static_cast<SOCKET>(~0);

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
            for (int i = 0; i < 16; ++i)
            {
                w[i] = static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 0])) << 24 |
                       static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 1])) << 16 |
                       static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 2])) << 8  |
                       static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 3]));
            }
            
            // Extend the sixteen 32-bit words into eighty 32-bit words
            for (int i = 16; i < 80; ++i)
            {
                w[i] = LeftRotate(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
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
                e = d;
                d = c;
                c = LeftRotate(b, 30);
                b = a;
                a = temp;
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
ChromeDevToolsServer::ChromeDevToolsServer(sChromeDevToolsConfig config, V8Subsystem* v8Subsystem)
    : m_config(std::move(config)), m_v8Subsystem(v8Subsystem), m_serverSocket(INVALID_SOCKET_VALUE)
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
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
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
    serverAddr.sin_port = htons(static_cast<u_short>(m_config.port));
    
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

    m_isRunning = true;
    m_shouldStop = false;

    // Start server thread
    m_serverThread = std::thread(&ChromeDevToolsServer::ServerThreadMain, this);

    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
              StringFormat("Chrome DevTools Server started successfully"));
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
              StringFormat("DevTools Discovery: http://{}:{}/json/list", m_config.host, m_config.port));
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
                          if (t.joinable()) {
                              return false; // Keep active threads
                          }
                          return true; // Remove finished threads
                      }),
        m_clientThreads.end());
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::Stop()
{
    if (!m_isRunning)
        return;

    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Stopping Chrome DevTools Server..."));

    m_shouldStop = true;
    m_isRunning = false;

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
void ChromeDevToolsServer::SetInspector(v8_inspector::V8Inspector* inspector, 
                                        v8_inspector::V8InspectorSession* session)
{
    m_inspector = inspector;
    m_inspectorSession = session;
    
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
              StringFormat("Chrome DevTools Server connected to V8 Inspector"));
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::SendToDevTools(const std::string& message)
{
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
              StringFormat("SendToDevTools called with message: {}", message));
    
    if (!m_isRunning || m_activeConnections.empty())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning, 
                  StringFormat("Cannot send to DevTools: running={}, connections={}", 
                              m_isRunning, m_activeConnections.size()));
        return;
    }

    std::string wsFrame = EncodeWebSocketFrame(message);
    
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
              StringFormat("Sending WebSocket frame ({} bytes) to {} connections", 
                          wsFrame.length(), m_activeConnections.size()));
    
    // Send to all active WebSocket connections
    for (SOCKET clientSocket : m_activeConnections)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                  StringFormat("Sending to client socket: {}", static_cast<int>(clientSocket)));
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
        int clientAddrSize = sizeof(clientAddr);
        
        SOCKET clientSocket = accept(m_serverSocket, 
                                    reinterpret_cast<sockaddr*>(&clientAddr), 
                                    &clientAddrSize);

        if (clientSocket == INVALID_SOCKET)
        {
            if (m_shouldStop)
                break;
                
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
void ChromeDevToolsServer::ClientHandlerThread(SOCKET clientSocket)
{
    OnClientConnected(clientSocket);
    
    std::string receivedData;
    bool isWebSocket = false;
    
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
                
                DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                          StringFormat("Received HTTP request:\n{}", httpRequest));
                
                // Check if this is a WebSocket upgrade request (case insensitive)
                std::string httpLowerCase = httpRequest;
                std::transform(httpLowerCase.begin(), httpLowerCase.end(), httpLowerCase.begin(), ::tolower);
                if (httpLowerCase.find("upgrade: websocket") != std::string::npos)
                {
                    DAEMON_LOG(LogScript, eLogVerbosity::Display, "Detected WebSocket upgrade request");
                    if (ProcessWebSocketUpgrade(clientSocket, httpRequest))
                    {
                        DAEMON_LOG(LogScript, eLogVerbosity::Display, "WebSocket upgrade successful");
                        isWebSocket = true;
                        m_activeConnections.push_back(clientSocket);
                    }
                    else
                    {
                        DAEMON_LOG(LogScript, eLogVerbosity::Error, "WebSocket upgrade failed");
                        break; // Upgrade failed
                    }
                }
                else
                {
                    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                              StringFormat("Not a WebSocket upgrade request - processing as HTTP"));
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
void ChromeDevToolsServer::OnClientConnected(SOCKET clientSocket)
{
    sWebSocketConnection connection;
    connection.socket = clientSocket;
    connection.isUpgraded = false;
    connection.isActive = false;
    
    m_connections[clientSocket] = connection;
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::OnClientDisconnected(SOCKET clientSocket)
{
    // Remove from active connections
    auto it = std::find(m_activeConnections.begin(), m_activeConnections.end(), clientSocket);
    if (it != m_activeConnections.end())
    {
        m_activeConnections.erase(it);
    }
    
    // Remove connection
    m_connections.erase(clientSocket);
    
    DAEMON_LOG(LogScript, eLogVerbosity::Log, 
              StringFormat("Chrome DevTools client {} disconnected", static_cast<int>(clientSocket)));
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::ProcessHttpRequest(SOCKET clientSocket, const std::string& request)
{
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
              StringFormat("Processing HTTP request from client {}", static_cast<int>(clientSocket)));
    
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
              StringFormat("HTTP Request Content:\n{}", request));

    std::string response;
    
    // Parse request line
    std::istringstream requestStream(request);
    std::string method, path, version;
    requestStream >> method >> path >> version;
    
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
              StringFormat("Parsed HTTP: method='{}' path='{}' version='{}'", method, path, version));

    if (method == "GET" && (path == "/json/list" || path == "/json"))
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, "Handling discovery request");
        response = HandleDiscoveryRequest();
    }
    else
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                  StringFormat("Unknown request: {} {} - returning 404", method, path));
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
    DAEMON_LOG(LogScript, eLogVerbosity::Log, 
              StringFormat("Processing WebSocket upgrade for client {}", static_cast<int>(clientSocket)));

    // Extract WebSocket key from request
    std::string wsKey;
    std::istringstream requestStream(request);
    std::string line;
    
    while (std::getline(requestStream, line) && line != "\r")
    {
        if (line.find("Sec-WebSocket-Key:") == 0)
        {
            wsKey = line.substr(19); // Skip "Sec-WebSocket-Key: "
            // Remove carriage return
            if (!wsKey.empty() && wsKey.back() == '\r')
                wsKey.pop_back();
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
                  StringFormat("Chrome DevTools WebSocket connection established for client {}", 
                              static_cast<int>(clientSocket)));
    }
    
    return success;
}

//----------------------------------------------------------------------------------------------------
void ChromeDevToolsServer::ProcessWebSocketMessage(SOCKET clientSocket, const std::string& data)
{
    DAEMON_LOG(LogScript, eLogVerbosity::Display, 
              StringFormat("ProcessWebSocketMessage called with {} bytes of data", data.length()));
    
    if (data.empty())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning, "ProcessWebSocketMessage: Empty data received");
        return;
    }

    try
    {
        std::string decodedMessage = DecodeWebSocketFrame(data);
        
        DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                  StringFormat("Decoded WebSocket message: '{}'", decodedMessage));
        
        if (!decodedMessage.empty())
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                      StringFormat("Chrome DevTools Protocol message: {}", decodedMessage));
            
            // Forward to V8 Inspector
            if (m_inspectorSession)
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                          StringFormat("Forwarding message to V8 Inspector session"));
                
                // Convert std::string to v8_inspector::StringView
                v8_inspector::StringView messageView(
                    reinterpret_cast<const uint8_t*>(decodedMessage.c_str()), 
                    decodedMessage.length()
                );
                
                // Dispatch message to V8 Inspector
                m_inspectorSession->dispatchProtocolMessage(messageView);
                
                DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                          StringFormat("Message dispatched to V8 Inspector successfully"));
            }
            else
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Error, 
                          StringFormat("Cannot forward message: V8 Inspector session is null"));
            }
        }
        else
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Warning, 
                      StringFormat("WebSocket frame decoding returned empty message"));
        }
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, 
                  StringFormat("Error processing WebSocket message: {}", e.what()));
    }
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
    if (frame.length() < 2)
        return "";

    uint8_t firstByte = static_cast<uint8_t>(frame[0]);
    uint8_t secondByte = static_cast<uint8_t>(frame[1]);
    
    bool isFinal = (firstByte & 0x80) != 0;
    eWebSocketOpcode opcode = static_cast<eWebSocketOpcode>(firstByte & 0x0F);
    bool isMasked = (secondByte & 0x80) != 0;
    uint64_t payloadLength = secondByte & 0x7F;
    
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
    if (frame.length() < headerLength + payloadLength)
        return "";
    
    std::string payload = frame.substr(headerLength, payloadLength);
    
    // Unmask payload if necessary
    if (isMasked)
    {
        for (size_t i = 0; i < payload.length(); ++i)
        {
            payload[i] ^= maskingKey[i % 4];
        }
    }
    
    return payload;
}

//----------------------------------------------------------------------------------------------------
std::string ChromeDevToolsServer::EncodeWebSocketFrame(const std::string& payload, 
                                                      eWebSocketOpcode opcode)
{
    std::string frame;
    
    // First byte: FIN=1, RSV=000, Opcode
    frame.push_back(0x80 | static_cast<uint8_t>(opcode));
    
    // Second byte and payload length
    size_t payloadLength = payload.length();
    if (payloadLength < 126)
    {
        frame.push_back(static_cast<uint8_t>(payloadLength));
    }
    else if (payloadLength <= 0xFFFF)
    {
        frame.push_back(126);
        frame.push_back((payloadLength >> 8) & 0xFF);
        frame.push_back(payloadLength & 0xFF);
    }
    else
    {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i)
        {
            frame.push_back((payloadLength >> (i * 8)) & 0xFF);
        }
    }
    
    // Payload
    frame.append(payload);
    
    return frame;
}

//----------------------------------------------------------------------------------------------------
bool ChromeDevToolsServer::SendRawDataToSocket(SOCKET socket, const std::string& data)
{
    if (socket == INVALID_SOCKET || data.empty())
        return false;
    
    const char* buffer = data.c_str();
    int totalSent = 0;
    int dataSize = static_cast<int>(data.length());
    
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
    if (socket == INVALID_SOCKET)
        return "";
    
    char buffer[4096];
    int received = recv(socket, buffer, sizeof(buffer) - 1, 0);
    
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
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const char* chars = "0123456789abcdef";
    std::string uuid = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    
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
    int val = 0;
    int valb = -6;
    
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