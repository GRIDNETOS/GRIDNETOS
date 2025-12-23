#include "chain.h"

uint64_t chain::GetBlockValue(int nHeight, uint64_t nFees)
{
	uint64_t  nSubsidy = 50;// *COIN;

	// Subsidy is cut in half every 210000 blocks, which will occur approximately every 4 years
	nSubsidy >>= (nHeight / 210000);

	return nSubsidy + nFees;
}
