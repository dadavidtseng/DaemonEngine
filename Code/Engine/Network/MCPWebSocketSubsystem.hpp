//----------------------------------------------------------------------------------------------------
// MCPWebSocketSubsystem.hpp
// Model Context Protocol WebSocket Subsystem (Stub Implementation)
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Network/BaseWebSocketSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
// MCP Server Configuration (extends base WebSocket config)
//----------------------------------------------------------------------------------------------------
struct sMCPConfig : sBaseWebSocketConfig
{
    String serverName = "ProtogameJS3D MCP Server";
    String version    = "1.0.0";
};

//----------------------------------------------------------------------------------------------------
// MCP WebSocket Subsystem (Stub Implementation)
// Provides Model Context Protocol server for AI agent integration
// Inherits WebSocket protocol implementation from BaseWebSocketSubsystem
//
// @remark This is a stub implementation demonstrating the architecture
//         Full MCP protocol implementation to be added in future updates
//----------------------------------------------------------------------------------------------------
class MCPWebSocketSubsystem : public BaseWebSocketSubsystem
{
public:
    explicit MCPWebSocketSubsystem(sMCPConfig config);
    ~MCPWebSocketSubsystem() override;

    // MCP-specific API
    void SendMCPMessage(String const& message);

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

    /// @brief Generate HTTP discovery endpoint response
    String HandleDiscoveryRequest() override;

    /// @brief Process queued MCP messages on main thread
    void ProcessQueuedMessages() override;

private:
    // MCP Protocol Handling (Stub)
    bool HandleMCPCommand(String const& message);

    sMCPConfig m_config;
    // Thread-safe message queue for MCP communication
    std::queue<String> m_mcpMessageQueue;
    std::mutex         m_messageQueueMutex;
};
