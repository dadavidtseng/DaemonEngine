[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Network**

# Network Module Documentation

## Module Responsibilities

The Network module provides comprehensive networking capabilities including TCP/UDP networking foundation, WebSocket support for modern protocols, Chrome DevTools Protocol integration for debugging, and Model Context Protocol (MCP) support for AI integration. It handles client-server architecture, message queuing, heartbeat systems, connection management, and event-driven communication for multiplayer game development and networked applications.

## Entry and Startup

### Primary Entry Points
- `NetworkTCPSubsystem.hpp` - TCP networking system and connection management
- `BaseWebSocketSubsystem.hpp` - WebSocket protocol base implementation
- `ChromeDevToolsWebSocketSubsystem.hpp` - Chrome DevTools Protocol WebSocket client
- `MCPWebSocketSubsystem.hpp` - Model Context Protocol WebSocket support
- `NetworkCommon.hpp` - Shared networking constants, enums, and data structures

### Initialization Pattern
```cpp
// TCP Server setup
sNetworkTCPSubsystemConfig serverConfig;
serverConfig.m_mode = eNetworkMode::SERVER;
serverConfig.hostAddressString = "127.0.0.1:7777";
serverConfig.maxClients = 8;
serverConfig.enableHeartbeat = true;
serverConfig.heartbeatInterval = 2.0f;

NetworkTCPSubsystem* tcpNetwork = new NetworkTCPSubsystem(serverConfig);
tcpNetwork->StartUp();
tcpNetwork->StartServer(7777);

// WebSocket client setup (Chrome DevTools)
ChromeDevToolsWebSocketSubsystem* cdpClient = new ChromeDevToolsWebSocketSubsystem();
cdpClient->Connect("ws://localhost:9222/devtools/browser");

// WebSocket client setup (MCP)
MCPWebSocketSubsystem* mcpClient = new MCPWebSocketSubsystem();
mcpClient->Connect("ws://localhost:3000/mcp");

// Message handling loop
while (tcpNetwork->HasPendingMessages()) {
    sNetworkMessage message = tcpNetwork->GetNextMessage();
    ProcessGameMessage(message);
}
```

## External Interfaces

### Core Network API
```cpp
// TCP Networking API
class NetworkTCPSubsystem {
    // System lifecycle
    void StartUp();
    void Update();
    void BeginFrame();
    void EndFrame();
    void ShutDown();

    // Connection status
    bool IsEnabled() const;
    bool IsServer() const;
    bool IsClient() const;
    bool IsConnected() const;
    eNetworkMode GetNetworkMode() const;
    eConnectionState GetConnectionState() const;

    // Server operations
    bool StartServer(int newPort = -1);
    void StopServer();
    int GetConnectedClientCount() const;
    std::vector<int> GetConnectedClientIds() const;
    bool SendMessageToClient(int clientId, sNetworkMessage const& message);
    bool SendMessageToAllClients(sNetworkMessage const& message);

    // Client operations
    bool ConnectToServer(String const& address, int port);
    void DisconnectFromServer();
    bool SendMessageToServer(sNetworkMessage const& message);
};

// WebSocket API
class BaseWebSocketSubsystem {
    // Connection management
    bool Connect(String const& url);
    void Disconnect();
    bool IsConnected() const;

    // Message sending
    bool SendTextMessage(String const& message);
    bool SendBinaryMessage(std::vector<uint8_t> const& data);

    // Message handling (override in derived classes)
    virtual void OnMessage(String const& message);
    virtual void OnBinaryMessage(std::vector<uint8_t> const& data);
    virtual void OnConnectionEstablished();
    virtual void OnConnectionClosed();
};

// Chrome DevTools Protocol API
class ChromeDevToolsWebSocketSubsystem : public BaseWebSocketSubsystem {
    // CDP command execution
    void SendCommand(String const& method, String const& params = "{}");
    void EnableDomain(String const& domain);

    // Event handling
    virtual void OnCDPEvent(String const& method, String const& params);
};

// Model Context Protocol API
class MCPWebSocketSubsystem : public BaseWebSocketSubsystem {
    // MCP-specific operations
    void SendMCPRequest(String const& request);

    // MCP event handling
    virtual void OnMCPResponse(String const& response);
};

// KADI Broker Integration API (Assignment 7 Integration)
class KADIWebSocketSubsystem {
    // Lifecycle
    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    // Connection Management
    void Connect(std::string const& brokerUrl, std::string const& publicKey, std::string const& privateKey);
    void Disconnect();
    bool IsConnected() const;
    eKADIConnectionState GetConnectionState() const;

    // Tool Management (MCP Tool Registration)
    void RegisterTools(nlohmann::json const& tools);
    void SendToolResult(int requestId, nlohmann::json const& result);
    void SendToolError(int requestId, std::string const& errorMessage);

    // Event System
    void SubscribeToEvents(std::vector<std::string> const& channels);
    void PublishEvent(std::string const& channel, nlohmann::json const& data);

    // Callback Registration
    void SetToolInvokeCallback(KADIToolInvokeCallback callback);
    void SetEventDeliveryCallback(KADIEventDeliveryCallback callback);
    void SetConnectionStateCallback(KADIConnectionStateCallback callback);

    // Message Sending
    void QueueMessage(std::string const& message);
};

// KADI Connection States
enum class eKADIConnectionState {
    DISCONNECTED,      // Not connected to broker
    CONNECTING,        // Connection in progress
    CONNECTED,         // WebSocket connection established, not authenticated
    AUTHENTICATING,    // Sending hello and authentication
    AUTHENTICATED,     // Authentication complete
    REGISTERING_TOOLS, // Registering capabilities
    READY              // Fully connected and operational
};

// KADI Callbacks
using KADIToolInvokeCallback = std::function<void(int requestId, std::string const& toolName, nlohmann::json const& arguments)>;
using KADIEventDeliveryCallback = std::function<void(std::string const& channel, nlohmann::json const& data)>;
using KADIConnectionStateCallback = std::function<void(eKADIConnectionState oldState, eKADIConnectionState newState)>;
```

### Message System
```cpp
// High-level message API
void SendRawData(String const& data);
void SendGameData(String const& gameData, int targetClientId = -1);
void SendChatMessage(String const& message, int targetClientId = -1);

// Message retrieval
bool HasPendingMessages() const;
sNetworkMessage GetNextMessage();
void ClearMessageQueue();
```

### Connection Management
```cpp
// Address and port configuration
String GetCurrentIP() const;
unsigned short GetCurrentPort() const;
String GetHostAddressString() const;
void SetCurrentIP(String const& newIP);
void SetCurrentPort(unsigned short newPort);
void SetHostAddressString(String const& newHostAddress);
```

## Key Dependencies and Configuration

### External Dependencies
- **Winsock2**: Windows socket programming (`ws2_32.lib`)
- **Windows Networking**: Socket creation and management
- **Standard Threading**: For non-blocking network operations

### Internal Dependencies
- Core module for string utilities, timing, and basic data structures
- Event system for network event broadcasting
- Clock system for heartbeat and timeout management

### Configuration Structure
```cpp
struct sNetworkSubsystemConfig {
    eNetworkMode m_mode;                    // NONE, CLIENT, SERVER
    String       hostAddressString;         // IP:Port format
    int          sendBufferSize;            // TCP send buffer size
    int          recvBufferSize;            // TCP receive buffer size
    int          maxClients;                // Server: maximum connections
    bool         enableHeartbeat;           // Heartbeat system enable
    float        heartbeatInterval;         // Heartbeat interval (seconds)
    bool         enableConsoleOutput;       // Debug logging enable
};
```

### Network Modes and States
```cpp
enum class eNetworkMode : int8_t {
    NONE,       // No networking
    CLIENT,     // Client connection mode
    SERVER      // Server hosting mode
};

enum class eConnectionState : int8_t {
    DISCONNECTED,   // Not connected
    CONNECTING,     // Connection in progress
    CONNECTED,      // Successfully connected
    DISCONNECTING   // Disconnection in progress
};
```

## Data Models

### Network Message Structure
```cpp
struct sNetworkMessage {
    eMessageType  messageType;      // Message classification
    String        messageData;      // Payload content
    int           fromClientId;     // Source client ID (-1 for server)
    int           targetClientId;   // Target client ID (-1 for broadcast)
    float         timestamp;        // Message timestamp
    size_t        messageSize;      // Data size in bytes
};

enum class eMessageType : int8_t {
    SYSTEM,         // System/connection messages
    GAME_DATA,      // Game state data
    CHAT,           // Chat messages
    HEARTBEAT,      // Keep-alive messages
    CUSTOM          // User-defined messages
};
```

### Client Connection Management
```cpp
struct sClientConnection {
    int           clientId;         // Unique client identifier
    uintptr_t     socket;          // Client socket handle
    String        ipAddress;        // Client IP address
    unsigned short port;            // Client port number
    eConnectionState connectionState; // Current connection status
    float         lastHeartbeat;    // Last heartbeat timestamp
    String        clientName;       // Optional client identifier
};

// Server-side client tracking
std::vector<sClientConnection> m_clientList;
int m_nextClientId = 1;
```

### Socket and Buffer Management
```cpp
// Socket handles (platform-abstracted)
uintptr_t m_clientSocket = ~0ull;    // Client connection socket
uintptr_t m_listenSocket = ~0ull;    // Server listening socket

// Network buffers
char* m_sendBuffer = nullptr;        // Outgoing data buffer
char* m_recvBuffer = nullptr;        // Incoming data buffer

// Message queues
std::deque<String> m_sendQueue;              // Outgoing messages
String m_recvQueue;                          // Incoming data buffer
std::deque<sNetworkMessage> m_incomingMessages; // Processed messages
```

### Heartbeat and Statistics
```cpp
// Heartbeat system
float m_heartbeatTimer = 0.f;
float m_lastHeartbeatReceived = 0.f;

// Network statistics
int m_messagesSent = 0;
int m_messagesReceived = 0;
int m_connectionsAccepted = 0;
int m_connectionsLost = 0;
```

## Testing and Quality

### Built-in Quality Features
- **Connection State Tracking**: Comprehensive connection lifecycle management
- **Heartbeat System**: Automatic connection health monitoring
- **Message Serialization**: Robust message formatting and parsing
- **Error Recovery**: Graceful handling of network failures and disconnections

### Current Testing Approach
- Manual multiplayer testing with multiple client instances
- Connection stability testing with simulated network interruptions
- Message delivery verification through console output
- Performance monitoring of message throughput

### Quality Assurance Features
- Automatic socket cleanup and resource management
- Thread-safe message queue operations
- Comprehensive error logging and reporting
- Network statistics for performance monitoring

### Recommended Testing Additions
- Automated network unit tests with mock sockets
- Stress testing with high client counts and message volumes
- Latency and bandwidth optimization testing
- Cross-platform networking compatibility validation

## FAQ

### Q: How do I handle different types of game messages?
A: Use the `eMessageType` enum to categorize messages, then implement custom message handlers based on the type field in received messages.

### Q: What's the difference between SendGameData() and SendRawData()?
A: `SendGameData()` wraps data in structured messages with type information, while `SendRawData()` sends unformatted strings directly.

### Q: How does the heartbeat system prevent timeouts?
A: Automatic heartbeat messages are sent at regular intervals to maintain connection health, with timeout detection for unresponsive clients.

### Q: Can I run both client and server in the same application?
A: The current architecture supports one mode per NetworkSubsystem instance, but you can create multiple instances for hybrid scenarios.

### Q: How do I handle client disconnections gracefully?
A: The system automatically detects disconnections and cleans up client resources, firing appropriate events for application-level handling.

### Q: Is the networking system thread-safe?
A: Message queues are thread-safe, but network operations should primarily occur on the main thread for consistency and reliability.

## Related Files

### Core Implementation
- `NetworkTCPSubsystem.cpp` - TCP networking implementation with socket management
- `BaseWebSocketSubsystem.cpp` - WebSocket protocol base implementation
- `ChromeDevToolsWebSocketSubsystem.cpp` - Chrome DevTools Protocol integration
- `MCPWebSocketSubsystem.cpp` - Model Context Protocol support
- `NetworkCommon.cpp` - Shared utilities, message serialization, and constants

### Message Processing
The Network module includes comprehensive message handling:
- **Message Serialization**: Automatic conversion between structured messages and network data
- **Queue Management**: Thread-safe message queuing for reliable delivery
- **Connection Lifecycle**: Complete connection state management from establishment to cleanup

### Integration Points
- **Event System**: Network events for connection changes and message reception
- **Core Module**: String utilities, timing systems, and logging integration
- **Game Logic**: High-level game state synchronization and multiplayer coordination

### Protocol Support
- **TCP**: Reliable connection-oriented communication for critical game data
- **WebSocket**: Modern protocol for real-time bidirectional communication
- **Chrome DevTools Protocol (CDP)**: Browser debugging and automation support
- **Model Context Protocol (MCP)**: AI integration and context management
- **KADI Broker Protocol**: Game-to-AI integration with tool registration, authentication, and event delivery
  - **Assignment 7 Integration**: Primary system for AI agent control via Claude Desktop/Code
  - **Tool Registration**: Expose game functions as MCP tools (spawn agent, mine block, etc.)
  - **Authentication**: Public/private key authentication with KADI broker
  - **Event System**: Subscribe/publish pattern for game state updates
  - **Heartbeat**: Automatic connection health monitoring with ping/pong
- **Connection Management**: Automatic client tracking and connection health monitoring
- **Message Framing**: Proper message boundaries and data integrity

### Planned Extensions
- UDP support for low-latency, unreliable messaging (physics, audio)
- Advanced message compression and encryption
- NAT traversal and punch-through for peer-to-peer connections
- Bandwidth management and quality-of-service features
- Cross-platform socket abstraction (Linux, macOS)
- Advanced security features and anti-cheat integration

## Changelog

- 2025-11-24: **Network Module Documentation Updated for Assignment 7**
  - Added comprehensive `KADIWebSocketSubsystem` documentation for KADI broker integration
  - Documented KADI connection states (DISCONNECTED → READY 7-state flow)
  - Documented MCP tool registration API for game function exposure
  - Documented authentication flow with public/private key pairs
  - Documented event subscribe/publish system for game state updates
  - Added Assignment 7 integration notes (AI agent control, Claude Desktop/Code)
  - Updated protocol support section with KADI broker details
- 2025-10-01: **Major Network Module Refactoring** - Modular WebSocket Architecture
  - Removed monolithic `NetworkWebsocketSubsystem` (1,274 lines deleted)
  - Created modular WebSocket architecture with base class and specialized implementations
  - Added `BaseWebSocketSubsystem` - Abstract base for WebSocket connections
  - Added `ChromeDevToolsWebSocketSubsystem` - Chrome DevTools Protocol support
  - Added `MCPWebSocketSubsystem` - Model Context Protocol integration for AI systems
  - Added `KADIWebSocketSubsystem` - KADI broker protocol for game-to-AI integration
  - Renamed `NetworkSubsystem` → `NetworkTCPSubsystem` for clarity (TCP-specific implementation)
  - Improved separation of concerns between TCP and WebSocket protocols
  - Updated Visual Studio project files with new modular structure
- 2025-09-06 21:17:11: Initial Network module documentation created
- Recent developments: Comprehensive client-server architecture, heartbeat system, message queue management, connection state tracking