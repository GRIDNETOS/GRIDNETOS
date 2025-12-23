#include "WebSocketResponse.h"
#include "DataConcatenator.h"


std::vector<uint8_t> CWebSocketResponse::getPackedData()
{
	Botan::DER_Encoder enc;
	enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
		.encode(static_cast<size_t>(mServerStatus))
		.encode(static_cast<size_t>(mServiceStatus))
		.start_cons(Botan::ASN1_Tag::SEQUENCE)
		.encode(mData, Botan::ASN1_Tag::OCTET_STRING)
		.end_cons().end_cons();
	return enc.get_contents_unlocked();
}

bool CWebSocketResponse::setData(std::vector<uint8_t> data)
{
	mData = data;
	return true;
}

CWebSocketResponse::CWebSocketResponse()
{
	mServerStatus = eWebSocketProcessingResult::OK;
	mServiceStatus = 0;

}

void CWebSocketResponse::setServerResponse(eWebSocketProcessingResult::eWebSocketProcessingResult sResult)
{
	mServerStatus = sResult;
}

void CWebSocketResponse::setServiceStatus(uint8_t status)
{
	mServiceStatus = status;
}

CWebSocketResponse::CWebSocketResponse(eWebSocketProcessingResult::eWebSocketProcessingResult sResult, uint8_t sServiceResult, std::vector<uint8_t> data)
{
	mServerStatus = sResult;
	mServiceStatus = sServiceResult;
	mData = data;

}
