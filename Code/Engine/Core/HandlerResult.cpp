//----------------------------------------------------------------------------------------------------
// HandlerResult.cpp
// GenericCommand System - Handler Result Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/HandlerResult.hpp"

//----------------------------------------------------------------------------------------------------
HandlerResult HandlerResult::Success(std::unordered_map<String, std::any> resultData)
{
	HandlerResult result;
	result.data = std::move(resultData);
	return result;
}

//----------------------------------------------------------------------------------------------------
HandlerResult HandlerResult::Error(String const& message)
{
	HandlerResult result;
	result.error = message;
	return result;
}
