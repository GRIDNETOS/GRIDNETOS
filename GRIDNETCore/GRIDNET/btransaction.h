#pragma once
#include "stdafx.h"
#include "transitpool.h"
#include "transonion.h"
#include "botan_all.h"
#include <cassert>

class CTransaction {
public:

	std::vector<uint8_t> getPackedData();

	static CTransaction * instantiateTransaction(std::vector<uint8_t> serializedData);

	bool setRecipient(std::vector<uint8_t> recipient);
	bool setInitCode(std::vector<uint8_t> code);
	bool setData(std::vector<uint8_t> data);
	bool genID();

	size_t getNonce();
	uint32_t getTime();
	uint32_t getErgPrice();
	uint32_t getErgLimit();
	uint32_t getRecipient();
	uint32_t getExtData();
	uint32_t getValue();
	uint32_t getVersion();
	std::vector<uint8_t> getTransactionID();
	std::vector<uint8_t> getInitCode();
	bool setNonce(size_t nonce);
	//hash of the transaction header

private:

	bool verifyInitCode(std::vector<uint8_t> code);
	size_t nonce;
	std::vector<uint8_t> initCode;
	std::vector<uint8_t> tranactionID;
	uint32_t time;
	uint32_t ergPrice;
	uint32_t ergLimit;
	std::vector<uint8_t> recipient;
	std::vector<uint8_t> extData;
	uint32_t value;
	uint32_t version;
	uint32_t lockTime;

};
