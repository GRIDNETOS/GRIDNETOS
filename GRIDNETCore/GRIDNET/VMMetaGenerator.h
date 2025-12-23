#pragma once
#include <stdafx.h>
#include <vector>
#include <mutex>
#include "enums.h"
#include "ScriptEngine.h"
#include "ConsensusTask.h"

/// <summary>
///Generates VM-runtime execution meta-data. The data can be accessed and exchanged efficiently between the GRIDNET-Core full-node back-end
// and the web-based GUI. Single meta-data package can contain multiple Sections. Each section is of a specific type (eVMMetaType).
// Only entries of a compatible type (eVMMetaEntryType) can be included in a given section.
/// </summary>
class CTools;
class CTokenPool;
class CBlockHeader;
class CTransactionDesc;
class CBlockDesc;
class CDomainDesc;

class CVMMetaGenerator
{
public:
	CVMMetaGenerator();

	bool finalize();

	//sections
	bool beginSection(eVMMetaSectionType::eVMMetaSectionType sType,std::vector<uint8_t> metaData=std::vector<uint8_t>());
	bool endSection();

	//entries
	bool addRAWGridScriptCmd(std::string cmd, eVMMetaCodeExecutionMode::eVMMetaCodeExecutionMode mode,uint64_t inRespToReqID =0, uint64_t windowID =0, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addMarketDepth(const BigInt& totalMarketCap,
		const uint64_t inRespToReqID,
		const uint64_t appID = 0,
		const std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addConsensusTaskNotification(std::shared_ptr<CConsensusTask> task, uint64_t processID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>(), uint64_t inRespToReqID = 0);
	bool addVMStatus(eVMStatus::eVMStatus status, SE::vmFlags &flags, uint64_t processID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>(), std::vector<uint8_t> terminalID = std::vector<uint8_t>(), std::vector<uint8_t> conversationID = std::vector<uint8_t>(), uint64_t inRespToReqID=0);
	bool addThreadOperationStatus(eThreadOperationType::eThreadOperationType oType, std::vector<uint8_t> vmID, uint64_t processID = 0, std::vector<uint8_t> terminalID= std::vector<uint8_t>(), std::vector<uint8_t> conversationID= std::vector<uint8_t>(), uint64_t inRespToReqID=0);
	bool addGridScriptResult(eVMMetaProcessingResult::eVMMetaProcessingResult status, std::vector<uint8_t> BERMetaData, uint64_t inRespToReqID = 0, std::string txt="", uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addTerminalData(std::vector<uint8_t> data1, std::vector<uint8_t> data2= std::vector<uint8_t>(), eTerminalDataType::eTerminalDataType dType = eTerminalDataType::output, uint64_t inRespToReqID = 0, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addAppData(std::vector<uint8_t> data1, std::vector<uint8_t> data2= std::vector<uint8_t>(), uint64_t inRespToReqID = 0, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addFileListingEntry(std::string name, uint64_t timestamp, std::string accessRights, eDFSElementType::eDFSElementType eType, uint64_t inRespToReqID = 0, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addTXProofEntry(std::shared_ptr<CReceipt> receipt, std::shared_ptr<CTransaction> tx, std::shared_ptr<CBlockHeader> header,const std::string & address, const BigSInt &value, uint64_t inRespToReqID, uint64_t appID=0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addSearchResults(std::shared_ptr<CSearchResults> results, uint64_t inRespToReqID, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	//(eLivenessState::eLivenessState livenessState, uint64_t inRespToReqID, uint64_t appID,  std::vector<uint8_t>& vmID)
	bool addLivenessInfo(eLivenessState::eLivenessState livenessState, uint64_t inRespToReqID, uint64_t appID = 0,  std::vector<uint8_t> &vmID = std::vector<uint8_t>());
	bool addBlockchainHeight(const uint64_t height, const uint64_t keyHeight, const uint64_t inRespToReqID, const uint64_t appID=0, const std::vector<uint8_t>& vmID = std::vector<uint8_t>());
	bool addTransactionInfo(const std::shared_ptr<CTransactionDesc>& transactionDesc,
		const uint64_t inRespToReqID,
		const uint64_t appID = 0,
		const std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addBlockchainStatus(const BigInt& totalSupply, 
		const uint64_t height,
		const uint64_t keyHeight, 
		const uint64_t inRespToReqID=0, 
		const uint64_t appID=0, 
		const std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addDomainInfo(const std::shared_ptr<CDomainDesc>& domainDesc,
		const uint64_t inRespToReqID,
		const uint64_t appID = 0,
		const std::vector<uint8_t> vmID = std::vector<uint8_t>());


	bool addBlockInfo(const std::shared_ptr<CBlockDesc>& blockDesc,
		const uint64_t inRespToReqID,
		const uint64_t appID = 0,
		const std::vector<uint8_t> vmID = std::vector<uint8_t>());


	bool addTransactionDailyStats(const std::vector<uint64_t>& dates,
		const std::vector<uint64_t>& counts,
		const uint64_t inRespToReqID ,
		const uint64_t appID = 0,
		const std::vector<uint8_t> vmID = std::vector<uint8_t>());

	bool addUSDTPrice(const BigInt& usdtPrice, const  uint64_t inRespToReqID, const uint64_t appID = 0, const std::vector<uint8_t> &vmID = std::vector<uint8_t>());
	bool addFileContent(std::string fileName, const std::vector<uint8_t> &data, eDataType::eDataType dType, uint64_t inRespToReqID = 0, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>(),std::vector<uint8_t> NFTMeta = std::vector<uint8_t>());
	bool addStateLessChannelElement(std::string name, eStateLessChannelsElementType::eStateLessChannelsElementType eType, std::vector<uint8_t> serializedData, uint64_t inRespToReqID, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addNotification(eNotificationType::eNotificationType eType,std::string title, std::string msg, uint64_t inRespToReqID = 0, uint64_t appID=0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool addNotification(eNotificationType::eNotificationType eType, std::string title, std::vector<uint8_t> data, uint64_t inRespToReqID = 0, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());

	
	uint64_t addDataResponse(const std::vector <eDataType::eDataType>&  dataTypes, const std::vector <std::vector<uint8_t>>& mDataFields, uint64_t reqID, uint64_t appID = 0, const std::vector<uint8_t> &vmID = std::vector<uint8_t>());
	//The data request-response mechanics constitutes a seperate one from the NetMsg level request-response mechanics.
	//Each data-request made contains request ID, which exists also in response.
	//The requesting node needs to remember request's ID  so to be able to associate response with former query.
	//Each response field has its type. All responses however are delivered within a single eVMMetaEntryType.dataResponse section.
	//Each section has mMetaData field which can be used to depict information having section-wide relevance.
	uint64_t addDataRequest(eDataRequestType::eDataRequestType eType, std::string title, std::string msg, std::vector<uint8_t>defaultValue = std::vector<uint8_t>(), uint64_t expiresInSec = 0, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());

	uint64_t addGenTokenPoolRequest(std::shared_ptr<CTokenPool> tp, uint64_t appID = 0 ,uint64_t reqID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());
	bool encodeRAWEntry(eVMMetaEntryType::eVMMetaEntryType eType, std::vector <std::vector<uint8_t>> dataFields, uint64_t inRespToReqID=0, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());

	bool encodeTypedRAWEntry(eVMMetaEntryType::eVMMetaEntryType eType, std::vector<eDataType::eDataType> basicDataTypes, std::vector<std::vector<uint8_t>> dataFields, uint64_t inRespToReqID = 0, uint64_t appID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>());

	uint64_t getTotalEntriesCount();
	
	//interface
	void reset();
	
	// Plain Data Response - BEGIN
	uint64_t addDataResponse(const BigInt& value, uint64_t reqID, uint64_t appID = 0 ,const std::vector<uint8_t>& vmID = std::vector<uint8_t>());
	uint64_t addDataResponse(uint64_t value, uint64_t reqID, uint64_t appID = 0,const  std::vector<uint8_t> &vmID = std::vector<uint8_t>());
	uint64_t addDataResponse(double value, uint64_t reqID, uint64_t appID = 0, const  std::vector<uint8_t> &vmID = std::vector<uint8_t>());
	uint64_t addDataResponse(const std::vector<std::string>& values, uint64_t reqID, uint64_t appID = 0, const std::vector<uint8_t>& vmID = std::vector<uint8_t>());
	uint64_t addDataResponse(const std::vector<double>& values, uint64_t reqID, uint64_t appID, const std::vector<uint8_t>& vmID = std::vector<uint8_t>());
	uint64_t addDataResponse(const std::vector<BigInt>& values, uint64_t reqID, uint64_t appID, const  std::vector<uint8_t> &vmID = std::vector<uint8_t>());
	uint64_t addDataResponse(const std::string& value, uint64_t reqID, uint64_t appID = 0, const std::vector<uint8_t> vmID = std::vector<uint8_t>());
	// Plain Data Respnse - END

	std::vector<uint8_t> getData();

	static uint64_t genReqID();


private:
	static std::mutex sGuardian;
	static uint64_t sRecentReqID;
	bool getInSection();
	void setInSection(bool inSection=true);
	std::recursive_mutex mGuardian;
	size_t mVersion;
	size_t mSectionsVersion;
	uint64_t mSectionsCount;
	uint64_t mTotalEntriesCount;
	std::vector<uint8_t> mData;
	bool mFinalized;
	eVMMetaSectionType::eVMMetaSectionType mCurrentSectionType;
	void setCurrentSectionType(eVMMetaSectionType::eVMMetaSectionType sType);
	bool mInSection;
	Botan::DER_Encoder mEncoder;
	std::shared_ptr<CTools> mTools;
};
