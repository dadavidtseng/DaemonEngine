//----------------------------------------------------------------------------------------------------
// MCPWebSocketSubsystem.cpp
// Model Context Protocol WebSocket Subsystem (Stub Implementation)
//----------------------------------------------------------------------------------------------------

#include "Engine/Network/MCPWebSocketSubsystem.hpp"

#include <sstream>

#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
MCPWebSocketSubsystem::MCPWebSocketSubsystem(sMCPConfig config)
    : BaseWebSocketSubsystem(config)  // Call base class constructor with config
      , m_config(std::move(config))
{
    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("MCP WebSocket Subsystem created: %s v%s",
                   m_config.serverName.c_str(), m_config.version.c_str()));
}

//----------------------------------------------------------------------------------------------------
MCPWebSocketSubsystem::~MCPWebSocketSubsystem()
{
    Stop();  // Base class Stop() method
}

//----------------------------------------------------------------------------------------------------
// MCP-specific API
//----------------------------------------------------------------------------------------------------

void MCPWebSocketSubsystem::SendMCPMessage(String const& message)
{
    // Use base class BroadcastToAllClients method
    BroadcastToAllClients(message);
}

//----------------------------------------------------------------------------------------------------
// BaseWebSocketSubsystem Pure Virtual Implementations
//----------------------------------------------------------------------------------------------------

void MCPWebSocketSubsystem::OnClientConnected(SOCKET clientSocket)
{
    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("MCP client connected from socket %llu",
                   static_cast<unsigned long long>(clientSocket)));

    // Send welcome message (stub implementation)
    String welcomeMessage = R"({"jsonrpc":"2.0","method":"initialize","params":{"serverInfo":{"name":"ProtogameJS3D MCP Server","version":"1.0.0"}}})";
    SendToClient(clientSocket, welcomeMessage);
}

//----------------------------------------------------------------------------------------------------
void MCPWebSocketSubsystem::OnClientDisconnected(SOCKET clientSocket)
{
    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("MCP client disconnected: socket %llu",
                   static_cast<unsigned long long>(clientSocket)));
}

//----------------------------------------------------------------------------------------------------
void MCPWebSocketSubsystem::OnClientMessage(SOCKET clientSocket, String const& message)
{
    UNUSED(clientSocket);  // Parameter used only for logging in stub implementation

    DAEMON_LOG(LogNetwork, eLogVerbosity::Verbose,
               StringFormat("MCP message received: %s", message.c_str()));

    // Handle MCP protocol commands (stub implementation)
    if (HandleMCPCommand(message))
    {
        return; // Command handled
    }

    // Queue the message for main-thread processing
    {
        std::lock_guard<std::mutex> lock(m_messageQueueMutex);
        m_mcpMessageQueue.push(message);
    }
}

//----------------------------------------------------------------------------------------------------
String MCPWebSocketSubsystem::HandleDiscoveryRequest()
{
    // Generate MCP discovery response (stub implementation)
    std::ostringstream json;

    json << "{\n";
    json << "  \"name\": \"" << m_config.serverName << "\",\n";
    json << "  \"version\": \"" << m_config.version << "\",\n";
    json << "  \"protocol\": \"mcp\",\n";
    json << "  \"capabilities\": [\n";
    json << "    \"tools\",\n";
    json << "    \"resources\",\n";
    json << "    \"prompts\"\n";
    json << "  ],\n";
    json << "  \"endpoint\": \"ws://" << m_config.host << ":" << m_config.port << "/mcp\"\n";
    json << "}";

    return json.str();
}

//----------------------------------------------------------------------------------------------------
void MCPWebSocketSubsystem::ProcessQueuedMessages()
{
    std::lock_guard<std::mutex> lock(m_messageQueueMutex);

    while (!m_mcpMessageQueue.empty())
    {
        String message = m_mcpMessageQueue.front();
        m_mcpMessageQueue.pop();

        // Process MCP message (stub implementation - just log for now)
        DAEMON_LOG(LogNetwork, eLogVerbosity::Verbose,
                   StringFormat("Processing MCP message: %s", message.c_str()));

        // TODO: Implement full MCP protocol handling
        // - Parse JSON-RPC 2.0 messages
        // - Handle MCP tool calls, resource requests, prompt execution
        // - Send responses back to client
    }
}

//----------------------------------------------------------------------------------------------------
// MCP Protocol Handling (Stub)
//----------------------------------------------------------------------------------------------------

bool MCPWebSocketSubsystem::HandleMCPCommand(String const& message)
{
    // Stub implementation - recognize basic MCP commands

    if (message.find("\"method\":\"initialize\"") != String::npos)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
                   "MCP initialize command received");
        // Send initialization response (stub)
        return false; // Let queue processing handle it
    }

    if (message.find("\"method\":\"tools/list\"") != String::npos)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
                   "MCP tools/list command received");
        return false;
    }

    return false; // No custom handling needed
}
