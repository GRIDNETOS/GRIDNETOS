#include "NetworkNode.h"

CNetworkNode::CNetworkNode(CEndPoint ep, std::vector<uint8_t> id, CIdentityToken idToken )
{
}

uint64_t CNetworkNode::getRank()
{
	return mToken.getRank();
}
