#include "DataPubResult.h"
#include "Receipt.h"


CDataPubResult::CDataPubResult()
{
}

eDataPubResult::eDataPubResult CDataPubResult::getResult()
{
	return mResult;
}

void CDataPubResult::setResult(eDataPubResult::eDataPubResult res)
{
	mResult = res;
}

void CDataPubResult::addReceiptToHotCache(CReceipt rec)
{
	mReceipts.push_back(rec);
}

std::vector<CReceipt> CDataPubResult::getReceipts()
{
	return mReceipts;
}
