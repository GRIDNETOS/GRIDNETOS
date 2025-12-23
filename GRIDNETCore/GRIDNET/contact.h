#pragma once
#include "NetworkNode.h"
class CContact 
{
private:
	std::string name;
	std::string surname;
	std::string email;
	std::vector<uint8_t> image;
	std::string other;
	CIdentityToken id;
public:
	CContact();

};