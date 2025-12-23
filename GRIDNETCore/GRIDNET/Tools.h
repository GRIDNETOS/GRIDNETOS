#ifndef TOOLS_H
#define TOOLS_H
typedef unsigned long       DWORD;
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>
#include <limits>  
#include <stdlib.h>
#include <algorithm>  
#include "TerminalLine.h"
#include "stdafx.h"
#include "arith_uint256.h"
#include "uint256.h"
#include <iostream>
#include <cctype>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include "enums.h"
#include "botan_all.h"
#include <regex>
#include "randomcolor.h"
#include "instructionSet.h"
#include <queue>

#define WS " " 
#define MAX_TIME_STANDARD_BAR_NOT_SHOWN 30
#define MIN_TIME_STANDARD_BAR_SHOWN 5
#define CUSTOM_STATUS_BAR_STALE_AFTER 60
#define STATUS_BAR_FLASH_INTERVAL_MS 100
struct HardwareSpecs {
	int cpuCores = 0;
	uint64_t totalMemory = 0;
	bool hasSSDs = false;
};
const size_t MAX_TERMINAL_QUEUE_LENGTH = 10000;
const size_t THROTTLE_LIMIT = 2;
class CDTI;
class CBERCacheManager;
class point3D;
class CNetworkManager;
namespace mp = boost::multiprecision;
using BigInt = mp::uint256_t;
using BigSInt = mp::int256_t;
using BigFloat = mp::cpp_dec_float_50;
struct nmFlags;
struct sdFlags;
class CTrieNode;
class CVerifiable;
class CSettings;
class CTrieDB;
class CReceipt;
class CNetMsg;
class CTransmissionToken;
class CTransaction;
class CBlock;
class CIdentityToken;
class CScriptEngine;
class CCryptoFactory;
class CCMDExecutor;
class CDTI;
class CSolidStorage;


#define MAX_EVENTS_BACKGLOG_LENGTH 300

struct nibblePair {
	unsigned  int mA : 4;
	unsigned  int mB : 4;
	bool mHasLeft = true;
	bool mHasRight = true;

};
struct Color
{
	uint8_t R;
	uint8_t G;
	uint8_t B;

	Color()
	{
		R = 255;
		G = 255;
		B = 255;
	}

	Color(uint8_t pR, uint8_t pG, uint8_t pB)
	{
		R = pR;
		G = pG;
		B = pB;
	}

	Color& operator=(Color other)
	{
		R = other.R;
		G = other.G;
		B = other.B;
		return *this;
	}
};
class CKeyChain;
namespace SE {
	class CScriptEngine;
}
typedef uint64_t b58_maxint_t;
typedef uint32_t b58_almostmaxint_t;
static const char b58digits_ordered[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
#define b58_almostmaxint_bits (sizeof(b58_almostmaxint_t) * 8)
static const b58_almostmaxint_t b58_almostmaxint_mask = ((((b58_maxint_t)1) << b58_almostmaxint_bits) - 1);
static const int8_t b58digits_map[] = {
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
	-1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
	22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
	-1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
	47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
};



class CTools {

	static std::default_random_engine mRandomEngine;
	static std::shared_ptr <CTools> sInstance;
	static std::mutex sInstanceGuardian;
private:

	std::random_device mRandomDevice;
	static bool sShowStatusBar;
	static bool  sOpenCLEvaluationActive;
	mutable std::mutex mBERCacheMutex;
public:
/// <summary>
/// Converts a Block Perspective back to a Trie Perspective using the parent block hash
/// </summary>
/// <param name="blockPerspective">Block Perspective (32 bytes)</param>
/// <param name="parentBlockHash">Parent block hash used in encoding (32 bytes)</param>
/// <returns>Original Trie Perspective, or empty vector if inputs are invalid</returns>
	std::vector<uint8_t> blockPerspectiveToTriePerspective(const std::vector<uint8_t>& blockPerspective, const std::vector<uint8_t>& parentBlockHash);


/// <summary>
/// Converts a Trie Perspective to a Block Perspective by incorporating parent block hash
/// </summary>
/// <param name="triePerspective">Original Trie Perspective (32 bytes)</param>
/// <param name="parentBlockHash">Parent block hash to incorporate (32 bytes)</param>
/// <returns>Block Perspective incorporating parent influence, or empty vector if inputs are invalid</returns>
	std::vector<uint8_t> CTools::triePerspectiveToBlockPerspective(const std::vector<uint8_t>& triePerspective,
	const std::vector<uint8_t>& parentBlockHash);
	void EncodeVarInt(uint64_t value, std::vector<uint8_t>& output) noexcept;
	bool DecodeVarInt(const std::vector<uint8_t>& input, size_t& index, uint64_t& value) noexcept;
	HardwareSpecs CTools::assessHardware();
	static void showStatusBar(bool doIt=true);
	static bool getShowStatusBar();
	static bool getIsOpenCLEvaluationActive();
	static void setIsOpenCLEvaluationActive(bool doIt);
	static bool findStringIC(const std::string& dataStr, const std::string& lookedFor);
	bool isStringPrintable(std::string& string);
	static double CTools::getAppCPUUsagePercentage();
	static double CTools::getCPUUsagePercentage();
	std::vector<std::string> getSupersetNoDuplicates(const std::vector<std::string>& vec1, const std::vector<std::string>& vec2);
	std::string uint32ToIPStr(uint32_t ip);
	const static InstructionSetCPU CPU;
	std::string  transportTypeToString(eTransportType::eTransportType tType);
	std::string sessionDescriptionFlagsToString(sdFlags flags);
	std::string netTaskResultToString(eNetTaskProcessingResult::eNetTaskProcessingResult result);
	bool iequals(const std::string& a, const std::string& b);
	static bool iequalsS(const std::string& a, const std::string& b);
	std::string netTaskStateToString(eNetTaskState::eNetTaskState tType);
	std::string netTaskTypeToString(eNetTaskType::eNetTaskType tType);
	std::string netMsgFlagsToString(nmFlags flags);
	std::string msgEntTypeToString(eNetEntType::eNetEntType eType);
	std::string msgReqTypeToString(eNetReqType::eNetReqType rType);
	int b58check(const uint8_t* bin, size_t binsz, const char* base58str, size_t b58sz);
	bool base58CheckDecode(std::vector<uint8_t> encodedData, std::vector<uint8_t>& output, bool doSatoshi = false);
	bool b58tobin(uint8_t* bin, size_t* binszp, const uint8_t* b58, size_t b58sz);
	bool b58enc(char* b58, size_t* b58sz, const uint8_t* data, size_t binsz, bool reportStatus = false, std::shared_ptr<CDTI> dti = nullptr);
	std::string CPValidationResultToString(const eChainProofValidationResult::eChainProofValidationResult& result);
	bool sanitizePath(std::string& path);
	bool CTools::isWindowsReservedDeviceName(const std::string& pathLower);
	bool validateIPv4(std::string ip);
	static std::shared_ptr<CTools> getInstance();
	bool isDTI();
	static void stopAllOperations();
	static bool isTerminalReadyForProcessing();
	static void setWipedUserInputBuffer(bool set = true);
	static bool getWipedUserInputBuffer();
	std::string GridScriptVarToString(std::vector<uint8_t> data, uint64_t vt, bool& ok, eBinaryStrEncFormat::eBinaryStrEncFormat encoding = eBinaryStrEncFormat::ASCII);
	std::string idTokenTypeToString(eIdentityTokenType::eIdentityTokenType type);
	static void setChangedCLIPrefix(bool set = true);
	static bool getChangedCLIPrefix();

/**
 * @brief Converts a timestamp to a formatted string.
 *
 * This function converts a Unix timestamp to a formatted string.
 * In regular mode, it returns the format "Mon DD HH:MM:SS YYYY"
 * In abbreviated mode, it returns "MM/DD/YY HH:MM"
 *
 * @param timestamp The Unix timestamp to convert
 * @param abbreviated If true, uses shortened date/time format
 * @return A formatted string representing the timestamp
 */
	std::string timeToString(size_t timestamp, bool abbreviated = false, bool handleNever=false);
	std::string secondsToFormattedString(uint64_t totalSeconds, bool abbreviated = false);
	std::vector<uint8_t> getProofOfFraudID(std::vector<uint8_t> keyBlockID);
	static void suicide();
	static void setTerminalReady(bool isReady = true);
	static bool isHotKey(char key);

	std::string endpointTypeToString(eEndpointType::eEndpointType eType);
	static std::string getFTTruncateAtLE();
	static std::string getFTNoRendering();
	 std::string CTools::getBackgroundCode(eColor::eColor color);
	 std::string CTools::getColorCode(eColor::eColor color);
	 std::string CTools::colorToANSI(int colorWithFlags);
	 std::string CTools::getProgressBarTxt(double percentage, eColor::eColor color = eColor::neonBlue);
	 std::string colorToANSI(eColor::eColor color, bool blinkingSlow = false, eColor::eColor backgroundColor = eColor::none);
	 void colorizeString(std::string& str, eColor::eColor color, bool doPrefix = true, bool doSuffix = true, bool blink = false);
	 std::string getColoredString(const std::string& str, uint8_t R, uint8_t G, uint8_t B);
	 void CTools::colorizeString(std::string& str, int colorWithFlags);
	 std::string getColoredString(std::string str, int colorWithFlags);
	static  std::string  getDisableLineWrapping();
	static  std::string  getEnableLineWrapping();
		void colorizeString(std::string& str, uint8_t R, uint8_t G, uint8_t B, bool doPrefix = true, bool doSuffix = true);
	void setNeglectColorRequests(bool doIt = true);

	 bool getNeglectColorRequests();
	 std::string getColoredString(const char* str, eColor::eColor color, bool doPrefix = true, bool doSuffix = true, bool blink = false);
	 std::string getColoredString(std::string str, eColor::eColor color, bool doPrefix = true, bool doSuffix = true, bool blink = false);

	std::string getColoredString(std::string& str, uint8_t R, uint8_t G, uint8_t B);
	std::vector<uint8_t> getNullHash();
	bool isNullHash(const std::vector<uint8_t>& hash);
	static bool prepareLogFile();
	static void clearScreen();

	std::string colorizeJSON(const std::string& str,
		eJSONColor::eJSONColor keyColor = eJSONColor::eJSONColor::GREEN,
		eJSONColor::eJSONColor valueColor = eJSONColor::eJSONColor::CYAN,
		eJSONColor::eJSONColor bracesColor = eJSONColor::eJSONColor::ORANGE);

	std::string colorCode(eJSONColor::eJSONColor color);
	bool GetServiceDetailsByPID(DWORD processId, std::vector<std::string>& serviceNames);
	bool isRunningAsAdmin();
	bool isElevatedAndInNetworkConfigurationOperatorsGroup();

	bool isOpenCLPresent();
	bool terminateProcessByID(DWORD processId);

	bool stopServiceByName(const std::string& serviceName, bool waitForTermination, int timeoutSeconds=10);

	DWORD GetRunningProcessPID();
	static std::shared_ptr<CCMDExecutor> getCommandExecutor();

	
private:


	// Forward declarations only in header

		static std::mutex sHardwareMetricsGuardian;

		// Linux-specific structures (needed in header for Linux implementation)
		struct CPUStat {
			long long user;
			long long nice;
			long long system;
			long long idle;
			long long iowait;
			long long irq;
			long long softirq;
			long long steal;

			CPUStat() : user(0), nice(0), system(0), idle(0),
				iowait(0), irq(0), softirq(0), steal(0) {
			}
		};

		static bool parseProcStat(CPUStat& stat);
	


	// Linux helper implementation
#ifndef _WIN32
	bool CTools::parseProcStat(CPUStat& stat) {
		std::ifstream file("/proc/stat");
		if (!file.is_open()) return false;

		std::string line;
		if (std::getline(file, line)) {
			std::istringstream ss(line);
			std::string cpu;
			ss >> cpu; // Skip "cpu" prefix
			ss >> stat.user >> stat.nice >> stat.system
				>> stat.idle >> stat.iowait >> stat.irq
				>> stat.softirq >> stat.steal;
			return true;
		}
		return false;
	}
#endif

	// Hardware CPU assessment Helpers - END

	//
	static std::ofstream sLogFile;
	std::mutex mNeglectColorGuardian;
	//Events Throttling Logic - BEGIN
	static std::mutex sThrottlingStatsGuardian;
	static void mConsoleThrottlingThreadF();
	static uint64_t getEventsLoggedBucket();
	static void incEventsLoggedBucket();
	static std::deque<std::shared_ptr<CTerminalLine>> sTerminalQueue;
	static std::mutex sTerminalQueueGuardian;
	static uint64_t sConsoleMsgsInFront;
	static uint64_t sEventsLoggedBucket;//the number of events in the current time-frame-window; The window is 1000ms wide  (after which the bucket is cleared).
	bool isZeroByteVector(const std::vector<uint8_t>& byteVector);
	//should the number of lines in it go above a threshed; consecutive incoming line would be Dropped.
	static bool enqueTLine(std::shared_ptr<CTerminalLine> line);
	static uint64_t sDroppedTLines;
	static uint64_t getDroppedTLines();
	static void incDroppedTLines(uint64_t by=1);

	static std::shared_ptr<CTerminalLine> dequeuTLine();
	//Events Throttling Logic - BEGIN

	std::mutex mColorGenGuardian;
	std::unique_ptr<RandomColor::RandomColorGenerator> mColorGenerator;
	std::unique_ptr<std::regex> mANSISequenceReg;
	std::unique_ptr<std::regex> mLocalNewLineReg;
	bool mNeglectColorRequests;
	static uint64_t getCursorPositionInBuffer();
	static void setCursorPositionInBuffer(uint64_t position);
	static bool incCursorPositionInBuffer(uint64_t by = 1);
	static bool decCursorPositionInBuffer(uint64_t by = 1, bool allowPastSolidChars = false);
	static bool sRestoreCursorPosition;
	static void setRestoreCursorPosition(bool doIt = true);
	static bool getRestoreCursorPosition();
	static uint64_t sCursorPosition;
	static std::mutex mCursorPositionGuardian;
	std::string mPreviousEventTxt = "";
	std::mutex mPreviousEventTxtGuardian;
	bool getIsConsoleMuted();
	std::mutex mFieldsGuardian;
	bool mLocalConsoleMuted;
	std::mutex mDTIGuardian;
	std::shared_ptr<CDTI> mDTI;
	static std::mutex terminalReadyGuardian;
	static bool mTerminalReadyForProcessing;
	static bool commitSuicide();
	static bool sCursorVisible;
	static bool sThrottlingEnabled;
	bool mSilentDropAllMsgs;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	eViewState::eViewState mDefaultOutput;
	static uint32_t commandHistoryViewIndex;
	static std::vector<std::string> prefixHistory;
	static std::vector<std::string> commandsHistory;
	static std::vector<std::string> eventViewLines;
	static std::vector<std::string> GridScriptDebuggerViewLines;
	
	static void printOutView(eViewState::eViewState view);
	static void pushLineToView(std::string line = "", eViewState::eViewState view = eViewState::eViewState::eventView);

	static bool writeLineToEventsLog(std::string& line);
	bool clearEventsLogFile();
	std::vector<std::string> getViewLines(eViewState::eViewState view);

	uint64_t getViewLinesCount(eViewState::eViewState view);

	static void pauseConsoleOutput();
	static void resumeConsoleOutput();
	void forceCommandLineMode(eViewState::eViewState = eViewState::eViewState::GridScriptConsole);
	void forceCommandLineModeExit();
	static void writeLineI(std::string str = "", bool appendToLog = false);
	static std::mutex CLIModeGuardian;
	static std::string consoleInputBuffer;
	static std::string consoleInputCmdBuffer;
	static eViewState::eViewState currentView;
	static bool dataPendingToBeWritten;
	static bool killMe;
	static std::mutex mKillMeGuardian;
	static std::mutex sFieldsGuardian;
	static bool isConsoleOutputPaused;
	static uint32_t currentWriteType;//0- writeLine 1-flash
	std::recursive_mutex  mQuestionsGuardian;
	std::shared_ptr<SE::CScriptEngine> mScriptEngine;//WARNING: only for byte-code compilation usage.
	std::string mLastLine;
	static bool pendingQuestion;
	static std::string defaultAnswer;
	static bool wipedUserInputBuffer;
	static std::mutex  wipedUserInputBufferGuardian;
	static bool prefixChanged;


	static std::mutex prefixChangedGuardian;
	static bool pendingMultiCharQuestion;
	static eViewState::eViewState pendingViewChange;
	static bool allowOnlyIntInput;
	static char answerToLastestQuestion;
	static std::thread mConsoleInputThread;
	static std::thread  mConsoleManagmentThread;
	static std::thread  mConsoleThrottlingThread;
	static std::shared_ptr<CCMDExecutor> sCMDExecutor;
	static std::string currentInput;
	static bool lastLineWasFlashed;
	static uint32_t lastFlashAnim;
	static unsigned int mTDR;
	static std::mt19937 mGen;
	static std::uniform_int_distribution<> mDis;
	static uint32_t previousFlashedLineLength;
	static std::string lineToBeWritten;
	static eViewState::eViewState targetView;
	static bool sDoMarginAnimation;
	static std::recursive_mutex consoleBufferGuardian;
	std::string mOwnerName;
	const char* mPrize = "              .-=========-.\n"
		"              \'-=======-'/\n"
		"              _|   .=.   |_\n"
		"             ((|  {{1}}  |))\n"
		"              \|   /|\   |/\n"
		"               \__ '`' __/\n"
		"                 _`) (`_\n"
		"          jgs  _/_______\_\n"
		"              /___________\\n"
		"\n";
	static bool inputBufferChanged;
	static eBlockchainMode::eBlockchainMode mBlockchainModeToUseByTerminal;
	static std::string consolePrefix;
	static bool isInSD;
	static std::string currentPath;
	static std::string currentSDID;
	static uint32_t solidCharsLength;
	static std::shared_ptr<CTools> sDataOnlyToolsInstance;
	std::shared_ptr<CBERCacheManager> mCacheManager;
	static bool s_utf8Enabled;
	static bool s_ansiEnabled;
public:
	 void enableConsoleFeatures();
	bool stringStartsWith(const std::string& str, const std::string& lookedFor);
	bool  hexStringToUint(std::string str, uint64_t& nr);
	static void setIsThrottlingEnabled(bool isIt = true);
	static void hideCursorANSI();
	static void showCursorANSI();
	static bool getIsCursorVisible();
	static bool getIsThrottlingEnabled();
	std::vector<uint8_t>ConvertArrayToVector(void* data, size_t size);
	void setDefaultOutput(eViewState::eViewState view);
	bool enableUTF8Output();
	bool enableANSISequences();
	std::shared_ptr<CNetMsg> wrapMsg(std::shared_ptr<CNetMsg> msg, std::vector<uint8_t> destination = std::vector<uint8_t>(), eEndpointType::eEndpointType endpointType = eEndpointType::WebSockConversation
		, std::vector<uint8_t> key = std::vector<uint8_t>(), bool isSessionKey = false, std::shared_ptr<CTransmissionToken> token = nullptr);
	std::string operationScopeToString(eOperationScope::eOperationScope scope);
	std::string operationStatusToString(eOperationStatus::eOperationStatus status);
	std::string conversationStateToString(eConversationState::eConversationState state, bool colorize=true);
	std::string logEntryTypeToStr(eLogEntryType::eLogEntryType eType);
	std::string logEntryCategoryToStr(eLogEntryCategory::eLogEntryCategory cat);
	std::string formatByteSize(uint64_t bytes, uint64_t precision = 3);
	std::string rtUpdateReasonToStr(eRTUpdateReason::eRTUpdateReason reason);
	std::vector<uint8_t> getSigBytesFromNumber(uint64_t nr);
	bool hasEnding(std::string const& fullString, std::string const& ending);
	bool verifyIDToken(eBlockchainMode::eBlockchainMode blockchainMode, std::shared_ptr<CIdentityToken> token, double& PoWDiff);
	std::vector<uint8_t> getReceiptIDForTransaction(eBlockchainMode::eBlockchainMode blockchainMode, std::vector<uint8_t> transactionID = std::vector<uint8_t>());
	std::vector<uint8_t> getReceiptIDForVerifiable(eBlockchainMode::eBlockchainMode blockchainMode);
	std::string routeKowledgeSourceToString(eRouteKowledgeSource::eRouteKowledgeSource source);
	uint8_t blockchainModeToNetID(eBlockchainMode::eBlockchainMode blockchainMode);

	eBlockchainMode::eBlockchainMode getBlockchainFromReceiptID(std::vector<uint8_t> id);
	std::string getExecutablePath(const std::string& errorMsg="");


	/**
	 * @brief Calculates SHA-256 hash of a file in chunks with progress reporting
	 *
	 * @param filePath Path to the file to be hashed
	 * @param maxChunkSize Maximum size of chunks to read at once (bytes)
	 * @param errorMsg Reference to store error messages if operation fails
	 *
	 * @return std::vector<uint8_t> SHA-256 hash of the file (32 bytes) or empty vector on error
	 *
	 * @throws None, but sets errorMsg on failures
	 *
	 * @details
	 * - Reads file in chunks to handle large files efficiently
	 * - Reports progress via progress bar during processing
	 * - Uses Botan cryptography library for SHA-256 calculation
	 * - Thread-safe if called with different files
	 *
	 * @note Modifies errorMsg parameter despite const qualifier using const_cast
	 */
	std::vector<uint8_t> getFileHash(const std::string& filePath, size_t maxChunkSize= 256000, const std::string& errorMsg="");
	std::vector<uint8_t> getImageSelf(size_t maxChunkSize, std::string& errorMsg);
	eCompileMode::eCompileMode strToCompileMode(std::string str);
	std::string compileModeToStr(eCompileMode::eCompileMode mode);
	eBlockchainMode::eBlockchainMode strToBlockchainMode(std::string str);
	static std::shared_ptr<CTools> getTools();
	std::string secondsToString(uint64_t seconds);
	void setOwnerName(std::string name);
	std::string getOwnerName();
	std::string managerStateToString(eManagerStatus::eManagerStatus status);
	static std::string blockchainmodeToString(eBlockchainMode::eBlockchainMode mode);
	bool doStringsMatch(const char* s1, std::string& s2);
	bool doStringsMatch(std::string& s1, std::string& s2);
	bool doStringsMatch(std::string& s1, const char* s2);
	std::string serviceIDToCmd(uint64_t serviceID);
	static bool activateView(eViewState::eViewState view, bool blocking = true);
	//logEvent(ss.str(), eLogEntryCategory::network, eColor::blue, 0, eLogEntryType::notification);
	void logEvent(std::string info, eLogEntryCategory::eLogEntryCategory category, eColor::eColor color = eColor::none, uint64_t priority = 1, eLogEntryType::eLogEntryType nType = eLogEntryType::notification,  bool forceIt = false);
	void logEvent(std::string info, std::string owner = "", eLogEntryCategory::eLogEntryCategory category = eLogEntryCategory::localSystem, uint64_t priority = 1, eLogEntryType::eLogEntryType nType = eLogEntryType::notification, eColor::eColor color = eColor::lightWhite,  bool forceIt = false);
	void logEvent(std::string info, eLogEntryCategory::eLogEntryCategory category, uint64_t priority = 1, eLogEntryType::eLogEntryType nType = eLogEntryType::notification, eColor::eColor color = eColor::lightWhite, bool forceIt = false);
	bool absolutePathToRelative(std::string path, const std::string& relativePath = std::string());
	bool parsePath(bool& isAbsolutePath, std::string input, const  std::string& fileName = std::string(), const std::vector<std::string>& directories = std::vector<std::string>(), const std::string& dirPart = std::string(),
		const  std::string& stateDomainPart = std::string());
	static bool isEmptyString(std::string str);
	//static bool stubBoolValue;
	void writeLine(std::string str = "", bool endLine = true, bool includeOwner = true, eViewState::eViewState view = eViewState::eViewState::unspecified, std::string forcedOwnername = "", bool broadcastToAllTerminals = false, bool asynchronous = true);
	static void mConsoleInputThreadF();
	static void initCMDExecutor();
	bool validateTransactionSemantics(CTransaction t);
	bool validateVerifiableSemantics(CVerifiable t);
	static void showCmdResult(CReceipt& rec, std::string msg);
	char getAnswerToLatestQuestion();
	bool isValidPathCharacter(char c);
	int hexCharToInt(char c);
	void testPathSanitization();
	bool isDangerousCharSequence(char prev, char current);
	std::string  getAnswerToLatestMultiCharQuestion();
	std::string urlDecode(const std::string& str);
	bool isDangerousPattern(const std::string& path, size_t pos, size_t length);
	bool isNormalizedPathSafe(const std::string& path);
	bool containsEncodingAttacks(const std::string& path);
	std::string  fullyDecodeURL(const std::string& str);
	std::vector<uint8_t> doubleToByteVector(double val);
	std::string normalizePath(const std::string& path);
	std::string replace_all(const std::string& str, const std::string& find, const std::string& replace);
	std::string replace_once(const std::string& str, const std::string& find, const std::string& replace);

	bool compareByteVectors(std::string& vec1, std::string& vec2);
	bool compareByteVectors(const std::vector<uint8_t>& vec1, const std::vector<uint8_t>& vec2);
	bool compareByteVectors(std::vector<uint8_t>& vec1, std::string& vec2);
	bool compareByteVectors(std::string& vec1, std::vector<uint8_t>& vec2);
	bool compareByteVectors(Botan::secure_vector<uint8_t> vec1, Botan::secure_vector<uint8_t> vec2);
	bool setThreadPriority(ThreadPriority priority);
	static void mConsoleManagmentThreadF();
	bool isAlphaNumChar(const char& elem);
	bool isAlphaNumericStr(const std::string& str, bool allowForSign=false);
	bool isDouble(const std::string& s, double& result);
	void stripNonAlphaNum(std::string& s);
	bool isNotASCII(char c);
	void stripNonASCII(std::string& str);
	void stripNonAlphaNumSpecial(std::string& s);
	~CTools();
	static std::mutex  writeLineIncentiveLock;
	static   std::recursive_mutex consoleGuardian;
	static void setCurrentPath(std::string folderName);
	static std::string getConsolePrefix();
	static void setCurrentSD(std::string sdID);
	static std::mutex eventViewLinesGuardian;
	static   std::recursive_mutex pendingViewChangeGuardian; 

	//Random Numbers - BEGIN
	static const int RNG_POOL_SIZE = 1000; 
	static std::vector<Botan::AutoSeeded_RNG> rngPool;
	static thread_local uint64_t threadRngIndex;
	static std::recursive_mutex randomLock;

	//Random Numbers - END
	char getSingleChar(bool quit = false);
	bool introduceRandomChange(std::vector<uint8_t>& vec);
	bool introduceRandomChange(Botan::secure_vector<uint8_t>& vec);
	bool askYesNo(std::string question, bool defaultV, std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false, bool forceAsk = false, uint64_t timeout = 180, bool isAGlobalConfigurationSetting=false);
	std::vector<uint8_t> BigIntToBytes(BigInt const& i, bool minOutput = true);
	std::vector<uint8_t> BigSIntToBytes(BigSInt const& i);

	/**
	 * @brief Formats GNC values from atto-GNC units to human-readable format with appropriate scale
	 *
	 * Converts raw atto-GNC values into human-readable format using appropriate unit scales
	 * from TeraGNC down to atto-GNC. Handles precise decimal formatting with configurable precision.
	 *
	 * Scale hierarchy (from largest to smallest):
	 * - T GNC  (TeraGNC)  = 10^30 atto-GNC
	 * - G GNC  (GigaGNC)  = 10^27 atto-GNC
	 * - M GNC  (MegaGNC)  = 10^24 atto-GNC
	 * - k GNC  (KiloGNC)  = 10^21 atto-GNC
	 * - GNC    (GNC)      = 10^18 atto-GNC
	 * - m GNC  (milliGNC) = 10^15 atto-GNC
	 * - � GNC  (microGNC) = 10^12 atto-GNC
	 * - n GNC  (nanoGNC)  = 10^9  atto-GNC
	 * - p GNC  (picoGNC)  = 10^6  atto-GNC
	 * - f GNC  (femtoGNC) = 10^3  atto-GNC
	 * - a GNC  (attoGNC)  = 1     atto-GNC
	 *
	 * Example outputs:
	 * - 1500000000000000000000000000000 atto-GNC -> "1.5 T GNC"
	 * - 1500000000000000000000000000 atto-GNC    -> "1.5 G GNC"
	 * - 1500000000000000000000000 atto-GNC       -> "1.5 M GNC"
	 * - 1500000000000000000000 atto-GNC          -> "1.5 k GNC"
	 * - 1500000000000000000 atto-GNC             -> "1.5 GNC"
	 * - 1500000000000000 atto-GNC                -> "1.5 m GNC"
	 * - 1500000000000 atto-GNC                   -> "1.5 � GNC"
	 * - 1500000000 atto-GNC                      -> "1.5 n GNC"
	 * - 1500000 atto-GNC                         -> "1.5 p GNC"
	 * - 1500 atto-GNC                            -> "1.5 f GNC"
	 * - 1 atto-GNC                               -> "1 a GNC"
	 * - 0                                        -> "0 GNC"
	 *
	 * @param value_in_attoGNC The value in atto-GNC units
	 * @param precision The number of decimal places to show (default: 6)
	 * @return Formatted string with appropriate scale and suffix
	 */
	static std::string formatGNCValue(const BigInt& value_in_attoGNC, uint64_t precision=6);

	std::string formatGNCValue(const BigSInt& value_in_attoGNC, uint64_t precision = 6);
	static  std::string formatBigFloat(const BigFloat& value, uint64_t precision=6);
	void sortVector(std::vector<uint64_t>& vec, bool ascending=true);

	// GNC Conversions - BEGIN
	std::string attoToGNCStr(BigInt value, uint64_t precision=3);
	std::string attoToGNCStr(BigSInt value, uint64_t precision = 3);
	float attoToGNCFloat(BigInt value);
	uint64_t attoToGNC(BigInt value);
	int64_t attoToGNC(BigSInt value);
	BigInt GNCToAtto(BigInt value);
	BigInt GNCToAtto(BigFloat value);
	BigInt GNCToAtto(double value);
	// GNC Conversions - END

	std::string stripQuotes(const std::string& input);
	std::vector<uint8_t> stripQuotes(std::vector<uint8_t>& input);
	std::string convertVectorToString(const std::vector<uint64_t>& vec);

	std::vector<uint8_t> BigFloatToBytes(BigFloat const& i);
	BigInt BytesToBigInt(std::vector<uint8_t> bytes);
	BigSInt BytesToBigSInt(std::vector<uint8_t> bytes);
	BigFloat BytesToBigFloat(std::vector<uint8_t> bytes);
	bool is_number(char c);
	arith_uint256 targetVec2arithUint(std::vector<uint8_t>  vec);
	std::vector<uint8_t> diff2target(const BigFloat& diff);
	void multiplyTarget(std::vector<uint8_t>& vec, const BigFloat& factor, bool allowOverflow = false);
	bool BigIntToUint64(Botan::BigInt number, uint64_t& result);
	double packedTarget2diff(uint32_t target);
	double target2diff(std::vector<uint8_t> target);
	uint32_t target2packedTarget(std::vector<uint8_t> target);
	bool is_number(const std::string& s);
	bool is_number(void* adr, size_t size);
	std::string getPrize();
	void SetThreadName(const char* threadName);
	void SetThreadName(std::string threadName);
	void SetThreadName(std::thread* thread, const char* threadName);
	void SetThreadName(uint32_t dwThreadID, const char* threadName);
	bool prepareCertificate(std::shared_ptr<CNetworkManager> nm);
	Color getRandomColor();
	arith_uint256 bytesToArithUint256(std::vector<uint8_t> bytes);
	static size_t getFreeHotStorage();
	static size_t getUsedHotStorage();
	std::vector<uint8_t> readFileEx(std::string fileName);
	static void lockConsole();
	static void unlockConsole();
	bool writeToFileEx(std::string path, std::string content, bool wipe = false);
	bool CTools::writeToFileEx(std::string fileName, const std::vector<uint8_t>& content, bool wipe=false);
	std::string getAppDir();
	bool createDirectoryRecursive(const std::string& dirName);
	bool createStatsDirs(CSolidStorage* solidStorage);
	bool createStatsDirs();


		/**
		 * @brief Get the number of CPU cores on the current platform.
		 *
		 * This method determines the number of CPU cores on the system, with the option
		 * to include or exclude logical cores created by technologies like Intel's Hyper-Threading.
		 * It supports both Windows and Linux platforms.
		 *
		 * On Windows, it uses the GetSystemInfo and GetLogicalProcessorInformation API calls.
		 * On Linux, it uses sysconf and parses /proc/cpuinfo.
		 *
		 * If an error occurs during the primary detection methods, the function falls back
		 * to using std::thread::hardware_concurrency(). If even that fails, it returns 1.
		 *
		 * @param includeHyperThreading If true, includes logical cores in the count.
		 *                              If false, attempts to return only physical core count.
		 *                              Defaults to true.
		 *
		 * @return The number of CPU cores. If includeHyperThreading is true, returns
		 *         the number of logical cores. Otherwise, returns the number of physical cores.
		 *         Returns at least 1, even if detection fails.
		 *
		 * @throws std::runtime_error Internally, but catches and logs the error.
		 *         Possible reasons include:
		 *         - Failure to retrieve system information
		 *         - Failure to open or read necessary system files
		 *         - Detection of invalid core count (0 or negative)
		 *
		 * @note Errors are logged using the logEvent method with category localSystem,
		 *       priority 1, and type failure.
		 *
		 * @warning The method of excluding Hyper-Threading may not be accurate for all
		 *          CPU architectures, especially on more complex server systems.
		 */
		 int getCPUCoreCount(bool includeHyperThreading = true);

		
	void maintainBERCache();

	bool writeToFileSS(std::string fileName, std::string content, bool wipe=false, CSolidStorage* solidStorage= nullptr, eBlockchainMode::eBlockchainMode blockchainMode = eBlockchainMode::TestNet);
	bool writeToFile(std::string fileName, std::string content, bool wipe = false, eBlockchainMode::eBlockchainMode = eBlockchainMode::TestNet);
	double getDinstance(double x1, double y1, double z1, double x2, double y2, double z2);
	bool getAverageSpeed(std::vector<point3D>& path, double& speed);
	double bytesToDouble(std::vector<uint8_t> bytes);
	std::wstring utf8_to_wstring(const std::string& str);
	std::string wstring_to_utf8(const std::wstring& str);

	CTools(const CTools& sibling);
	std::mutex mBlockchainModeGuardian;
	eBlockchainMode::eBlockchainMode getBlockchainMode();
	std::pair<std::string, eColor::eColor> livenessToString(eLivenessState::eLivenessState liveness);
	std::string doubleToString(const double& number, const uint64_t& precision = 3);
	static std::string cleanDoubleStr(const std::string &doubleStr, uint64_t maxPrecision = 3);
	std::string getSortModeDescription(eSecurityReportSortMode::eSecurityReportSortMode mode);
	CTools(std::string ownerName, eBlockchainMode::eBlockchainMode blockchainMode, eViewState::eViewState deafultOutput = eViewState::eViewState::eventView, std::shared_ptr<CDTI> DTI = nullptr, bool localConsoleMuted = false);
	eTerminalFormatTag::eTerminalFormatTag getFormatTagFromString(std::string& str, bool removeIt = true);
	void showWelcomeScreen();
	uint8_t getSignificantBytes(uintptr_t number);
	uint64_t bytesToUint64(std::vector<uint8_t> bytes);
	void setSilentDropMsgs(bool doIt = true);
	bool decodeWebString(std::string& str, eWebEncoding::eWebEncoding encType, size_t maxLength = 256000);
	bool encodeWebString(std::string& str, eWebEncoding::eWebEncoding encType, int compressionLevel = 6);
	bool getSilentDropMsgs();
	std::string base64CheckEncode(std::vector<uint8_t> bytes, bool reportStatus = false, std::shared_ptr<CDTI> dti = nullptr);
	std::string base64Encode(std::vector<uint8_t> bytes, bool reportStatus = false, std::shared_ptr<CDTI> dti = nullptr);
	bool base64CheckDecode(std::vector<uint8_t> encodedData, std::vector<uint8_t>& output);
	bool base64Decode(std::vector<uint8_t> encodedData, std::vector<uint8_t>& output);
	std::string base58CheckEncode(std::vector<uint8_t> bytes, bool reportStatus = false, std::shared_ptr<CDTI> dti = nullptr, bool doSatoshi = false);
	std::string base58Encode( std::vector<uint8_t> bytes, bool reportStatus = false, std::shared_ptr<CDTI> dti = nullptr, bool doSatoshi = false);
	bool base58CheckDecode(std::string encodedData, std::vector<uint8_t>& output, bool doSatoshi = false);
	eBlockchainMode::eBlockchainMode getBlockchainModeFromReceiptID(std::vector<uint8_t> receiptID);
	bool  stringToUint(std::string str, uint64_t& nr);
	bool stringToInt(std::string str, int64_t& nr);
	uint64_t getUint(std::vector<uint8_t> vec);
	int64_t getInt(std::vector<uint8_t> vec);
	bool isDomainIDValid(std::vector<uint8_t> domainID);
	bool isBlockIDValid(std::vector<uint8_t> blockID);

	bool isReceiptIDValid(const std::vector<uint8_t> &receiptID, const std::vector<uint8_t>& confirmed=std::vector<uint8_t>());
	bool isReceiptIDValid(const std::string& receiptID, const std::vector<uint8_t>& confirmed = std::vector<uint8_t>());

	std::string nodeTypeToStr(eNodeType::eNodeType nType);
	std::string nodeSubTypeToStr(eNodeSubType::eNodeSubType nType);
	std::string genTableLine(std::vector<std::string> columnLines, uint32_t firstColFixedWifth = 0, uint32_t lastColFixedWidth = 0);
	bool GetProcessInfoByPort(unsigned short port, eFirewallProtocol::eFirewallProtocol protocol, DWORD& processId, std::string& processName, std::vector<std::string>& serviceNames);
	std::string genTable(std::vector<std::vector<std::string>> columnLines,
		eColumnFieldAlignment::eColumnFieldAlignment fieldAlignment = eColumnFieldAlignment::center,
		bool highlightTopRow = true,
		std::string title = "",
		std::string newLine = "\n",
		uint64_t terminalWidth = 120,
		std::vector<uint64_t> maxWidths = std::vector<uint64_t>(),
		bool allowColumnsToOverflow = false,
		std::vector<eColor::eColor> colors = std::vector<eColor::eColor>(),
		bool dummyHeaders = false,
		eColor::eColor headerColor = eColor::blue
	);

	// === Section Frame Helper System - BEGIN ===
	/**
	 * @brief Generates a complete framed section with title and rows
	 *
	 * Creates a formatted section with box-drawing characters, automatic width
	 * calculation, and proper padding. Eliminates manual width calculations.
	 *
	 * @param title Section title (displayed in header bar)
	 * @param rows Vector of {label, value} pairs to display
	 * @param frameColor Color for frame characters (border and edges)
	 * @param labelColor Color for row labels
	 * @param valueColor Color for row values (can be overridden per-row)
	 * @param valueColors Optional per-row value colors (empty = use valueColor)
	 * @param newLine Line separator (default "\n")
	 * @param minWidth Minimum frame width (default 60)
	 * @param useDoubleFrame Use double-line frame (╔═╗) vs single-line (┌─┐)
	 * @return Complete framed section as string
	 *
	 * Example usage:
	 *   std::vector<std::pair<std::string, std::string>> rows = {
	 *       {"ERG Bid", currentBid.str()},
	 *       {"ERG Limit", currentLimit.str()}
	 *   };
	 *   output += mTools->genSectionFrame("Kernel-Mode Parameters", rows,
	 *                                     eColor::orange, eColor::blue,
	 *                                     eColor::lightGreen, {}, "\n", 60, false);
	 */
	std::string genSectionFrame(
		const std::string& title,
		const std::vector<std::pair<std::string, std::string>>& rows,
		eColor::eColor frameColor = eColor::lightCyan,
		eColor::eColor labelColor = eColor::blue,
		eColor::eColor valueColor = eColor::lightGreen,
		const std::vector<eColor::eColor>& valueColors = std::vector<eColor::eColor>(),
		const std::string& newLine = "\n",
		uint64_t minWidth = 60,
		bool useDoubleFrame = false
	);

	/**
	 * @brief Generates frame header with title
	 *
	 * @param title Section title
	 * @param width Total frame width (auto-calculated if 0)
	 * @param frameColor Color for frame characters
	 * @param useDoubleFrame Use ╔═╗ (true) or ┌─┐ (false)
	 * @return Header line with title bar
	 */
	std::string genSectionFrameHeader(
		const std::string& title,
		uint64_t width,
		eColor::eColor frameColor = eColor::lightCyan,
		bool useDoubleFrame = false
	);

	/**
	 * @brief Generates a single row within a frame
	 *
	 * @param label Row label (left-aligned)
	 * @param value Row value (right-aligned)
	 * @param width Total frame width
	 * @param frameColor Color for frame characters (│)
	 * @param labelColor Color for label text
	 * @param valueColor Color for value text
	 * @return Formatted row with proper padding
	 */
	std::string genSectionFrameRow(
		const std::string& label,
		const std::string& value,
		uint64_t width,
		eColor::eColor frameColor = eColor::lightCyan,
		eColor::eColor labelColor = eColor::blue,
		eColor::eColor valueColor = eColor::lightGreen
	);

	/**
	 * @brief Generates frame footer (closing line)
	 *
	 * @param width Total frame width
	 * @param frameColor Color for frame characters
	 * @param useDoubleFrame Use ╚═╝ (true) or └─┘ (false)
	 * @return Footer line
	 */
	std::string genSectionFrameFooter(
		uint64_t width,
		eColor::eColor frameColor = eColor::lightCyan,
		bool useDoubleFrame = false
	);

	/**
	 * @brief Calculates required frame width for given content
	 *
	 * @param title Section title
	 * @param rows Vector of {label, value} pairs
	 * @param minWidth Minimum width constraint
	 * @return Calculated width that fits all content
	 */
	uint64_t calculateSectionFrameWidth(
		const std::string& title,
		const std::vector<std::pair<std::string, std::string>>& rows,
		uint64_t minWidth = 60
	);
	// === Section Frame Helper System - END ===
	void eraseSubStr(std::string& mainStr, const std::string& toErase);
	void replace(std::vector<CTrieNode*>& elements, CTrieNode* lookedFor, CTrieNode* toReplaceWith);
	bool checkVectorContained(const std::vector<uint8_t> &vec, const std::vector<std::vector<uint8_t>>& vectors);
	bool checkStringContained(const std::string& str, const std::vector<std::string>& strings);
	bool checkStringContainedInByteVectors(const std::string& str, const std::vector<std::vector<uint8_t>>& vectors);



	std::string trim(const std::string& str);

	/**
 * @brief Formats large numbers into a concise human-readable format using scale suffixes
 *
 * Converts numbers to abbreviated forms using K (thousands), M (millions),
 * B (billions), T (trillions), and Q (quadrillions) suffixes. The method preserves
 * numerical precision using appropriate decimal places for each scale.
 *
 * Scale details:
 * - Numbers < 1000: No suffix, includes comma separators
 * - K (thousands): 1 decimal place
 * - M (millions): 2 decimal places
 * - B (billions): 2 decimal places
 * - T (trillions): 2 decimal places
 * - Q (quadrillions): 2 decimal places
 *
 * Formatting behavior by scale:
 *
 * Below 1000 (with commas):
 * - 1        -> "1"
 * - 10       -> "10"
 * - 100      -> "100"
 * - 999      -> "999"
 *
 * Thousands (K):
 * - 1000     -> "1K"
 * - 1100     -> "1.1K"
 * - 1500     -> "1.5K"
 * - 9999     -> "9.9K"
 * - 10000    -> "10K"
 * - 999999   -> "999.9K"
 *
 * Millions (M):
 * - 1000000      -> "1M"
 * - 1100000      -> "1.1M"
 * - 1500000      -> "1.5M"
 * - 9999999      -> "9.9M"
 * - 10000000     -> "10M"
 * - 999999999    -> "999.9M"
 *
 * Billions (B):
 * - 1000000000       -> "1B"
 * - 1100000000       -> "1.1B"
 * - 1500000000       -> "1.5B"
 * - 9999999999       -> "9.9B"
 * - 10000000000      -> "10B"
 * - 999999999999     -> "999.9B"
 *
 * Trillions (T):
 * - 1000000000000        -> "1T"
 * - 1100000000000        -> "1.1T"
 * - 1500000000000        -> "1.5T"
 * - 9999999999999        -> "9.9T"
 * - 10000000000000       -> "10T"
 * - 999999999999999      -> "999.9T"
 *
 * Quadrillions (Q):
 * - 1000000000000000     -> "1Q"
 * - 1100000000000000     -> "1.1Q"
 * - 1500000000000000     -> "1.5Q"
 * - 9999999999999999     -> "9.9Q"
 * - 10000000000000000    -> "10Q"
 *
 * Special cases:
 * - 0        -> "0"
 * - Numbers with no decimal remainder show no decimal places
 * - Numbers at exact scale transitions use the smaller scale
 *
 * @param number The BigInt number to format
 * @return std::string The formatted string representation
 */
	static std::string shortFormatNumber(const BigInt& number);

/**
 * @brief Formats double numbers into a concise human-readable format using scale suffixes
 *
 * Converts double numbers to abbreviated forms using K (thousands), M (millions),
 * B (billions), T (trillions), and Q (quadrillions) suffixes. Handles both positive
 * and negative numbers, and maintains appropriate precision for floating point values.
 *
 * Scale details:
 * - Numbers < 1000: Shows up to 2 decimal places, no suffix
 * - K (thousands): 1 decimal place
 * - M (millions): 2 decimal places
 * - B (billions): 2 decimal places
 * - T (trillions): 2 decimal places
 * - Q (quadrillions): 2 decimal places
 *
 * Formatting behavior by scale:
 *
 * Small decimals:
 * - 0.0      -> "0"
 * - 0.1      -> "0.1"
 * - 0.01     -> "0.01"
 * - 0.001    -> "0.001"
 *
 * Below 1000:
 * - 1.0      -> "1"
 * - 1.23     -> "1.23"
 * - 999.99   -> "999.99"
 *
 * Thousands (K):
 * - 1000     -> "1K"
 * - 1100     -> "1.1K"
 * - 1500     -> "1.5K"
 * - 9999     -> "9.9K"
 * - 10000    -> "10K"
 * - 999999   -> "999.9K"
 *
 * Millions and above follow the same pattern as the BigInt version
 *
 * Negative numbers maintain the same precision but include the minus sign:
 * - -1.23    -> "-1.23"
 * - -1500    -> "-1.5K"
 * - -1.1M    -> "-1.1M"
 *
 * @param number The double number to format
 * @return std::string The formatted string representation
 */
	static std::string shortFormatNumber(const double& number);
	bool testCppExceptionHandling(volatile uint64_t magicValue);
	bool validateExceptionHandlingIntegrity();
	CTrieNode* nodeFromBytes(const std::vector<uint8_t> & bytes, eBlockchainMode::eBlockchainMode mode, std::vector<uint8_t> perspective = std::vector<uint8_t>(), bool introduceRandomError = false);
	void HotStorageEffect(CTrieNode* node);

	std::vector <std::vector<uint8_t>> instantiateByteVectors(std::vector<uint8_t> packed);
	std::vector<uint8_t> packByteVectors(std::vector <std::vector<uint8_t>> vectors);
	std::string blockCachingResultToString(eSetBlockCacheResult::eSetBlockCacheResult result);

	CTransaction genAtomicTransaction(std::vector<uint8_t> targetSDID,
		BigInt value,
		std::vector<uint8_t> sourcePubKey = std::vector<uint8_t>(),
		Botan::secure_vector<uint8_t> privKey = Botan::secure_vector<uint8_t>(),
		uint64_t nonce = 0, uint64_t ERGLinmit = 1000, uint64_t ERGPrice = 1);
	bool addLeadingNibbles(std::vector<nibblePair> toAdd, std::vector<nibblePair>& nibbles);
	std::string getFileExtensionFromPath(const std::string &path);
	/// <summary>
	/// Wraps raw byte vector around a BER sequence.
	/// </summary>
	/// <returns></returns>
	std::vector<uint8_t> BERVector(const std::vector<uint8_t> &bytes);
	std::vector<uint8_t> BERVector(std::vector <uint64_t>& numbers);
	std::vector<uint8_t>  BERVector(const std::vector<std::vector<uint8_t>>& byteVectors, unsigned int threadCount = std::thread::hardware_concurrency(),bool useCache=true);
	
/**
 * @brief Encode a list of byte vectors using variable-length prefix encoding (VarInt).
 *
 * This function encodes a list of byte vectors into a single byte vector by
 * prefixing each vector with its length encoded as a variable-length integer.
 *
 * @param byteVectors The list of byte vectors to encode.
 * @param threadCount Number of threads to use for parallel encoding.
 *                    If threadCount is 1 or less, encoding is done in the calling thread.
 * @param useCache    Indicates whether to use caching (not implemented in this example).
 *
 * @return A single encoded byte vector containing all input vectors with their VarInt length prefixes.
 */
	std::vector<uint8_t> VarLengthEncodeVector(
		const std::vector<std::vector<uint8_t>>& byteVectors,
		unsigned int threadCount = std::thread::hardware_concurrency(),
		bool useCache = true);

/**
 * @brief Decode a byte vector encoded with variable-length prefixes into a list of byte vectors.
 *
 * This function decodes a single byte vector that was encoded using the
 * VarLengthEncodeVector function back into the original list of byte vectors.
 *
 * @param bytes       The encoded byte vector to decode.
 * @param byteVectors The output list of decoded byte vectors.
 *
 * @return True if decoding was successful; false otherwise.
 */
	bool VarLengthDecodeVector(const std::vector<uint8_t>&bytes, std::vector<std::vector<uint8_t>>&byteVectors);
	bool BERVectorToCPPVector(std::vector<uint8_t>& bytes, std::vector <std::vector<uint8_t>>& byteVectors);
	std::string pollIDToString(uint64_t pollID);
	bool BERVectorToCPPVector(std::vector<uint8_t>& bytes, std::vector <uint64_t>& numbersVector);
	void throwIf(bool condition, std::string description = "error");
	Botan::secure_vector<uint8_t> BERVector(Botan::secure_vector<uint8_t> bytes);

	bool addEndingNibbles(std::vector<nibblePair> toAdd, std::vector<nibblePair>& nibbles);

	bool isNibbleEmpty(nibblePair* np);

	std::vector<nibblePair> removeLeadingNibbles(int count, std::vector<nibblePair> nibbles);

	std::vector<nibblePair> removeEndingNibbles(int count, std::vector<nibblePair> nibbles);

	bool  PointsToThreadStack(const void* pv);
	std::vector<nibblePair> bytesToNibbles(std::vector<uint8_t> id, bool hasRightMostNibble = true);

	std::vector<uint8_t> stringToBytes(std::string str);
	std::vector<uint8_t> andVectors(std::vector<uint8_t>& v1, std::vector<uint8_t>& v2);
	std::vector<uint8_t> xorVectors(std::vector<uint8_t>& v1, std::vector<uint8_t>& v2);
	std::string eHTTPencodingToString(eWebEncoding::eWebEncoding encType);
	std::string bytesToString(std::vector<uint8_t> bytes, bool zeroOnEmpty = false);
	bool isAbsolutePath(std::string path);
	int getNrOfNibbles(std::vector<nibblePair> nibblePairs);

	int getCommonNibblesCount(std::vector<nibblePair> nib1, std::vector<nibblePair> nib2);
	char* strremove(char* str, const char* sub);
	std::vector<nibblePair> getNibbleSubstring(int count, std::vector<nibblePair> nibbles);

	uint64_t getTrueStrLength(const std::string& str);

	uint8_t getNibbleAtIndex(int index, std::vector<nibblePair> nibbles);
	std::vector<uint8_t>  genRandomVector(uint32_t length);
	std::string convEndReasonToStr(eConvEndReason::eConvEndReason reason);
	std::vector<nibblePair> getNextStateDomainCredentials(eBlockchainMode::eBlockchainMode mode, std::string chainName = "main");
	std::string SDPTypeToString(eSDPEntityType::eSDPEntityType type);

	std::vector<uint8_t> getSDCredentialsAtDepth(eBlockchainMode::eBlockchainMode mode, uint32_t& depth, Botan::secure_vector<uint8_t>& privKey, std::vector<uint8_t>& pubKey, bool getCurrentDepth = false, std::string chainName = "main");
	std::vector<uint8_t> getNextStateDomainIDVec(eBlockchainMode::eBlockchainMode mode, std::string chainName = "main");
	bool getSDCredentials(CKeyChain keyChain, std::vector<uint8_t> stateDomainID, Botan::secure_vector<uint8_t>& privKey, std::vector<uint8_t>& pubKey, uint32_t searchSpace = 10, uint32_t keyType = 0);
	uint64_t genRandomNumber(uint64_t min, uint64_t max);
	size_t getTime(bool milliseconds = false);

	std::vector<uint8_t> nibblesToBytes(std::vector<nibblePair> nibbles, bool& hasRightMostNibble);
	std::string getRandomStr(size_t length);
	static void flashLineI(std::string str, bool doMarginAnimation = true);
	std::string transactionStatusText(uint64_t status);
	eColor::eColor transactionStatusColor(uint64_t status);
	void flashLine(std::string str, bool doMarginAnimation = true, eViewState::eViewState view = eViewState::eViewState::eventView, bool broadcastToAllTerminals = false);
	void clearLines(uint32_t nrOfLines);

	std::string getBanner();

	std::vector<std::string> getBannerArray();

	// Kernel Firewall Integration - BEGIN
	bool addKernelFirewallRule(const std::string& ipAddress);
	bool cleanKernelFirewallRules();
	bool removeKernelFirewallRule(const std::string& ipAddress);
	// Kernel Firewall Integration - END

	static void writeLineS(std::string str = "", std::string issuer = "", eViewState::eViewState view = eViewState::eViewState::unspecified, eBlockchainMode::eBlockchainMode bMode = eBlockchainMode::eBlockchainMode::LocalData);
	static void lowMemShutdown();
	void hidecursor();
	inline float fast_log(float val);
	bool enableWindowsUnicodeConsoleOutput();
	bool tweakWindowsTDR(unsigned int seconds);
	uint64_t getCurrentTDRValue();
	long GetDWORDRegKey(void* hKey, const std::wstring& strValueName, unsigned long& nValue, unsigned long nDefaultValue);
	std::string transactionManagerModeToStr(eTransactionsManagerMode::eTransactionsManagerMode mode);
	unsigned int getTDR();
	BigSInt askInt(std::string question, BigSInt defaultV = 1, std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false, uint64_t timeout = 180);
	std::vector<std::string> getKernelFirewallRules();

	std::string askString(std::string question, std::string defaultV = "", std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false, uint64_t timeout = 180);
	static bool addFirewallRule(unsigned int port, eFirewallProtocol::eFirewallProtocol proto, eFirewallDirection::eFirewallDirection direction = eFirewallDirection::INBOUND, std::string ruleName= "GRIDNET Core");
	std::string to_string_with_precision(double a_value, const int n = 2);
	std::string to_string(const std::vector<uint8_t>& vec);
	std::wstring prepareIpListString(const std::string& ipAddress, bool add, const std::wstring& currentAddrString);
	bool isPrivateIpAddress(const std::string& ipAddress);
	std::string getVerifiableTypeStr(eVerifiableType::eVerifiableType type);

	bool isValidDecimalNumber(const std::string& str, bool allowSign=false);
};

#endif

