
#include "EEndpoint.h"
#include <assert.h>
#include "CGlobalSecSettings.h"
#include "BlockchainManager.h"
#include "NetworkManager.h"
#include "NetworkDevice.h"
#include "conversation.h"
#include "NetMsg.h"

void CEndPoint::initFields()
{
	mEnabled = true;
	mLastTimeSeen = 0;
	mPortNumber = 0;
	mType = eEndpointType::IPv4;
	mNetworkDevice = nullptr;
}

CEndPoint::CEndPoint(std::vector<uint8_t> address, eEndpointType::eEndpointType type, CNetworkDevice * device,uint32_t portNumber)
{
	initFields();
	switch (type)
	{
	case eEndpointType::IPv4:
		if (portNumber == 0)
		{
			portNumber = CGlobalSecSettings::getDefaultPortNrDataExchange(eBlockchainMode::TestNet, true);
		 }
		mPortNumber = portNumber;
		break;
	case eEndpointType::IPv6:
		if (portNumber == 0)
		{
			portNumber = CGlobalSecSettings::getDefaultPortNrDataExchange(eBlockchainMode::TestNet, true);
		}
		mPortNumber = portNumber;
		break;
	case eEndpointType::MAC:
	 assertGN(portNumber == 0);
		break;

	}
	mType = type;
	mAddress = address;
	mLastTimeSeen =  std::time(0);
	
}

CEndPoint::CEndPoint(const CEndPoint & sybling)
{
	mLastTimeSeen = sybling.mLastTimeSeen;
	mPendingTasks = sybling.mPendingTasks;
	////conversation is NOT moved
	mEnabled = sybling.mEnabled;
	mAddress = sybling.mAddress;
	mPortNumber = sybling.mPortNumber;
	mType = sybling.mType;
	mNetworkDevice = sybling.mNetworkDevice;
}


CEndPoint::CEndPoint()
{
	mEnabled = true;
}


CEndPoint::~CEndPoint()
{
	//if (mConversation != nullptr)
	//	mConversation->end();
}

void CEndPoint::addTask(CNetMsg msg)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mPendingTasks.push_back(msg);
}

uint32_t CEndPoint::getPort() const
{
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mGuardian));
	return mPortNumber;
}

bool CEndPoint::endConversation(bool blockTillFinished)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return false;
}

size_t CEndPoint::getLastTimeSeen()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLastTimeSeen;
}

void CEndPoint::ping()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLastTimeSeen =  std::time(0);
}

std::vector<uint8_t> CEndPoint::getPubKey()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mPubKey;
}

void CEndPoint::setPubKey(std::vector<uint8_t>& pubKey)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mPubKey = pubKey;
}

eEndpointType::eEndpointType CEndPoint::getType()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mType;
}

std::vector<uint8_t> CEndPoint::getAddress() const
{
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mGuardian));
	return mAddress;
}

/// <summary>
/// Returns a brief description of the CNetMsg container.
/// </summary>
/// <param name="includeAddresses"></param>
/// <param name="includeEndpointTypes"></param>
/// <returns></returns>
std::string CEndPoint::getDescription(bool includeAddresses, bool includeEndpointTypes) const
{
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mGuardian));
	std::string toRet;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	bool base58ID = true;

	if (includeAddresses || includeEndpointTypes)
	{
		if (mType == eEndpointType::IPv4 || mType == eEndpointType::MAC || mType == eEndpointType::PeerID)
			base58ID = false;

		toRet += (includeEndpointTypes ? tools->endpointTypeToString(mType) :"")
			+ "(" + std::string(base58ID ? tools->base58CheckEncode(mAddress) : tools->bytesToString(mAddress)) + ")";

		if (toRet.size() > 0 && mPortNumber > 0)
		{
			toRet += " ";
			toRet += "Port:" + std::to_string(mPortNumber);
		}
	}
	return toRet;
}

/// <summary>
/// Compares two endpoints.
/// Note: the STATE variables are NOT compared.
/// </summary>
/// <param name="e1"></param>
/// <param name="e2"></param>
/// <returns></returns>
bool operator==(const CEndPoint& e1, const CEndPoint& e2)
{
	std::shared_ptr<CTools> tools = CTools::getInstance();
	return (tools->compareByteVectors(e1.mAddress,e2.mAddress)
		&& e1.mPortNumber == e2.mPortNumber
		&& e1.mType == e2.mType
		);
}

bool operator!=(const CEndPoint& e1, const CEndPoint& e2)
{
	return !(e1 == e2);
}
