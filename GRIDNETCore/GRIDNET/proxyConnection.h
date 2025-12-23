#pragma once
#include "mongoose/mongoose.h"
struct mg_connection;

class CCookie
{
private:
	std::mutex mGuardian;
	size_t mTimeStamp;
	std::string mName;
	std::string mValue;
	std::string mExpires;
	std::string mPath;
	std::string mDomain;
	bool mHttpOnly;
	bool mSameSiteNone;
	bool mIsSecure;
	static std::mutex sRegexGuardian;
	static std::regex sCookieRgx;
	static std::regex sCookieSetRgx;

public:
	static std::vector<std::shared_ptr<CCookie>> cookiesFromClientCookieHeader(std::string header);
	static std::shared_ptr<CCookie> cookieFromServerSetCookieReq(std::string cookieBody);
	CCookie(std::string name, std::string value, bool isSecure = true, std::string domain = "", std::string path = "", size_t timestamp = 0, bool httpOnly = false, bool sameSiteNone = false);
	std::string getSetCookieDirective(bool enforceSecure=true,bool URLencode=false);
	size_t getTimeStamp();
	bool getIsSecure();
	std::string getPath();
	std::string getDomain();
	std::string getName();
	void setName(std::string name);
	std::string getValue();
	void setValue(std::string valu7e);
	std::string getRAWNameValue();
	CCookie& operator=(CCookie other);

};
//struct mg_tls_opts;
/// <summary>
/// Wrapper around mg_connection provided by Mongoose.
/// That is to allow for a one to many realtionship between a client connectinon and web-server endpoints.
/// </summary>
class CProxyConnection
{
private:
	std::mutex mGuardian;
	std::string mEndpointURL;
	mg_connection* mMGConnection;
	bool mIsAlive;
	mg_tls_opts mTLSOpts;
	std::vector <std::shared_ptr<CCookie>> mCookies;
	eHttpVersion::eHttpVersion mHttpVersion;
	uint64_t mEndpointDataCount;
public:
	void incEndpointsDataCount(uint64_t count);
	uint64_t getEndpointDataCount();
	unsigned long getID();
	void setHTTPVersion(eHttpVersion::eHttpVersion version);
	eHttpVersion::eHttpVersion getHTTPVersion();

	void updateCookies(std::string header);
	std::shared_ptr<CCookie> processServerCookieReq(std::string header);
	std::vector<std::shared_ptr<CCookie>> getCookies();
	std::string getCookiesHTTPClientElement();
	CProxyConnection(mg_connection* connection, std::string endpointURL);
	
	mg_tls_opts* getClientTLSOpts();

	bool getIsAlive();

	mg_connection* getMGConn();
	void setMGConn(mg_connection* conn);

	std::string getEndpointURL();
	std::string getCookieBasedHostname();
	const char * getEndpointURLPtr();
	mg_str getEndpointURLNoSchema();
	void setEndpointURL(std::string endpointURL);

};