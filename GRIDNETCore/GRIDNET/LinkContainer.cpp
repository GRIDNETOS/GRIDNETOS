#include "LinkContainer.h"

CLinkContainer::CLinkContainer(std::vector<uint8_t> key, std::vector<uint8_t> value, eLinkType::eLinkType type)
{
 assertGN(key.size()>= 32 && key.size()<=35);
	mKey = key;
	mValue = value;
	mType = type;
}

std::vector<uint8_t> CLinkContainer::getValue()
{
	return mValue;
}

std::vector<uint8_t> CLinkContainer::getKey()
{
	return mKey;
}

eLinkType::eLinkType CLinkContainer::getType()
{
	return mType;
}

CLinkContainer & CLinkContainer::operator=(const CLinkContainer & t)
{
	mType = t.mType;
	mValue = t.mValue;
	mKey = t.mKey;
	return *this;
}

CLinkContainer::CLinkContainer(const CLinkContainer & sibling)
{
	mKey = sibling.mKey;
	mValue = sibling.mValue;
	mType = sibling.mType;
}

