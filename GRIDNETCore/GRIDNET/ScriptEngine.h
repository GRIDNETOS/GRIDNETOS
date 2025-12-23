#pragma once
#include <list>
#include <vector>
#include <array>
#include <mutex>
#include <string>
#include <exception>
#include <stdexcept>
#include <iostream>
#include "SearchResults.hpp"
#include "Settings.h"
#include "utilstrencodings.h"
#include "CryptoFactory.h"
#include "enums.h"
#ifndef GRIDSCRIPT_hpp_included
#define GRIDSCRIPT_hpp_included

#define MAX_THREADS_COUNT 5
//#define AVAILABLE_UPDATES_COUNT 1 Update: it's auto-detected
//Note: we can NEVER allow an agent to modify the below flags directly.
#define ERG_COST_PER_RAM_BYTE 1
#define REG_EXEC_SD 0
#define REG_DATA_TYPE 1
#define REG_SIGN 2
#define REG_CURRENT_SD 3
#define REG_CURRENT_DIR 4
#define REG_THROW_ON_FAILURE 5
#define REG_PRIV_KEY 6
#define REG_RECEIPT_ID 7
#define REG_BASE58_DEBUG_VIEW 8
#define REG_MAKING_SACRIFICE 9
#define REG_EXECUTING_CODE_ID 10

//Extreme Caution - BEGIN
#define REG_CALLERS_ID 11 // WARNING: explicit TX issuer's identity, derived from signature. It's NOT the identity of dApp caller ! It's the identity of entity executing CODE BUNDLE!!!
//Extreme Caution - END

#define REG_AUTHENTICATED_SD 12// in contrast with REG_CALLERS_ID, this represents the entity in whose name the code is executing.
							   // usually it corresponds to REG_CALLERS_ID, but it might not be the case (when impersonation is used).
							   // this is set only though verifySecurityTokenEx() and MUST be cleared after dApp exits.

#define REG_DAPP_INC_BALANCE_CHANGE 13
#define REG_DOING_COLON 14
#define REG_SACRIFICED_VALUE 15
#define REG_EXECUTING_BY_OVERWATCH 16
#define REG_TALKING_TO 17
#define REG_APP_RUNNING 18
#define REG_EXTERNAL_DATA 19
#define REG_REQ_APP_ABORT 20
#define REG_SUPPRESS_THROW 21//suppress throw on invocation of *ANY SINGLE* instruction which is to follow (excluding security checks)
#define REG_SUPPRESS_DFS_THROW 22 //suppress throw of a SINGLE DFS-related instruction which is to follow. It does not need to be the first to follow.
#define REG_COMMIT_PENDING 23
#define REG_THREAD_READY 24 // thread/transaction formulation has finished and is ready for commitment
#define REG_THREAD_PAUSED 25
#define REG_EXECUTING_INSTRUCTION 26
#define REG_EXECUTING_PROGRAM 27
#define REG_HIGH_PRECISION 28
#define REG_VIEW_CAN_OVERFLOW 28
#define REG_LOGGED_IN_AS 29
#define REG_WAS_SANDBOX_PRE_AUTHED 30//per command SANDBOX ONLY pre-auth
#define REG_IS_THREAD_UI_AWARE 31//SECURITY CRITICAL. CANNOT be changed from user-mode.
#define REG_IS_TEXT_OUTPUT_ENABLED 32
#define REG_HAS_DETACHED_THREAD 33//indicates whether there's a detached NATIVE thread currently attached to this Thread/VM.
#define REG_KERNEL_THREAD 34//EXTREME caution - only kernel threads can affect the decentralized state-machine. Do not allow for modifications from user mode.
#define REG_DO_NOT_COMPILE_READS 35
#define REG_NEW_LINE 36
#define REG_PP_MODE 37	// Executing the GridScript++ context
#define REG_ABORT 38 // Indicated by GridScript++ context
#define REG_LAST_ERROR 39 //which occurred in GridScript++
#define REG_CURRENT_CODE_PATH 40
#define REG_ASSETS_RECEIVED_FROM 41
#define REG_PRESERVE_RECENT_CALL 42 //causes instructions leading to recent dApp's invocation to remain in the compiled source code.
#define REG_IMPLICIT_CALL_THREAD_BEGAN 43
#define REG_CALLERS_ID_BEFORE_DAPP_CALLED 44//reverted to once dApp exits
#define REG_EXCUSE_ERG_USAGE 45 // when block is checkpointed and code processing had succeeded with prior build we MAY excuse ERG usage checks and charge prior costs
#define REG_KERNEL_ERG_BID 46   // ERG Bid for kernel-mode execution (transaction commitment). If set - the VM would NOT issue an explicit data request regarding it.
#define REG_KERNEL_ERG_LIMIT 47 // ERG Limit for kernel-mode execution (transaction commitment). If set - the VM would NOT issue an explicit data request regarding it.
#define REG_KERNEL_NONCE 48     // Transaction nonce override for manual nonce management. If set - commitThreads() will use this value instead of mCurrentStateDomain->getNonce()+1. Allows enqueueing multiple consecutive transactions.
#define REGISTERS_COUNT 49 //<= *IMPORTANT* - UPDATE when adding new registers VALUE equal to LATEST_INDEX+1



#define SEC_TOKEN_LENGTH 5
#define SEC_TOKEN_POS_FUNCTION_ID 4
#define SEC_TOKEN_POS_DOMAIN_ID 3
#define SEC_TOKEN_POS_PUB_KEY 2
#define SEC_TOKEN_POS_NONCE 1
#define SEC_TOKEN_POS_SIG 0

class CFSResult;
class CAccessToken;
class RequestedPermissions;
class CNetTask;
class CEndPoint;
class CConversation;
class CStateDomainManager;
class CQRIntentResponse;
class CQRIntent;
class CSolidStorage;
class CSearchFilter;
class GridScriptPPEngine;
class CStateDomain;
class CDataTrieDB;
class CGlobalSecSettings;
class CWorkManager;
class CUltimium;
class CBlockchainManager;
class ThreadPool;
class CTests;
class CTools;
class CReceipt;
class CVerifiable;
class CTransactionManager;
class CGridScriptCompiler;
class CVMMetaGenerator;
class CKeyWordState;
class CVMMetaSection;
class CBreakpointFactory;
class CBreakpoint;
class CCMDExecutor;
class CFileMetaData;
class CSecDescriptor;
class CSearchFilter;
class CSearchResults;
class CDTI;
namespace SE {

	class AbortException : public std::runtime_error {
	public:
		AbortException(const std::string& msg) : std::runtime_error(msg) {}
		AbortException(const char* msg) : std::runtime_error(msg) {}
	};

	static std::mutex globalDataLock;
	struct memory_region;
	extern const char* GRIDSCRIPT_version;
#define GRIDSCRIPT_NO_MAIN 1

#ifdef GRIDSCRIPT_USE_READLINE
#include "readline/readline.h"
#include "readline/history.h"
#endif


#define GRIDSCRIPT_DISABLE_FILE_ACCESS 1
//#define GRIDSCRIPT_DISABLE_MEMORY_ALLOCATION 0
// Note: GRIDSCRIPT_VERSION is defined as uint64_t in CGlobalSecSettings.h
// Use CGlobalSecSettings::getGridScriptVersion() to retrieve at runtime
//#ifndef GRIDSCRIPT_VERSION
//#define GRIDSCRIPT_VERSION "1.0.0"
//#endif

#ifndef GRIDSCRIPT_DATASPACE_SIZE
#define GRIDSCRIPT_DATASPACE_SIZE (16 * 1024 * sizeof(Cell))
#endif

#ifndef GRIDSCRIPT_DSTACK_COUNT
#define GRIDSCRIPT_DSTACK_COUNT (256)
#endif

#ifndef GRIDSCRIPT_RSTACK_COUNT
#define GRIDSCRIPT_RSTACK_COUNT (256)
#endif
	
	
	class CScriptEngine;

	struct Definition;
	using std::cerr;
	using std::cout;
	using std::endl;
	using std::exception;
	using std::ptrdiff_t;
	using std::runtime_error;
	using std::size_t;
	using std::string;

	


	

	using Cell = uint64_t;   // unsigned value
	using SCell = int64_t;    // signed value

	using Char = unsigned char;
	using SChar = signed char;

	using CAddr = Char*;     // Any address
	using AAddr = Cell*;     // Cell-aligned address

#define CELL(x)  reinterpret_cast<Cell>(x)
#define CADDR(x)  reinterpret_cast<Char*>(x)
#define AADDR(x)  reinterpret_cast<AAddr>(x)
#define CHARPTR(x) reinterpret_cast<char*>(x)
#define SIZE_T(x) static_cast<size_t>(x)
#define VOIDP(x) reinterpret_cast<void*>(x)


//check if memory at an address equal to X is valid and has S allocated at least S bytes
#define CHECKMEM(x,s) RUNTIME_ERROR_IF(!checkMemInRegisteredRegion(CELL(x),s),"Prohibited memory access");
//check if memory at an address equal to X is valid and has S allocated at least 1 Byte allocated
#define CHECKMEMS(x) RUNTIME_ERROR_IF(!checkMemInRegisteredRegion(CELL(x),1),"Prohibited memory access");

//The below checks GridScript memory *POINTERS*. In C++ terms these are double pointers. First pointing to an actual address.

#define CHECKMEMPTR(x,s) RUNTIME_ERROR_IF(!checkMemInRegisteredRegion(CELL(x),s),"Prohibited memory access"); 
#define CHECKMEMPTRNC(x,s) RUNTIME_ERROR_IF(!checkMemInRegisteredRegion(x,s),"Prohibited memory access"); 
#define CHECKMEMPTRS(x) RUNTIME_ERROR_IF(!checkMemInRegisteredRegion(CELL(x),1),"Prohibited memory access"); 
#define CHECKMEMPTRSNC(x,s) RUNTIME_ERROR_IF(!checkMemInRegisteredRegion(x,1),"Prohibited memory access"); 
constexpr auto CellSize = sizeof(Cell);
	
constexpr Cell False = 0;
constexpr Cell True = 1;

using Code = void(CScriptEngine::*)(void);	

using Xt = Definition*;


#define XT(x) reinterpret_cast<Xt>(x)

struct memory_region
{
	Cell region_start;
	Cell region_end;
	size_t requested_size = 0;
	bool isNativeCode = false;
	bool selfAllocated = false;
};

#ifndef CODEWORD_H
#define CODEWORD_H

// Forward declaration to avoid circular dependency


struct CodeWord {
	std::string name;                // The name of the codeword
	Code code;                       // Pointer to member function in CScriptEngine
	unsigned int baseERGCost = 0;    // The base ERG cost
	unsigned int hashCode;           // Unique hash code for the codeword

	bool hasLength = false;          // Indicates if the codeword has a length component

	bool onlyFromTerminal = false;   // Indicates if the codeword can only be executed from a terminal
	bool requiresSecurityToken = false; // Indicates if executing the codeword requires a security token

	unsigned int exHashCode = 0;     // Hash code for the 'Ex' (extended) representation of the codeword
	unsigned int reqStackWordsProceeding = 0; // Required stack words proceeding the codeword

	bool doNotIncludeInSource = false; // Indicates if the codeword should not be included in the source
	unsigned int inlineParamsCount = 0; // Number of inline parameters

	bool hasBase58BinaryInLineParams = false; // Indicates if the codeword has base58 binary inline parameters
	bool allowToBeExecutedFromRAWCodeMode = false; // Indicates if the codeword allows execution from RAW code mode
	bool allowedOnlyByAnOverwatch = false; // Indicates if execution is allowed only by an overwatch
	bool hasBase64BinaryInLineParams = false; // Indicates if the codeword has base64 binary inline parameters
	bool isDataReadFunction = false; // Indicates if the codeword is a data read function
};

#endif // CODEWORD_H


class CD //CellDescriptor
{
private:
	

	std::string mFunctionName;
	std::string mVarName;
	eDataType::eDataType mDataType;


public:
	
	CD();
	CD& operator =(const CD& other);
	CD(const CD& sibling);
	CD(std::string name, eDataType::eDataType type, std::string varName = "");
	std::string getFunctionName();
	std::string getDescription();
	void setDescription(std::string description);
	void setFunctionName(std::string name);

	eDataType::eDataType getDataType();
	void setDataType(eDataType::eDataType type);
};

enum eArithOperation {
	addition,
	subtraction,
	multiplication,
	division,
	ldiv
};

struct vmFlags
{
	bool cmdExecutorAvailable : 1;
	bool uiAttached : 1;
	bool isThread : 1;//if not set then it's a System thread
	bool privateThread : 1;
	bool nonCommittable: 1;
	bool isDataThread : 1;
	bool reserved5 : 1;
	bool reserved6 : 1;
	vmFlags(const vmFlags& sibling) {
		std::memcpy(this, &sibling, sizeof(vmFlags));
	}

	vmFlags()
	{
		std::memset(this, 0, sizeof(vmFlags));
	}
};


struct txStatFlags
{
	// Note: keep adding new fields after the present ones to avoid bit-shifts and prior values chaging meaning.
	bool valueTransfer : 1;
	bool genesisReward : 1;
	bool blockReward   : 1;
	bool fraudProcessing : 1;
	bool reserved : 4;
	txStatFlags(uint64_t val = 0)
	{
		std::memset(this, 0, sizeof(txStatFlags));
		std::memcpy(this, &val, sizeof(txStatFlags));
	}

	txStatFlags(const std::vector<uint8_t>& valBytes);

	txStatFlags& operator=(const txStatFlags& sibling)
	{
		std::memcpy(this, &sibling, sizeof(txStatFlags));
		return *this;
	}

	txStatFlags(const txStatFlags& sibling)
	{
		std::memcpy(this, &sibling, sizeof(txStatFlags));
	}


	uint64_t getUInt()
	{
		uint64_t res = 0;
		std::memcpy(&res, this, 1);
		return res;
	}

	std::vector<uint8_t> getBytes()
	{
		return CTools::getInstance()->getSigBytesFromNumber(getUInt());
	}

};


struct sendFlags
{
	bool notax : 1;
	bool isInGNC : 1;
	bool reserved : 6;
	sendFlags(uint64_t val=0)
	{
		std::memcpy(this, &val, sizeof(sendFlags));
	}

	sendFlags& operator=(const sendFlags& sibling)
	{
		notax = sibling.notax;
		isInGNC = sibling.isInGNC;
		reserved = sibling.reserved;
		return *this;
	}

	sendFlags(const sendFlags& sibling)
	{
		notax = sibling.notax;
		isInGNC = sibling.isInGNC;
		reserved = sibling.reserved;
	}


	uint64_t getUInt()
	{
		uint64_t res = 0;
		std::memcpy(&res, this, 1);
		return res;
	}

};

struct sacrificeFlags
{
	bool isInGNC : 1;
	bool reserved : 7;
	sacrificeFlags(uint64_t val=0)
	{
		std::memcpy(this, &val, sizeof(sacrificeFlags));
	}

	sacrificeFlags& operator=(const sacrificeFlags& sibling)
	{
		isInGNC = sibling.isInGNC;
		reserved = sibling.reserved;
		return *this;
	}

	sacrificeFlags(const sacrificeFlags& sibling)
	{
		isInGNC = sibling.isInGNC;
		reserved = sibling.reserved;
	}


	uint64_t getUInt()
	{
		uint64_t res = 0;
		std::memcpy(&res, this, sizeof(sacrificeFlags));
		return res;
	}

};

	class CScriptEngine :public std::enable_shared_from_this<CScriptEngine>
	{

		// Friends - BEGIN
		friend class CScriptEngineExt;
	    // Friends - END
	public:
		vmFlags getFlags();
		void setFlags(vmFlags &flags);
		std::vector<std::string> getCMDHistory();
		void setParentThread(std::shared_ptr<SE::CScriptEngine> thread);
		std::shared_ptr<SE::CScriptEngine> getParentThread();
		void addNativeThread(std::shared_ptr<std::thread> thread);
		size_t getNativeThreadCount();
		bool removeNativeThreadByID(std::thread::id id);


		void setExcuseERGUsage(bool doIt = true);
		bool getExcuseERGUsage();

		//GridScript++ Bindings - BEGIN
		void writeLineGPP(const std::string& str);
		void writeErrorGPP(const std::string& str);
		bool askYesNoGPP(const std::string& str, bool defaultA=false);

		//Decentralized Threads API - BEGIN
		bool beginThreadGPP();
		bool commitThreadGPP();
		bool abortThreadGPP();
		std::shared_ptr<SE::CScriptEngine> startThreadGPP(const std::vector<uint8_t>& threadID);
		std::shared_ptr<SE::CScriptEngine> getMainThreadGPP();
		bool readyThreadGPP();
		bool pauseThreadGPP(const std::vector<uint8_t>& threadI);
		bool freeThreadGPP(const std::vector<uint8_t>& threadID);
		//Decentralized Threads API - END

		//File System API - BEGIN
		std::shared_ptr<CFSResult> writeFileGPP(const std::string & path, const std::vector<uint8_t>& data, eDataType::eDataType dType = eDataType::bytes, const std::vector<uint8_t>& threadID= std::vector<uint8_t>());
		std::shared_ptr<CFSResult>  readFileGPP(const std::string& path, const std::vector<uint8_t>& threadID = std::vector<uint8_t>());
		std::shared_ptr<CFSResult> fileExistsGPP(const std::string& path, const std::vector<uint8_t>& threadID = std::vector<uint8_t>());
		std::shared_ptr<CFSResult> createDirectoryGPP(const std::string& path, const std::vector<uint8_t>& threadID = std::vector<uint8_t>());
		
		//File System API - END

		
		std::vector<uint8_t> perspectiveGPP();
		std::vector<uint8_t> getCallersID();
		std::string getPathSelf();
		BigInt getReceivedGNCGPP();
		std::vector<uint8_t> getReceivedGNCFromGPP();
		//GridScript++ Bindings - END
	private:
		uint64_t mLastMemoryWarningReport;
		bool mDefMemRegionsParsed;
		std::shared_ptr<CScriptEngineExt> mExtensions;
		std::shared_ptr<CScriptEngineExt> getExtensions();
		//std::unordered_set<std::vector<uint8_t>> registeredMemoryHashes;
		std::mutex mNativeThredsGuardian;
		std::vector<std::shared_ptr<std::thread>> mNativeThreads;
		std::weak_ptr<SE::CScriptEngine> mParentThread;
		std::mutex mParentThreadGuardian;
		std::vector<std::string> mCMDHistory;
		bool mOwningTransactionManager;
		vmFlags mFlags;
		Botan::secure_vector<uint8_t> mEphPrivKey;
		std::vector<uint8_t> mEphPubKey;
		void genEphPrivKey();
		static std::mutex mSnakePoolBufferGuadian;
		static std::mutex mMinecraftPoolBufferGuadian;
		static std::shared_ptr<CTokenPool> mSnakePoolBuffer;
		static std::shared_ptr<CTokenPool> mMinecraftPoolBuffer;
		static void setSnakePool(std::shared_ptr<CTokenPool> pool);
		static std::shared_ptr<CTokenPool> getSnakePool();

		void setMinecraftPool(std::shared_ptr<CTokenPool> pool);

		std::shared_ptr<CTokenPool> getMinecraftPool();

		uint64_t mLastMsgBroadcast;
		uint64_t mMsgsBroadcastCount;
		bool mRequestWebPartRenderedQRs;
		bool mResetOnException;
		std::mutex mRemoteTerminalGuardian;
		std::mutex mFlagsGuardian;
		std::mutex mRequestFieldsGuardian;
		std::mutex mResponseFieldsGuardian;
		bool mIsCmdExecutingFromGUI;
		bool mIsGUICommandLineTerminal;
		size_t mWaitStartTime;
	
		void setQRWaitStartTimeNow();
		std::mutex mWaitingForQrIntentHelpDataGuardian;
		bool getIsCMDExecutingFromGUI();
		//bool mIsTextutalOutputEnabled;
		bool validateQRIntentResponse(std::shared_ptr<CQRIntentResponse> response, std::shared_ptr<CQRIntent> intent, std::vector<uint8_t> stateDomainID);
		bool validateDataResponseFields(std::shared_ptr<CQRIntent> intent, std::vector<uint8_t> stateDomainID);
		std::shared_ptr<CQRIntentResponse> mQRIntentResponseReceived;


		// Meta Data Data - BEGIN
	public:
		std::vector<uint8_t> getRecentBERMetaData();
		void setGenerateBERMetaData(bool doIt = true);
		bool getGenerateBERMetaData();
		bool getIsWaitingForVMMetaData();
		void setIsWaitingForVMMetaData(bool isWaiting = true);
		std::vector<uint8_t> setQRIntentResponse(std::shared_ptr<CQRIntentResponse> response, bool waitForReceipt = true, bool forceIt = false);
		std::vector<uint8_t> setVMMetaDataResponse(std::vector<std::shared_ptr<CVMMetaSection>>, bool waitForReceipt = true, bool forceIt = false);
		std::vector<uint8_t> getVec(AAddr cellAdr, bool pop = false, bool isADataSpacePointer = false, bool beStrict = true);


		std::vector<std::vector<uint8_t>> getRequstFields();
		std::vector<std::vector<uint8_t>> geResponseFields();
		private:
		std::vector<std::vector<uint8_t>> mDataFieldsInResponse; // to be returned from GridScript
		std::vector<std::vector<uint8_t>> mDataFieldsInRequest; // to be processed to be processed by GridScript
		// Meta Data Data - END


		bool mVMMetaDataResponseReceived = false;
		bool mWaitingForQRIntentResponse;
		std::mutex mThreadsGuardian;//guarding the thread pool
		std::mutex mWaitingForVMMetaDataGuardian;
		std::mutex mVMMetaDataResponseReceivedGuardian;
		//std::vector<std::vector<uint8_t>> getReceivedDataFields();

		void flashLine(std::string str, bool doMarginAnimation=false, eViewState::eViewState view= eViewState::GridScriptConsole, eFlashPosition::eFlashPosition position = eFlashPosition::bottom);
		void writeLine(std::string str = "", bool endLine = true, bool includeOwner = true, eViewState::eViewState view = eViewState::eViewState::unspecified, std::string forcedOwnername = "",bool showInGUI=false, bool noRendering=false, bool enforceLineWrapping=false);
		bool getIsTextualOutputEnabled();
		
		bool notifyCommitStatus(eCommitStatus::eCommitStatus = eCommitStatus::success, std::string receiptID="");
		//int64_t //(std::string question, int64_t defaultV = 1, std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false);
		BigSInt askInt(std::string question, BigSInt defaultV = 1, std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false);
		std::string askString(std::string question, std::string defaultV = "", std::string whoAsks = "", bool ommitBlockchainMode=false, bool ommitOwner=false);
		
		std::vector<uint8_t>  askBytes(std::string question, std::vector<uint8_t> defaultV = std::vector<uint8_t>(), std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false);

		bool askYesNo(std::string question, bool defaultV, std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false);
		std::weak_ptr<CDTI> mRemoteTerminal;
		std::shared_ptr<CCMDExecutor> mCMDExecutor;
		bool mInMaintenanceMode;
		std::vector<std::string> mProtectedVariables;
		std::vector<std::string> mNotTransferableVariables;
		uint64_t mKeyBlockHeight;
		uint64_t mLastPushedPrimitiveID;
		bool enterMaintenanceMode();
		bool exitMaintenanceMode();
		bool isInManintenanceMode();
		bool mGenerateBERMetaData;
		std::mutex mRecentBERMetaDataGuardian;
		std::vector<uint8_t> mRecentBERMetaData;
		void clearMetaData();
		std::unique_ptr<CVMMetaGenerator> mMetaGenerator;
		std::mutex mRegistryGuardian;
		void logMeIn();
		void showMsgBox(std::string msg, std::string title ="");
		void putIntoClipboard(std::string text);
		void debugui();
		void updateUI();
		//std::string mLoggedInAs; replace with a registry
		std::weak_ptr<CConversation> mDataDeliveryConversation;
		
		bool getIsLoggedIn();
	public:
		void setLoggedInAs(std::string userID);
		std::string getLoggedInAs();
		std::vector<std::shared_ptr<SE::CScriptEngine>> getThreads(bool onlyCommitable=false);
		void setDataDeliveryConversation(std::weak_ptr<CConversation> conv);
		std::shared_ptr<CConversation> getDataDeliveryConversation();
		std::vector<uint8_t> getEphPubKey();
		Botan::secure_vector<uint8_t> getEphPrivKey();
		uint64_t getRunningAppID();
		void setExternalData(std::vector<uint8_t> data);
		bool getIsAppRunning();
		bool getIsReadyForCommitment();
		void requestAppAbort();
		void quitCurrentApp();
		void setRemoteTerminal(std::shared_ptr<CDTI> terminal);
		size_t getQRWaitStartTime();
		
		BigFloat getBigFloat(AAddr cellAdr, bool popP);
		BigSInt getBigSInt(AAddr cellAdr, bool popP);
		BigInt getBigInt(AAddr cellAdr, bool popP=false);
		AAddr getdTop();
		size_t getMemSize(Cell start_address, bool onlyRequested);
		CD getCDD(AAddr cell);
		CD getCDR(AAddr cell);
		void setAllowLocalSecData(bool allow = true);
		bool getAllowLocalSecData();
		eCompileMode::eCompileMode getCompileMode();
		//bool getCompileToExplicitParams();
		// setCompileToExplicitParams(bool doIt = true);
		Xt findDefinition(const string & name);
		bool isValidDefinition(Xt xtRef);
		bool verifySacrificialTransactionSemantics(CTransaction t, uint64_t & value);
		bool findInvocation(std::vector<std::string >mTransactionLines, std::string functionName, std::vector<std::string> &stackParams, std::vector<std::string> &securityTokenParams, std::vector<std::string> &inlineParams, uint64_t &sourceLineBegin, uint64_t &sourceLineEnd);
		std::vector<uint8_t> getRecentReceiptID();
		bool getTransactionBegan();
		void revertStateAfterAppQuit();
		std::vector<uint8_t> getID();


	
		void getID(std::vector<uint8_t> id);

		void setID(std::vector<uint8_t> id);

		std::vector<std::string> getThreadLines(bool onlyIfReady = true);

		std::shared_ptr<CEndPoint> getAbstractEndpoint();
		bool refreshAbstractEndpoint(bool updateRT = true);
		std::shared_ptr<CNetTask> getCurrentVmTask();
		void abortCurrentVMTask();
		void showVmContextHelp();
		std::string getCurrentPath();
	private:

		// Key-Word State - BEGIN
		void setKeywordState(const std::shared_ptr<CKeyWordState>& state);
		std::shared_ptr<CKeyWordState> getKeywordState(const std::vector<uint8_t>& keywordStateID);
		// Map to store keyword states
		robin_hood::unordered_map<std::vector<uint8_t>, std::shared_ptr<CKeyWordState>> mKeywordStates;
		// Key-Word State - END

		uint64_t mPreCallLineIndex = 0;
		std::shared_ptr<CNetTask> mCurrentVMTask;
		void vmContext();

		// CVMContext (ECMA6) API Handlers - BEGIN

/**
 * @brief Retrieves and displays details of recent transactions in the blockchain and mempool.
 *
 * This method fetches a specified number of recent transactions, constructs
 * CTransactionDesc objects for each transaction, and either generates metadata
 * or displays the information textually in a table format.
 *
 * Pagination Mechanism:
 * 1. Total Transaction Count:
 *    - Mempool transactions count + On-chain transactions count
 *    - Used to calculate the total number of available pages
 *
 * 2. Page Calculation:
 *    - Start Index = (page - 1) * size
 *    - End Index = min(Start Index + size, Total Transactions)
 *    - Ensures that we don't exceed the total number of transactions
 *
 * 3. Transaction Retrieval:
 *    a. If Start Index < Mempool Transaction Count:
 *       - Retrieve transactions from mempool first
 *       - If End Index > Mempool Transaction Count, also fetch on-chain transactions
 *    b. If Start Index >= Mempool Transaction Count:
 *       - Retrieve only on-chain transactions
 *
 * 4. Edge Cases:
 *    - Handles cases where transactions span both mempool and on-chain data
 *    - Correctly processes pages with no transactions in either mempool or on-chain
 *    - Checks for invalid page numbers
 *
 * 5. Results:
 *    - Creates a CSearchResults object with:
 *      * Retrieved transaction descriptions
 *      * Total transaction count (for accurate pagination on frontend)
 *      * Current page number
 *      * Page size
 * 
 *   Mempool Transactions Presented First:
	Mempool transactions appear on the initial pages.
	Subsequent pages contain only on-chain transactions.
 *
 * @param size The number of transactions to retrieve per page.
 * @param page The page number to retrieve (starting from 1).
 * @param includeMemPoolTXs Whether to include tranasctions that are only in mem-pool.
 */
		void handleGetRecentTransactions(uint64_t size, uint64_t page, bool includeMemPoolTXs=true);
/**
 * @brief Generates and displays a security report for a specific block in the blockchain.
 *
 * This method retrieves the specified block, analyzes its security characteristics,
 * and presents a detailed security report. The report includes information about
 * potential timestamp manipulations, PoW anomalies, and other security-relevant metrics.
 * The output can be generated either as a formatted table for terminal display or as
 * BER-encoded metadata for programmatic processing.
 *
 * Key features:
 * - Analyzes block-specific security indicators
 * - Provides color-coded security assessments
 * - Supports both textual and BER metadata output formats
 * - Handles terminal width and wrapping preferences
 * - Includes proper error handling and logging
 *
 * @param blockIndex The index/height of the block to analyze in the blockchain
 * @param targetChainProof The chain proof to use (verified/cached or heaviest/cached)
 *
 * @throws std::runtime_error If the Blockchain Manager is not available
 * @throws std::runtime_error If the block retrieval fails
 * @throws std::runtime_error If block description generation fails
 *
 * Output format (table mode):
 * - Index: Sequential number of the security finding
 * - Security Analysis: Detailed description of the security concern or confirmation of safety
 *
 * BER metadata includes:
 * - Block identifier
 * - Security analysis results
 * - Severity indicators
 * - Timestamp analysis results
 */
		void handleBlockSecurityReport(uint64_t blockIndex, eChainProof::eChainProof targetChainProof = eChainProof::verifiedCached);
		/**
		 * @brief Generates and displays a comprehensive security report for all operators in the network.
		 *
		 * This method analyzes the entire blockchain, compiles security statistics for all operators,
		 * and presents a paginated report of security metrics. Operators are sorted by their
		 * "maliciousness score" (combined count of timestamp manipulations and PoW wave attacks),
		 * with the most concerning operators displayed first.
		 *
		 * Key features:
		 * - Aggregates security metrics across the entire blockchain
		 * - Supports pagination for large datasets
		 * - Provides color-coded severity indicators
		 * - Sorts operators by combined security risk score
		 * - Supports both textual and BER metadata output formats
		 * - Includes detailed reporting of specific security incidents
		 *
		 * @param pageSize Number of operators to display per page
		 * @param pageNumber The page number to display (1-based indexing)
		 *
		 * @throws std::runtime_error If the Blockchain Manager is not available
		 * @throws std::runtime_error If the page number exceeds available pages
		 *
		 * Output format (table mode):
		 * - Operator ID: Base58-encoded identifier of the operator
		 * - Timestamp Manipulations: Count of detected timestamp manipulation attempts
		 * - PoW Wave Attacks: Count of detected Proof-of-Work wave attack attempts
		 * - Total Score: Combined security risk score (color-coded by severity)
		 * - Detailed Reports: Specific security incidents and analyses
		 *
		 * BER metadata includes:
		 * - Operator identifiers
		 * - Security metrics
		 * - Detailed incident reports
		 * - Pagination information
		 * - Total operator count
		 *
		 * Security scoring:
		 * - Red: High risk (score > 10)
		 * - Orange: Medium risk (score 6-10)
		 * - Green: Low risk (score 0-5)
		 */
		void handleGlobalSecurityReport(uint64_t pageSize = 10,
			uint64_t pageNumber = 1,
			uint64_t depth = 0,
			int minTotalOffenses = 0,
			int minTimestampManipulationCount = 0,
			int minPowWaveAttackCount = 0,
			eSecurityReportSortMode::eSecurityReportSortMode sortMode = eSecurityReportSortMode::confidenceDesc);

		void handleGetRecentBlocks(uint64_t size, uint64_t page);

/* @brief Retrieves and displays daily transaction statistics for the past 'DAYS' days.
*
* This method fetches transaction counts for each of the past 'DAYS' days,
* computing the exact number of transactions that occurred on each individual day.
* The processing is performed using multi-threading for efficiency.
* The results are either provided as metadata or displayed in a formatted table.
*
* @param DAYS The number of past days to retrieve statistics for.
* @param numThreads The number of threads to use for processing. If 1, processing occurs in the main thread.
* @param threadPool Optional thread pool to use for processing. If not provided and numThreads > 1, a new thread pool is created.
*/
		void handleGetTransactionDailyStats(uint64_t DAYS= 14, size_t numThreads = 1, std::shared_ptr<ThreadPool> threadPool=nullptr);
		/**
			*@brief Handles the retrieval and reporting of the blockchain's current status, including the total GNC supply, blockchain height, and key height.
			*
			*This method fetches critical blockchain status information such as the total supply of GNC(Green Network Coin), the current blockchain height,
			* and the key height.It obtains this data by querying the `CBlockchainManager` and `CStateDomainManager`.
			*
			* Depending on the execution context, the method reports the data either via the BER(Basic Encoding Rules) encoded Meta Data exchange sub - protocol
			* or directly to the terminal if the command is executed from a terminal session.
			*
			* The specific data reported includes :
		*-**Total Supply * *: The total supply of GNC across all State Domains.
			* -**Blockchain Height * *: The current height of the blockchain(the total number of blocks in the chain).
			* -**Key Height * *: The height associated with the current key block.
			*
			*@note Data Reporting :
		*-If BER Meta Data generation is enabled(via `getGenerateBERMetaData()`), the method will add the blockchain status to the Meta Data
			* using the Meta Generator(`mMetaGenerator- > addBlockchainStatus()`).
				* -If the method is invoked in a terminal context(i.e., Meta Data generation is disabled), the blockchain status will be printed
				* directly to the terminal using `writeLine()`, showing:
		*-Total Supply in GNC
			* -Blockchain Height
			* -Key Height
			*
			*@return void This method does not return any data directly.The data is either encoded into BER Meta Data or printed to the terminal.
			*
			*@warning The method performs no action if the Blockchain Manager(`CBlockchainManager`) or State Domain Manager(`CStateDomainManager`) is unavailable.
				*/
		void handleGetBlockchainStatus();
		void handleSearchBlockchain(const std::string& query, uint64_t size, uint64_t page, std::shared_ptr<CSearchFilter> filter);
		uint64_t getResultTimestamp(const CSearchResults::ResultData& result);
/**
 * @brief Handles a paginated search request across the blockchain, including transactions, blocks, and domains.
 *
 * This method performs a comprehensive search operation on the blockchain data based on the provided query and filter.
 * It utilizes a session-based state management system to maintain search progress across multiple invocations,
 * enabling efficient pagination and resumption of searches. The method employs multiple specialized iterators
 * (TransactionSearchIterator, BlockSearchIterator, DomainSearchIterator) to concurrently search different
 * data categories and merge results based on their timestamps.
 *
 * Key features:
 * - Utilizes session state (CSearchBlockchainState) to store and retrieve search cursors.
 * - Implements pagination to limit the number of results returned per invocation.
 * - Merges results from multiple iterators using a priority queue to ensure proper ordering.
 * - Supports both textual output and BER metadata generation for result presentation.
 *
 * @param query The search query string to match against blockchain data.
 * @param size The number of results to return per page.
 * @param page The current page number of results to retrieve.
 * @param filter A shared pointer to a CSearchFilter object specifying which data categories to include in the search.
 * @param useSession When set the state of Iterators would be restored based on prior search requests
 * @param sessionID Use a custom session identifier. By default, a session identifier is based on search request.
 * @note This method relies on several member variables and methods of the CScriptEngine class, including:
 *       - getBlockchainManager()
 *       - getRemoteTerminal()
 *       - getKeywordState()
 *       - setKeywordState()
 *       - getGenerateBERMetaData()
 *       - getDataDeliveryConversation()
 *       - getMetaRequestID()
 *       - getUIProcessID()
 *       - getID()
 *       - getIsTextualOutputEnabled()
 *       - writeLine()
 *
 * @warning This method assumes that the necessary managers (BlockchainManager, StateDomainManager) are available
 *          and properly initialized. It includes error handling for cases where these dependencies are not met.
 */
		void handleSearchBlockchainIt(const std::string& query, uint64_t size, uint64_t page, std::shared_ptr<CSearchFilter> filter, bool useSession, std::vector<uint8_t> sessionID);
		void handleGetNetworkUtilization24h();
		void handleGetBlockSize24h();
		void handleGetBlockRewards24h();
		void handleGetAverageBlockTime24h(bool keyOnly=true);
		void handleGetBlockDetails(const std::string& blockID, bool includeTXdetails=false, bool includeSecReport=false);
		//std::shared_ptr<CSearchResults> CScriptEngine::handleGetDomainHistory(const std::string& address, const uint64_t size, const uint64_t page, const bool includeDetails)
		std::shared_ptr<CSearchResults>  handleGetDomainHistory(const std::string& address, const uint64_t size=10, const uint64_t page=1, const bool includeDetails=true);
	/**
	 * @brief Retrieves and displays metadata details for a State Domain
	 *
	 * This method fetches metadata associated with a State Domain (account) on the
	 * GRINDET OS decentralized operating system. If executed from the network,
	 * metadata is returned. If executed from the command line, results are written
	 * to the terminal. The State Domain is retrieved only after the perspective has been set.
	 *
	 * @param address The address of the State Domain to retrieve details for
	 * @param perspectiveA The perspective from which to view the State Domain data
	 */
		void handleGetDomainDetails(const std::string& address, bool includeSecurityData =false,const std::vector<uint8_t>& perspectiveA= std::vector <uint8_t>());

		void handleGetTransactionDetails(const std::string& transactionID);

	

	/**
	 * @brief Retrieves and displays the current network liveness state.
	 *
	 * This method fetches the network liveness state and either generates metadata
	 * or displays the information textually in a table format.
	 */
		void handleGetLiveness();
		void handleGetUSDTPrice();
		void handleGetHeight();
		// CVMContext (ECMA6) API Handlers - END

		void setCurrentVMTask(std::shared_ptr<CNetTask> task);
		std::mutex mCurrentVMTaskGuardian;

		//EXTREME CAUTION - BEGIN
		std::vector<uint8_t> mUserModeRegisters{REG_DO_NOT_COMPILE_READS, REG_THREAD_PAUSED, REG_HIGH_PRECISION, REG_VIEW_CAN_OVERFLOW, REG_IS_THREAD_UI_AWARE};
		//EXTREME CAUTION - END

		bool isUserModeRegister(uint8_t reg);
		uint8_t regNameToUInt(std::string regName);
		std::shared_ptr<CEndPoint> mAbstractEndpoint;
		std::mutex mAbstarctEndpointGuardian;
	
		void setBlockchainManager(std::shared_ptr<CBlockchainManager> bm);
		void freeThread();
		std::mutex mThreadIDGuardian;
		std::vector<std::shared_ptr<SE::CScriptEngine>> mThreads;// representing Decentralized Processing Threads
		uint64_t mThreadID;
		uint64_t mUIProcessID;//that's for informative purposes only. Any process MAY take ownership of this thread anytime.
		std::recursive_mutex mThreadLinesGuardian;//little bit inefficiency won't hurt us that much...for now.
		std::vector<uint8_t> mID;
		bool mDuringAuth;//to detect when exception thrown during AUTH (secToken verfiication)
		bool keepAppRunning();
		std::mutex mNativeActiveAppGuardian;
		std::mutex mInstructionGuardian;
		std::mutex mIDGuardian;
		std::vector<uint8_t> mRecentReceiptID;
		void setRecentReceiptID(std::vector<uint8_t> id= std::vector<uint8_t>());
		std::mutex mRecentReceiptIDGuardian;
		std::shared_ptr<CGridScriptCompiler> mCompiler;
		std::recursive_mutex mGuardian;
		std::mutex mMemoryGuardian;
		std::mutex mScriptProcessingGuardian;
		std::recursive_mutex mVMSettingsGuardian;
		std::recursive_mutex mCommitTargetGuardian;
		eBlockchainMode::eBlockchainMode mCommitTarget;
		eBlockchainMode::eBlockchainMode mBlockchainMode;
		bool isDataUploadFunction(std::string functionName);
		bool mAllowLocalSecDataAccess;
		std::mutex mTransactionsManagerGuardian;
		std::mutex mBlockchainManagerGuardian;
		std::mutex mStateDomainsManagerGuardian;
		bool mTransactionBegan;
		bool mRAWCodeBegan;
		uint64_t mRAWCodeBeginsAt;
		uint64_t mRAWCodeEndsAt;
		uint64_t mCurrentLine;
		bool mReplaceModeEnabled;
		uint64_t mWordsPushedToTransaction;
		//bool mCompileToExplicitParameters;
		eCompileMode::eCompileMode mCompileAuthMode;
		std::vector<std::string> mThreadLines;
		uint64_t preRecentCallLinesIndex;// used by 'thread.includeSelf()' GridScript++ method. 
		void clearRecentPreCallLineIndex();
		void clearRecentCallLines();//if dApp does not decide to keep these
		void markPreCallLineIndex();

		std::vector<std::string> getRecentCallLines();
		 
		bool replaceFunctionInSource(std::string toBeReplaced, std::string newFunction,bool addIfMissing=false, uint64_t overrideStackDummyParams=0);
		bool addDataPushToSource(uint64_t val);
		bool addDataPushToSource(std::vector<uint8_t> value, eParamInsertionMethod::eParamInsertionMethod method);
		bool setNulledParam(std::string functionName, uint64_t value, uint32_t paramIndex = 0, bool isSecTokenParam = false,eParamInsertionMethod::eParamInsertionMethod method= eParamInsertionMethod::eParamInsertionMethod::number, bool affectERGUsageEstimate = true, uint64_t overridenNrofStackParams = 0);
		bool setNulledParam(std::string functionName, BigInt value, uint32_t paramIndex = 0, bool isSecTokenParam = false, eParamInsertionMethod::eParamInsertionMethod method = eParamInsertionMethod::eParamInsertionMethod::number, bool affectERGUsageEstimate = true, uint64_t overridenNrofStackParams = 0);
		bool setNulledParam(std::string functionName, BigSInt value, uint32_t paramIndex = 0, bool isSecTokenParam = false, eParamInsertionMethod::eParamInsertionMethod method = eParamInsertionMethod::eParamInsertionMethod::number, bool affectERGUsageEstimate = true, uint64_t overridenNrofStackParams = 0);
		bool setNulledParam(std::string functionName, std::vector<uint8_t> value, uint32_t paramIndex = 0, bool isSecTokenParam = false, eParamInsertionMethod::eParamInsertionMethod method =eParamInsertionMethod::eParamInsertionMethod::number, bool affectERGUsageEstimate = true, uint64_t overridenNrofStackParams = 0);
		bool setNulledParam(std::string functionName,std::string value, uint32_t paramIndex = 0, bool isSecTokenParam = false,eParamInsertionMethod::eParamInsertionMethod method= eParamInsertionMethod::eParamInsertionMethod::RAWString,bool affectERGUsageEstimate=true,uint64_t overridenNrofStackParams=0);
		void popSecurityToken();
		bool verifySecurityTokenEx(bool updateNonce = true, uint32_t paramsCount = 0, bool popToken = false, CTrieNode* accessedObject = nullptr, bool accessingAsSystem=false, uint32_t tokenAtStackDepth = 0, const std::shared_ptr<CAccessToken> &accessToken = nullptr);
		CDataTrieDB * mCurrentDir;
		CStateDomain * mCurrentStateDomain;
		::eViewState::eViewState mTargetOutputView;
		bool mIsReady = false;
		bool mConsumeERG;
		//uint64_t mAllowedCoinMintValuePool;
		std::vector<CVerifiable> mVerifiablesCount;//verified verifiables. i.e. proofs of certains events. used by coinmint etc. Verfifiables need to be verified first by CVerifier
		std::shared_ptr<CWorkManager> mWorkManager;
		std::unique_ptr<CSettings> mSettings;
		bool mStateDomainsAltered = false;
		
		void checkedMemCpy(void* _Dst, void const* _Src, size_t _Size);
		bool mInSandBoxMode; // sandbox mode does not perform any writes to cold storage(HDD) only RAM data structures are affected
		std::weak_ptr<CTransactionManager> mTransactionManager;//that ualways used to be a weak relationshop (used to be a raw pointer).
		std::shared_ptr<CTransactionManager> mOwnTransactionManager;
		::std::shared_ptr<CBlockchainManager>mBlockchainManager;
		::std::unique_ptr<CGlobalSecSettings> mSecSettings;
		std::array<std::vector<uint8_t>, REGISTERS_COUNT> mRegisters;
		std::shared_ptr<CTools> mTools; 
		::std::shared_ptr<CStateDomainManager> mStateDomainManager;
		::CTests * mTests;
		bool mIsTestNet = false;
		bool mExecutingFromTerminal;
		bool mIsGUITerminalAttached;

		std::vector<uint8_t> mExecutingStateDomainID;
		std::shared_ptr<GridScriptPPEngine> mScriptPP;
		BigInt mERGLimit = 0;
		uint64_t mUsedMemory = 0;
		uint64_t mMemoryLimit;
		
		std::vector<memory_region> registeredMemoryRegions;
		BigInt mERGUsed = 0;
		BigInt mFinalTransactionERGOverhead;//signed on purpose as terminal-only commands are deducted from the estimtated ERG cost
#ifdef GRIDSCRIPT_SKIP_RUNTIME_CHECKS

#define RUNTIME_NO_OP()           do { } while (0)
#define RUNTIME_ERROR(msg)          RUNTIME_NO_OP()
#define RUNTIME_ERROR_IF(cond, msg)     RUNTIME_NO_OP()
#define REQUIRE_DSTACK_DEPTH(n, name)    RUNTIME_NO_OP()
#define REQUIRE_DSTACK_AVAILABLE(n, name)  RUNTIME_NO_OP()
#define REQUIRE_RSTACK_DEPTH(n, name)    RUNTIME_NO_OP()
#define REQUIRE_RSTACK_AVAILABLE(n, name)  RUNTIME_NO_OP()
#define REQUIRE_ALIGNED(addr, name)     RUNTIME_NO_OP()
#define REQUIRE_VALID_HERE(name)       RUNTIME_NO_OP()
#define REQUIRE_DATASPACE_AVAILABLE(n, name) RUNTIME_NO_OP()

#else

#define RUNTIME_ERROR(msg)          do {mStateDomainsAltered =false; throw AbortException(msg); } while (0)
#define CHECK_SEC_DATA_ACCESS				 do { if (!getAllowLocalSecData() ) RUNTIME_ERROR("Access to the local Security-Data-Store is DISABLED!"); } while (0)
#define RUNTIME_ERROR_IF(cond, msg)     do { if (cond) RUNTIME_ERROR(msg); } while (0)

#define HANDLE_ERROR_IF(condition, msg, throwIt) HANDLE_ERROR_IF_NOTIFY(condition, msg, throwIt, true);

#define HANDLE_ERROR_IF_NOTIFY(condition, msg, throwIt, notifyConversation) if(handleErrorIf(condition,msg,throwIt, notifyConversation)) {\
if(!getIsGUITerminalAttached())\
		writeLine(mTools->getColoredString(msg, eColor::cyborgBlood), true, false, eViewState::GridScriptConsole, "Write"); \
		return;} //WARNING: the macro does NOT support optional parameters
		//thus for accessing the additional parameters(status,scope,conversation) use the underlying handleErrorIf() maintain EXTREME caution to handle its result.

#define HANDLE_ERROR(msg, throwIt)			 handleError(msg,throwIt); return; 
#define REQUIRE_DSTACK_DEPTH(n, name)    requireDStackDepth(n, name)
#define REQUIRE_DSTACK_AVAILABLE(n, name)  requireDStackAvailable(n, name)
#define REQUIRE_DSTACK_TYPE(n, name)     requireDStackType(n, name)
#define REQUIRE_RSTACK_TYPE(n, name)     requireRStackType(n, name)
#define REQUIRE_RSTACK_DEPTH(n, name)    requireRStackDepth(n, name)
#define REQUIRE_RSTACK_AVAILABLE(n, name)  requireRStackAvailable(n, name)
#define REQUIRE_ALIGNED(addr, name)     checkAligned(addr, name)

#define REQUIRE_BLOCKCHAIN_HEIGHT(n) RUNTIME_ERROR_IF(mBlockchainManager->getHeight()!=n,"This can be executed only at blockchain-height: "+ std::to_string(n))
#define REQUIRE_BLOCKCHAIN_KEY_HEIGHT(n) RUNTIME_ERROR_IF(mBlockchainManager->getKeyHeight()!=n,"This can be executed only at blockchain-key-height: "+ std::to_string(n))

//Special registers
#define REQUIRE_REG(n)           requireRegN(n)
#define CLEAR_REG(n)            clearRegN(n)
		
//clear special registers

#define REQUIRE_VALID_HERE(name)       checkValidHere(name)
#define REQUIRE_DATASPACE_AVAILABLE(n, name) requireDataSpaceAvailable(n, name)
#define REQUIRE_MEM_REGISTERED(addr) RUNTIME_ERROR_IF(getMemSize((unsigned int)addr,true) <1, "DATA: memory overflow");
#endif
		/****

		The input buffer is a `std::string`. This makes it easy to use the C++ I/O
		facilities, and frees me from the need to allocate a statically sized buffer
		that could overflow. I also have a current offset into this buffer,
		corresponding to the Forth `>IN` variable.

		****/

		string sourceBuffer;
		Cell sourceOffset = 0;
		Cell currentExtWordStart = 0;//indicated the offset at which the current extended (supporitng inline parameters word began).

		/****

		I need a buffer to store the result of the Forth `WORD` word. As with the
		input buffer, I use a `string` so I don't need to worry about memory
		management.

		Note that while this is a `std:string`, its format is not a typical strings.
		The buffer returned by `WORD` has the word length as its first character.
		That is, it is a Forth _counted string_.

		****/

		string wordBuffer;

		/****

		I need a buffer to store the result of the Forth `PARSE` word. Unlike `WORD`,
		this is a "normal" string and I won't need to store the count at the beginning
		of this buffer.

		****/

		string parseBuffer;

		/****

		I store the `argc` and `argv` values passed to `main()` so I can make them
		available for use by the Forth program by our non-standard `#ARGS` and `ARG`
		Forth words, defined later.

		****/

		size_t commandLineArgCount = 0;
		const char** commandLineArgVector = nullptr;

		/****

		I need a variable to store the result of the last call to `SYSTEM`, which
		the user can retrieve by using `$?`.

		****/

		int systemResult = 0;

		/****

		I need a flag to track whether we are in interpreting or compiling state.
		This corresponds to Forth's `STATE` variable.

		****/

		Cell isCompiling = False;
		Cell afterFatalError = False;

		/****

		I provide a variable that controls the numeric base used for conversion
		between numbers and text. This corresponds to the Forth `BASE` variable.

		Whenever using C++ stream output operators, I will need to ensure the stream's
		numeric output base matches `numericBase`. To make this easy, I'll define a
		macro `SETBASE()` that calls the `std::setbase` I/O manipulator and use it
		whenever writing numeric data using the stream operators.

		****/
		
		Cell numericBase = 10;

#define SETBASE() std::setbase(static_cast<int>(numericBase))

	Xt doLiteralXt = nullptr;
	Xt doSignedLiteralXt = nullptr;
	Xt doUnsignedLiteralXt = nullptr;
	Xt doDoubleLiteralXt = nullptr;
	Xt setDoesXt = nullptr;
	Xt exitXt = nullptr;
	Xt endOfDefinitionXt = nullptr;
		Char dataSpace[GRIDSCRIPT_DATASPACE_SIZE];
		Cell dStack[GRIDSCRIPT_DSTACK_COUNT];
		Cell rStack[GRIDSCRIPT_RSTACK_COUNT];

		//meta-data (for debugging)
		CD dStackMeta[GRIDSCRIPT_DSTACK_COUNT];
		CD rStackMeta[GRIDSCRIPT_RSTACK_COUNT];

		CAddr dataSpaceLimit = &dataSpace[GRIDSCRIPT_DATASPACE_SIZE];
		AAddr dStackLimit = &dStack[GRIDSCRIPT_DSTACK_COUNT];
		AAddr rStackLimit = &rStack[GRIDSCRIPT_RSTACK_COUNT];

		void registerMemoryRegion(memory_region region, bool selfAllocated=false);
	
	
	
		void unregisterMemoryRegion(Cell memoryStartAddress);
		CAddr dataPointer = nullptr;
		CAddr mBackedUpNativeDataSpacePointer = nullptr;
		AAddr dTop = nullptr;
		AAddr rTop = nullptr;

		int64_t dTopMeta = -1;
		int64_t rTopMeta = -1;

		Xt* nextInstruction = nullptr;
		
		std::string dataTypeToString(eDataType::eDataType type);
		void state();
		void doCreate();
		bool isRegisteredDefinition(Xt def);
		void create();
		void sig();
		void check_sig();
		void hex();
		void data64();
		void adata64();
		void adata58();
		void data();
		void adata();
		std::vector<std::string> getAllInLineParams(bool pop=true);
		std::vector<std::tuple<std::string, std::string>> getAllInLineParamsNames(bool pop=true);
		bool getInlineParamVal(std::string paramName, const std::string &val= std::string(),bool pop=true, bool requestRealString=false);
		std::vector<uint8_t> getInlineParam(uint64_t paramIndex =0);
		uint64_t getInlineUInt(uint64_t paramIndex=0,bool pop=true);
		bool getInlineBool(uint64_t paramIndex = 0,bool pop=true);

		std::string getInlineString(uint64_t paramIndex=0, bool pop=true);
		BigInt getInlineBigInt(uint64_t paramIndex = 0,bool pop=true);
		bool pushInlineParam();
		void addr();
		void checkAddr();
		void ultimium();
		void colon();
		void noname();
		void 
			doDoes();
		void setDoes();
		void does();
		void endOfDefinition();
		void 
			semicolon();
		void 
			immediate();
		void hidden();
		void doLiteral();
		void doSignedLiteral();
		void doUnsignedLiteral();
		void doDoubleLiteral();
		void branch();
		void zbranch();
		void doParams();
		void doW();
		void loop();
		void ploop();
		void i();
		void j();
	
		void definePrimitive(uint64_t id, bool allowedInKernelMode, bool requiresLocalAdmin, const char * name, Code code,unsigned int gasCost=1,bool extType=false,bool terminalOnly=false,bool reqSecToken=false,unsigned int exByteCode=0,unsigned int reqStackWordsProceeding=0,bool doNotIncludeInSource=false, unsigned int inlineParamsCount=0,bool hasBase58BinaryInLineParams=false,bool allowToBeExecutedFromRAWCodeMode=false, bool overwatchOnly = false, bool hasBase64BinaryInLineParams = false, bool isDataReadFunction = false);
		bool doNamesMatch(CAddr name1, CAddr name2, Cell nameLength);

		Xt findDefinition(CAddr nameToFind, Cell nameLength);
		
		void find();
		void execute();
		void toBody();
	
		void xtToName();
		void words();
		Xt findXt(Cell x);
		void seeDoes(AAddr does);
		void see();
		uint64_t mNestedAppDestructors = 0;
		void evalGPPFromGS1();
		bool isValidDigit(Char c);
		Cell digitValue(Char c);
		void parseUnsignedNumber();
		void parseSignedNumber();
		void parseFloat();
		void interpret();
		bool notifyMobileApp(eOperationStatus::eOperationStatus status , eOperationScope::eOperationScope scope = eOperationScope::peer, std::string notes="", std::vector<uint8_t> ID=std::vector<uint8_t>(), uint64_t reqID=0);
		void evaluate();
		void prompt();
		void quit();
		void readOnly();
		void readWrite();
		void writeOnly();
		void bin();
		void createFile();
		void openFile();
		void readFile();
		void readLine();
		void readChar();
		void writeFile();
		void writeLine();
		void writeChar();
		void flushFile();
		void closeFile();
		void deleteFile();
		void renameFile();
		void includeFile();
		void callTransferEx();
		void pushArgument(eDataType::eDataType dt, const std::string& argName,const std::string& arg, const std::string& funcName, int i, uint64_t stubParamsCount);
		void callTransfer();
		void pushStrArgToStack(eDataType::eDataType dt, std::string value, std::string funcName, int argIndex, uint64_t stubParamsCount);
		void callEx();
		void call();
		void endCall();

#ifndef GRIDSCRIPT_DISABLE_MEMORY_ALLOCATION 
		void memclr();
		void memAllocate();
		void memResize();
		void memFree();
#endif
		void flag();
		void clearStacks();
		void clearRegisters();
		void viewRegisters();
		void definePrimitives();
		void setfacl();
		void poll();
		void pollEx();
		void getmeta();
		void setmeta();
		void setMetaEx();
		void updateObjSecDesc(CTrieNode* managedObject, std::shared_ptr<CSecDescriptor>& token, std::vector<uint8_t>& perspective, size_t& cost, std::vector<uint8_t>& path, std::vector<uint8_t>& currentDir, const std::shared_ptr<CAccessToken>& accessToken);
		void handleListBreakpoints(std::shared_ptr<CBreakpointFactory> factory);
		void handleBreakpointStats(std::shared_ptr<CBreakpointFactory> factory);
		void handleBreakpointCreation(std::shared_ptr<CBreakpointFactory> factory, const std::string& type, std::string value);
		void handleClearBreakpoints(std::shared_ptr<CBreakpointFactory> factory);
		void handleActivateBreakpoints(std::shared_ptr<CBreakpointFactory> factory);
		void handleDeactivateBreakpoints(std::shared_ptr<CBreakpointFactory> factory);
		void grid();
		void chain();
		void hotStart();
		void chown();
		void chownEx();
		void setfaclEx();
		void chmodEx();
		void getReg();
		void getExecutingID();
		void getReceivedGNC();
		void getTransactionResult();
		void exitStateDomain();
		void setCompileMode();
		void getTransactionStatistics();
		void defineGridScriptWords();
		size_t parseDefMemRegions();
		void initializeDefinitions();
		
		std::string getThreadSource(uint64_t fromLine = 0, uint64_t tillLine = 0, bool prettyPrint = false);
		std::string getCodeFromAllThreads(bool ensureScopeProtection=true, const BigInt& ERGEstimate=0, const std::vector<std::shared_ptr<SE::CScriptEngine>> &vmS= std::vector<std::shared_ptr<SE::CScriptEngine>>(), bool prettyPrint=false, bool onlyReadyThreads=true, bool executedSoFar=false);
	
		void resetDStack();
		void resetRStack();
		ptrdiff_t dStackDepth();
		ptrdiff_t rStackDepth();
		void push(BigInt x, CD meta = CD("unknown", eDataType::eDataType::BigInt), bool notify = false);
		void push(Cell x,CD meta=CD("unknown", eDataType::eDataType::unsignedInteger),bool notify=false);
		void pop();
		void rpush(Cell x,CD description);
		void rpop();
		void abort();
		void abortMessage();
		void c_assert();
		void c_assert0();
		void depth();
		void drop();
		void pick();
		void roll();
		void toR();
		void rFrom();
		void rFetch();
		void exit();
		void store();
		void fetch();
		void cstore();
		void cfetch();
		void count();
		void alignDataPointer();
		void align();
		void aligned();
		void here();
		void allot();
		void cells();
		void data(Cell x);
		void unused();
		void cMove();
		void cMoveUp();
		void fill();
		void compare();
		void fcompare();
		void key();
		void emitGS();
		void type();
		void cr();
		void dot();
		void uDot();
		void dotR();
		void base();
		void source();
		void toIn();
		void refill();
		void accept();
		void word();
		void parse();
		void bl();
		void plus();
		void minus();
		void arithmeticOperation2(eArithOperation oType);
		void arithmeticOperation(eArithOperation type);
		void star();
		void slash();
		void mod();
		void slashMod();
		void bitwiseAnd();
		void bitwiseOr();
		
		//decentralized virtual file-system commands
		void setKey();
		void setKeyChain();
		bool checkAccessToLocalDataStore();
		void getKey();
		void dir();
		void ls();
		void showMarketHelp();
		void market();
		void displayMarketResults(const std::shared_ptr<CSearchResults>& results, const BigInt& totalGNC, bool showTotalDepth, bool accountsWithBalances, const std::shared_ptr<CDTI>& dti);
		void rm();
		void touch();
		void echo();
		void echoEx();
		void time();
		void sudo();
		void route();
		void cat();
		void makeDirectory();
		void ffind();
		void ffindEx();
		std::vector<uint8_t> getAuthenticatedStateDomainID();
		std::vector<uint8_t> getStateDomainIDFromSecToken();
		std::vector<uint8_t> getPubKeyFromSecToken();
		uint64_t getNonceFromSecToken();
		uint64_t getFunctionIDFromSecToken();
		std::vector<uint8_t> getSignatureFromSecToken();
		bool prepareAndPushSecToken(eCompileMode::eCompileMode mode,std::string functionName, std::vector<uint8_t> stateDomainID = std::vector<uint8_t>(),
			std::vector<uint8_t> pubKey= std::vector<uint8_t>(),uint64_t nonce=0, std::vector<uint8_t> signature = std::vector<uint8_t>());
	
		void pushEmptySecToken();
		void flagEx();
		void makeDirectoryEx();
		void rmEx();
		void changeDirectory();
		void exportF();

		void csds();
		std::string getNewLine(bool forWeb=false);
		bool isInTextTerminal();
		void csd();
		bool isLocalTerminal();
		void more();
		void locate();

		void bitwiseXor();
		void lshift();
		void rshift();
		void equals();
		void lessThan();
		void uLessThan();
		void timeAndDate();
		void utcTimeAndDate();
		void dotS();
		void dotM();
		void dotRS();
		void ERGLeft();
		void memLeft();
		void doColon();

		// local State Domain storage
		void setVar();
		void setVarEx();
		void getVar();
		void getVarEx();
		bool validateAccessBoxed(CStateDomain* domain, std::string& path, std::shared_ptr<CAccessToken>& accessToken, std::vector<uint8_t>& currentDir);
		void nonce();
		//EXTREME WARNING: under no condition, do not EVER change the DEFAULT value of the throwIt parameter below:
		bool handleErrorIf(bool condition, std::string msg, bool throwIt = true, bool notifyConversation = true,eOperationStatus::eOperationStatus status = eOperationStatus::failure, eOperationScope::eOperationScope scope = eOperationScope::VM, std::shared_ptr<CConversation> conversation = nullptr);
		//EXTREME WARNING: under no condition, do not EVER change the DEFAULT value of the throwIt parameter below:
		void handleError(std::string msg, bool throwIt=true, bool notifyConversation = true, eOperationStatus::eOperationStatus status = eOperationStatus::failure, eOperationScope::eOperationScope scope = eOperationScope::VM, std::shared_ptr<CConversation> conversation= nullptr);
	
		void less();
		void minecraftMove();
		void tail();
		void importF();
	
		
		void getVarDepracated();

		void getData();
		void setData();
		//inter-domain value transfer
		void xValue();
		void send();
		void sacrifice();
	
		void sacrificeEx();
		void ERGUsage();
		void ERGUsageS();
		void txconfig();
		void compiler();
		void compilerEx();
		void sendEx();
		void pops(uint32_t popCount = 1);
		void takeCareOfSecToken(bool updateNonce=true,uint32_t nrOfParams=0);
		std::vector<uint8_t> getVecBase58Check(AAddr cellAdr,bool &ok);
		//terminal only commands
		void getChain();
		void bye();
		bool addCodeWord(std::string word, uint64_t lineNr=0);

	

		void readyThread();
		bool notifyConsensusTask(eConsensusTaskType::eConsensusTaskType type, std::vector<uint8_t> threadID = std::vector<uint8_t>(), std::string text = "", bool blocking = true);
		void beginThread();
		void beginRAWCodeForumulation();
		uint64_t getRAWCodeEndsAt();
		uint64_t getRAWCodeBeginsAt();
		void goToLine();
		void insert();
		void warnings();
		void difficulty();
		void endRAWCodeForumulation();
		void viewThreadsInstructions();
		void clean();
		void doVMMaintenance();
		void update();
		void doBackup();
		void updateSystemDomains();
		void beginTransactionReset();
		void proxy();
		void firewall();
		void cstorage();
		void version();
		void overwatch();
		void saveCode();
		void clearCode();
		void wall();
		void nick();
		void whoamI();
		void who();
		void write();
		
		void saveCodeEx();
		void commitThreads();
		
		bool validateWorkspaceDimensionsForQR(std::shared_ptr<CDTI>& remoteTerminal, const uint64_t& QRWidth, const uint64_t& QRHeight);
		void abortThread();
		void exitTransactionFormation();
		void setCommitTarget();
		void sync();
		void getCommitTarget();

	

		void clearCommitTarget();
		//conditional script abortion
		void anz();
		void az();
		void keychain();
		void getNextID();
		void setActiveChainID();
		void templateFunction();
		void net();
		void genKey();
		void genPool();
		void getPool();
		void getPoolEx();
		void processTransmissionToken();
		void processTransmissionTokenEx();
		void genPoolEx();

		void regPool();
		void regPoolEx();
		void genID();
		void genIDEx();
		void markSacrificialTXAsConsumed(std::string receiptID);
		bool saveSysValue(eSysDir::eSysDir dir, std::vector<uint8_t> key, std::vector<uint8_t> value,uint64_t &cost, bool revertPath=true);
		std::vector<uint8_t> loadSysValue(eSysDir::eSysDir dir, std::vector<uint8_t> key, eDataType::eDataType& vt);
		bool wasSacrificialTXConsumed(std::string receiptID);
		void regID();
	
	
		Definition& lastDefinition();
		void latest();
		Botan::AutoSeeded_RNG rng;
		std::shared_ptr<CCryptoFactory> mCryptoFactory;
	
		//debuggin/
	void getSDID();
	void getSDCredentials();
	void enc_auth_x();
	void enc_auth_s();
	void enc_s();
	void enc_x();
	void dec_x();
	void dec_s();
	void sign();
	void dotM58();
	void getPubX();
	void getPub();
	void info();
	void getfacl();
	void balance();

	void receipts();

	void transactions();
	
	std::vector<uint8_t> getThis();
	void concat();
	void concat2();
	void startFlowS();
	void endFlowS();
	void stepBack();
	void setPerspective();
	void getPerspective();
	void versig();
	void setExecutingStateDomain(std::vector<uint8_t> accountID);
	void setExecutingStateDomain(CStateDomain * domain);
	void setCurrentStateDomain(CStateDomain * domain);
	CStateDomain * getCurrentStateDomain();
	
	void setCurrentDir(CDataTrieDB * dir);
	bool setCurrentDir(std::string path,const std::string &errorMsg= std::string());

	CDataTrieDB * getCurrentDir();
	eDataType::eDataType inferDataTypeFromString(std::string str, Cell & value, const bool& isBase64=false);
	//Computational Platform
	void getTaskStatus();
	void kill();
	void pause();
	void calculatePerformance();
	void workersStat();
	void getWorkResults();
	void genPoW();
	void getTasks();
	void getNonce();
	void* createMemory(std::string data);
	void * createMemory(std::vector<uint8_t> data);
	void keygen_s();
	void keygen_x();
	bool isATransferableVariable(std::string varName);
	void chacha();
	std::mutex mConversationGuardian;
	std::weak_ptr<CConversation> mConversation;
	uint64_t mMetaRequestID;
	std::shared_ptr<void> mNativeActiveApp;
	BigInt mBasicOpcodeErgCost;
	BigInt getBasicInstructionERGCost();
	public:
		std::shared_ptr<CTools> getTools();
		bool syncEx(const std::vector<uint8_t>& perspective = std::vector<uint8_t>());
		void appDestructorCalled();
		void appDestructorExits();
		uint64_t getAppDestrucotorLevel();
		bool isFormulatingTX(bool checkIfToInclude = true);
		BigInt balanceGPP(std::string domainID, bool inGNC=true);
		std::shared_ptr<CBlockchainManager> getBlockchainManager();
		std::shared_ptr<CAccessToken>  CScriptEngine::genDefaultAccessTokenGPP();
		bool processInlineCode(std::string& script ,const std::string& errorMsg = std::string());
		std::shared_ptr<GridScriptPPEngine> getScriptPPEngine();
		void handleDepletedAssets(GridScriptPPEngine* wrapper);
		int notifyGPPBranch(GridScriptPPEngine* wrapper);
		int notifyGPPOpcode(GridScriptPPEngine* wrapper);
		void handleFatalError(GridScriptPPEngine* duktape, std::string msg);
		eBlockchainMode::eBlockchainMode getCommitTargetExC();
		void postDistributedCommitProcessing(bool success=true, std::vector<uint8_t> receiptID= std::vector<uint8_t>());
		bool isRegSet(unsigned int n);
		bool hasCommitableCode();
		bool isThreadRunning(std::vector<uint8_t> threadID);
		
		bool freeThreadExC(std::vector<uint8_t> threadID, uint64_t requestID=0, bool reportStatus = true);
		void startThread();
		std::shared_ptr<SE::CScriptEngine> getMainThread();
		void pauseThread();
		std::shared_ptr<SE::CScriptEngine> startThreadExC(std::vector<uint8_t> id = std::vector<uint8_t>(), uint64_t requestID = 0, uint64_t processID = 0, std::shared_ptr<CDTI> dti=nullptr, SE::vmFlags flags = SE::vmFlags());
		void ifconfig();
		//available both through #GridScript and for external objects (thread-safe)
		void setNativeActiveApp(std::shared_ptr<void> app);
		std::shared_ptr<void> getNativeActiveApp();
		void setRequestWebPartRenderedQRs(bool doIt = true);
		void setThreadNumID(uint64_t id);
		void setUIProcessID(uint64_t id);
		uint64_t getUIProcessID();
		uint64_t getThreadNumID();
		
		std::shared_ptr<SE::CScriptEngine> getSubThreadByVMID(std::vector<uint8_t> id);
		std::shared_ptr<SE::CScriptEngine> getSubThreadByThreadID(uint64_t id);
		bool getRequestWebPartRenderedQRs();

		void setCommitTargetEx(eBlockchainMode::eBlockchainMode commitTarget);
		uint64_t getMetaRequestID();
		void setTextualOutputEnabled(bool isIt=true);
		void setIsGUITerminalAttached(bool isIt=true);
		bool getIsGUITerminalAttached();
		BigInt getFinalTransactionOverheadEstimation();
		void setFinalTransactionOverheadEstimation(BigInt overhead);
		bool checkMemInRegisteredRegion(Cell memAddress, size_t size);
		void cleanMemory(bool cleanNativeMemory=true);
		void setOutputView(eViewState::eViewState view);
		bool isReady();
		void addVerifiedVerifiable(CVerifiable verifiable);
		bool isCmdTerminalOnly(std::string name);
		bool getIsGUICommandLineTerminal();
		//bool setAvailableCoinMintValuePool(uint64_t poolValue);
		enum eProsessingResult {
			valid,
			invalid
		};
		void clearErrorFlag();
		bool isInlineParametrizedFunction(uint64_t currentLine,uint64_t &nrOfParams);
		void reset(bool resetGridScriptDefinitions=false);
		bool enterSandbox();
		bool leaveSandbox();
		bool isInSandbox();
		bool setTerminalMode(bool state);
		bool getIsKernelMode();
		bool getIsInTerminalMode();
		bool getIsRemoteTerminal();
		std::shared_ptr<CDTI> getRemoteTerminal(bool prepareIfNeeded=false);
		void setStateDomainManager(std::shared_ptr<CStateDomainManager> sdm);
		void setCmdExecutor(std::shared_ptr<CCMDExecutor> executor);
		void setDefaultOutput(eViewState::eViewState view);
		CScriptEngine(std::shared_ptr<CTransactionManager> manager, std::shared_ptr<CBlockchainManager> bManager, bool executingFromTerminal, eViewState::eViewState view = eViewState::eViewState::eventView, std::weak_ptr<CDTI> remoteTerminal = std::weak_ptr<CDTI>());

		void setColorOutputEnabled(bool isIt);

		void setTransactionsManager(std::shared_ptr<CTransactionManager> tm, bool owning=false);
		std::shared_ptr<CTransactionManager> getTransactionsManager();
		std::shared_ptr<CStateDomainManager> getStateDomainManager();

		void head();
		void snake();
		bool enqueDataToOwningUIdApp(std::vector<uint8_t> bytes);
		bool getNextGoldVeinPosFromPlayerPos(point3D playerPos, point3D& goldVenPos);
		~CScriptEngine();
		bool isAProtectedVariable(std::string varName);
		std::vector<uint8_t>  getExecutingStateDomainID();
		//NetWork managment
		void startTests();
		void stopTests();
		void startNet();
		void stopNet();
		void status();
		void netStatus();
		//tests
		//code retrieval
		void setRegNVal(unsigned int n, const string& val);
		bool getByteCode(std::string code, std::vector<uint8_t> &mResult);
		std::string getSourceCode(std::vector<uint8_t> bytecode, const std::vector < std::string > &lines = std::vector<std::string>());
		//code processing
		CReceipt processScript(std::string script,std::vector<uint8_t> execuringIdentityID, uint64_t &topMostValue, BigInt &ERGUsed,uint64_t keyBlockHeight, bool resetVM = true,bool isTransaction=true,std::vector<uint8_t> receiptID = std::vector<uint8_t>(), bool resetERGUsage=true,bool GenerateBERMetadata=false,bool muteDuringProcessing=false,bool isCmdExecutingFromGUI=false,bool isGUICommandLineTerminal=false,uint64_t metaRequestID=0,bool resetOnException=false, bool detachedProcessing = false, bool excuseERGusage=false);

		void cleanNativeThread();
		std::recursive_mutex mDefintionsGuardian;
		std::list<Definition> mDefinitions;
	
		bool getExRepresentation(std::string bareFuncName, Definition&def);
		bool getDefinition(std::string bareFuncName, Definition&def);
		static std::list<Definition> mPervasiveDefinitions;
		static bool werePervasiveDefsInitialized;
	
		void setConversation(std::shared_ptr<CConversation> conversation);

		std::shared_ptr<CConversation> getConversation();

		bool setERGLimit(BigInt ERGLimit);
		BigInt getERGLeft();
		std::list<Definition> getDefinitions();
		std::list<Definition> getPervasiveDefinitions();
		BigInt getERGUsed();
		uint64_t getMemLeft();
		void executed(BigInt ERGUsed=1,bool isATerminalOnlyCommand=true);
		int main();

		// === ERG Aggregation Helper Methods - BEGIN ===
		BigInt getHighestERGBidFromSubThreads(bool onlyReadyThreads = true);
		BigInt getCumulativeERGLimitFromSubThreads(bool onlyReadyThreads = true);
		bool adjustERGParametersFromSubThreads(bool onlyReadyThreads = true);
		// === ERG Aggregation Helper Methods - END ===



	

		template<typename T>
		AAddr alignAddress(T addr);

#ifndef GRIDSCRIPT_SKIP_RUNTIME_CHECKS
		template<typename T>
		void checkAligned(T addr, const char * name);
		void clearRegN(unsigned int n);
		bool requireRegN(unsigned int n);
		void setRegN(unsigned int n);
		void clearAllRegs(bool clearKernelMode=false);
		void setRegNVal(unsigned int n, uint64_t val);
		bool getRegN(unsigned int n, uint64_t & val);
		void setPreserveRecentCallGPP(bool doIt=true);
		bool getPreserveRecentCallGPP();
		void setRegNVal(unsigned int n, BigInt val);
		void setRegNVal(unsigned int n, std::vector<uint8_t> val);
		bool getRegN(unsigned int n,  std::string& val = std::string());
		bool getRegN(unsigned int n , const std::vector<uint8_t>& val= std::vector<uint8_t>());
		bool getRegN(unsigned int n,  BigInt& val);
		bool getRegN(unsigned int n, const Botan::secure_vector<uint8_t>& val = Botan::secure_vector<uint8_t>());
		void requireDStackDepth(size_t n, const char * name);
		void requireDStackAvailable(size_t n, const char * name);
		void requireDStackType(AAddr adr,string name, eDataType::eDataType type);
		Cell getCell(AAddr adr,bool pop=false);

		// GridScript Extensions -  BEGIN
		void thunkHexi();
		// GridScript Extensions -  END
		double getDouble(AAddr adr,bool pop=false);
		void requireRStackType(AAddr adr, string name, eDataType::eDataType type);
		void requireRStackDepth(size_t n, const char * name);
		void requireRStackAvailable(size_t n, const char * name);
		void checkValidHere(const char * name);
		void requireDataSpaceAvailable(size_t n, const char * name);
#endif 
		void update2();
		void update1();
};
	

	struct Definition {

		bool requiresLocalAdmin = false;
		CScriptEngine *mScriptEngine=nullptr;
		Code  code = nullptr;
		AAddr does = nullptr;
		AAddr parameter = nullptr;
		Cell  flags = 0;
		string name;
		unsigned int byteCode=0;
		unsigned int ERGCost = 0;
		unsigned int inlineParamCount = 0;
		bool extType=false;
		bool userDefined = true;
		bool terminalOnly=false;
		bool requiresSecToken = false;
		unsigned int ExByteCode = 0;
		unsigned int reqStackWordsProceeding = 0;
		bool doNotIncludeInSource = false;//used for Debugging instructions
		uint64_t id = 0;
		bool hasBase58BinaryInLineParams = false;
		bool hasBase64BinaryInLineParams = false;
		bool allowToBeExecutedFromRAWCodeMode = false;
		bool isNativeWord = false;
		bool overwatchOnly = false;
		bool allowedInKernelMode = true;
		bool isDataReadFunction = false;
		static constexpr Cell FlagHidden = (1 << 1);
		static constexpr Cell FlagImmediate = (1 << 2);

		static const Definition* executingWord;

		void execute() const {
			if (!this)
				return;
			if ((terminalOnly || mScriptEngine->isCmdTerminalOnly(name)) && !mScriptEngine->getIsInTerminalMode())
				return;// [SECURITY]

			mScriptEngine->setRegN(REG_EXECUTING_INSTRUCTION);// Note, there's also the REG_EXECUTING_PROGRAM
			
			auto saved = executingWord;
			executingWord = this;

			//todo: execution of terminal-only commands should not affect the estimated ERG usage of finfal transaction.
			//solution already within executed() ?
			
			mScriptEngine->executed(ERGCost, terminalOnly);//deduct minimal ERG before execution in case the execution throws.
			//otherwise ERG would not be deducted IF the line was below, due to an jeopardized execution flow (exception thrown).
			(mScriptEngine->*code)();

			mScriptEngine->clearRegN(REG_SUPPRESS_THROW);
			
			executingWord = saved;
			mScriptEngine->clearRegN(REG_EXECUTING_INSTRUCTION);
			
		}

		bool isHidden() const { return (flags & FlagHidden) != 0; }

		void toggleHidden() { flags ^= FlagHidden; }

		bool isImmediate() const { return (flags & FlagImmediate) != 0; }

		void toggleImmediate() { flags ^= FlagImmediate; }

		bool isFindable() const { return !name.empty() && !isHidden(); }
	};


#endif
	static const char* GridScriptDefinitions[] = {

		/****

		I'll start by defining the remaining basic stack operations. `PICK` and
		`ROLL` are the basis for many of them.

		Note that while I'm not implementing any of the Forth double-cell arithmetic
		operations, double-cell stack operations are still useful.

		****/

		": dup   0 pick ;",
		": over  1 pick ;",
		": swap  1 roll ;",
		": rot   2 roll ;",
		": nip   swap drop ;",
		": tuck  swap over ;",
		": 2drop  drop drop ;",
		": 2dup  over over ;",
		": 2over  3 pick 3 pick ;",
		": 2swap  3 roll 3 roll ;",
		": 2>r   swap >r >r ;",
		": 2r>   r> r> swap ;",
		": 2r@   r> r> 2dup >r >r swap ;",

		/****

		`FALSE` and `TRUE` are useful constants.

		****/

		": false  0 ;",
		": true  1 ;",

		/****

		`]` enters compilation mode.

		`[` exits compilation mode.

		****/

		": ]  true state ! ;",
		": [  false state ! ; immediate",

		/****

		Forth has a few words for incrementing/decrementing the top-of-stack value.

		****/

		": 1+  1 + ;",
		": 1-  1 - ;",

		": cell+  1 cells + ;",
		": char+  1+ ;",
		": chars  ;",

		/****

		`+! ( n|u a-addr -- )` adds a value to a cell in memory.

		****/

		": +!  dup >r @ + r> ! ;",

		/****

		`NEGATE` and `INVERT` can be implemented in terms of other primitives.

		****/

		": negate  0 swap - ;",
		": invert  true xor ;",

		/****

		`, ( x -- )` places a cell value in dataspace.

		`C, ( char -- )` places a character value in dataspace.

		****/

		": ,  here 1 cells allot ! ;",
		": c,  here 1 chars allot c! ;",

		/****

		`ERASE` fills a region with zeros.

		****/

		": erase 0 fill ;",

		/****

		We have a few extended relational operators based upon the kernel's relational
		operators. In a lower-level Forth system, these might have a one-to-one
		mapping to CPU opcodes, but in this system, they are just abbreviations.

		****/

		": >   swap < ;",
		": u>  swap u< ;",
		": <>  = invert ;",
		": 0<  0 < ;",
		": 0>  0 > ;",
		": 0=  0 = ;",
		": 0<>  0= invert ;",

		/****

		`2*` and `2/` multiply or divide a value by 2 by shifting the bits left or
		right.

		****/

		": 2*  1 lshift ;",
		": 2/  1 rshift ;",

		/****

		A Forth variable is just a named location in dataspace. I will use `CREATE`
		and reserve a cell.

		****/

		": variable  create 0 , ;",
		": ?     @ . ;",

		/****

		A Forth constant is similar to a variable in that it is a value stored in
		dataspace, but using the name automatically puts the value on the stack. I can
		implement this using `CREATE...DOES>`.

		****/

		": constant  create ,  does> @ ;",
		": 2constant  create , , does> dup cell+ @ swap @ ;",

		/****

		`/CELL` is not a standard word, but it is useful to be able to get the size
		of a cell without using `1 CELLS`.

		****/

		"1 cells  constant /cell",

		/****

		`DECIMAL` and `HEX` switch the numeric base to 10 or 16, respectively.

		****/

		": decimal  10 base ! ;",
		": hex    16 base ! ;",

		/****

		`'` gets the next word from the input stream and looks up its execution token.

		****/

		": '  bl word find drop ;",

		/****

		The word `LITERAL` takes a cell from the stack at compile time, and at runtime
		will put that value onto the stack. I implement this by compiling a call to
		`(lit)` word followed by the value.

		Because I will be using `(lit)` in other word definitions, I'll create a
		constant `'(lit)` containing its XT.

		****/

		"' (lit)   constant '(lit)",
		"' (ulit)   constant '(ulit)",
		"' (slit)   constant '(slit)",
		"' (dlit)   constant '(dlit)",
		": literal  '(lit) , , ; immediate",

		/****

		`[']` is like `'`, but is an immediate compiling word that causes the XT to be
		put on the stack at runtime.

		****/

		": [']  ' '(lit) , , ; immediate",

		/****

		`RECURSE` compiles a call to the word currently being defined.

		****/

		": recurse   latest , ; immediate",

		/****

		`CHAR` gets the next character and puts its ASCII value on the stack.

		`[CHAR]` is like `CHAR`, but is an immediate compiling word.

		****/

		": char   bl word char+ char+ c@ ;",
		": [char]  char '(lit) , , ; immediate",

		/****

		Control Structures
		------------------

		See the [Control Structures[jonesforthControlStructures] section of
		`jonesforth.f` for an explanation of how these words work.

		[jonesforthControlStructures]: http://git.annexia.org/?p=jonesforth.git;a=blob;f=jonesforth.f;h=5c1309574ae1165195a43250c19c822ab8681671;hb=HEAD#l118

		One word we have here that is not described in JONESFORTH is `AHEAD`, which is
		essentially equivalent to `FALSE IF`. It is the start of an unconditional
		forward jump. It is useful for words, like `SLITERAL` below, that need to
		store data while compiling. Such words can use `AHEAD`, then use words like
		`,`, `C,`, or `ALLOT` to put data into the dictionary at the current
		compilation point, then use `THEN` so that at runtime the inner interpreter
		will jump over that data.

		****/

		": if    ['] (zbranch) , here 0 , ; immediate",
		": then   dup here swap - swap ! ; immediate",
		": else   ['] (branch) , here 0 , swap dup here swap - swap ! ; immediate",
		": ahead  ['] (branch) , here 0 , ; immediate",
			 
		": begin  here ; immediate",
		": again  ['] (branch) , here - , ; immediate",
		": loop  ['] (loop) , here - , ; immediate",
		": +loop  ['] (+loop) , here - , ; immediate",
		": until  ['] (zbranch) , here - , ; immediate",
		": while  ['] (zbranch) , here swap 0 , ; immediate",
		": repeat  ['] (branch) , here - , dup here swap - swap ! ; immediate",
		": postpone   bl word find  1 = if , else  '(lit) , ,  ['] , ,  then ; immediate",
		//": do ['] (doParams) here ; immediate",
		": DO postpone (do) here ; immediate",
		": i postpone (i) ; immediate",
		": j postpone (j) ; immediate",


		
			/*": postpone  bl word find 1 = if , else '(lit) , , ['] , , then ; immediate",
		": rdrop postpone r> postpone drop ; immediate",
		": (+loop) dup >r over - dup 3 pick xor 0 < 0 = if drop swap r> + 0 exit then dup 3 pick + xor 0 < 0 = if swap r> + 0 exit then swap r> + true ;",
		": p postpone postpone ; immediate",
		": unloop p rdrop p rdrop ; immediate",
		"CREATE LEAVE-SP 32 CELLS ALLOT",
		"LEAVE-SP LEAVE-SP !",
		": LEAVE ['] UNLOOP , ['] BRANCH , LEAVE-SP @ LEAVE-SP - 31 CELLS > IF ABORT THEN 1 CELLS LEAVE-SP + ! HERE LEAVE-SP @ ! 0 , ; IMMEDIATE",
		": RESOLVE-LEAVES BEGIN LEAVE-SP @ @ OVER > LEAVE-SP @ LEAVE-SP > AND WHILE HERE LEAVE-SP @ @ - LEAVE-SP @ @ ! 1 CELLS NEGATE LEAVE-SP + ! REPEAT DROP ;",
		": (do) p begin p 2>r ;",
		": do ['] (do) , here 0 ; immediate",
		": ?DO ['] 2DUP , ['] <> , ['] ZBRANCH , 0 , ['] (DO) , HERE 1 ; immediate",
		": RESOLVE-DO IF DUP HERE - , DUP 2 CELLS - HERE OVER - SWAP ! ELSE DUP HERE - , THEN ;",
		": +LOOP  ['] (+LOOP) , RESOLVE-DO RESOLVE-LEAVES ; IMMEDIATE",
		": loop 1 p literal p +loop ; immediate",
		": unloop p rdrop p rdrop ; immediate",
		": i p r@ ; immediate",
		": j p 2r> p r@ p -rot p 2>r ; immediate",
		*/
		/****

		Here are some common Forth words I can define now that I have control
		structures.

		****/

		": ?dup    dup if dup then ;",

		": abs    dup 0 < if negate then ;",

		": max    2dup < if swap then drop ;",
		": min    2dup > if swap then drop ;",

		": space   bl emit ;",
		": spaces   begin dup 0> while space 1- repeat drop ;",
		
		/****

		I wish I could explain Forth's `POSTPONE`, but I can't, so you will just have
		to Google it.

		****/
			/*
			//https://gist.github.com/ruv/0b0bfdbe2759254a5318d76f9b05d262
		": postpone  bl word find 1 = if , else '(lit) , , ['] , , then ; immediate",
			": rdrop postpone r> postpone drop ; immediate",
		": (+loop) dup >r over - dup 3 pick xor 0< 0= if drop swap r> + 0 exit then dup 3 pick + xor 0< 0= if swap r> + 0 exit then swap r> + true ;",
		": p postpone postpone ; immediate",
		"variable ac variable d0",
		": initialize-loop ac @ ac 0! d0 @ depth d0 ! "
		": finalize-loop begin ac @ while p then ac 1-! repeat depth d0 @ <> -22 and throw d0 ! ac ! ;",
		": ac+ ac 1+! ;",
		": cs-cnt depth d0 @ - ;",
		": (do) p begin p 2>r ;",
		": do initialize-loop (do) ; immediate",
		": ?do initialize-loop p 2dup p <> p if (do) ac+ ; immediate",
		": leave p 2r> cs-cnt n>r p ahead nr> drop ac+ ; immediate",
		": +loop p 2r> p (+loop) p until finalize-loop p 2drop ; immediate",
		": loop 1 p literal p +loop ; immediate"
		": unloop p rdrop p rdrop ; immediate",
		": i p r@ ; immediate",
		": j p 2r> p r@ p -rot p 2>r ; immediate"
		*/
		/****

		A Forth `VALUE` is just like a constant in that it puts a value on the stack
		when invoked. However, the stored value can be modified with `TO`.

		`VALUE` is, in fact, exactly the same as `CONSTANT` in this Forth. And so you
		could use `TO` to change the value of a constant, but that's against the rules.

		****/

		": value  constant ;",

		": value!  >body ! ;",

		": to    state @ if",
		"        postpone ['] postpone value!",
		"      else",
		"        ' value!",
		"      then ; immediate",

		/****

		`DEFER` and `IS` are similar to `VALUE` and `TO`, except that the value is an
		execution token, and when the created word is used it invokes that xt. `IS`
		can be used to change the execution token. In C++ terms, you can think of this
		as a pointer to a function pointer.

		`DEFER` and `IS` are not ANS Forth standard words, but are in common use, and
		are described formally at <http://forth-standard.org/standard/core/DEFER>.

		****/

		": defer    create ['] abort ,",
		"       does> @ execute ;",

		": defer@   >body @ ;",
		": defer!   >body ! ;",

		": is     state @ if",
		"         postpone ['] postpone defer!",
		"       else",
		"         ' defer!",
		"       then ; immediate",

		": action-of  state @ if",
		"         postpone ['] postpone defer@",
		"       else",
		"         ' defer@",
		"       then ; immediate",

		/****

		Strings
		-------

		`S" ( "ccc<quote>" -- caddr u )`

		This word parses input until it finds a `"` (double quote) and then puts the
		resulting string's address and length on the stack. It works in both
		compilation and interpretation mode.

		In interpretation mode, it just returns the address and length of the string in
		the input buffer.

		In compilation mode, I have to copy the string somewhere where it can be found
		at execution time. The word `SLITERAL` implements this. It compiles a
		forward-branch instruction, then copies the string's characters into the
		current definition between the branch and its target instruction, then at the
		branch target location I use a couple of `LITERAL`s to put the address and
		length of the word in the definition onto the stack.

		`." ( "ccc<quote>" -- )`

		This word prints the given string. We can implement it in terms of `S"` and
		`TYPE`.

		`.( ( "ccc<quote>" -- )`

		This is like `."`, but is an immediate word. It can be used to display output
		during the compilation of words.

		****/

		": sliteral",           // ( c-addr len )
		"  postpone ahead",       // ( c-addr len orig )
		"  2dup swap >r >r",       // ( c-addr len orig ) ( R: len orig )
		"  cell+ swap",         // copy into the first byte after the offset
		"  dup allot cmove align",   // allocate dataspace and copy string into it
		"  r> dup postpone then",    // resolve the branch
		"  cell+ postpone literal",   // compile literal for address
		"  r> postpone literal",     // compile literal for length
		"; immediate",

		": s\"  [char] \" parse",
		"    state @ if postpone sliteral then ; immediate",

		": .\"  postpone s\" postpone type ; immediate",

		": .(  [char] ) parse type ; immediate",

		/****

		`/STRING ( c-addr1 u1 n1 -- c-addr2 u2 )` adjusts a character string by adding
		to the address and subtracting from the length.

		****/

		": /string  dup >r - swap r> + swap ;",

		/****

		`ABORT"` checks whether a result is non-zero, and if so, it throws an exception
		that will be caught by `QUIT`, which will print the given message and then
		continue the interpreter loop.

		****/

		": (abort\")  rot if abort-message then 2drop ;",
		": abort\"   postpone s\" postpone (abort\") ; immediate",

		/****

		`INCLUDED` is the word for reading additional source files. For example, you
		can include the file `tests/hello.fs` and then run its `hello` word by doing
		the following:

		s" tests/hello.fs" INCLUDED
		hello

		`INCLUDE` is a simpler variation, used like this:

		INCLUDE tests/hello.fs
		hello

		`INCLUDE` is not part of the AND standard, but is in Forth 2012.

		****/

#ifndef GRIDSCRIPT_DISABLE_FILE_ACCESS

		": included",
		"  r/o open-file abort\" included: unable to open file\"",
		"  dup include-file",
		"  close-file abort\" included: unable to close file\" ;",

		": include  bl word count included ;",

#endif // #ifndef GRIDSCRIPT_DISABLE_FILE_ACCESS

		/****

		Comments
		--------

		There is a good reason that none of the Forth defintions above have had any
		stack diagrams or other comments: our Forth doesn't support comments yet. I
		have to define words to implement comments.

		I will support two standard kinds of Forth comments:

		- If `\` (backslash) appears on a line, the rest of the line is ignored.
		- Text between `(` and `)` on a single line is ignored.

		Also, I will support `#!` as a synonym for `\`, so that we can start a UNIX
		shell script with something like this:

		#! /usr/local/bin/GridScript

		Note that a space is required after the `\`, `(`, or `#!` that starts a
		comment. They are blank-delimited words just like every other Forth word.

		To-Do: `(` should support multi-line comments.

		****/

		": \\  source nip >in ! ; immediate",
		": #!  postpone \\ ; immediate",
		": (  [char] ) parse 2drop ; immediate",

		/****

		`ABOUT` is not a standard word. It just prints licensing and credit
		information.

		`.DQUOT` is also not a standard word. It prints a double-quote (") character.

		****/

		": .dquot  [char] \" emit ;",


		": about",
		"   cr",
		u8"   .\" \033[1;36mGRIDNET\033[0m \033[1;37mOS\033[0m\" cr",
		u8"   .\" by \033[1;36mRafal Skowronski\033[0m and other \033[1;36mWizards.\033[0m\" cr",
		"   cr",
		"   .\" This is free and unencumbered software released into the public domain.\" cr",
		"   cr",
		"   .\" Anyone is free to copy, modify, publish, use, compile, sell, or distribute this\" cr",
		"   .\" software, either in source code form or as a compiled binary, for any purpose,\" cr",
		"   .\" commercial or non-commercial, and by any means as long as this notice in its entirety remains intact .\" cr",
		"   cr",
		"   .\" In jurisdictions that recognize copyright laws, the author or authors of this\" cr",
		"   .\" software dedicate any and all copyright interest in the software to the public\" cr",
		"   .\" domain. We make this dedication for the benefit of the public at large and to\" cr",
		"   .\" the detriment of our heirs and successors. We intend this dedication to be an\" cr",
		"   .\" overt act of relinquishment in perpetuity of all present and future rights to\" cr",
		"   .\" this software under copyright law.\" cr",
		"   cr",
		"   .\" THE SOFTWARE IS PROVIDED \" .dquot .\" AS IS\" .dquot .\" WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\" cr",
		"   .\" IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\" cr",
		"   .\" FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\" cr",
		"   .\" AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN\" cr",
		"   .\" ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION\" cr",
		"   .\" WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\" cr",
		"   cr",
		"   .\" For more, visit GridNet.org.\" cr ;",

		/****

		The C++ `main()` function will look for the Forth word `MAIN` and execute it.

		The `MAIN` word calls `PROCESS-ARGS`, which is not a standard word. It looks
		at the number of command-line arguments. If there are no arguments other than
		the executable path, then it prints the `WELCOME` message. If there are
		arguments, then it attempts to call `INCLUDED` on each of them.

		If you want to write your own custom startup code, `MAIN` is the place to put
		it.

		****/

		": welcome",
		"  .\" GridScript VM\" cr",
		"  .\" Type \" .dquot .\" about\" .dquot .\" for more information. \"",
		"  .\" Type \" .dquot .\" bye\" .dquot .\" to exit.\" cr ;",
		": main welcome quit ;",
	};
	
}

