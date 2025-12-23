#pragma once
#include "enums.h"
#include "stdafx.h"
#include <vector>
class CReceipt;
class CDataPubResult
{
public:
	CDataPubResult();
	eDataPubResult::eDataPubResult getResult();
	void setResult(eDataPubResult::eDataPubResult res);
	void addReceiptToHotCache(CReceipt rec);
	std::vector<CReceipt> getReceipts();

private:
	eDataPubResult::eDataPubResult mResult;
	std::vector<CReceipt> mReceipts;//received from each contacted neighboor
};