[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Network**

# Network Module Documentation

## Module Responsibilities

The Network module provides a comprehensive TCP/UDP networking foundation with client-server architecture, message queuing, heartbeat systems, connection management, and event-driven communication for multiplayer game development and networked applications.

## Entry and Startup

### Primary Entry Points
- `NetworkSubsystem.hpp` - Main networking system and connection management
- `NetworkCommon.hpp` - Shared networking constants, enums, and data structures

### Initialization Pattern
```cpp
// Server setup
sNetworkSubsystemConfig serverConfig;
serverConfig.m_mode = eNetworkMode::SERVER;
serverConfig.hostAddressString = "127.0.0.1:7777";
serverConfig.maxClients = 8;
serverConfig.enableHeartbeat = true;
serverConfig.heartbeatInterval = 2.0f;

NetworkSubsystem* networkSystem = new NetworkSubsystem(serverConfig);
networkSystem->StartUp();
networkSystem->StartServer(7777);

// Client setup
sNetworkSubsystemConfig clientConfig;
clientConfig.m_mode = eNetworkMode::CLIENT;
clientConfig.hostAddressString = "127.0.0.1:7777";

NetworkSubsystem* clientNetwork = new NetworkSubsystem(clientConfig);
clientNetwork->StartUp();
clientNetwork->ConnectToServer("127.0.0.1", 7777);

// Message handling loop
while (networkSystem->HasPendingMessages()) {
    sNetworkMessage message = networkSystem->GetNextMessage();
    ProcessGameMessage(message);
}
```

## External Interfaces

### Core Network API
```cpp
class NetworkSubsystem {
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
- `NetworkSubsystem.cpp` - Main networking implementation with socket management
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

- 2025-09-06 21:17:11: Initial Network module documentation created
- Recent developments: Comprehensive client-server architecture, heartbeat system, message queue management, connection state tracking