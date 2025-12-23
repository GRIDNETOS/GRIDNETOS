#pragma once
#include "stdafx.h"
#include "PoW.h"
#include "BlockchainManager.h"
#include "GRIDNET.h"



void CPoW::setNonce(const uint32_t nonce)
{
	mAssignedTarget = mNrOfConcats = mNonce = 0;
	mNonce = nonce;
	setType(eWorkResultType::Ultimium);
	std::vector < std::vector<uint8_t>> data;
	data.push_back(Botan::DER_Encoder()
		.encode(static_cast<size_t>(nonce)).get_contents_unlocked());
	setWorkResults(data);
	//this->mGUID = CTools::getTools()->genRandomVector(32);
	this->mLIsPartialProof = false;
}







CPoW::CPoW(uint32_t nonce, std::vector<uint8_t> pDataWorkedOn, bool isPartialProof) : IWorkResult()
{
	mPercentge = 0;
	setType(eWorkResultType::Ultimium);
	mAssignedTarget = mNrOfConcats = mNonce = 0;
	this->mGUID = CTools::getTools()->genRandomVector(32);
	setNonce(nonce);
    //this->mNonce = nonce;
	if(pDataWorkedOn.size())
	{
    this->mDataWorkedOn.resize(pDataWorkedOn.size());
    std::memcpy(this->mDataWorkedOn.data(), pDataWorkedOn.data(), pDataWorkedOn.size());
	}
    this->mLIsPartialProof = isPartialProof;
}
CPoW::CPoW(const CPoW & pow)
{
	mPercentge = pow.mPercentge;
	mAssignedTarget = pow.mAssignedTarget;
	mLIsPartialProof = pow.mLIsPartialProof;
	mDataWorkedOn = pow.mDataWorkedOn;
	mNrOfConcats = pow.mNrOfConcats;
	mNonce = pow.mNonce;
	mHash = pow.mHash;
	mGUID = pow.mGUID;
}
CPoW::CPoW()
{
	mPercentge = 0;
	mAssignedTarget = mNrOfConcats = mNonce = 0;
	setType(eWorkResultType::Ultimium);

	this->mGUID = CTools::getTools()->genRandomVector(32);
	this->mLIsPartialProof = false;
}

bool CPoW::isAccessible()
{
	return true;
}

uint64_t  CPoW::getEffectiveDifficulty()
{
	return CGRIDNET::getTools()->target2diff(mHash);
}
CPoW::CPoW(std::vector<uint8_t> data)
{
	mAssignedTarget = mNrOfConcats = mNonce = 0;
	processPackedData(data);
	size_t nonce;
	if(mResult[0].size()>0)
	{
		Botan::BER_Decoder dec1 = Botan::BER_Decoder(mResult[0]).decode(nonce);
		setNonce(nonce);
	}
}
void CPoW::setAssignedTarget(arith_uint256 assignedTarget)
{
	mAssignedTarget = assignedTarget;
}

arith_uint256 CPoW::getAssignedTarget()
{
	return mAssignedTarget;
}

uint32_t CPoW::getNrOfConcats() {
        return mNrOfConcats;
	}
arith_uint256  CPoW::getExactTarget()
	{
	base_blob<256> asd(mHash);
	uint256 diffInt =  uint256(asd);
	return   UintToArith256(diffInt);


	}
	unsigned char CPoW::reverse(unsigned char b) {
		b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
		b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
		b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
		return b;
	}
	void CPoW::setNrOfConcats(int nrOfConcats) {
        this->mNrOfConcats = nrOfConcats;
	}
	uint32_t CPoW::getNonce() {
		size_t nonce;
		if (mNonce > 0) return mNonce;
		else 
		if (mResult[0].size() > 0)
		{
			Botan::BER_Decoder dec1 = Botan::BER_Decoder(mResult[0]).decode(nonce);
			setNonce(nonce);
		}
		return mNonce;
	}
	void CPoW::setHeader(unsigned char * header, int size)
	{
        this->mDataWorkedOn.resize(size);
		std::memcpy(header, header, size);
	}
	
	std::vector<uint8_t> CPoW::getHash() {
        return mHash;
	}
	void CPoW::setHash(std::vector<uint8_t> hash) {
        this->mHash = hash;
	}

	bool CPoW::isPartialProof()
	{
        return mLIsPartialProof;
	}

	std::vector<uint8_t> CPoW::getGUID()
	{
		return mGUID;
	}

	/**
 * Gets a wizardly comment based on progress percentage
 * Provides colorful, thematic feedback for mining progress
 */
	std::string CPoW::getWizardlyComment(double percentage) {
		if (percentage >= 99.9) return "🧙 By my magical beard! That was practically perfect!";
		if (percentage >= 95.0) return u8"✨ Merlin's monocle! Such arcane precision!";
		if (percentage >= 90.0) return u8"🌟 Great balls of mana! Getting warmer!";
		if (percentage >= 85.0) return u8"🔮 Enchanting progress, young apprentice!";
		if (percentage >= 80.0) return u8"⚡ Thunder and lightning! Not bad at all!";
		if (percentage >= 75.0) return u8"🎭 Hocus pocus! You're getting there!";
		if (percentage >= 70.0) return u8"🌙 By the light of the midnight moon...";
		if (percentage >= 65.0) return u8"🍄 Magic mushrooms! Keep stirring that pot!";
		if (percentage >= 60.0) return u8"🎪 Abracadabra! The magic builds...";
		if (percentage >= 55.0) return u8"🎲 Rolling the cosmic dice, are we?";
		if (percentage >= 50.0) return u8"🎯 Halfway to legendary status!";
		if (percentage >= 45.0) return u8"🎨 Painting with magical probability!";
		if (percentage >= 40.0) return u8"🌈 Still needs more unicorn dust...";
		if (percentage >= 35.0) return u8"🕯️ The crystal ball is warming up!";
		if (percentage >= 30.0) return u8"🦉 My wise owl says keep trying!";
		if (percentage >= 25.0) return u8"🍵 More eye of newt needed!";
		if (percentage >= 20.0) return u8"📚 Back to the spellbooks with you!";
		if (percentage >= 15.0) return u8"🧪 The potion needs more brewing...";
		if (percentage >= 10.0) return u8"🔍 A wizard's work is never done...";
		return u8"🐌 Even a snail's pace is still progress!";
	}
	/**
 * Gets ANSI color based on progress percentage
 */
	eColor::eColor CPoW::getProgressColor(double percentage) {
		if (percentage >= 90.0) return eColor::lightGreen;
		if (percentage >= 70.0) return eColor::lightCyan;
		if (percentage >= 50.0) return eColor::blue;
		if (percentage >= 30.0) return eColor::orange;
		return eColor::lightWhite;
	}

	void CPoW::setProgressPercentage(double percentage)
	{

		mPercentge = percentage;
	}

	double CPoW::getProgressPercentage()
	{

		return mPercentge;
	}