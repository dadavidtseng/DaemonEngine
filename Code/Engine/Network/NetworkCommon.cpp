//----------------------------------------------------------------------------------------------------
// NetworkCommon.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Network/NetworkCommon.hpp"

NetworkMessage::NetworkMessage(String const& type, String const& msgData, int clientId): messageType(type), data(msgData), fromClientId(clientId)
{
}
