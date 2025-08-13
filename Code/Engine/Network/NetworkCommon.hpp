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
// i.	NET_STATE_INACTIVE: The NetworkSystem has not been Started up.
// ii.	NET_STATE_IDLE: WSAStartup has been called, but we are neither listening nor connecting.
// iii.	NET_STATE_SERVER_LISTENING: A non-blocking listen socket has been created using socket() and configured to be non-blocking with ioctlsocket(), bound to a port using bind(), and listen() called on it; incoming connections to that port have accept() called on them, creating a new socket for communication with the newly-connected client.
// iv.	NET_STATE_CLIENT_CONNECTING: A non-blocking socket has been created using socket() and configured to be non-blocking with ioctlsocket(), then connect() is called initiating attempts to connect to a remote (or local) server IP address and Port number.
// v.	NET_STATE_CLIENT_CONNECTED: The above socket has been tested for writeability using select() and is confirmed connected to the server.
enum class eConnectionState : uint8_t
{
    DISCONNECTED = 0,
    CONNECTING,
    CONNECTED,
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
