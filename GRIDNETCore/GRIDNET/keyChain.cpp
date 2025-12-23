#include "keyChain.h"
#include "hdkeys.h"
#include "CryptoFactory.h"
#include "BlockchainManager.h"


/// <summary>
/// Key Chain is a cryptographic construct capable of encapsulating multiple identities.
/// The assumption is that each identity is derived from a private key generated based on a IV vector
/// and the Master Private Key.  IV in continuously incremented from 0 on demand based on the number
/// of required sub-identities.
/// </summary>
/// <param name="genKeys"></param>
/// <param name="isFlat"></param>
CKeyChain::CKeyChain(bool genKeys, bool isFlat)
{
	mCf = CCryptoFactory::getInstance();
	mFlags.flat = isFlat;
	Botan::secure_vector<uint8_t> privKey;
	std::vector<uint8_t> pubKey;
	mCurrentIndex = 0;
	mFurthestIndex = 1;
	if (genKeys == true)
	{
		mCf->genKeyPair(privKey, pubKey);
		mMainPrivKey = privKey;

		std::vector<uint8_t> firstChainPub = getPubKey();
		//thus domain address is based on the first priv/pub OFFSPRING key-pair. not the master key pair.
		std::vector<uint8_t> address;
		mCf->genAddress(firstChainPub, address);
		mName = std::string(address.begin(), address.end());
	}

}

kcFlags CKeyChain::getFlags()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mFlags;
}

std::shared_ptr<CKeyChain> CKeyChain::instantiate(std::vector<uint8_t> bytes)
{
	std::shared_ptr<CKeyChain> toRet = std::make_shared<CKeyChain>(false);
	
	if (toRet->unpack(Botan::secure_vector<uint8_t>(bytes.begin(), bytes.end())))
		return toRet;

	return nullptr;
}

/// <summary>
/// Retrieves a detailed human-readable description of this Key-Chain.
/// </summary>
/// <param name="includePrivateKey"></param>
/// <param name="newLine"></param>
/// <param name="headerColor"></param>
/// <param name="exportName"></param>
/// <returns></returns>
std::string CKeyChain::getDescription(bool includePrivateKey,  std::string newLine, eColor::eColor headerColor, bool exportName)
{
	std::stringstream ss;
	std::shared_ptr<CTools> tools = CTools::getInstance();

	ss << tools->getColoredString("[Name]: ", headerColor) << std::string(mName.size() == 0 ? "empty" : mName)  << newLine;
	if (includePrivateKey)
	{
		ss << tools->getColoredString("[Master Private HD Key]: ",headerColor) << CTools::getInstance()->base58CheckEncode(getPackedData(exportName)) << newLine;

		ss << tools->getColoredString("[Master Private Key]: ", headerColor) << CTools::getInstance()->base58CheckEncode(Botan::unlock(mMainPrivKey)) << newLine;
	}

	ss << tools->getColoredString("[Active Identity At Index]: ", headerColor) << std::to_string(mCurrentIndex) << newLine;
	ss << tools->getColoredString("[Key-Chain Depth]: ", headerColor) <<  std::to_string(mFurthestIndex+1) << newLine;

	std::shared_ptr<CCryptoFactory>  cf= CCryptoFactory::getInstance();
	std::vector<uint8_t> tmp;

	uint64_t activeIDAt = getCurrentIndex();
	if (cf->genAddress(getPubKey(), tmp))
	{
		ss << tools->getColoredString("[Active Domain]: ", headerColor) << tools->getColoredString(tools->bytesToString(tmp), eColor::lightCyan) << newLine;
	}

	CKeyChain tmpChain = *this;
	ss << newLine<<tools->getColoredString("v--[All Domains]--v ", headerColor) << newLine;

	for (uint64_t i = 0; i <= tmpChain.getUsedUptillIndex(); i++)
	{
		tmpChain.setIndex(i);
		cf->genAddress(tmpChain.getPubKey(), tmp);
		ss << tools->getColoredString(std::to_string(i), headerColor)<<") " << tools->getColoredString(tools->bytesToString(tmp), activeIDAt!=i?eColor::none:eColor::lightCyan) << newLine;
		if (includePrivateKey)
		{
			ss << tools->getColoredString("[Private Key]: ", eColor::blue) << tools->base58CheckEncode(Botan::unlock(tmpChain.getPrivKey()));
			ss << newLine;
		}
	}

	return ss.str();
}
/// <summary>
/// Retrieves textual name of this Key-Chain.
/// </summary>
/// <param name="ID"></param>
std::string CKeyChain::getID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mName;
}

/// <summary>
/// Sets textual name of this Key-Chain.
/// </summary>
/// <param name="ID"></param>
void CKeyChain::setID(std::string ID)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mName = ID;
}

CKeyChain::CKeyChain(std::shared_ptr<CCryptoFactory> cf,Botan::SecureVector<uint8_t> privateKey, uint32_t index, std::string name, bool isFlat)
{
 assertGN(cf != NULL);
	mMainPrivKey = privateKey;
	mCurrentIndex = index;
	mName = name;
	mFlags.flat = isFlat;
	mFurthestIndex = 1;
	mCf = cf;
}
CKeyChain::CKeyChain(std::shared_ptr<CCryptoFactory> cf, bool isFlat)
{
	mCurrentIndex = 0;
	mFurthestIndex = 1;
	mFlags.flat = isFlat;
	mCf = cf;
}

/// <summary>
/// WARNING: for efficiency packedData is not BER encoded.
/// KeyChain which has name also needs to be extended.
/// </summary>
/// <returns></returns>
bool CKeyChain::unpack(Botan::secure_vector<uint8_t>  packedData)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	//Local Variables - BEGIN
	bool hasName = false;
	uint64_t currentIndex = 0;
	//Local Variables - END

	if (packedData.size() < 33)//flags + priv
	return false;

	//Retrieve flags - BEGIN
	std::memcpy(&mFlags, packedData.data()+ currentIndex, 1);
	currentIndex++;
	//Retrieve flags - END

	//Retrieve private-key - BEGIN
	mMainPrivKey.resize(32);
	std::memcpy(mMainPrivKey.data(), packedData.data()+ currentIndex, 32);
	currentIndex += 32;
	//Retrieve private-key - END

	//handle a HD-chain- BEGIN
	if (!mFlags.flat)
	{
		//Retrieve Current Index - BEGIN
		mCurrentIndex = *(packedData.data() + currentIndex);
		currentIndex += 4;//sizeof(uint32_t)
		//Retrieve Current Index - END

		//Extract Depth - BEGIN
		mFurthestIndex = *(packedData.data() + currentIndex);
		currentIndex += 4;//sizeof(uint32_t)
		//Extract Depth - END
	}
	else {
		mFurthestIndex = 1;
		mCurrentIndex = 0;
	}

	if (packedData.size() > currentIndex)
	{
		//Extract Name - BEGIN (optional)
		mName.resize(packedData.size() - currentIndex);
		std::memcpy(&mName[0], packedData.data() + currentIndex, mName.size());
		//Extract Name - END (optional)
	}

	//fix depth (there always is at least one nested identity)
	if (mFurthestIndex == 0)
	{
		mFurthestIndex = 1;
	}
	
	return true;
}

/// <summary>
/// Sets the currently active sub-identity by taking its index.
/// </summary>
/// <param name="index"></param>
void CKeyChain::setIndex(uint32_t index)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	if (mFlags.flat)
		return;
	mCurrentIndex = index;

	mFurthestIndex = std::max(mFurthestIndex, mCurrentIndex);
}


CKeyChain::CKeyChain(const CKeyChain & sibling)
{
	mFurthestIndex = sibling.mFurthestIndex;
	mMainPrivKey = sibling.mMainPrivKey;
	mCurrentIndex = sibling.mCurrentIndex;
	mName = sibling.mName;
	mCf = sibling.mCf;
	mFlags = sibling.mFlags;

}
bool CKeyChain::operator==(CKeyChain& rhs)
{
	std::shared_ptr<CTools> tools = CTools::getInstance();

	return (mFurthestIndex == rhs.mFurthestIndex &&
		tools->compareByteVectors(mMainPrivKey, rhs.mMainPrivKey)&&
		mCurrentIndex == rhs.mCurrentIndex &&
		mName.compare(rhs.mName)==0 &&
		mFlags == rhs.mFlags);
}

bool CKeyChain::operator!=(CKeyChain& rhs)
{
	return !((*this) == rhs);
}
CKeyChain & CKeyChain::operator=(const CKeyChain & t)
{
	mFurthestIndex = t.mFurthestIndex;
	mMainPrivKey = t.mMainPrivKey;
	mCurrentIndex = t.mCurrentIndex;
	mName = t.mName;
	mCf = t.mCf;
	mFlags = t.mFlags;
	return *this;
}

/// <summary>
/// Increments the index representing current sub-identity.
/// </summary>
/// <returns></returns>
uint32_t CKeyChain::getCurrentIndex()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mCurrentIndex;
}

/// <summary>
/// Sets the furthest index representing the sub-key / sub-identity / sub-wallet
/// up till which sub-private keys have been generated.
/// </summary>
/// <param name="depth"></param>
void CKeyChain::setUsedUpTillIndex(uint32_t depth)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	if (mFlags.flat)
		return;
	 mFurthestIndex = depth;
}

/// <summary>
/// Retrieves the master private key.
/// </summary>
/// <returns></returns>
Botan::secure_vector<uint8_t> CKeyChain::getMainPrivKey()
{
	return mMainPrivKey;
}


/// <summary>
/// Increments the index representing the current sub-identity.
/// </summary>
void CKeyChain::incIndex()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	if (mFlags.flat)
		return;

	// Check if mCurrentIndex is at its maximum value
	if (mCurrentIndex == std::numeric_limits<decltype(mCurrentIndex)>::max()) {
		// Handle the edge case, e.g., reset or throw an exception
		// For example, reset both indices to 0 (or handle appropriately)
		mCurrentIndex = 0;
		mFurthestIndex = 1; // Ensuring depth is at least +1 compared to index
		return;
	}

	mCurrentIndex++;

	// Ensure mCurrentDepth is always at least one more than mCurrentIndex
	if (mCurrentIndex >= mFurthestIndex) {
		// Check if increasing mCurrentDepth would exceed its maximum value
		if (mFurthestIndex == std::numeric_limits<decltype(mFurthestIndex)>::max()) {
			// Handle the edge case, e.g., reset or throw an exception
			// Example: resetting both indices to 0 (or handle appropriately)
			mCurrentIndex = 0;
			mFurthestIndex = 1;
			return;
		}

		mFurthestIndex = mCurrentIndex + 1;
	}
}


/// <summary>
/// WARNING: for efficiency packedData is not BER encoded.
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CKeyChain::getPackedData(bool exportName)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	std::vector<uint8_t> packed;

	uint64_t bytesForName = exportName ? mName.size() : 0;

	packed.resize((mFlags.flat)?(1+32+ bytesForName):(1+32+4+4+ bytesForName));
	std::memcpy(packed.data(), &mFlags,1);
	std::memcpy(packed.data()+1, mMainPrivKey.data(), 32);//offset = flags

	if (!mFlags.flat)
	{
		std::memcpy(packed.data() + 1 + 32, &mCurrentIndex, 4);//offset = flags + privkey
		std::memcpy(packed.data() + 1 + 32 + 4, &mFurthestIndex, 4);//offset = flags + privkey + currentIndex
	}

	if (exportName & !mName.empty())//optional
	{
		std::memcpy(packed.data() + 1 + 32 + (mFlags.flat?0:(4 + 4)), mName.c_str(), bytesForName);
	}
	
	return packed;
}

/// <summary>
/// Retrieves the furthest consecutive index up till which sub-identities meaning sub-wallets
/// have been generated.
/// </summary>
/// <returns></returns>
uint32_t CKeyChain::getUsedUptillIndex()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	if (mFlags.flat)
		return 0 ;
	if (mCurrentIndex > mFurthestIndex)
		mFurthestIndex = mCurrentIndex;
	return mFurthestIndex;
}

/// <summary>
/// Sets the Master Private Key.
/// All sub-keys are derived from it.
/// </summary>
/// <param name="mainPriv"></param>
/// <returns></returns>
bool CKeyChain::setMainPrivKey(Botan::SecureVector<uint8_t> mainPriv)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mMainPrivKey = mainPriv;
	return true;
}

Botan::SecureVector<uint8_t> CKeyChain::getPrivKey()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	Botan::SecureVector<uint8_t> childPriv;

	if (mFlags.flat)
	{
		return this->mMainPrivKey;
	}

	std::vector<uint8_t> childPub;
	
	mCf->getChildKeyPair(mMainPrivKey, mCurrentIndex, childPriv, childPub);
	return childPriv;
}

/// <summary>
/// Retrieves the current public-key.
/// Current public key is specified by the internal mCurrentIndex index.
/// </summary>
/// <returns></returns>
std::vector<uint8_t>  CKeyChain::getPubKey()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	if (mFlags.flat)
	{
		return CCryptoFactory::getInstance()->getPubFromPriv(this->mMainPrivKey);
	}

	Botan::SecureVector<uint8_t> childPriv;
	std::vector<uint8_t> childPub;
	mCf->getChildKeyPair(mMainPrivKey, mCurrentIndex, childPriv, childPub);
	return childPub;
}


/// <summary>
/// Traverses the currently utilized key-space for a slot matching a given pub-Key.
/// Returns false if not found
/// </summary>
/// <param name="pubKey"></param>
/// <returns></returns>
bool  CKeyChain::searchForPubKey(std::vector<uint8_t> pubKey,uint64_t &slot)
{
	if (pubKey.size() != 32)
		return false;

	std::lock_guard<std::mutex> lock(mGuardian);

	bool found = false;
	uint64_t previousIndex = mCurrentIndex;
	Botan::SecureVector<uint8_t> childPriv;
	std::vector<uint8_t> childPub;

	if (mFlags.flat)
	{
		childPub = CCryptoFactory::getInstance()->getPubFromPriv(mMainPrivKey);

		if (CTools::getInstance()->compareByteVectors(pubKey, childPub))
		{
			slot = 0;
			return true;

		}
		else return false;	
	}

	// Traverse from furthest index down to 0 (inclusive)
	// Note: Using do-while with explicit break to avoid unsigned underflow
	uint64_t i = mFurthestIndex;
	do
	{
		mCf->getChildKeyPair(mMainPrivKey, i, childPriv, childPub);

		if (std::memcmp(childPub.data(), pubKey.data(), 32) == 0)
		{
			found = true;
			slot = i;
			break;
		}

		if (i == 0)
			break;
		i--;
	} while (true);

	if (!found)
	{
		slot = 0;
	}
	return found;
}

/// <summary>
/// Traverses the currently utilized key-space for a slot matching the address.
/// Note: this is LESS efficient than looking up an explicit pub-key.
/// Returns false if not found
/// </summary>
/// <param name="pubKey"></param>
/// <returns></returns>
bool  CKeyChain::searchForAddress(std::vector<uint8_t> addressP, uint64_t &slot)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	bool found = false;
	uint64_t previousIndex = mCurrentIndex;
	Botan::SecureVector<uint8_t> childPriv;
	std::vector<uint8_t> childPub;
	bool usingOld = false;
	bool alreadyTriedOld = false;
	std::shared_ptr<CTools> tools = CBlockchainManager::getInstance(eBlockchainMode::LocalData)->getTools();
	if (!tools->isDomainIDValid(addressP))
		return false;
	std::vector<uint8_t> address;

	if (mFlags.flat)
	{
		childPub = CCryptoFactory::getInstance()->getPubFromPriv(mMainPrivKey);

		if (!CCryptoFactory::getInstance()->genAddress(childPub, address))
			return false;

		if (CTools::getInstance()->compareByteVectors(address, addressP))
		{
			slot = 0;
			return true;

		}
		else return false;
	}

	// Traverse from furthest index down to 0 (inclusive)
	// Note: Using do-while with explicit break to avoid unsigned underflow
	uint64_t i = mFurthestIndex;
	do
	{
		mCf->getChildKeyPair(mMainPrivKey, i, childPriv, childPub, false);
		mCf->genAddress(childPub, address);

		if (tools->compareByteVectors(address, addressP))
		{
			found = true;
			slot = i;
			break;
		}

		if (i == 0)
			break;
		i--;
	} while (true);
	if (!found)
	{
		slot = 0;
	}

	return found;
}



