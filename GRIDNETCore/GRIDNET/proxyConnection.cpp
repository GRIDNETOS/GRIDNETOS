#include "stdafx.h"
#include "proxyConnection.h"


std::mutex CCookie::sRegexGuardian;
std::regex CCookie::sCookieRgx("\\s*(([^=]+)=([^;=]+))(?:=[^;=]+){0,1}\\s*;?\\s*");// Regex for taking MULTIPLE cookies from the 'Cookie' header delivered by *CLINET* only. not the server.
std::regex CCookie::sCookieSetRgx("(?:\\s*([^=;]+)(?:=([^;=]+))?\\s*;?\\s*);?", std::regex_constants::ECMAScript | std::regex_constants::optimize | std::regex_constants::icase);

void CProxyConnection::incEndpointsDataCount(uint64_t count)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mEndpointDataCount += count;
}

uint64_t CProxyConnection::getEndpointDataCount()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mEndpointDataCount;
}

unsigned long CProxyConnection::getID()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	if (mMGConnection != nullptr)
		return mMGConnection->id;
	else return 0;
}

void CProxyConnection::setHTTPVersion(eHttpVersion::eHttpVersion version)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mHttpVersion = version;
}



eHttpVersion::eHttpVersion CProxyConnection::getHTTPVersion()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mHttpVersion;
}

/// <summary>
/// Invoked on client connection.
/// </summary>
/// <param name="header"></param>
void CProxyConnection::updateCookies(std::string header)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mCookies.clear();
	mCookies = CCookie::cookiesFromClientCookieHeader(header);
}

/// <summary>
/// Invoked on web-endpoint's connection on 'set-cookie' headers valu-body.
/// </summary>
/// <param name="header"></param>
std::shared_ptr<CCookie> CProxyConnection::processServerCookieReq(std::string setCookieBody)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	//Local Variables - BEGIN
	using strmatch = std::match_results<std::string::iterator>;
	strmatch res;
	std::shared_ptr<CCookie> cookie;
	std::string::iterator it = setCookieBody.begin();
	bool alreadyThere = false;
	//Local Variables - END

	//Operational Logic - BEGIN

	cookie = CCookie::cookieFromServerSetCookieReq(setCookieBody);

	if (!cookie)
		return nullptr;

	for (uint64_t a = 0; a < mCookies.size(); a++)
	{

		for (uint64_t i = 0; i < mCookies.size(); i++)
		{
			if (mCookies[i]->getName().compare(cookie->getName()) == 0)
			{
				alreadyThere = true;
				mCookies[i] = cookie; //the previous cookie simply gets destroyed. 
				//^More efficient than assignment operator.
				break;
			}
		}
		
	}

	if (!alreadyThere)
	{
		mCookies.push_back(cookie);
	}
	return cookie;

	//Operational Logic - END
}

std::vector<std::shared_ptr<CCookie>> CProxyConnection::getCookies()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mCookies;
}


/// <summary>
/// Returns just the Value rendering with possibly multiple name-value pairs i.e. for multiple Cookies. 
/// (header name not included)
/// Note: these are simple POSSIBLY MULTIPLE semicolon-delimited name-value pairs. There's NOTHING more to it. Ever.
/// </summary>
/// <returns></returns>
std::string CProxyConnection::getCookiesHTTPClientElement()
{
	std::string toRet;
	std::vector<std::shared_ptr<CCookie>> cookies = getCookies();

	for (uint64_t i = 0; i < cookies.size(); i++)
	{
		toRet += (cookies[i]->getRAWNameValue() + (i<cookies.size()-1?"; ":";"));
	
	}

	return toRet;
}


CProxyConnection::CProxyConnection(mg_connection* connection, std::string endpointURL)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mEndpointDataCount = 0;
	mMGConnection = connection;

	mEndpointURL = endpointURL;

	if (connection)
	{
		mIsAlive = true;
	}
	else
	{
		mIsAlive = false;
	}
	

	memset(&mTLSOpts, 0, sizeof(mTLSOpts));
	memset(&mTLSOpts.srvname, 0, sizeof(mg_str));
	mTLSOpts.ciphers = "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256"; /* Cipher list string.*/
	mTLSOpts.ca = nullptr;// "C:\\Users\\Anonymous\\Documents\\GRIDNET-OS\\GRIDNETCore\\ca.pem";

}

mg_tls_opts* CProxyConnection::getClientTLSOpts()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return &mTLSOpts;
}
bool CProxyConnection::getIsAlive()
{
	//Note: connections get auto-removed from the pool of available connections as MG_EV_CLOSE events are fired.
	std::lock_guard<std::mutex> lock(mGuardian);
	return mMGConnection != nullptr && !mMGConnection->is_closing;
}

mg_connection* CProxyConnection::getMGConn()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mMGConnection;
}

void CProxyConnection::setMGConn(mg_connection* conn)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mMGConnection = conn;

	if (conn)
	{
		mIsAlive = true;
	}
}

std::string CProxyConnection::getEndpointURL()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mEndpointURL;
}

std::string CProxyConnection::getCookieBasedHostname()
{
	//'x-gn-hostname' Header Support - BEGIN

	for (uint64_t i = 0; i < mCookies.size(); i++)
	{
		if (CTools::iequalsS(mCookies[i]->getName(), "x-gn-hostname"))
		{
			return  mCookies[i]->getValue();
			//  int len = mg_url_decode(domain.c_str(), domain.size(), &domain[0], domain.size(), false);
			// domain = domain.substr(0, len);
			//  ctx->setRecentHostname(domain, true);
		
		}
	}
	return "";
	//'x-gn-hostname' Header Support - END
}

const char* CProxyConnection::getEndpointURLPtr()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mEndpointURL.c_str();
}

mg_str CProxyConnection::getEndpointURLNoSchema() {
	std::lock_guard<std::mutex> lock(mGuardian);
	std::size_t pos = mEndpointURL.find("://");

	if (pos != std::string::npos) {
		// Found schema, omit it
		return mg_str_n(mEndpointURL.c_str()+pos + 3, mEndpointURL.size()-(pos+3));
	}
	else {
		// No schema found, return as is
		return mg_str_n(mEndpointURL.c_str(), mEndpointURL.size());
	}
}

void CProxyConnection::setEndpointURL(std::string endpointURL)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mEndpointURL = endpointURL;
}

/// <summary>
/// Constructs a series of cookies provided the CONTENTS of a Cookies header.
/// WARNING: this can be used ONLY for headers comring from the Web-Browser.
/// Web-browser provided multiple cookies within a single 'Cookie' header.
/// 
/// For 'set-cookies' delivered from server, it is the opposite.
/// </summary>
/// <param name="header"></param>
/// <returns></returns>
std::vector<std::shared_ptr<CCookie>> CCookie::cookiesFromClientCookieHeader(std::string header)
{

	std::lock_guard<std::mutex> lock(sRegexGuardian);
	//Local Variables - BEGIN
	using strmatch = std::match_results<std::string::iterator>;
	strmatch res;
	std::vector<std::shared_ptr<CCookie>> toRet;
	std::string::iterator it = header.begin();
	//Local Variables - END

	while (std::regex_search(it, header.end(), res, sCookieRgx)) {
		if (res[2].matched && res[3].matched)
		{
			toRet.push_back(std::make_shared<CCookie>(res[2].str(), res[3].str()));
		}
		it += res.position() + res.length();
	}
	return toRet;
}

/// <summary>
/// Attempts to instantiate CCookie from a set-cookie request coming from *SERVER8
/// </summary>
/// <param name="cookieBody"></param>
/// <returns></returns>
std::shared_ptr<CCookie> CCookie::cookieFromServerSetCookieReq(std::string cookieBody)
{

	std::lock_guard<std::mutex> lock(sRegexGuardian);
	//Local Variables - BEGIN
	using strmatch = std::match_results<std::string::iterator>;
	strmatch res;
	std::string::iterator it = cookieBody.begin();
	bool isSecure = false;
	uint64_t expires = 0;
	std::string domain,path,name,value;
	int iterations=0;
	//Local Variables - END

	//find all name-value pairs
	while (std::regex_search(it, cookieBody.end(), res, sCookieSetRgx)) {

		if (!iterations)
		{
			//first always is the name-value pair (payload)
			if (res[1].matched && res[2].matched)
			{
				name = res[1].str();
				value = res[2].str();
			}
			else
				break;//invalid cookie 
		}
		else{
			if (res[1].matched)
			{
				if (CTools::iequalsS(res[1].str(), "expires"))
				{
					CTools::getInstance()->stringToUint(res[2].str(), expires);
				}
				else if (CTools::iequalsS(res[1].str(), "path"))
				{
					path = res[2].str();
				}
				else if (CTools::iequalsS(res[1].str(), "domain"))
				{
					domain = res[2].str();
				}
				else if (CTools::iequalsS(res[1].str(), "secure"))
				{
					isSecure = true;
				}

			}
			
		}
		
		iterations++;
		it += res.position() + res.length();
	}
	if (!name.size())
		return nullptr;

	return std::make_shared<CCookie>(name, value,isSecure,domain,path,expires);

}


CCookie::CCookie(std::string name, std::string value, bool isSecure, std::string domain, std::string path, size_t timestamp, bool httpOnly,bool sameSiteNone)
{
	mName = name;
	mValue = value;
	mTimeStamp = timestamp;
	mIsSecure = isSecure;
	mDomain = domain;
	mPath = path;
	mHttpOnly = httpOnly;
	mSameSiteNone = sameSiteNone;
}

std::string CCookie::getSetCookieDirective(bool enforceSecure, bool URLencode)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	URLencode = false;
	std::string toRet = "Set-Cookie: ";

	std::string val;
	size_t encodedLength = 0;
	if (URLencode && mValue.size())
	{
		val.resize(mValue.size() * 4);
		encodedLength = mg_url_encode(mValue.c_str(), mValue.size(), &val[0], val.size());
		val = val.substr(0, encodedLength);
	}
	else
	{ 
		val= mValue;
	}

	toRet += (mName+"="+ val);

	if (mTimeStamp)
	{
		toRet += ("; expires = " + std::to_string(mTimeStamp));
	}

	if (mDomain.size() )
	{
		toRet += ("; domain = " + mDomain);
	}

	if (mPath.size())
	{
		toRet += ("; path = " + mPath);
	}
	if (mHttpOnly)
	{
		toRet += "; HttpOnly";
	}
	if (mSameSiteNone)
	{
		toRet += "; SameSite=None";
	}

	if (mIsSecure || enforceSecure)
	{
		toRet += "; Secure";
	}
	

	return toRet;
}

size_t CCookie::getTimeStamp()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	return mTimeStamp;
}

bool CCookie::getIsSecure()
{std::lock_guard<std::mutex> lock(mGuardian);
	return false;
}

std::string CCookie::getPath()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mPath;
}

std::string CCookie::getDomain()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDomain;
}

std::string CCookie::getName()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	return mName;
}

void CCookie::setName(std::string name)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	mName = name;
}

std::string CCookie::getValue()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	return mValue;
}

void CCookie::setValue(std::string value)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	 mValue = value;
}

std::string CCookie::getRAWNameValue()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	std::string rendering;

	rendering+=( mName +": "+ mValue);

	return rendering;
}

CCookie& CCookie::operator=(CCookie other)
{
	mTimeStamp = other.mTimeStamp;
	mName = other.mName;
	mValue = other.mValue;
	mExpires = other.mExpires;
	mPath = other.mPath;
	mDomain = other.mDomain;
	mIsSecure = other.mIsSecure;
	return *this;
}
