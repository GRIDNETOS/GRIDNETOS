#pragma once

#define _REGEX_MAX_COMPLEXITY_COUNT 1000L//10000000L // set to 0 to disable

#define _REGEX_MAX_STACK_COUNT 3000L // set to 0 to disable

#define PROXY_CACHE_LENGTH 3
#include "stdafx.h"
#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>
#include <limits>  
#include <stdlib.h>
#include <algorithm>  
#include "uint256.h"
#include "robin_hood.h"
#include <iostream>
#include <cctype>
#include "enums.h"
#include "IManager.h"
#include "mongoose/mongoose.h"
#include <mutex>
#include <boost/regex.hpp>
//lru_cache requires Abseil (for the most efficient algorithm).
//vcpkg install abseil --triplet x64-windows
//vcpkg install zlib openssl --triplet x64-windows
//vcpkg install boost-date-time --triplet x64-windows
//vcpkg install brotli --triplet x64-windows
//#include "lru_cache/lru_cache.h" <- used to cause troubles
#include "lru/lru.hpp"
#include "llhttp.h"
#define MAX_CLIENT_BUF_SIZE 3000000
#define CACHE_SIZE = 1000;
class CProxyConnection;
class CConversation;
class CBlockchainManager;
class CNetworkManager;
class CCORSProxy;
class CCORSConnCtx;
class CConversationState;
/// <summary>
/// Implements a WWW Server. The server manages access to the Web-UI for remote peers. The incomming/outgoing commands have no state. 
//	Sessions CAN be maintained through underlying 'protocols' accessed through the WWW Server. Decomposition and processing of such sessions SHOULD BE
//	managed by the coresponding protocol's server. Still, the WWW server comes with an auto-firewall capabilties, tracking intensivity of queries per IP.
/// </summary>

static void log_via_log_file(const void* buf, size_t len, void* userdata);
using Cache = LRU::TimedCache<std::vector<uint8_t>, std::string>;

class CCORSProxy : public  std::enable_shared_from_this<CCORSProxy>, public IManager
{
public:

	bool getIsOperational();
	void setIsOperational(bool isIt = true);
	std::string getAssetNameFromURL(const std::string url);
	void supressURLRewrites();
	bool getAreURLRewritesSupressed();
	bool handleRedirect(uint64_t status,std::string &httpPreamble, CCORSConnCtx *ctx);
	CCORSProxy(std::shared_ptr<CBlockchainManager> bm);
	bool getIsCacheEnabled();
	void setIsCacheEnabled(bool isIt=true);
	bool initialize();
	int onHttpChunkComplete(llhttp_t* parser);
	int onHttpMessageComplete(llhttp_t* parser);
	int onHttpHeadersComplete(llhttp_t* parser);

	// Inherited via IManager
	virtual void stop() override;
	virtual void pause() override;
	virtual void resume() override;
	void mControllerThreadF();
	virtual eManagerStatus::eManagerStatus getStatus() override;
	virtual void setStatus(eManagerStatus::eManagerStatus status) override;

	// Inherited via IManager
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status) override;
	virtual eManagerStatus::eManagerStatus getRequestedStatusChange() override;
	size_t getLastTimeCleanedUp();
	size_t getActiveSessionsCount();
	size_t getMaxSessionsCount();
	void setLastTimeCleanedUp(size_t timestamp = 0);

	//commands
	std::string getCertificate();
	std::string getPrivKey();
	std::vector<std::shared_ptr<CConversation>> getConversationsByIP(std::vector<uint8_t> IP);
	bool registerConversation(std::shared_ptr<CConversation> conversation);
	std::shared_ptr<CConversation> getConversationByID(std::vector<uint8_t> id);
	void cb2(mg_connection* c, int ev, void* ev_data, void* fn_data);
	void serverConnectionCallback(mg_connection* c, int ev, void* ev_data, void* fn_data);
	void addTLSCompatibilityModeHost(std::string host);
	bool isTLSCompabilityModeHost(std::string host);

	bool isMgConnPresent(uintptr_t ptr);

	uint64_t getStatusCode();

	void cacheResponse(std::string URL, std::string &payload);
	bool getCachedResponse(std::string URL, std::string& payload, uint64_t maxStaleFor = 10);//21600);


private:
	bool mIsOperational;
	std::mutex mCacheTimetableGuardian;
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t> mCachedAt;//the last time the site was updated within the cache
	bool mCachedEnabled;
	std::mutex mPropertyGuardian;
	std::mutex mCacheGuardian;
	std::mutex mCryptoGuardian;
	//lru_cache::NodeLruCache<std::vector<uint8_t>, std::string> mCache = lru_cache::make_cache<std::vector<uint8_t>, std::string>(PROXY_CACHE_LENGTH);
	Cache mCache = Cache(std::chrono::hours(8), 10000);
	std::mutex mProcessingGuardian;
	eHTTPMethod::eHTTPMethod parseHttpMethod(std::string str);
	eHttpVersion::eHttpVersion parseHTTPVersion(std::string &str);
	std::string replaceCallback(const std::smatch& m, void* fn_data);
	std::mutex mTLSCompListGurdian;
	std::vector<std::string> mTLSCompatibilityModeHosts;
	struct mg_tls_opts mServerOpts;
	std::mutex mServerOptsGuardian;
	void removeCtxFromPool(CCORSConnCtx* ctx);
	void cleanCTXs();
	std::vector<CCORSConnCtx*> mCtxPool;
	std::mutex mCtxPoolGuardian;

	void addActiveCtx(CCORSConnCtx* ctx);
	std::string getTopLevelDomainFromURL(const std::string url, bool withCORS =false, const bool &isMajorURL=false);
	std::regex mURLSoleRgx;
	boost::regex mDataReferenceRgx;
	
	std::regex mURLEncodableChars;
	boost::regex mChunkPrelude;
	uint64_t mEmdpointDataCount;//from web-server to client
	uint64_t mClientDataCount;//from client to web-server
	std::regex mRAWURLRgx;
	std::regex mComplexDataReferenceRgx;
	boost::regex mComplexNoPrefixDataReferenceRgx;
	std::regex mSimplifiedDataReferenceRgx;
	boost::regex mCSSRelativeDataReferenceRgx;
	void requestResource(std::string  endpoint, std::string url, std::string host, CCORSConnCtx* ctx);
	std::regex mPreludiumRgx;
	std::regex mInitWhitespace;
	std::regex mTrimWhiteSpace;
	std::regex mHTTPPreambleRgx;
	std::regex mHTTPPreambleRgxInitial;
	std::regex mHTTPBeginRgx;
	std::regex mHeaderRgx;
	//Mongoose Elements - BEGIN
	std::mutex mAcceptedConnectionsGuardian;
	std::vector<uint64_t> mAcceptedConnections;
	void removeAcceptedConn(uint64_t id);
	void acceptedConnection(uint64_t id);
	bool wasConnectionAccepted(uint64_t id);

	std::string mCert;
	std::string mPrivKey;

	struct mg_mgr mMongooseMgr;
	struct mg_connection* mMongooseConnection;
	//Mongoose Elements - END
	//Mongoose Settings - BEGIN
	void redirectToHttps(struct mg_connection* c, int ev, void* ev_data, void* fn_data);
	uint64_t getHttpStatusCode(mg_http_message* hm);
	std::string mDebugLevel;
	std::string mRootDir;
	std::string mListeningAddressTLS;
	std::string mListeningAddressRAW;

	std::string mEnableHexdump;
	std::string mSsPattern;
	//Mongoose Settings - END

	void flushData(CCORSConnCtx* ctx, std::shared_ptr<CProxyConnection> endpointConn);

	void withCORS(std::string &payload, std::string &header, CCORSConnCtx* ctx, eHTTPMsgMode::eHTTPMsgMode mode, std::shared_ptr<CProxyConnection> endpointConn, bool flushData = false);
	void sendCorsCheckResult(CCORSConnCtx* ctx, bool allowFromAnywhere=false, bool denyIFrame=true, bool wouldBeBlockedInFrame=true,  std::string reportedCORSSource="");
	void forward_request(mg_http_message* hm, CCORSConnCtx* ctx, std::shared_ptr<CProxyConnection> conn, std::string domain, bool isRelativeURL,  bool schemaMissing);

	std::string encodeURL(std::string url);
	bool handlePreflight(std::string &preamble, CCORSConnCtx* ctx);


	void modifyURLSchema(std::string& string, std::string schema="https://");

	std::shared_ptr<CBlockchainManager> getBlockchainManager();

	//Mongoose Callbacks - BEGIN
	void cb(struct mg_connection* c, int ev, void* ev_data, void* fn_data);
	//Mongoose Callbacks - END
	std::shared_ptr<CNetworkManager> getNetworkManager();
	std::mutex mLastAliveListingGuardian;
	uint64_t getLastAliveListing();
	uint64_t mLastAliveListingTimestamp;
	void lastAlivePrinted();
	std::vector<std::string> mObliviousGetParams;


	uint64_t doCleaningUp(bool forceKill = false);

	std::recursive_mutex mConversationsGuardian;
	std::vector<std::shared_ptr<CConversation>> mConversations;

	size_t mMaxWarningsCount;
	size_t mMaxSessions;

	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	eManagerStatus::eManagerStatus mStatus;
	std::thread mControllerThread;
	eManagerStatus::eManagerStatus mStatusChange;
	std::recursive_mutex mStatusGuardian, mGuardian;
	std::mutex mFieldsGuardian;
	bool mShutdown;
	std::thread mCORSProxy;
	void CorsControllerThreadF();

	bool initializeServer();
	std::regex mStatusCodeRgx;
	std::regex mGetParamsRgx;
	std::mutex mRegexGuardian;


	boost::regex mHttpResponseURLDetailsRgx;
	std::regex mSRCSetRgx;
	std::string mCertPath;
	std::string mCertKeyPath;
	std::mutex mCertDataGuardian;
	size_t mLastTimeCleaned;
	std::shared_ptr<CTools> mTools;

	std::mutex mStatusChangeGuardian;
	std::mutex mPreventedHeadersGuardian;
	std::weak_ptr<CNetworkManager> mNetworkManager;
	std::shared_ptr<CCryptoFactory> mCryptoFactory;
	bool doPreventInboundHeader(const std::string& header);
	bool doPreventOutboundHeader(const std::string& header);

	bool isKnownSafeFile(const std::string& filename);
	std::vector <std::string> mPreventedInboundHeaders;
	std::vector <std::string> mPreventedOutboundHeaders;
	std::vector <std::string> mKnownSafeFiles;
};


/// <summary>
/// Facilitates a processing context for a single (HTTP) TCP connection.
/// </summary>
class CCORSConnCtx
{
private:
	
	bool mAbortLongRunningTask;
	bool mIsAUserIssuedRequest;
	eSecFetchMode::eSecFetchMode mFetchMode;
	std::mutex mHeaderReconstructionGuardian;
	std::mutex mQueueGuardian;
	std::tuple<std::string,std::string,std::string> mJustRedirectedTo;//hostname, path
	bool mReconstructingHeader;
	uint64_t mHeaderReconstructionSteps;
	std::mutex mContentSecurityPolicyTagGuardian;
	std::deque<std::tuple<std::string, eSecFetchMode::eSecFetchMode>> mProcessingQueue;//queue of incoming requests. New requests are addeded in fron
	uint64_t mBytesToClientForReq;//the number of bytes sent to client, associated with the current requst ONLY
	bool mRewritesSuppressed;
	bool mOnlyCORSCheck;
	bool mIsClientTLS;
	std::string mMethod, mContentSecurityPolicyTag;
	std::string mNodeURI;//the URI clients used to access the node
	uint64_t mStatusCode;
	eHTTPMethod::eHTTPMethod mHTTPMethod;
	std::string mHeader;
	uint64_t mExpctedCurrentChunkBytes;
	uint64_t mExpectedNrOfBytes;
	bool mIsChunked;
	std::vector<std::shared_ptr<CProxyConnection>> mOutboundConnections; //there might be multiple persistent connections per a single Connection's Context; while only a single inbound client connection
	std::mutex mOutboundConnectionsGuardian;
	std::mutex mGuardian;
	bool mWaitingForMoredata;
	bool mEnabled;
	CCORSProxy* mCorsProxy;//the proxy apparatus
	mg_connection* mClientConn;//the connection to client (not web-end-point)
	size_t mShutdownAttempt;
	//mg_connection* mEndpointConn;
	llhttp_t* mHttpParser;// the HTTP parser; single one per connection
	llhttp_settings_t* mHttpParserSettings; //settings for the HTTP parser
	uint64_t mRedirects;
	bool mIsServerCtx;
	bool mWasProcessed;
	std::string mData;//currently buffored data, received from the remote endpoint.
	//it contains payloads of ex. accumulated HTTP chunks as these keep arriving and/or of incoming TCP datagrams.
	//at the 'end' it contains the entire payload. For passthrough data (images/video) it's not being used.
	std::string mPendingServerHeaderData;
	std::vector<std::tuple<std::string,bool>> mHostnames;
	bool mHasHeaders;//whether the currently received datagrams boards headers
	bool mHTTPHeaderReconstructed;//whether the local state-machine already reconstructed headers from server
	std::string mURLSelf;
	std::string mURLOrigin;
	bool mToBeClosed;
	bool mRedirecting;
	bool mOperational;
	std::string mCert;
	std::string mPrivKey;
	std::shared_ptr<CConversationState> mState;
	std::mutex mStateGuardian;
	bool mDelete;
	
	bool mRewriteURLs;
	bool iequals(const std::string& a, const std::string& b);
	uint64_t mLockCounter = 0;
	//struct mg_tls_opts mClientTLSopts;
	eWebEncoding::eWebEncoding mEncoding;
	bool mHeadersSent;
	bool mIsMajorRequest;
	uint64_t mClientDataCount;
	std::string mContentType;
	std::string mRecentRequstFilename;
	std::string mRecentRequstPath;
	std::string mCookieBasedDomain;
	std::string mURLBeforeRedirects;
	std::mutex abortLRT;
	uint64_t mConnectingSince;
public:
	void setJustRedirectedTo(std::string& endpoint,std::string& hostaname, std::string& path);
	bool  getJustRedirectedTo(std::string& endpoint,std::string &hostname, std::string &path);
	void clearRecentRedirect();
	void setAbortLongRunningTask(bool doIt = true);
	bool getAbortLongRunningTask();
	std::string getContentPolicySecTag();
	bool prepareContentPolicySecTag();
	void setContentPolicySecTag(const std::string &tag);
	eSecFetchMode::eSecFetchMode getFetchMode();
	void setFetchMode(eSecFetchMode::eSecFetchMode mode);
	void setIsAUserIssuedRequest(bool isIt = true);
	bool getIsAUserIssuedRequest();
	void cleanProcessingQeueue();
	std::vector<std::string> mHistoryOfDequedRequests;
	std::string getURLBeforeRedirects();
	void setURLBeforeRedirects(std::string URL);
	void setIsReconstructingHeader(bool isIt = true);
	bool getIsReconstructingHeader();
	void incHeaderReconstructionSteps();
	void clearHeaderReconstructionSteps();
	uint64_t getHeaderReconstuctionSteps();
	void enqeueURLRequest(std::string URL, eSecFetchMode::eSecFetchMode fetchMode, bool toFrontInstead=false);
	void finalizeRequestProcessing(bool cleanURLBeforeRedirects=true,bool deqeue=true);
	std::string getRecentURLRequest(bool deqeue=false, const eSecFetchMode::eSecFetchMode &fetchMode = eSecFetchMode::notSet );
	std::string getNodeURI();
	void setNodeURI(std::string uri);
	uint64_t getBytesToClientForReq();
	void incBytesToClientForReqBy(uint64_t count);
	void setBytesToClientForReq(uint64_t count);
	std::string getRecentRequestFileName();
	void setRecentRequestFileName(std::string filename);
	void setSuppressURLRewrites(bool doIt=true);
	bool getAreURLRewritesSuppressed();

	void setRecentRequestPath(std::string path);

	std::string getRecentRequestPath();

	void setDoOnlyCORSCheck(bool diIt = true);
	bool getDoOnlyCORSCheck();
	std::string getContentType();
	void setContentType(std::string type);

	uint64_t getTotalExchangedCount();
	uint64_t getClientDataCount();
	uint64_t getEndpointsDataCount();
	void incClientDataCount(uint64_t count);


	eHTTPMethod::eHTTPMethod getMethod();
	void setMethod(eHTTPMethod::eHTTPMethod method);

	bool getIsRedirect();
	bool getIsClientTLS();
	void setIsClientTLS(bool isIt = true);
	std::shared_ptr<CProxyConnection> getConnectionByID(unsigned long id);
	std::string getHeader();
	void addPendingServerHeaderData(std::string &data);
	std::string getPendingServerHeaderData();
	void addHeader(std::string name, std::string value);
	void clearHeader();
	void setStatusCode(uint64_t code);
	uint64_t getStatusCode();
	std::string getDataStr();
	std::string getCookieBasedDomain();
	void setCookieBasedDomain(std::string domain);
	uint64_t getDataLength();
	uint64_t getExpectedCurrentChunkBytes();
	void setExpectedCurrentChunkBytes(uint64_t nr);
	uint64_t getExpectedNrOfBytes();
	void setExpectedNrOfBytes(uint64_t nr);
	eWebEncoding::eWebEncoding getActiveEncoding();
	void setActiveEncoding(eWebEncoding::eWebEncoding enc);
	bool getIsChunked();
	void setIsChunked(bool isIt=true);
	uint64_t getConnectingSince();
	void pingConnecting();
	void pingConnected();
	bool getRewriteURLs();
	void setRewriteURLs(bool doIt = true);
	bool isMgConnPresent(uintptr_t ptr);
	//mg_tls_opts* getClientTLSOpts();
	bool removeConnectionByAddress(uintptr_t address, bool check=false);
	size_t getFirstShutdownAttempt();
	//Outbound Connections Management - BEGIN
	bool getHasOutboundConnections();
	void killOutBoundConnections();
	void addOutboundConnection(std::shared_ptr<CProxyConnection> connection);
	void addOutboundConnection(mg_connection* connection, std::string endpointURL);
	std::shared_ptr<CProxyConnection> getConnectionForEndpoint(std::string endpointURL);

	bool removeConnectionByID(unsigned long id);
	bool removeConnectionByURL(std::string endpointURL);
	//Outbound Connections Management - END
	void end();
	void markForDeletion();
	bool getIsMarkedForDeletion();
	void setIsRedirecting(bool isIt=true);
	bool getIsRedirecting();
	void pingDataActivity();
	bool getIsTobeClosed();
	void setIsToBeClosed(bool isIt = true);
	bool scheduleClose();

	void setWaitingForMoreData(bool isWaiting = true);
	bool getWaitingForMoreData();
	void setData(std::string data);
	std::recursive_mutex mDataGuardian;
	void lockData();
	void unlockData();
	bool getIsEnabled();
	void setIsEnabled(bool isIt = true);
	void clearData();
	void clearPendingServerHeaderData();
	uint64_t getRedirects();
	void incRedirects();
	void addData(std::string &data);
	void addHeaderData(std::string data);
	bool getIsServerCtx();
	void setIsServerCtx(bool isIt = true);
	std::string getCORSSelfURL(bool onlyHostname=false);
	void setCORSSelfURL(std::string proxyURL);
	std::string getCORSOriginURL(bool onlyHostname = false);
	void setCORSOriginURL(std::string requestOrigin);
	bool getHeadersSent();
	void setHeadersSent(bool wereThey = true);
	void setCurrentState(eConversationState::eConversationState state);
	void cleanPerRequestFlags();
	std::shared_ptr<CConversationState>  getState();


	void unwrapURL( std::string& url);
	CCORSConnCtx(CCORSProxy* proxy, mg_connection* clientConn=nullptr, mg_connection* endpointConn=nullptr)
	{
	    setCORSSelfURL("https://test.gridnet.org");//would be overriden by first query from the Web-browser UI dApp.
	    mFetchMode = eSecFetchMode::notSet;
		mOperational = false;
	    mConnectingSince = 0;
		mAbortLongRunningTask =false;
		mIsAUserIssuedRequest = false;
		mHTTPHeaderReconstructed = false;
		mReconstructingHeader =0;
		mHeaderReconstructionSteps =0;
		mBytesToClientForReq = 0;
		mOnlyCORSCheck = false;
		mRewritesSuppressed = false;
		mOnlyCORSCheck = false;
		mClientDataCount = 0;
		mIsClientTLS = false;
		mStatusCode = 0;
		mHeadersSent = false;
		mIsMajorRequest = false;
		mExpctedCurrentChunkBytes = 0;
		mExpectedNrOfBytes = 0;
		mEncoding = eWebEncoding::none;
		mRewriteURLs = false;
		mShutdownAttempt = 0;
		mDelete = false;
		mEnabled = true;
		mState = std::make_shared<CConversationState>();
		mRedirecting = false;
		mToBeClosed = false;
		mWaitingForMoredata = false;
		mWasProcessed = false;
		mIsServerCtx = false;
		mRedirects = 0;
		mHasHeaders = false;
		mHttpParser = new llhttp_t();
		mHttpParserSettings = new llhttp_settings_t();
		this->mClientConn = clientConn;
		this->mCorsProxy = proxy;

		/* Initialize user callbacks and settings */
		llhttp_settings_init(mHttpParserSettings);
		//WARNING: first initialize the memory region
		llhttp_init(mHttpParser, HTTP_RESPONSE, mHttpParserSettings);

		//THEN, set any intrinsic data-fields

		/* Set user callback */
		mHttpParser->data = this;



		mHttpParserSettings->on_message_complete = [](llhttp_t* parser)->int {

			if (!reinterpret_cast<CCORSConnCtx*>(parser->data))
			{
				return -1;
			}
			
			return reinterpret_cast<CCORSConnCtx*>(parser->data)->getProxy()->onHttpMessageComplete(parser);
		};// on_message_complete


		mHttpParserSettings->on_headers_complete = [](llhttp_t* parser)->int {

			if (!reinterpret_cast<CCORSConnCtx*>(parser->data))
			{
				return -1;
			}

			return reinterpret_cast<CCORSConnCtx*>(parser->data)->getProxy()->onHttpHeadersComplete(parser);
		};

	
/*
		mHttpParserSettings->on_url_complete = [](llhttp_t* parser)->int {
		
			if (!reinterpret_cast<CCORSConnCtx*>(parser->data))
			{
				return -1;
			}
			return reinterpret_cast<CCORSConnCtx*>(parser->data)->getProxy()->onURL(parser);
		};// on_message_complete
		*/

		mHttpParserSettings->on_chunk_complete = [](llhttp_t* parser)->int {
		
			if (!reinterpret_cast<CCORSConnCtx*>(parser->data))
			{
				return -1;
			}
			return reinterpret_cast<CCORSConnCtx*>(parser->data)->getProxy()->onHttpChunkComplete(parser);
		};// on_message_complete;


		/* Initialize the parser in HTTP_BOTH mode, meaning that it will select between
		 * HTTP_REQUEST and HTTP_RESPONSE parsing automatically while reading the first
		 * input.
		 */
	
	}

	~CCORSConnCtx()
	{
		llhttp_finish(mHttpParser);
		delete mHttpParser;
		delete mHttpParserSettings;
	}

	void  setCurrentReqHasHeaders(bool hasIt = true);
	bool getCurrentReqHasHeaders();
	bool getHTTPHeaderWasReconstructed();
	void setHTTPHeaderWasReconstructed(bool wasIt=true);
	std::string  getCertificate();
	std::string getPrivKey();
	llhttp_t* getParser();
	CCORSProxy* getProxy();
	mg_connection* getClientConn();
	//mg_connection* getEndpointConn();
	void setIsMajorRequest(bool isIt = true);
	bool getIsMajorRequest();

	void enqeueRecentHostname(std::string hostname, bool major = false, bool toFrontInstead = false);
	std::string getInitialHostname(bool majorOnly=false);
	std::string getRecentHostname(bool majorOnly=false);
	std::string getData();
	void setWasProcessed(bool wasIt = true);
	bool getWasProcessed();
	void setClientConn(mg_connection* conn);
	
	//void setEndpointConn(mg_connection* conn);
};


