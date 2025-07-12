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
struct sClientConnection
{
    uintptr_t        m_socket   = ~0ull;
    int              m_clientId = -1;
    eConnectionState m_state    = eConnectionState::DISCONNECTED;
    String           m_address;
    unsigned short   m_port              = 0;
    float            m_lastHeartbeatTime = 0.f;
    String           m_recvQueue;
};

//----------------------------------------------------------------------------------------------------
// Network message structure
struct sNetworkMessage
{
    String m_messageType;
    String m_data;
    int    m_fromClientId = -1;  // -1 for server messages, client ID for client messages

    sNetworkMessage() = default;
    sNetworkMessage(String const& type, String const& msgData, int clientId = -1);
};
