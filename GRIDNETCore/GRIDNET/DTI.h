#pragma once
#include "stdafx.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>
#include <limits>  
#include <stdlib.h>
#include <algorithm>  
#include <regex>
#include "arith_uint256.h"
#include "uint256.h"
#include <iostream>
#include <cctype>
#include <boost/multiprecision/cpp_int.hpp> //////vcpkg install boost-multiprecision --triplet x64-windows
#include <boost/multiprecision/cpp_dec_float.hpp>
#include "enums.h"
#include "libssh/libssh.h"
#include "CMDExecutor.h"
#include "TransactionManager.h"
#include "semaphore.h"
#include "logoAnim.h"

class CVMMetaSection;
class CQRIntentResponse;
class CTransactionManager;
class CConversation;
class CEndPoint;
class CNetworkManager;
/// <summary>
/// Class handling the Decentralized Terminal Interface functionality.
/// Connectivity is maintained through SSH with reliance upon the libssh library.
/// </summary>
class CDTI :public  std::enable_shared_from_this<CDTI>
{
public:
	bool getIsUIAware();
	void setIsUIAware(bool isIt = true);
	char getRecentKey();
	void setRecentKey(char key);
	void clearScreen();
	void setScreenWrapEnabled(bool doIt=true);
	bool getIsScreenWrapEnabled();
	bool getIsServer();
	void setIsServer(bool isIt);
	uint64_t getCursorInViewLine();
	void hideCursor();
	void showCursor();
	void setPlayerTT(std::shared_ptr<CTransmissionToken> tt);
	std::shared_ptr<CTransmissionToken> getPlayerTT();
	bool getHasQuestionPending();
	uint64_t getTerminalHeight();
	bool setTerminalHeight(uint64_t height);
	bool setTerminalWidth(uint64_t width);
	bool dequeInputChar(uint8_t& c);
	std::vector<uint8_t> dequeOutputByteVec();
	uint64_t getUIProcessID();
	std::vector<uint8_t> getThreadID();
	void setThreadID(std::vector<uint8_t> id);
	void setUIProcessID(uint64_t id);
	void enqueInputChar(uint8_t c);
	void enqueOutputByteVec(std::vector<uint8_t> bytes);
	void enqueDataToOwningUIdApp(std::vector<uint8_t> bytes, std::vector<uint8_t> threadID = std::vector<uint8_t>());
	
	bool getIsSupposedToBeUIAware();
	void setIsSupposedToBeUIAware(bool isIt=true);
	std::shared_ptr<CEndPoint> getAbstractEndpoint();
private:
	uint64_t mLastTimeEventsLinePushed;
	uint64_t mLastTimeEventsLineFlashed;
	bool mIsSupposedToBeUIAware;
	std::queue<uint8_t>  mInputChars;//input processed per byte,still we might need to buffer more than one before taken form consumer
	std::queue<std::vector<uint8_t>> mOutputByteVectors;//output processed as byte-vectors to limit undelrying web-sock wrappers
	uint64_t mProcessID;
	std::vector<uint8_t> mThreadID;

	std::mutex mOutputCharsGuardian;
	std::mutex mInputCharsGuardian;
	
	
	std::shared_ptr<CTransmissionToken> mPlayerTT;
	bool mIsServer;
	bool mInputGoesToBuffer;
	
	void setHasQuestionPending(bool hasIt=true);
	std::mutex mExternalKeyGuardian;
	char mExternalKey;//recently pressed key for external use.
	bool mInputEnabled;
	bool mViewSwtichEnabled;
	Color mTextColor;
	uint64_t mLoggedInTimeStamp;
	uint64_t getConfirmedViewIndex();
	void setConfirmedViewIndex(uint64_t index);
	uint64_t mConfirmedViewIndex = 0;// in-view prefix-index as a result of non-user-related input ex.as a result of #GridScript output
	//Local Variables - END
	uint64_t getSolidCharsLength();
	void setSolidCharsLength(uint64_t length);
	uint64_t mLastTimeViewSwitched;
	uint64_t mViewSwitchesCounter;
	bool mCursorVisible;
	//the thread safety of std::regex may vary across implementations thus to stafe of the safe side while maintaining highest performance
	//we'll be using a binary mutex for synchronizaiton.
	//the major rationale is to save on very expensive constructor of these. Copy operation is MUCH less expensive when accessng one of these.
	//we do not introduce addtional getters to save on additional dereferences.
	std::mutex mRegexLock;
	std::unique_ptr<std::regex> mANSISequenceReg;//optimization, local pointed to objects
	std::unique_ptr<std::regex> mGoTo0PosANSIReg;
	std::unique_ptr<std::regex> mX3Reg;
	std::unique_ptr<std::regex> mLocalNewLineReg;
	//std::shared_ptr<std::regex> getANSISequenceReg();//the regexes are used in two threads.
	//std::shared_ptr<std::regex> getGoTo0PosANSIReg();
	bool mWasShellRequested;
	bool mIsInShell;
	//bool mForceCursorSyncing;//even though shell might not be available
	eCaretSyncMode::eCaretSyncMode mCaretSyncMode;
	int64_t getLineOfUserInputStart();
	void setLineOfUserInputStart(int64_t lineNr);
	void incLineOfUserInputStart();
	void decLineOfUserInputStart();
	int64_t mLineOfUserInputStart;
	uint64_t mCursorInViewLine ;
	
	uint64_t getCursorInBufferLine(bool terminalRelativePos=false, bool windowRelativeePos=false,uint64_t ansiCodesWithin=0);
	void setCursorInViewLine(uint64_t viewLine);
	uint64_t incCursorInViewLine(uint64_t by=1);
	uint64_t decCursorInViewLine(uint64_t by=1);
	uint64_t getBackspaceOrdered();
	void setBackspaceOrdered(uint64_t setIt= 1);

	void setGoBackLines(int64_t lineCount);

	int64_t getGoBackLines();

	uint64_t getCarretForwardOrdered();
	void setCarretForwardOrdered(uint64_t setIt = 1);
	//Taken into effect by the Output Thread - BEGIN
	//ordered by input thread
	uint64_t mBackspaceOrdered;
	int64_t mGoBackLines;
	uint64_t mCarretForwardsOrdered;
	//Taken into effect by the Output Thread - END
	bool getIsInProcessingFlow();
	void setIsInProcessingFlow(bool isIt = true);
	uint64_t mCurrentBottomLine;
	void setCurrentBottomLine(uint64_t line);
	uint64_t getCurrentBottomLine();
	uint64_t mTerminalWidth;
	bool mOutputThreadExited;
	bool mInputThreadExited;
	uint64_t mTerminalHeight;
	void setConsoleInputBuffer(std::string buffer="");
	std::string  getConsoleInputBuffer();
	uint64_t getConsoleInputBufferSize();
	std::vector<uint8_t>  mID;//the terminalID
	std::string mUserID;//initially random ("Anon[number]"), might changed by 
	//1) explicit 'nick' command
	//2) user logged-in through a QR-Intent
	//Guardians - BEGIN
	std::mutex mDTIGuardian;
	std::recursive_mutex mQuestionsGuardian;
	std::mutex mKillMeGuardian;
	std::mutex mSolidCharsLengthGuardian;
	std::recursive_mutex  mConsoleGuardian;
	std::mutex mPrefixChangedGuardian;
	std::mutex mEventViewLinesGuardian;
	std::mutex  mWipedUserInputBufferGuardian;
	std::mutex  mFlushInputBufferGuardian;
	std::recursive_mutex mConsoleBufferGuardian;
	std::recursive_mutex mFieldsGuardian;
	semaphore mProcessingFlowLock;
	bool mIsInProcessingFlow;
	std::mutex mShuttingDownGuardian;
	std::mutex mInitGuardian;
	std::mutex mStatusRequestGuardian;
	std::mutex mStatusGuardian;
	std::recursive_mutex mPendingViewChangeGuardian;
	size_t mRushingSince;//the point in time, when user began charasing key inputs
	std::mutex mTerminalReadyGuardian;
	std::mutex mCLIModeGuardian;
	std::mutex mSSHChannelOperationGuardian;
	std::mutex mIDGuardian;
	std::mutex mTransportModeGuardian;
	std::mutex mConversationGuardian;
	std::recursive_mutex mCursorPositionGuardian;
	//Guardians - END

	std::weak_ptr<CConversation> mConversation;
	std::string getSSHClientIP(ssh_session session);
	size_t mLastWarningTimestamp;
	
	void setReady(bool isReady = true);
	void setSSHSession(ssh_session session);
	ssh_session getSSHSession();

	void setConversation(std::shared_ptr<CConversation> conversation);
	
	
	eViewState::eViewState mDefaultOutput;
	eViewState::eViewState mTargetView;

	bool mDoMarginAnimation;
	
	bool mCommitSuicide;
	std::shared_ptr<CTools> mTools;
	std::weak_ptr<CNetworkManager> mNetworkManager;
	
	std::unique_ptr<CCMDExecutor> mCMDExecutor;
	bool mWipedUserInputBuffer;
	bool mFlushInputBuffer;
	bool mPrefixChanged;
	
	std::string mCurrentPath;
	bool mAllowOnlyIntInput;
	std::string mAnswerToLatestMultiCharQuestion;

	void pushLineToView(std::string line, eViewState::eViewState view);
	void mControllerThreadF();
	bool mLastLineWasFlashed;
	void writeLineI(std::string str = "", bool appendToLog = false);
	std::vector<std::string> mPrefixHistory;
	std::vector<std::string> mCommandsHistory;
	std::vector<std::string> mEventViewLines;
	std::vector<std::string> mGridScriptDebuggerViewLines;
	std::vector<std::string> mWallViewLines;

	uint32_t mCurrentWriteType;
	bool  mIsConsoleOutputPaused;
	eFlashPosition::eFlashPosition mFlashPosition;
	bool mDataPendingToBeWritten;
	std::shared_ptr<SE::CScriptEngine> mScriptEngine;
	size_t mSessionStartedTimeStamp;
	size_t mLastInteractionTimeStamp;

	 std::thread mConsoleInputThread;//responsible for takin input from user and output
	 std::thread  mConsoleOutputThread;//resposible for managing terminal's state/view
	 std::thread mController;
	 eViewState::eViewState mCurrentView;
	 uint32_t mCommandHistoryViewIndex;
	
	 void setWipedUserInputBuffer(bool set = true);
	 void setFlushInputBuffer(bool set = true);
	 bool  getFlushInputBuffer();
	 uint64_t getCursorPositionInBuffer();
	 void setCursorPositionInBuffer(uint64_t position);
	 bool incCursorPositionInBuffer(uint64_t by = 1);
	 uint64_t decCursorPositionInView(uint64_t by=1, bool allowPastZero=false);
	 uint64_t decCursorPositionInBuffer(uint64_t by = 1,bool allowPastSolidChars=false);

	 bool sRestoreCursorPosition;
	 void setRestoreCursorPosition(bool doIt = true);
	 bool getRestoreCursorPosition();

	 void mConsoleInputThreadF();
	 std::string getNewLine();
	 void mConsoleOutputThreadF();
	 std::string mConsoleInputBuffer;
	 void flashLineI(std::string str, bool doMarginAnimation=false, uint64_t length=0, eFlashPosition::eFlashPosition position = eFlashPosition::bottom);
	
	 uint32_t mLastFlashAnim;
	 uint32_t mPreviousFlashedLineLength;
	
	 uint32_t mSolidCharsLength;
	 uint64_t mCursorPositionInBuffer;
	 uint64_t mCursorPositionInView;
	 std::string getConsolePrefix();
	 std::string mCurrentSDID;

	 eViewState::eViewState mPendingViewChange;
	 bool pendingMultiCharQuestion;
	 std::string mConsoleInputCmdBuffer;
	 bool getChangedCLIPrefix();
	 bool isHotKey(char key);
	
	 bool getWipedUserInputBuffer();
	
	
	 void clearScreenI();
	 void printOutView(eViewState::eViewState view);
	 bool isTerminalReadyForProcessing();
	 bool commitSuicide(bool wait=true);
	 bool getCommitSuicide();
	 bool mPendingQuestion;
	 bool mIsScreenWrapEnabled;
	 char mAnswerToLastestQuestion;
	 std::string mDefaultAnswer;
	 std::string mLineToBeWritten;

	 bool mTerminalReadyForProcessing;
	 eBlockchainMode::eBlockchainMode mBlockchainModeToUseByTerminal;
	 ssh_channel mSSHDataChannel;
	 ssh_session mSSHSession;
	 char getChar();
	 std::string mLastLine;
	
	 bool doCout(std::string str, bool prepareForOutputFromCurrentPos=true, eColor::eColor color = eColor::none);
	 std::string prepareForOutputFromCurrentPos(std::string outputTxt);
	 eDTISessionStatus::eDTISessionStatus mStatus;
	 eDTISessionStatus::eDTISessionStatus mStatusChangeRequest;
	 void setStatus(eDTISessionStatus::eDTISessionStatus status);
	 void doCleaningUp();
	 std::vector<ssh_channel> mSSHChannels;
	 void addSSHChannel(ssh_channel channel);
	 std::shared_ptr<CTransactionManager> mTransactionManager;
	 eDTISessionStatus::eDTISessionStatus getRequestedStatusChange();
	 
	 void refreshAbstractEndpoint(bool updateRT=true);



	 bool initSSHInternals();

	 bool initWebSockInternals();
	 uint64_t mWarningsIssued;
	 int64_t mCharsPerSec;
	 int64_t mCharsAllowedWindow;
	 int64_t mCharsWindowRefill;
	 int64_t mCharsWindowRefillAfter;
	 size_t mCharsWindowLastRefilledTimeStamp;
	 std::string mClientIP;
	 void setClientIP(std::string ip);
	 int32_t ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT;
	 int32_t getKeyCode(eKeyCode::eKeyCode key);
	 uint64_t syncCursorPosition(int64_t referencePosition = -1);
	 eTransportType::eTransportType mTransport;
	 bool getInputThreadExited();
	 bool getOutputThreadExited();

	 void setInputThreadExited(bool didIt = true);
	 void setOutputThreadExited(bool didIt = true);
	 bool getWasShellRequested();
	 void setWasShellRequested(bool wasIt = true);
	
	 void setCurrentView(eViewState::eViewState view);//from outside -  use requestViewChange()
	 std::shared_ptr<CEndPoint> mAbstractEndpoint;
	 std::shared_ptr<CIdentityToken> mIdentityToken;
	 void animateLogo();
	 bool  mPushLinesToGSView;
public:
	void setLoggedInIDToken(std::shared_ptr<CIdentityToken> token);
	std::shared_ptr<CIdentityToken>   getLoggedInIDToken();
	bool executeCode(std::string code);
	bool getPushLinesToGSView();
	void setPushLinesToGSView(bool doIt = true);

	void moveCursorUP(uint64_t steps=1);
	void clearLineToCursor();
	void clearLineAfterCursor();
	void clearLine();
	uint64_t getTerminalWidth();
	uint64_t getLoggedInTime();
	void setLoggedInTime(uint64_t time);
	void setTextColor(Color color);
	Color getTextColor();
	bool getIsInShell();
	void setIsInShell(bool isIt = true);
	bool getForceCursorSyncing();
	void setForceCursorSyncing(bool isIt = true);
	eCaretSyncMode::eCaretSyncMode getCaretSyncMode();
	void setCaretSyncMode(eCaretSyncMode::eCaretSyncMode mode);
	uint64_t getLastTimeViewSwitched();
	void setLastTimeViewSwitched(uint64_t time);
	void pingViewSwitched();
	void moveCursorDOWN(uint64_t steps=1);
	void moveCursorLEFT(uint64_t steps=1);
	void moveCursorRIGHT(uint64_t steps=1);
	void setTextColor(uint8_t R=255, uint8_t G= 255, uint8_t B= 255,bool isForeground=true);
	void clearFormating();
	void slowBlink();
	std::string getUserID();
	void setUserID(std::string id);
	eViewState::eViewState getCurrentView();
	bool getIsCursorVisible();
	void setIsCursorVisible(bool isIt = true);
	void requestShell();
	bool getIsInputEnabled();
	void setIsInputEnabled(bool isIt);
	bool getIsViewSwitchEnabled();
	void setIsViewSwitchEnabled(bool isIt);
	bool getInputGoesToBuffer();
	void setInputGoesToBuffer(bool doesIt = true);
	eFlashPosition::eFlashPosition getFlashPosition();
	void changeRealm(eBlockchainMode::eBlockchainMode realm);
	bool processNetMsg(std::shared_ptr<CNetMsg> msg, const std::vector<uint8_t> &receiptID = std::vector<uint8_t>(),std::shared_ptr<CConversation>dataConv=nullptr);
	std::shared_ptr<CConversation>  getConversation();
	std::shared_ptr<SE::CScriptEngine> getScriptEngine();
	std::shared_ptr<CTransactionManager> getTransactionManager();
	std::vector<uint8_t> registerQRIntentResponse(std::shared_ptr<CQRIntentResponse> response);
	std::vector<uint8_t> getID();
	std::vector<uint8_t> registerVMMetaDataResponse(std::vector<std::shared_ptr<CVMMetaSection>>);

	void flashLine(std::string str, bool doMarginAnimation = true, eViewState::eViewState view = eViewState::eViewState::eventView, eFlashPosition::eFlashPosition position = eFlashPosition::bottom);
	~CDTI();
	std::string getClientIP();
	void logEvent(eDTIEvent::eDTIEvent eType);
	size_t getEventTime(eDTIEvent::eDTIEvent eType);
	void setChangedCLIPrefix(bool set=true);
	uint64_t getCursorPositionInView();
	void setCursorPositionInView(uint64_t inView);
	bool incCursorPositionInView(uint64_t by=1, bool allowPastTerminalBorder=false);
	void setCurrentSD(std::string sdID);
	void setCurrentPath(std::string path);
	void setBlockchainMode(eBlockchainMode::eBlockchainMode mode);
	eBlockchainMode::eBlockchainMode getBlockchainMode();
	void requestStatusChange(eDTISessionStatus::eDTISessionStatus status);
	eDTISessionStatus::eDTISessionStatus getStatus();
	std::string askString(std::string question, std::string defaultV = "", std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false);
	std::string  getAnswerToLatestMultiCharQuestion();
	BigSInt askInt(std::string question, BigSInt defaultV = 1, std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false);
	bool doCout(char c);
	char getAnswerToLatestQuestion();
	void forceCommandLineMode(eViewState::eViewState = eViewState::eViewState::GridScriptConsole);
	void ringABell();
	bool askYesNo(std::string question, bool defaultV, std::string whoAsks = "", bool ommitBlockchainMode = false, bool ommitOwner = false);
	bool activateView(eViewState::eViewState view, bool blocking = true);
	bool initialize(ssh_session session=nullptr);
	bool initialize(std::shared_ptr<CConversation> conversation = nullptr);
	CDTI(eBlockchainMode::eBlockchainMode mode, eTransportType::eTransportType transport = eTransportType::SSH);
	uint64_t getCharsPerSec();
	eTransportType::eTransportType getTransportType();
	void decCharsWindow();
	bool isCharsWindowEmpty();
	void incCharsPerSec();
	bool shutdown();
	bool isAlive();
	size_t getLastInteractionTimeStamp();
	std::string getRemoteHostIPAddress();
	bool getIsRemoteTerminal();
	uint64_t getWarningsCount();
	void writeLine(std::string str = "", bool endLine = true, eViewState::eViewState view = eViewState::eViewState::unspecified, std::string whoSays = "", bool preventDuplicates=true, bool wait=true);

	bool keepRunning();

};