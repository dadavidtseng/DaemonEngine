//----------------------------------------------------------------------------------------------------
// NetworkCommon.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Network/NetworkCommon.hpp"

sNetworkMessage::sNetworkMessage(String const& type,
                               String const& msgData,
                               int           clientId)
    : m_messageType(type),
      m_data(msgData),
      m_fromClientId(clientId)
{
}
