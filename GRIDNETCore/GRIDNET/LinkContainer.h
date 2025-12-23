#pragma once
#include "stdafx.h"
#include "enums.h"
class CLinkContainer
{
private:
	std::vector<uint8_t> mKey;
	std::vector<uint8_t> mValue;
	eLinkType::eLinkType mType;
public:
	CLinkContainer(std::vector<uint8_t> key, std::vector<uint8_t> value, eLinkType::eLinkType type);
	std::vector<uint8_t> getValue();
	std::vector<uint8_t> getKey();
	eLinkType::eLinkType getType();
	CLinkContainer & CLinkContainer::operator=(const CLinkContainer & t);
	CLinkContainer(const CLinkContainer &sibling);
};