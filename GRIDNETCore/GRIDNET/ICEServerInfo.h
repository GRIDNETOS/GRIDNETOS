#pragma once

class CICENode
{
private:
	std::mutex mFieldsGuardian;
	eICEServerType::eICEServerType mICEType;
	std::string mIP;
	std::string mCredentials;
	std::string mLogin;
	uint64_t mPort;
public:
	CICENode(std::string IP, eICEServerType::eICEServerType iceType, std::string login = "", std::string credentials="", uint64_t port = 0);
	eICEServerType::eICEServerType getType();
	std::string getIP();
	uint64_t getPort();
	std::string getCredentials();
	std::string getLogin();
};