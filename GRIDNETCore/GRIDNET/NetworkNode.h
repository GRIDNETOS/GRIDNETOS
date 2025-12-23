#pragma once
#include <vector>
#include "EEndPoint.h"
#include "IdentityToken.h"
class CNetworkNode
{
public:
	CNetworkNode(CEndPoint ep,std::vector<uint8_t> id,CIdentityToken idToken);
	CIdentityToken getIDToken();
	uint64_t getRank();
private:

	CEndPoint mInterface;
	size_t  mLastTimeSeen;
	CIdentityToken mToken;
	std::vector<uint8_t> ID;//256bit ID
};
struct CompareNetworkNode {
	bool operator()(CNetworkNode *lhs, CNetworkNode *rhs) const
	{
		return lhs->getRank() < rhs->getRank();
	}
};