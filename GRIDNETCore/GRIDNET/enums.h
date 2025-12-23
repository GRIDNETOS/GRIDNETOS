#pragma once


#define UI_VERSION "1.1.1 Alpha"
#define WEB_SERVER_VERSION "0.9.9 Alpha"
//#include "icftypes.h"



#ifdef IS_QT_BUILD
#include <QtEndian>
#define ntohl qFromBigEndian
#define ntohs qFromBigEndian
#define htonl qToBigEndian
#define htons qToBigEndian
#ifndef inet_ntoa
#include <arpa/inet.h>
#endif
#endif

namespace eDataAssetType
{
	enum eDataAssetType {
		file,
		dynamicObject// chain-proof has 'chain_proof' object name
	};
}

namespace eBlockAvailabilityConfidence
{
	enum eBlockAvailabilityConfidence
	{
		unavailable,
		available,
		possiblyAvailable
	};
}

namespace eIncomingConnectionResult
{
	enum eIncomingConnectionResult
	{
		allowed,
		DOS,
		limitReached,
		onlyBootstrapNodesAllowed,
		insufficientResources
	};
}

namespace eSetBlockCacheResult {
	enum  eSetBlockCacheResult {
		Success,
		NullBlock,
		EmptyCacheSuccess,
		InvalidKeyBlockInKeyCache,
		PreviousBlockExpired,
		NonContiguousHeight,
		NoValidAdjacentBlocks,
		KeyCacheInsertionFailed,
		UnknownError
	};
}

namespace eFirewallDirection {
	enum eFirewallDirection {
		INBOUND =1, //= 1NET_FW_RULE_DIR_IN,
		OUTBOUND //= NET_FW_RULE_DIR_OUT
	};
}
#ifndef ThreadPriorityD
#define ThreadPriorityD
enum class ThreadPriority {
	LOWEST,
	LOW,
	NORMAL,
	HIGH,
	HIGHEST
};
#endif // !ThreadPriorityD


namespace eViewState
{
	enum eViewState
	{
		eventView,
		GridScriptConsole,
		unspecified,
		Wall
	};
}
namespace eFirewallProtocol
{
	enum eFirewallProtocol {
		TCP,
		UDP
	};
}
namespace eJSONColor {
	enum class eJSONColor {
		BLUE,
		CYAN,
		RED,
		YELLOW,
		WHITE,
		ORANGE,
		GREEN
	};
}


namespace eKademliaSubsystem
{
	enum eKademliaSubsystem
	{
		engine,
		discover_neighbors_task,
		routing_table,
		find_value_task,
		store_value_task,
		lookup_task,
		notify_peer_task,
		response_router
	};
}


namespace eBinaryStrEncFormat
{
	enum eBinaryStrEncFormat
	{
		ASCII,
		base58Check,
		base64Check
	};
}
namespace eTerminalFormatTag
{
	enum eTerminalFormatTag
	{	none,
		truncateAtLineEnd,
		disableRendering
	};
}
namespace eConnCapabilities
{
	enum eConnCapabilities
	{
		data,
		audio,
		video,
		audioVideo
	};
}


namespace eQRIntentType
{
	enum eQRIntentType
	{
		QRSign,
		QRShow,
		QRStore,
		QRImportSK,
		QRImportEncSK,
		QRImportContact,
		QRUnknown,
		QRProcessVMMetaData, //for example request token pool generation
		QRLogMeIn
	};
}

namespace eIdentityTokenType
{
	enum eIdentityTokenType
	{
		identityOnly,
		PoW,
		Stake,
		Hybrid,
		Basic
	};


}
namespace eKeyCode
{
	enum eKeyCode
	{
		arrowUp,
		arrowDown,
		arrowLeft,
		arrowRight
	};


}
namespace eDTISessionStatus
{
	enum eDTISessionStatus
	{
		unknown, //used by state-change-request to indicate that no change is pending.
		initial,
		alive,
		ended
	};

}

namespace eDFSSessionStatus
{
	enum eDFSSessionStatus
	{
		initial,
		alive,
		ended
	};

}

namespace eDTIEvent
{
	enum eDTIEvent
	{
		interacted,
		sessionStarted,
		warningIssued
	};

}

namespace eFileMode
{
	enum eFileMode
	{
		string,
		binary
	};

}


namespace eTransactionsManagerMode
{
	enum eTransactionsManagerMode
	{
		LIVE,
		VerificationFlow,
		FormationFlow,
		Terminal
	};

}

namespace eEpochRewardCalculationMode
{
	enum eEpochRewardCalculationMode
	{
		total,
		ALeader, // when calculating reward for present block from pespective of current leader
				 // [ SECURITY ]: we rely on the fact that all ALeader calculations are prformed locally during current/ unitary leader candidate
				 //				  block verification and thus there's no reliance on from-the-past data structures.
		BLeader  // when calcilating reward for past EPOCH including prior 60% from 90 GNC and any fees from Data Blocks
				 // [ SECURITY ]: we rely on the fact that prior blocks were verified locally.

	};
}
namespace eDataPubResult
{
	enum eDataPubResult
	{
		success,
		failure
	};
}

/// <summary>
/// Defines the temporal pattern for kernel mode codeword execution overrides.
/// Controls when a codeword's kernel mode permission changes relative to key-block heights.
/// Used by the backwards compatibility layer for GridScript bytecode processing.
/// </summary>
namespace eCodewordOverrideType
{
	enum eCodewordOverrideType
	{
		/// Enable kernel mode execution starting from specified key-height (inclusive).
		/// Before key-height: uses default codeword permission
		/// At/After key-height: enabled in kernel mode
		/// Example: Allow 'newfeature' in kernel mode from key-height 150000 onwards
		EnableFrom = 0,

		/// Disable kernel mode execution starting from specified key-height (inclusive).
		/// Before key-height: uses default codeword permission
		/// At/After key-height: disabled in kernel mode
		/// Example: Disable 'txconfig' in kernel mode from key-height 108058 onwards
		DisableFrom = 1,

		/// Enable kernel mode execution until specified key-height (exclusive).
		/// Before key-height: enabled in kernel mode
		/// At/After key-height: uses default codeword permission
		/// Example: Temporarily enable 'migration' until key-height 120000
		EnableUntil = 2,

		/// Disable kernel mode execution until specified key-height (exclusive).
		/// Before key-height: disabled in kernel mode
		/// At/After key-height: uses default codeword permission
		/// Example: Temporarily disable 'experimental' until key-height 140000
		DisableUntil = 3
	};
}

namespace eCompileMode
{
	enum eCompileMode
	{
		FAPIImplicit,
		FAPIExplicit,
		PreAuthentication,
		Invalid
	};
}
namespace eDataType
{
enum eDataType
{
	signedInteger,
	unsignedInteger,
	booll,
	charr,
	doublee,
	bytes,
	pointer,
	noData,
	directory,
	BigInt,
	BigSInt,
	BigDouble
};
}
namespace eManagerStatus
{
	enum eManagerStatus 
	{
		running,
		paused,
		stopped,
		initial
	};
}

namespace eSecFetchMode
{
	enum eSecFetchMode
	{
		notSet,
		cors,
		navigate,
		noCors,
		sameOrigin,
		websocket
	};
}
namespace eBlockProcessingStatus
{
	enum eBlockProcessingStatus
	{
		idle,
		preProcessing,
		processing,
		validating,
		commiting,
		PostProcessing,
	};
}
namespace eDatabaseType
{
	enum eDatabaseType
	{
		BlockchainDB,
		StateDB,
		StagedStateDB
	};
}

namespace eBlockchainMode
{
	enum eBlockchainMode
	{
		LIVE,
		TestNet,
		LIVESandBox,//without access to SolidStorage at all.//refreshed on demand.
		TestNetSandBox,//without access to SolidStorage at all.//refreshed on demand
		LocalData,
		Unknown
	};
}
namespace eParamInsertionMethod
{
	enum eParamInsertionMethod
	{
		number,
		base58encoded,
		RAWString,
		remove,
		base64encoded
	};
}
namespace eCMDExecutorState
{
	enum eCMDExecutorState
	{
		GridScriptDebugger = 1,
		ViewControl = 0
	};

}

namespace eBlockInfoRetrievalResult
{
	enum eBlockInfoRetrievalResult
	{
		blockBodyUnavailable,
		OK
	};
}
namespace eNodeType
{
	enum eNodeType
	{
		extNode=1,
		branchNode=2,
		leafNode=3
	};
}
namespace eNodeSubType
{
	enum eNodeSubType
	{
		RAW=0,
		stateDomain=1,
		transaction=2,
		receipt=3,
		verifiable=4,
		TrieDB=5

	};
}
namespace eOperationStatus
{
	enum eOperationStatus
	{
		success,
		failure,
		inProgress,
		interrupted
	};
}




namespace eHTTPMsgMode
{
	enum eHTTPMsgMode
	{
		whole,
		chunk,
		headeredChunk
	};
}
namespace eWebSocketTransferMode
{
	enum eWebSocketTransferMode
	{
		text,
		binary
	};
}

namespace eBlockType
{
	enum eBlockType
	{
		keyBlock,
		dataBlock
	};
}
namespace eSearchResultElemType
{
	enum  eSearchResultElemType {
		transaction,
		block,
		domain
	};
}

namespace eOperationScope
{
	enum eOperationScope
	{
		dataTransit,//the operation is/was just about data-delivery
		peer,//local peer scope
		VM,//decentralized VM scope
		other,
		Consensus//Operation Scope above the VM scope. Indicates processing result in scope of a particular full-node.
		/*Ex. if a node deems the ERG bid not to be enough or locally known nonce associated with a particular State-Domain to be invalid,
		* an Operation Status notification of this scope would be generated and routed to transaction's originator.
		*/
	};
}
namespace eBlockVerificationResult {
	enum eBlockVerificationResult { 
		valid,
		invalid, 
		unknownBlockOnPath,
		unknownParent,
		insufficientTotalPoW, 
		insufficientPoW,
		parentPerspectiveUnavailavble,
		inconsistentFinalPerspective,
		flowInvalid,
		unableToStartFlow,
		receiptMissing,
		transactionMissing, 
		verifiableMissing,
		criticalFailure,
		timeDiffExceeded,
		headerMissing,
		invalidSignature,
		pubKeyMissing,
		sigMissing,
		noCorrespondingKeyBlock,
		keyParentUnknown,
		invalidBlockHeight,
		rewardVerifiableMissing,
		invalidKeyHeight,
		leaderAttemptedADoubleSpendAttack,
		invalidTotalFeesField,
		parentNotALeaderInVerifiedChainProof,
		heightNotToBeConsidered,
		notAForkLeadingBlock,
		invalidSecurityExecutionContext,
		incoherentInitialPerspective,
		excessivePoW,
		prevalidationOnly

	};
}
namespace eChainProofValidationResult
{
	enum eChainProofValidationResult
	{
		valid = 0,                            // Must be at index 0
		invalidGeneral,						  // invalid, no specific error code
		multiThreadValidationFailed,          // Multi-threaded sub-check returned false
		emptyChainProof,                      // chainProof.size() == 0
		invalidStartPosition,                 // e.g. startFromPosition > known PoW cache
		missingObligatoryCheckpoint,          // Checkpoint check fails
		blockHeaderInstantiationFailed,       // CBlockHeader::instantiate(...) returned nullptr
		invalidBlockHeight,                   // Mismatch between 'i' and header->getHeight()
		invalidKeyBlockPubKey,                // Key block pubkey != 32 bytes
		invalidSignature,                     // mCryptoFactory->verifyBlockSignature(...) == false
		invalidPoW,                           // mCryptoFactory->verifyNonce(...) failed (and not checkpointed/hard-forked)
		invalidParentReference,               // The current block's parentID doesn�t match
		invalidKeyHeight,                     // KeyHeight mismatch among consecutive key-blocks
		noKeyBlockFound,                      // Never encountered any key-block in the chain-proof
		blockTimeOutOfRange,                  // Time-stamp checks fail (into-the-future or into-the-past)
		blackListedBlockFound,                // A block was found to be blacklisted
		internalException,                    // Catch-all for thrown exceptions
		misplacedHardForkBlock,				  // missing checkpoint in code
		excessivePoW,						  // excessive PoW encountered in one of the blocks
		invalidCoreVersion					  // invalid Core version for this height
	};
}

namespace eChainProofUpdateResult
{
	enum eChainProofUpdateResult
	{
		updated,
		updatedLocalBestKnown,
		totalDiffLower,
		noCommonPointFound,
		invalidData,
		Error
	};
}
namespace eChainProofCommitResult
{
	enum eChainProofCommitResult
	{
		blocksEnqued,
		idle,
		error,
		empty
	};
}

namespace eChainProof
{
	enum eChainProof
	{
		verified,
		verifiedCached,//Notice: CANNOT be used during a Flow. For informative purposes only.
		heaviest,
		heaviestCached, //Notice: CANNOT be used during a Flow. For informative purposes only.
		fullTemporary, //used when invoking updateChainproofWithpath(0
		partial        // when processing partial chain proofs received from network; no checkpoint and cumiulative PoW checks
	};
}
namespace eBERObjectType
{
	enum eBERObjectType
	{
		keyBlock,
		keyBlockHeader,
		regularBlock,
		regularBlockHeader
	};
}
namespace eBlockInstantiationResult {
enum eBlockInstantiationResult {
	OK,
	Failure,
	headerInstantationFailure,
	parentMissing,
	blockFetchedFromCache,
	blockFetchedFromSS,
	blockFetchedFromPointer,
	blockDataUnavailableInCS,
	blockInstantiatedTransactionTrieNodesMissing,
	blockInstantiatedVerifiablesTrieNodesMissing,
	blockInstantiatedReceiptsTrieNodesMissing,
};
}
namespace eReceiptType
{
	enum eReceiptType {
		transaction,
		verifiable
	};
}
namespace  eVerifiableType
{
	enum eVerifiableType
	{
		minerReward,
		GenesisRewards,
		uncleBlock,
		dataPropagation,
		powerGeneration,
		powerTransit,
		proofOfFraud
	};
}
namespace eLinkType
{
	//we can traverse from Receipt's GUID, to Receipt's HASH and then
	//used that HASH to find in WHICH BLOCK the receipt is stored.
	//receipts, transactions and verifiables are retrieved from Blocks by their HASH.
	enum eLinkType
	{
		receiptsGUIDtoReceiptsHash,
		transactionHashToBlockID,
		verifiableHashToBlockID,
		receiptHashToBlockID,
		BHeightPKtoBlockHeader, //hash([blockHeight,LeaderPK]) to BlockHeader bytes *Used for proof-of-fraud*// the block does not need to be stored locally, but the body of link provides header
		PoFIDtoReceiptID,//used by Proof-of-Fraud
		friendlyIDtoAddr
	};
}
namespace eSysDir
{
	enum eSysDir
	{
		consumedSacrifices,
		state,
		identityTokens,
		tokenPools
	};
}

namespace eTokenPoolStatus
{
	enum eTokenPoolStatus
	{
		active,
		depleted,//only if all the banks within it became depleted
		banned//might be a result of misuese of at least one of its banks
	};
}
namespace eTokenPoolBankStatus
{
	enum eTokenPoolBankStatus
	{
		active,
		depleted
	};
}

namespace eICEServerType
{
	enum eICEServerType
	{
		STUN,
		TURN
	};
}

namespace eSDPEntityType
{
	enum eSDPEntityType
	{
		joining,//peer issued
		bye,//peer issued
		getOffer, //full-node issued. The full node asks existing swarm members to prepare an invitation for the new peer
		processOffer, //peer-issued, routed
		processOfferResponse,
		processICE,//peer-issued, routed
		control,//full-node issued (further info within eSDPControlStatus)
	    data,
		pingFullNode, //peer-issued. The sole purpose is to ping the WebSocket Conversation for it not to be shut-down automatically by full-node
		//the interval should be reasonable i.e. >=1 minute so not to cause overheads. The interval needs to be > than the automatic shut-down threshold. 
		//the responsibility of doing this lies on the Client UI dApp (including the in-browser VM-Context should it ever need access to the WebSocket conversation)
		//Note that the web-client's CSwarm will attempt to uphold the connection autonomously in its controller thread UNTILL shall the user/client UI dApp
		//indicate its will to quit the Swarm by calling CSwarm.quit(). UI dApp SHOULD call quit for instance when its window is closed (if it does not run in the background).
		pingPeer,//peer-issued
	};

}

namespace eSDPControlStatus
{
	enum eSDPControlStatus
	{   ok,
		joined,
		kickedOut,
		error,
		invalidIdentity,
		banned,
		nodeLimitsExceeded,
		swarmClosing,
		xToOtherFullNode//proposal of other full-node included in meta-data
	};

}


namespace eDeletedType
{
	enum eDeletedType
	{
		TrieDB
	};
}
namespace eFraudCheckResult
{
	enum eFraudCheckResult
	{
		keyBlockUnavailable,
		newFraud,
		OK,
		error,
		alreadyReported
	};
}
namespace eLogEntryCategory //Important: log events with priority 0 are considered as debug info despite their category and are NOT displayed within the terminal.
{
	enum eLogEntryCategory
	{
		network,//*WARNING: KEEP THIS ITEM FIRST* <- used by iterators
		localSystem,
		VM,
		debug,//DISABLED by default (performance)
		unknown//*WARNING: KEEP THIS ITEM LAST* <- used by iterators
	};
}

namespace eTTCollectionResult
{
	enum eTTCollectionResult
	{
		collected,//we can cash it out
		noTT,//no reward (TT) specified
		notForMe,//there's a targeted TT, but not targeting the local peer
		invalidTT
	};
}

namespace eLogEntryType
{
	enum eLogEntryType
	{
		notification,//*WARNING: KEEP THIS ITEM FIRST* <- used by iterators
		warning,
		failure,
		unknown//*WARNING: KEEP THIS ITEM LAST* <- used by iterators
	};
}

namespace eRouteKowledgeSource
{
	enum eRouteKowledgeSource
	{
		propagation,
		Kademlia,
		manual,
		WebRTCSwarm,
		sessionHandshake
	};
}
namespace eChatMsgType
{
	enum eChatMsgType {
		text,
		file,
		typing,
		avatar
	};
}



namespace eGridcraftMsgType
{
	enum eGridcraftMsgType {
		ping,//boards player's position, nickname and current transmission token
		ready,//sent by full-node when ready to receive ping messages
		failure,//ex. when token-pool depleted or no longer available. may include additional info.
		exit,
		goldVeinInfo,
		collected,//confirms collected hash, which is when new vein is generated
		newVein,//collection confirmed by full-node, new vein available
		processingOfGold
	};
}

namespace eCaretSyncMode
{
	enum eCaretSyncMode
	{
		allowed,
		forced,
		disabled
	};
}

namespace eTerminalDataType
{
	enum eTerminalDataType
	{
		input,
		output,
		windowDimensions
	};
}

namespace ePollActionID
{
	enum ePollActionID
	{
		none,//WARNING: the order does matter as pollEx sorts these in ascending order during processing.
		create,//or update
		activate,
		vote,
		checkstatus,
		powerCheck
		
	};
}
namespace eVoteResult
{
	enum eVoteResult
	{
		success,
		noVotingPower,
		alreadyVoted,
		failure,
		exclusiveGroupAlreadyFired
	};
}
namespace eSystemPollID
{
	enum eSystemPollID
	{
		writeAccessRightsPoll,
		executeAccessRightsPoll,
		ownershipChangeAccessRightsPoll,
		removalAccessRightsPoll,
		spendingsAcessRightsPoll
	};
}
namespace eCoreServiceID
{
	enum eCoreServiceID
	{
		unknown = 0,
		write = 257,
		wall = 256,
		snake = 255,
		commit = 254,
		logMeIn = 253,
		genPool = 252,
		minecraft = 251,
		hexi = 250,
		terminalCode = 1
	};
}
namespace eConvEndReason
{
	enum  eConvEndReason
	{
		otherEndTerminatedAbruptly,
		none,
		duplicateStream,
		security,
		QUICTransportLayer
	};
	
}

namespace eEndpointType
{
	enum eEndpointType
	{
		IPv4,
		IPv6,
		MAC,
		PeerID,//invariant of the transport layer
		TerminalID,
		WebSockConversation,
		UDTConversation,
		WebRTCSwarm,
		VM,
		SwarmMember,
		QUICConversation,
		MaxValue
	};
}


namespace eAddressType
{
	enum eAddressType
	{
		IPv4,
		IPv6,
		MAC,
		PeerID,
		TerminalID
	};

}
namespace eNetTaskProcessingResult
{
	enum eNetTaskProcessingResult
	{
		succeeded,
		aborted,
		inProgress,
		error
	};

}

namespace eDFSTaskProcessingResult
{
	enum eDFSTaskProcessingResult
	{
		succeeded,
		aborted,
		inProgress,
		error
	};

}

namespace eColumnFieldAlignment
{
	enum eColumnFieldAlignment
	{
		left,
		right,
		center
	};
}

namespace eNetEntType
{
	enum eNetEntType//>256 - application specific
	{
		unknown,
		block,//just a block-HEADER
		transaction,
		verifiable,
		msg,
		chainProof,
		longestPath,
		sec,
		receiptBody,
		receiptID,
		ping,
		hello,//contains client information, version etc. eReqType == request=>signature request of data within extraBytes; notify=>contains signature within extraBytes
		bye,
		QRIntentResponse,
		DFS,
		VMStatus,
		ConnectionStatus,
		VMMetaData,//might encapsulate QRIntentRequest,
		Swarm,
		NetMsg,//tunneling
		OperationStatus,
		sessionInfo,//same as hello but requires session to be established first. Required to carry extended CSessionDescription as payload.
		wrapper,//datagram is supposed to wrap yet another CNetMsg container.
		blockBody,
		Kademlia,
		MaxValue
	};
}

namespace eNetworkSystem
{
	enum eNetworkSystem
	{
		unknown,
		putOnStack,
		Kademlia,
		UDT,
		HTTPProxy,
		WebServer,
		WebRTC,
		WebSocket,
		QUIC,
		SSH
	};
}

namespace eReqDataType {
	enum eReqDataType
	{
		text,
		number,
		binary
	};
}


namespace eNetReqType {
	enum eNetReqType
	{
		unknown,
		notify,
		request,
		process,
		route,
		MaxValue
	};
}

namespace eConsensusTaskType
{
	enum eConsensusTaskType
	{
		DFS,
		readyForCommit,
		commitAborted,
		threadCommitted,
		threadCommitPending
	};
}
namespace eVMStatus
{
	enum eVMStatus
	{
		initializing,
		ready,
		aboutToShutDown,
		disabled,//Note: for commit status use DFS-messages and its commitPending/commitSucceeded/commitAborted flags
		limitReached,
		errored,
		synced,
		newPerspectiveAvailable
	};
}
namespace eConversationState
{
	enum eConversationState
	{
		initial,
		initializing,
		hello,
		dataExchange,
		ending,
		ended,
		running,
		unableToConnect,
		idle,
		bye,
		connecting
	};
}

namespace eHTTPMethod
{
	enum eHTTPMethod
	{
		GET,
		HEAD,
		POST,
		PUT,
		eDELETE,
		CONNECT,
		OPTIONS,
		TRACE,
		PATCH,
		UNKNOWN
	};
}

namespace eHttpVersion
{
	enum eHttpVersion
	{
		v10,
		v11,
		v20,
		unknown
	};
}
namespace eWebEncoding
{
	enum eWebEncoding
	{
		GZip,
		deflate,
		Brotli,
		unknown,
		none
	};
}
namespace eConnectionStatus
{
	enum eConnectionStatus
	{
		initializing,
		ready,
		aboutToShutDown,
		disabled
	};
}
namespace eVMMetaCodeExecutionMode
{
	enum eVMMetaCodeExecutionMode {
		RAW,
		RAWDetached,//creates a new worker thread; alive during particular code-package execution only.
		GUI,
		GUITerminal
	};
}
//TRIPPLE note: DFS has DFS-specific Sections, Elements AND DFS-level protocol specific entry-types (e.x. state-domain/folder/file - eDFSElementType)
namespace eVMMetaSectionType
{
	// DFS elements have their own VM-Meta Sections. That's for backwards compatbility.
	//any new protocol should have its own VMMetaSectionType
	enum eVMMetaSectionType
	{
		unknown,
		directoryListing,
		fileContents,
		notifications,
		requests,
		stateLessChannels,//used for OBJECTS' deliveries. requests for token pool generation etc are carried out through data-request VMMetaData.
		statistics,// specialised section containing historical or current data describing the DSM.
		responses// we basically do not employ this. We use 'notifications' with a requestID set instead.
        //sections/entries
	};
}

namespace eRTUpdateReason
{
	enum eRTUpdateReason
	{
		newPeer,
		shorterPath,
		newerPath,
		cheaperPath
	};
}
// DFS elements have their own eVMMetaEntryType types. That's for backwards compatbility.
//any new protocol should have just a single eVMMetaEntryType and describe protocol specific subtypes using data-field
  //TRIPPLE note: DFS has DFS-specific Sections, Elements AND DFS-level protocol specific entry-types (e.x. state-domain/folder/file - eDFSElementType)
namespace eVMMetaEntryType//stored within stateLessChannels sectionVMStatus
{
	enum eVMMetaEntryType
	{	unknown,
		directoryEntry,//DFS - then it might be any eDFSElementType::eDFSElementType within desribed using protocol-level data-field
		fileContent,//DFS
		notification,
		dataRequest,// ex. used to request delivery  of TP's banks' auth data from mobile app by the UI; Full-node: used to request a  dialog box asking for a String to be shown to user 
		dataResponse,// ex. used for deliveries of updates to the Token-Pool's banks by the mobile app  to web-ui; used to deliver String previously asked for by full-node from web-ui
		GridScriptCode,
		GridScriptCodeResponse,
		terminalData,
		StateLessChannelsElement,
		appNotification,
		VMStatus, //deprecates the CNetMsg, connection-wide notificaitons
		threadOperation,
		consensusTask,
		txProof,

		// Blockchain Explorer - BEGIN
		
		// History - BEGIN
		transactionsHistory,
		blocksHistory,
		// History - END

		// Details - BEGIN
		transactionDetails,	
		blockDetails,
		domainDetails,
		// Details - END

		searchResults,
		blockchainHeight, //both key and data
		usdtPrice,
		livenessInfo,
		blockchainStatus,
		transactionDailyStats,
		marketDepth,// 'market' GridScript comman
		preCompiledTransaction // For submitting locally compiled and signed transactions

		// Blockchain Explorer - END
	};
}
namespace eLivenessState
{
	enum eLivenessState  {
	noLiveness,
	lowLiveness,
	mediumLiveness,
	highLiveness
	};

}


/**
 * @brief Namespace containing breakpoint type enumeration
 */
namespace eBreakpointType {
	/**
	 * @brief Enumeration of possible breakpoint types
	 */
	enum eBreakpointType : uint8_t {
		code = 0,           ///< GridScript code breakpoint
		block = 1,          ///< Block-level breakpoint
		transaction = 2     ///< Transaction-level breakpoint
	};
}

/**
 * @brief Namespace containing breakpoint condition types
 */
namespace eBreakpointCondition {
	/**
	 * @brief Enumeration of possible breakpoint condition types
	 */
	enum eBreakpointCondition : uint8_t {
		none = 0,           ///< No specific condition
		height = 1,         ///< Break at specific block height
		keyHeight = 2,      ///< Break at specific key block height
		blockID = 3,        ///< Break at specific block ID
		receiptID = 4,      ///< Break at specific transaction receipt ID
		txSource = 5,       ///< Break at transactions from specific source
		txDestination = 6   ///< Break at transactions to specific destination
	};
}
/**
 * @brief Namespace containing breakpoint state enumeration
 */
namespace eBreakpointState {
	/**
	 * @brief Enumeration of possible breakpoint execution states
	 */
	enum eBreakpointState : uint8_t {
		none = 0,           ///< No execution state
		preExecution = 1,   ///< Before execution
		postExecution = 2   ///< After execution
	};
}


namespace eTxType
{
	enum eTxType
	{
	transfer, // [�Transfer� in UI]. Invocation of regular �send� GridScript command.
	blockReward,// [�Mined� in UI]. Block reward.
	offChain, // [�Instant� in UI]. An off-the-chain transfer from a Token Pool.
	offChainCashOut, // [�instant (completed)� in UI]. An on-the-chain cash-out of a Transmission Token.
	contract,// [�Contract� in UI]. An on-the-chain transfer, smart-contract evoked.
	genesisReward
	};
}

namespace eThreadOperationType
{//for reporting of status use  eConsensusOperationStatus
	enum eThreadOperationType
	{
		newThread,
		freeThread,
		threadAliveConfirmation,
		threadKilledConfirmation
	};
}

namespace eStateLessChannelsElementType
{
	enum eStateLessChannelsElementType
	{
		token,
		tokenPool,
		transitPool, 
		receipt,
		bankUpdate//prvides provisioning of a certain dimenion's sub-space
	};
}
//TRIPPLE note: DFS has DFS-specific Sections, Elements AND DFS-level protocol specific entry-types (e.x. state-domain/folder/file - eDFSElementType)
namespace eDFSElementType//note that DFS additionally has  VM MetaData specific sections and Elements!
{
	enum eDFSElementType
	{
		unknown,
		stateDomainEntry,
		fileEntry,
		directoryEntry,
		fileContent
	};
}

namespace eDataRequestType
{
	enum eDataRequestType //
	{
		unknown,
		QRIntentAuth,
		showRAWData,
		string,
		integer,
		binary,
		boolean,
		banksAuth,// the token-pool's id is delivered within the default-value field,
		tokenPoolGeneration,
		copyToClipboard,
		password,  // Added for keychain unlock prompts
		signature, // Added for returning signature data
		error     // Added for returning error messages
	};
}

namespace eTrieSize
{
	enum eTrieSize
	{
		smallTrie,
		mediumTrie,
		largeTrie
	};
}
namespace CategoryType
{
	enum CategoryType
	{
		Transaction,
		Block,
		Domain
	};
}

namespace eSecurityReportSortMode {
	enum eSecurityReportSortMode {
		confidenceDesc,          // Default - Sort by confidence level only (descending)
		hybridDesc,             // Combined score of attacks and confidence (descending)
		timestampAttacksDesc,   // Sort by timestamp attacks count (descending)
		powAttacksDesc,         // Sort by PoW attacks count (descending)
		totalAttacksDesc,       // Sort by total number of attacks (descending)
		recentActivityDesc      // Sort by most recent attack timestamp (descending)
	};
}

namespace eColor
{
	// Using smaller values for flags to avoid interference with enum values
	constexpr int BLINK_FLAG = 0x100;     // 256  - Normal blink (SGR 5)
	constexpr int RAPID_BLINK_FLAG = 0x200; // 512 - Fast blink (SGR 6)
	constexpr int BACKGROUND_FLAG = 0x400; // 1024 
	constexpr int BOLD_FLAG = 0x800;      // 2048 - Bold/intensity (SGR 1)

	// Helper to extract color and flags
	constexpr int COLOR_MASK = 0xFF;      // 255  (binary: 0000 0000 1111 1111)

	// Let's add some debug helpers
	static bool isBlinking(int flags) { return (flags & BLINK_FLAG) != 0; }
	static bool isBackground(int flags) { return (flags & BACKGROUND_FLAG) != 0; }
	static bool isBold(int flags) { return (flags & BOLD_FLAG) != 0; }
	static int getBaseColor(int flags) { return flags & COLOR_MASK; }

	enum eColor
	{
		none,
		// Base colors
		lightWhite,
		lightGreen,
		lightCyan,
		lightPink,
		blue,
		orange,
		greyWhiteBox,
		cyberWine,
		lightCyberWine,
		cyborgBlood,
		cyanOnBlack,
		// Extended palette - Neons
		neonPurple,      // Bright purple for highlights
		neonBlue,        // Electric blue for energy indicators
		neonGreen,       // Matrix-style green
		toxicGreen,      // Slightly yellower variant
		// UI Elements
		headerCyan,      // Cyan text for headers
		alertWarning,    // Blinking orange for warnings
		alertError,      // Blinking red for errors
		alertSuccess,    // Pulsing green for success
		// Cyberpunk Accents
		synthPink,       // Hot pink accent
		plasmaTeal,      // Bright teal for energy effects
		cyberYellow,     // Warm yellow for highlights
		ghostWhite,      // Subtle white for secondary text
		// Status Colors
		statusOnline,    // Bright green pulse
		statusOffline,   // Dim red
		statusIdle,      // Slow blinking yellow
		// Data Visualization
		dataPrimary,     // Bright cyan for primary data
		dataSecondary,   // Purple for secondary data
		dataTertiary,    // Pink for tertiary data
		dataHighlight    // Bright white for highlighted data
	};
}
namespace eSysPage
{
	enum eSysPage
	{
		IPBlocked,
		maintenace
	};
}
namespace eNotificationType
{
	enum eNotificationType
	{
		unknown,
		notification,
		warning,
		error,
		binary,
		copyToClipboard
	};
}
//TRIPPLE note: DFS has DFS-specific Sections, Elements AND DFS-level protocol specific entry-types (e.x. state-domain/folder/file - eDFSElementType)
namespace eDFSCmdType
{
	enum eDFSCmdType
	{
		unknown,
		init,//initiates decentralized file-sub system
		ready,//ready-indicator
		error,//error - more info in mData1
		exists,// check if path exists
		stat,
		execute,//executes dApp
		readDir,//list directory content (LS)
		readFile,//retrieve file ; path in mData1 (less)
		writeFile,//write file; path in mData1, content in mData2 (cat)
		rename,//rename file/dir. pathA in mData1, pathB in mData2
		copy,//copy file/dir. pathA in mData1, pathB in mData2
		unlink,
		search,//find file/dir. Regex in mData1
		touch,
		sessionStat,
		requestCommit,//request QRIntent
		QRIntentData,//serialized QRIntent in mData1
		commit,//provide serialized QRIntentResponse in mData1
		//content retrieval:
		fileContent,// file path in mData1, content in mData2
		directoryContent,//sequence of vectors in mData1
		enterDir,
		createDir,
		sync,
		commitSuccess,
		syncSuccess,
		enterDirLS,
		commitPending,
		commitAborted,
		fileCert,
		fileChunk,
		fileCertRequest,
		fileChunkRequest

	};
}

namespace eFlashPosition
{
	enum eFlashPosition {
		bottom,
		VCenter,
		VCenterHMiddle,
		VCenterRight,
		top
	};
}


namespace eCommitStatus
{
	enum eCommitStatus
	{
		pending,
		success,
		aborted
	};
}


namespace eVMMetaProcessingResult
{
	enum eVMMetaProcessingResult
	{
		success,
		failure
	};
}
namespace eWebSocketService
{
	enum eWebSocketService
	{
		fileSystem=0,
		transactions=1,

	};
}
namespace eWebSocketStatus
{
	enum eWebSocketStatus
	{
		killed = 0,
		alive = 1
	};
}

namespace eWebSocketProcessingResult
{
	enum eWebSocketProcessingResult
	{
		OK=0,
		invalidPacket=1,
		invalidService=2,
		invalidSession = 3

	};
}
namespace eQRProcessingResult
{
	enum eQRProcessingResult
	{
		error,
		Ok
	};
}

namespace eFSCmdResult
{
	enum eFSCmdResult
	{
		unknown,
		error,
		invalidCmd,
		Ok
	};
}

namespace eFSCommitResult
{
	enum eFSCommitResult
	{ unknown,
		error,
		Ok
	};
}

namespace eFSEntityType
{
	enum eFSEntityType
	{unknown,
		file,
		dir
	};
}

namespace eNetTaskType
{//Note: the AWAIT tasks would keep blocking the tasks' queue (NOT the netmsgs' processing!) until the conversation timeouts
//OR until case-specific finalization aims are reached.
	enum eNetTaskType//types of global network tasks; these tasks are distributed amonng neighbors
	{
		startConversation,
		endConversation,
		requestBlock,//request block of a given ID
		notifyBlock, // used moslty to notify about new leading block 
		downloadUpdate,
		checkForUpdate,
		sync,//query all neighbors nodes for a newer chainProof
		sendQRIntentResponse,
		notifyReceiptID,
		notifyReceiptBody,
		awaitReceiptID,
		sendRAWData,
		DFSTask,// command-details encapsulated within CNetMsgs's data field (trough a serialized CDFSMsg),
		sendVMMetaData, //i.e. may contain a QR-Intent processing request or other data-requests, user-interaction requests,
		requestData,
		route,//do NOT modify the netMsg
		sendNetMsg,//do all the authentication and ecryption as per the current Conversation's state
		awaitOperationStatus,
		awaitSecureSession,//Note: the secure session is deemed to be established when EITHER a symetric key through ECDH has been reached OR when
		// the other peer's public key suitable marked to be used for encryption has been made available.Awaiting secure session does NOT affect peer's ability of responding
		//to Netmsgs i.e. of participating in data-exchange allowing for session establishment.
		
		//new TASKS - BEGIN
		requestChainProof,//request entire or sub-chain of the other node's Verified Chain Proof
		notifyChainProof,
		//requestBlock ^above
		//notifyBlock ^above
		notifyBlockBody,//block body
		notifyUpdate,
		notifyChatMsg

		//new TASKS - END
	};
}

namespace eNetTaskState
{
	enum eNetTaskState
	{
		initial,
		assigned,
		working,
		completed,
		aborted
	};
}
namespace QUICExecutionProfile {
	enum  QUICExecutionProfile {
		LowLatency, // Maps to QUIC_EXECUTION_PROFILE_LOW_LATENCY
		HighThroughput, // Maps to QUIC_EXECUTION_PROFILE_TYPE_MAX_THROUGHPUT
		Scavenger,   // Maps to QUIC_EXECUTION_PROFILE_BALANCED
		RealTime       // Maps to QUIC_EXECUTION_PROFILE_TYPE_REAL_TIME
	};
	
}

namespace eTransportType
{
	enum eTransportType{
	SSH,
	WebSocket,
	UDT,
	local,
	HTTPRequest,
	HTTPConnection,
	Proxy,
	HTTPAPIRequest,
	QUIC
	};
}

namespace eDFSTaskState
{
	enum eDFSTaskState
	{
		initial,
		assigned,
		working,
		completed,
		aborted
	};
}

namespace eObjectSubType
{
	enum eObjectSubType
	{
		Transaction=2,
		Verifiable =4
	};
}