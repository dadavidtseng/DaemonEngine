//----------------------------------------------------------------------------------------------------
// NetworkCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
enum class eNetworkMode : uint8_t
{
    NONE = 0,
    CLIENT,
    SERVER
};

//----------------------------------------------------------------------------------------------------
enum class eConnectionState : uint8_t
{
    DISCONNECTED = 0,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    ERROR_STATE,
    DISABLED
};

//----------------------------------------------------------------------------------------------------
// Client connection info for server mode
struct ClientConnection
{
    uintptr_t        socket   = ~0ull;
    int              clientId = -1;
    eConnectionState state    = eConnectionState::DISCONNECTED;
    String           address;
    unsigned short   port              = 0;
    float            lastHeartbeatTime = 0.f;
    String           recvQueue;

    ClientConnection() = default;
};

//----------------------------------------------------------------------------------------------------
// Network message structure
struct NetworkMessage
{
    std::string messageType;
    std::string data;
    int         fromClientId = -1;  // -1 for server messages, client ID for client messages

    NetworkMessage() = default;
    NetworkMessage(String const& type, String const& msgData, int clientId = -1);
};
