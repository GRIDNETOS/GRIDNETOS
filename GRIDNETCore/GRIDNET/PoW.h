
#ifndef POW_H
#define POW_H
#include "arith_uint256.h"
#include "stdafx.h"
#include "uint256.h"
#include "enums.h"

#include "../interfaces/iworkresult.h"


class CPoW : public IWorkResult
{
public:
	CPoW(const CPoW & pow);
	CPoW();
	bool isAccessible();
	void setNonce(const uint32_t nonce);


	CPoW(uint32_t nonce, std::vector<uint8_t> pDataWorkedOn, bool isPartialProof);
	uint64_t getEffectiveDifficulty();
	CPoW(std::vector<uint8_t> data );
	void setAssignedTarget(arith_uint256 assignedTarget);
	arith_uint256 getAssignedTarget();

	uint32_t getNrOfConcats();

	arith_uint256 getExactTarget();

	void setNrOfConcats(int nrOfConcats);

	uint32_t getNonce();

	void setHeader(unsigned char * header, int size);

	

	std::vector<uint8_t> getHash();

	void setHash(std::vector<uint8_t> hash);
	bool isPartialProof();
	std::vector<uint8_t> getGUID();
	std::string getWizardlyComment(double percentage);
	eColor::eColor getProgressColor(double percentage);
	void setProgressPercentage(double percentage);

	double getProgressPercentage();
private:
	double mPercentge;
	arith_uint256 mAssignedTarget;
    bool mLIsPartialProof = true;
    std::vector<uint8_t> mDataWorkedOn;
	unsigned char reverse(unsigned char b);
    uint32_t mNrOfConcats;
    uint32_t mNonce;
    std::vector<uint8_t> mHash;

	std::vector<uint8_t> mGUID;
};
#endif
