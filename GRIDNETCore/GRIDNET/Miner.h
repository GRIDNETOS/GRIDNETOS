#pragma once
#include "stdafx.h"
//#include <thread>
 class CMiner
{
	 std::thread mainWorkerThread;

public:
	
private:
	int nrOfConcats;
	int nonce;
	std::array<unsigned char, hashSize> hash;
 };