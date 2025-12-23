#pragma once
#include "stdafx.h"
#include "TrieNode.h"
#include "TrieLeafNode.h"
#include "TrieDB.h"
#include "DataTrie.h"
#include "CGlobalSecSettings.h"
#include "TransactionManager.h"
#include <vector>f
#include "enums.h"
#include "transaction.h"
class CReceipt : public CTrieLeafNode
{
public:
	static std::shared_ptr<CReceipt> instantiate(std::vector<uint8_t> serializedData, eBlockchainMode::eBlockchainMode mode);
	static CReceipt * instantiateReceipt(std::vector<uint8_t> serializedData, eBlockchainMode::eBlockchainMode mode);
	static CReceipt * genNode(CTrieNode **node, bool useTestStorageDB=false);
	
	bool prepare(bool strore = true);
	CReceipt(eBlockchainMode::eBlockchainMode blockchainMode);
	CReceipt(CTransaction &t,eBlockchainMode::eBlockchainMode blockchainMode);
	CReceipt(std::vector<uint8_t> id= std::vector<uint8_t>());
	
	CReceipt(const CReceipt & sibling);
	CReceipt & operator=(const CReceipt & t);
	void setERGUsed(BigInt erg);
	BigInt getERGPrice();
	void setERGprice(BigInt price);
	//uint64_t getTargetNonce();
	//void setTargetNonce(uint64_t nonce);
	BigInt getERGUsed() const;
	void setResult(eTransactionValidationResult result);
	eTransactionValidationResult getResult() const;
	std::vector<uint8_t> getGUID() const;
	void logEvent(SE::Cell cell, SE::CD  contentDescription,std::shared_ptr<SE::CScriptEngine> se);
	void logEvent(std::string str);
	std::vector<std::string> getLog() const;
	size_t getLogSize() const;
	std::string getRenderedLog(uint64_t count, std::string newLine, uint64_t minEntryLength=1);
	void setReceiptType(eReceiptType::eReceiptType type);
	eReceiptType::eReceiptType getReceiptType() const;
	void setBlockInfo(std::shared_ptr<CBlock> block);
	void clearBlockInfo();
	uint64_t getBlockHeight();
	//std::vector<uint8_t> getBlockHash();
	void setVerifiableID(std::vector<uint8_t> ID);
	std::vector<uint8_t> getVerifiableID() const;
	//HotStorageProperties HotProperties;
	std::string toString();
	std::string translateStatus() const;
	void setID(std::vector<uint8_t> ID);
	void setBERMetaData(std::vector<uint8_t> data);
	std::vector<uint8_t> getBERMetaData();

	void setSacrificedValue(BigInt value);
	BigInt getSacrificedValue();
	

private:

	//uint64_t mTargetNonce;// NOT serialized
	//Decoding - BEGIN
	static void decodeReceiptV2(Botan::BER_Object& obj, Botan::BER_Decoder& dec, CReceipt* receipt);
	static void decodeReceiptV3(Botan::BER_Object& obj, Botan::BER_Decoder& dec, CReceipt* receipt);
	static void decodeReceiptV1(Botan::BER_Object& obj, Botan::BER_Decoder& dec, CReceipt* receipt);
	//Decoding - END
	
	//not serialized - BEGIN
	std::vector<uint8_t> mBERMetaData;
	//not serialized - END
	void initializeFields();
	bool genID(eBlockchainMode::eBlockchainMode blockchainMode);
	std::vector<uint8_t> mGUID;
	size_t mVersion, mOriginalVersion;
	eTransactionValidationResult mResult;
	BigInt mERGUsed;
	BigInt mERGPrice;
	BigInt mSacrificedValue;
	std::vector<std::string> mLog;
	eReceiptType::eReceiptType mReceiptType;//1 - Transaction Receipt; 2 - Verifiable receipt
	//std::vector<uint8_t> mBlockID;//in which block is the transaction stored?
	std::vector<uint8_t> mReceiptID;
	uint64_t mBlockHeight;
};