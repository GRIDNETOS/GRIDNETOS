#include "RTEntry.h"
#include "EEndPoint.h"
uint64_t CRTEntry::sIDGenerator = 0;
std::mutex CRTEntry::sIDGeneratorGuardian;

void CRTEntry::initFields()
{
	std::lock_guard<std::mutex> lock(sIDGeneratorGuardian);//lock more specialized mutex first
	std::lock_guard<std::mutex> lock2(mGuardian);

	eAddressType::eAddressType eType;
	mID = ++sIDGenerator;
	mHops = 0;
	mSrcSeq = 0;
	mDstSeq = 0;
	mTimestamp = CTools::getInstance()->getTime();
	mKnowledgeSource = eRouteKowledgeSource::propagation;
}


eRouteKowledgeSource::eRouteKowledgeSource CRTEntry::getKnowledgeSource()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mKnowledgeSource;
}

void CRTEntry::setKnowledgeSource(eRouteKowledgeSource::eRouteKowledgeSource source)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mKnowledgeSource = source;
}

std::string CRTEntry::getDescription()
{
	std::string toRet;
	toRet+= "[NextHop]:" + std::string(mNextHop != nullptr ? mNextHop->getDescription(): "none");
	toRet += " [Destination]:" + std::string(mDestination != nullptr ? mDestination->getDescription() : "none");
	toRet += " [Distance]:" + std::to_string(mHops);
	return toRet;
}

std::vector<uint8_t> CRTEntry::getDstPubKey()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDstPubKey;
}

void CRTEntry::setDstPubKey(std::vector<uint8_t>& pubKey)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	 mDstPubKey = pubKey;
}

uint64_t CRTEntry::getID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mID;
}

uint64_t CRTEntry::getHops()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mHops;
}

uint64_t CRTEntry::getSrcSeq()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mSrcSeq;
}

uint64_t CRTEntry::getDstSeq()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDstSeq;
}


void CRTEntry::ping()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mTimestamp = CTools::getInstance()->getTime();
}

void CRTEntry::setHops(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mHops = value;
}

void CRTEntry::setSrcSeq(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mSrcSeq = value;
}

void CRTEntry::setDstSeq(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDstSeq = value;
}

void CRTEntry::setDst(std::shared_ptr<CEndPoint> destination)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDestination = destination;
}

void CRTEntry::setSrc(std::shared_ptr<CEndPoint> source)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mNextHop = source;
}

std::shared_ptr<CEndPoint> CRTEntry::getNextHop()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mNextHop;
}

void CRTEntry::setNextHop(std::shared_ptr<CEndPoint> endpoint)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mNextHop = endpoint;
}

std::shared_ptr<CEndPoint> CRTEntry::getDst()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDestination;
}


uint64_t CRTEntry::getTimestamp()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mTimestamp;
}

CRTEntry::CRTEntry()
{
	this->initFields();
}

CRTEntry::CRTEntry(std::shared_ptr<CEndPoint> nextHop, std::shared_ptr<CEndPoint> destination, uint64_t hops, eRouteKowledgeSource::eRouteKowledgeSource knowledgeSource)
{
	initFields();
	mNextHop = nextHop;
	mDestination = destination;
	mKnowledgeSource = knowledgeSource;
	mHops = hops;
}
