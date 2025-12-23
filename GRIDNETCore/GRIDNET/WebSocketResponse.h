#pragma once
#include <stdafx.h>
#include <vector>
#include "enums.h"
class CWebSocketResponse
{
	//Binary Format [webSocketStatus:1Byte,serviceStatus:1Byte, data:rest]
public:
	std::vector<uint8_t> getPackedData();
	bool setData(std::vector<uint8_t> data);
	CWebSocketResponse();
	void setServerResponse(eWebSocketProcessingResult::eWebSocketProcessingResult sResult);
	void setServiceStatus(uint8_t status);
	CWebSocketResponse(eWebSocketProcessingResult::eWebSocketProcessingResult sResult, uint8_t sServiceResult, std::vector<uint8_t> data= std::vector<uint8_t>());
private:
	uint8_t mServerStatus;
	uint8_t mServiceStatus;
	std::vector<uint8_t> mData;
};