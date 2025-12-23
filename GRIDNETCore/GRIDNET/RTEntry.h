#pragma once
#include <stdafx.h>
#include <vector>
#include <memory>
#include <mutex>
#include "enums.h"


class CEndPoint;
class CRTEntry
{
private:
	std::mutex mGuardian;
	static uint64_t sIDGenerator;
	static std::mutex sIDGeneratorGuardian;
	uint64_t mID;
	uint64_t mHops;
	uint64_t mSrcSeq;
	std::vector<uint8_t> mDstPubKey;//might be unknown.
	uint64_t mDstSeq;
	std::shared_ptr<CEndPoint> mNextHop;
	std::shared_ptr<CEndPoint> mDestination;
	uint64_t mTimestamp;
	void initFields();
	eRouteKowledgeSource::eRouteKowledgeSource mKnowledgeSource;
public:
	eRouteKowledgeSource::eRouteKowledgeSource getKnowledgeSource();
	void setKnowledgeSource(eRouteKowledgeSource::eRouteKowledgeSource source);
	std::string getDescription();
	std::vector<uint8_t> getDstPubKey();
	void setDstPubKey(std::vector<uint8_t> & pubKey);
	uint64_t getID();
	uint64_t getHops();
	uint64_t getSrcSeq();
	uint64_t getDstSeq();

	void ping();

	void setHops(uint64_t value);
	void setSrcSeq(uint64_t value);
	void setDstSeq(uint64_t value);
	void setDst(std::shared_ptr<CEndPoint> dst);
	void setSrc(std::shared_ptr<CEndPoint> src);
	std::shared_ptr<CEndPoint> getNextHop();
	void setNextHop(std::shared_ptr<CEndPoint> endpoint);
	std::shared_ptr<CEndPoint> getDst();
	uint64_t getTimestamp();
	CRTEntry();//for de-serialization
	CRTEntry(std::shared_ptr<CEndPoint> nextHop, std::shared_ptr<CEndPoint> destination,uint64_t hops=0, eRouteKowledgeSource::eRouteKowledgeSource knowledgeSource = eRouteKowledgeSource::propagation);
};