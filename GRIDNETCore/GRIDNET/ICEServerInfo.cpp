#include "stdafx.h"
#include "ICEServerInfo.h"

CICENode::CICENode(std::string IP, eICEServerType::eICEServerType iceType, std::string login, std::string credentials, uint64_t port)
{
	mIP = IP;
	mICEType = iceType;
	mLogin = login;
	mCredentials = credentials;
	mPort = port;

	if (mPort == 0)
	{
		switch (mICEType)
		{
		case eICEServerType::STUN:
			mPort = 19302;
			break;
		case eICEServerType::TURN:
			mPort = 3478;
			break;
		default:
			break;
		}
	}
}

eICEServerType::eICEServerType CICENode::getType()
{
	std::lock_guard<std::mutex> locK(mFieldsGuardian);
	return mICEType;
}

std::string CICENode::getIP()
{
	std::lock_guard<std::mutex> locK(mFieldsGuardian);
	return mIP;
}
uint64_t  CICENode::getPort()
{
	std::lock_guard<std::mutex> locK(mFieldsGuardian);
	return mPort;
}

std::string CICENode::getCredentials()
{
	std::lock_guard<std::mutex> locK(mFieldsGuardian);
	return mCredentials;
}
std::string CICENode::getLogin()
{
	std::lock_guard<std::mutex> locK(mFieldsGuardian);
	return mLogin;
}

