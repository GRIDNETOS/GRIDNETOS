#pragma once
#include <stdlib.h>
#include "TrieDB.h"
#include "DataTrie.h"
#include "transaction.h"

class CTransactionTrieDB : CDataTrieDB
{

private:
	CTools mTools;
public:
	bool saveTransaction(std::vector<uint8_t> transactionBER);
	bool saveTransaction(CTransaction * transaction);

	std::vector<uint8_t> loadTransaction(std::vector<uint8_t> key);//returns ber-encoded value
};
