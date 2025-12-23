#pragma once
#include "stdafx.h"
#include <iostream>
#include <winsock2.h>

class CRouteOnion {
private:
	std::vector<uint8_t>  layers;
	SOCKADDR_IN nextHop;
public:
	CRouteOnion(std::vector<uint8_t> *onionBytes);
	int getEsitmatedNrOfLayer();
	


};
