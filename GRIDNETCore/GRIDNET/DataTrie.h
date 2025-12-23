#ifndef DATA_TRIE_H
#define DATA_TRIE_H
#include "tools.h"
#include <stdlib.h>
#include "TrieDB.h"
#include "ScriptEngine.h"

class CSecDescriptor;

class CDataTrieDB : public CTrieDB
{
public:

private:
	std::shared_ptr<CTools> mTools;
    std::shared_ptr<CCryptoFactory> mCf;
	eBlockchainMode::eBlockchainMode mBlockchainMode;

	
public:

	virtual ~CDataTrieDB();
	std::vector<CTrieNode*> getElements(bool doRecursion = false,bool doPruning=true);
	std::vector<uint8_t> getPackedData();
	static CDataTrieDB * genNode(CTrieNode **node,eBlockchainMode::eBlockchainMode blockchainMode);
	bool prepare(bool store);
	CDataTrieDB(const CDataTrieDB& sibling);
	eBlockchainMode::eBlockchainMode getBlockchainMode();
	CDataTrieDB(eBlockchainMode::eBlockchainMode mode , std::string name = "main",std::vector<uint8_t> perspectiveID = std::vector<uint8_t>());
	//storage

	//used to save any kind of 64-bit stack-immediates (double, signed unsigned integers, bool), dType stores variable type
	//within BER Tag.
	bool saveValue(std::string key, BigInt value,eDataType::eDataType dType,bool sandBoxModex = false, bool hidden = false, const uint64_t& cost = 0,std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);
	bool saveValue(std::string key, BigSInt value, eDataType::eDataType dType,bool sandBoxModex = false, bool hidden = false, const uint64_t& cost = 0,std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);
	bool saveValue(std::vector<uint8_t> key,CDataTrieDB directory, eDataType::eDataType dType, bool sandBoxMode = false,bool hidden=false, const uint64_t &cost=0, std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);
	bool saveValue(std::vector<uint8_t> key, CTrieNode node, eDataType::eDataType dType, bool sandBoxMode, bool hidden, const uint64_t& cost = 0, std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);
	bool updateSecToken(std::vector<uint8_t> key, CTrieNode* element, bool sandBoxMode);
	bool updateSecToken(std::string key, CTrieNode* element, bool sandBoxMode);
	bool saveValue(std::vector<uint8_t> key, intptr_t value, eDataType::eDataType dType, bool sandBoxMode = false, bool hidden = false, const uint64_t& cost = 0,std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);
	bool saveValue(std::string key, intptr_t value, eDataType::eDataType dType,bool sandBoxModex = false, bool hidden = false, const uint64_t& cost = 0,std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);

	
	//used to store byte-vectors of arbitary length.
	bool saveValue(std::vector<uint8_t> key, std::vector<uint8_t> value, eDataType::eDataType dType, bool sandBoxMode = false, bool hidden = false, const uint64_t& cost = 0,std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);
	bool saveValue(std::string key, std::vector<uint8_t> value, eDataType::eDataType dType, bool sandBoxMode = false, bool hidden = false, const uint64_t& cost = 0,std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);

	//retrieval
	std::vector<uint8_t> loadValue(std::string key, eDataType::eDataType &vt, const std::string& absPath = "");
	std::vector<uint8_t> loadValue(std::vector<uint8_t> key, eDataType::eDataType &valType, const std::string& absPath="");
};

#endif
