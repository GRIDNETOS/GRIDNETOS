////////////////////////////////////////////////////////////////////////////////
//
// hdkeys.cpp
//

#include "hdkeys.h"
#include <assert.h>

#include <sstream>
#include <stdexcept>

 extern std::vector<uint8_t> GRIDNET_SEED = Botan::hex_decode("260387636f696e2073656564"); // key = "GRIDNET seed"

const std::vector<uint8_t> CURVE_ORDER_BYTES =   Botan::hex_decode("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
const Botan::BigInt CURVE_ORDER(CURVE_ORDER_BYTES.data(), CURVE_ORDER_BYTES.size());

const uint32_t GRIDNET_MAIN_PRIV_VERSION = 0x0188ade1;
const uint32_t GRIDNET_MAIN_PUB_VERSION = 0x0188b212;
const uint32_t GRIDNET_TEST_PRIV_VERSION = 0x0288ade1;
const uint32_t GRIDNET_TEST_PUB_VERSION = 0x0288b212;

 std::vector<uint8_t>  HDSeed::getExtendedKey(bool bPrivate) const
{
    HDKeychain keychain(mMasterKey, mMasterChainCode);
	if (!bPrivate) { keychain = keychain.getPublic(); }

	return keychain.extkey();
}
 /*
 * In the current version of HD keys we do not take use of
  point additions.  first byte is reserved for sign bit. Will be needed for point-addition. Not in use now.
 */
HDKeychain::HDKeychain(const  std::vector<uint8_t> & key, const  std::vector<uint8_t> & chain_code, uint32_t child_num, uint32_t parent_fp, uint32_t depth,  bool main_net)
    : mDepth(depth), mParentFp(parent_fp), mChildNum(child_num), mChainCode(chain_code), mKey(key), mMainNet(main_net)
{
    if (mChainCode.size() != 32) {
		throw std::runtime_error("Invalid chain code.");
	}

    if (mKey.size() == 32)
	{
		// private keys are 32 bytes long
        mIsPrivate = true;
	}
    else if (mKey.size() == 33)
	{
		// public keys are 33 bytes long
		// first byte is reserved for sign bit. Will be needed for point-addition. Not in use now.
        mIsPrivate = false;
	}
	else
	{
		throw std::runtime_error("Invalid key.");
	}

        Botan::BigInt n(mIsPrivate ?mKey.data(): mKey.data()+1,mKey.size());
		if (n >= CURVE_ORDER || !n.is_nonzero()) {
			throw std::runtime_error("Invalid key.");
		}

        if(!mIsPrivate)
		{
        mPubKey.clear();
        mPubSign = mKey.begin()[0];
        mPubKey.insert(mPubKey.end(), mKey.begin() + 1, mKey.end());
		}
		else
			updatePubkey();
		
        if(mMainNet)
            mVersion = isPrivate() ? mPrivVersion : mPubVersion;
		else
            mVersion = isPrivate() ? mPrivVersionTest : mPubVersionTest;

    mValid = true;
}

HDKeychain::HDKeychain(const  std::vector<uint8_t> & extkey)
{
	if (extkey.size() != 78) { 
		throw std::runtime_error("Invalid extended key length.");
	}

    mVersion = ((uint32_t)extkey[0] << 24) | ((uint32_t)extkey[1] << 16) | ((uint32_t)extkey[2] << 8) | (uint32_t)extkey[3];
    mDepth = extkey[4];
    mParentFp = ((uint32_t)extkey[5] << 24) | ((uint32_t)extkey[6] << 16) | ((uint32_t)extkey[7] << 8) | (uint32_t)extkey[8];
    mChildNum = ((uint32_t)extkey[9] << 24) | ((uint32_t)extkey[10] << 16) | ((uint32_t)extkey[11] << 8) | (uint32_t)extkey[12];
    mChainCode.assign(extkey.begin() + 13, extkey.begin() + 45);
    mKey.assign(extkey.begin() + 45, extkey.begin() + 78);

	updatePubkey();

    mValid = true;
}

HDKeychain::HDKeychain(const HDKeychain& source)
{
    mValid = source.mValid;
    if (!mValid) return;
    mVersion = source.mVersion;
    mDepth = source.mDepth;
    mParentFp = source.mParentFp;
    mChildNum = source.mChildNum;
    mChainCode = source.mChainCode;
    mKey = source.mKey;
	updatePubkey();
}

HDKeychain& HDKeychain::operator=(const HDKeychain& rhs)
{
    mValid = rhs.mValid;
    if (mValid) {
        mVersion = rhs.mVersion;
        mDepth = rhs.mDepth;
        mParentFp = rhs.mParentFp;
        mChildNum = rhs.mChildNum;
        mChainCode = rhs.mChainCode;
        mKey = rhs.mKey;
        mPubKey = rhs.mPubKey;
        mPubSign = rhs.mPubSign;
		updatePubkey();
	}
	return *this;
}

bool HDKeychain::operator==(const HDKeychain& rhs) const
{
    return (mValid && rhs.mValid &&
        mVersion == rhs.mVersion &&
        mDepth == rhs.mDepth &&
        mParentFp == rhs.mParentFp &&
        mChildNum == rhs.mChildNum &&
        mChainCode == rhs.mChainCode &&
        mKey == rhs.mKey
        && mPubKey == rhs.mPubKey
        && mPubSign == rhs.mPubSign
        &&mIsPrivate == rhs.mIsPrivate);
}

bool HDKeychain::operator!=(const HDKeychain& rhs) const
{
	return !(*this == rhs);
}

 std::vector<uint8_t>  HDKeychain::extkey() const
{
	std::vector<uint8_t> extkey;

    extkey.push_back((uint32_t)mVersion >> 24);
    extkey.push_back(((uint32_t)mVersion >> 16) & 0xff);
    extkey.push_back(((uint32_t)mVersion >> 8) & 0xff);
    extkey.push_back((uint32_t)mVersion & 0xff);

    extkey.push_back(mDepth);

    extkey.push_back((uint32_t)mParentFp >> 24);
    extkey.push_back(((uint32_t)mParentFp >> 16) & 0xff);
    extkey.push_back(((uint32_t)mParentFp >> 8) & 0xff);
    extkey.push_back((uint32_t)mParentFp & 0xff);

    extkey.push_back((uint32_t)mChildNum >> 24);
    extkey.push_back(((uint32_t)mChildNum >> 16) & 0xff);
    extkey.push_back(((uint32_t)mChildNum >> 8) & 0xff);
    extkey.push_back((uint32_t)mChildNum & 0xff);

    extkey.insert(extkey.begin(), mChainCode.begin(), mChainCode.end());
    extkey.insert(extkey.begin(), mKey.begin(), mKey.end());

	return extkey;
}

 std::vector<uint8_t> HDKeychain::getPackedData() const
 {
	 return std::vector<uint8_t>();
 }

 Botan::secure_vector<uint8_t> HDKeychain::privkey() const
{
	if (isPrivate()) {
        return  Botan::secure_vector<uint8_t>(mKey.begin(), mKey.end());
	}
	else {
		return   Botan::secure_vector<uint8_t>();
	}
}



 std::vector<uint8_t>  HDKeychain::hash() const
{

	std::unique_ptr<Botan::HashFunction> sha(Botan::HashFunction::create("SHA-256"));
	std::unique_ptr<Botan::HashFunction> ripemd(Botan::HashFunction::create("RIPEMD-160"));
    sha->update(mPubKey);
	ripemd->update(Botan::unlock(sha->final()));

	return Botan::unlock(ripemd->final());
}

 std::vector<uint8_t>  HDKeychain::full_hash() const
{
	std::unique_ptr<Botan::HashFunction> sha(Botan::HashFunction::create("SHA-256"));
	std::unique_ptr<Botan::HashFunction> ripemd(Botan::HashFunction::create("RIPEMD-160"));

    std::vector<uint8_t>  data(mPubKey);
    data.insert(data.end(), mChainCode.begin(), mChainCode.end());


	sha->update(data);
	ripemd->update(Botan::unlock(sha->final()));

	return Botan::unlock(ripemd->final());

}

uint32_t HDKeychain::fp() const
{
	 std::vector<uint8_t>  hash = this->hash();
	return (uint32_t)hash[0] << 24 | (uint32_t)hash[1] << 16 | (uint32_t)hash[2] << 8 | (uint32_t)hash[3];
}

HDKeychain HDKeychain::getPublic() const
{
    if (!mValid) throw InvalidHDKeychainException();

	HDKeychain pub;
    pub.mValid = mValid;
    pub.mVersion = mPubVersion;
    pub.mDepth = mDepth;
    pub.mParentFp = mParentFp;
    pub.mChildNum = mChildNum;
    pub.mChainCode = mChainCode;
    pub.mKey = pub.mPubKey = mPubKey;
    pub.mIsPrivate = false;
	return pub;
}
void clamp(uint8_t* a)
{
	a[0] &= 248; a[31] &= 127; a[31] |= 64;
}


HDKeychain HDKeychain::getChild(uint32_t i) const
{
	if (!mValid) throw InvalidHDKeychainException();

	//and finally...we do not support the point-addition magic yet
	if (!mIsPrivate)
		throw std::runtime_error("Cannot do private key derivation on public key.");

	//Local Variables - BEGIN
	HDKeychain child;
	child.mValid = false;
	child.mIsPrivate = true;
	std::vector<uint8_t> data;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::vector<uint8_t> digest;
	std::vector<uint8_t>  left32;
	std::vector<uint8_t>  child_key;
	//Local Variables - BEGIN
	data.insert(data.end(), mKey.begin(), mKey.end());

	data.push_back(i >> 24);
	data.push_back((i >> 16) & 0xff);
	data.push_back((i >> 8) & 0xff);
	data.push_back(i & 0xff);
	digest = CCryptoFactory::getInstance()->genHMAC_sha256(mChainCode, data);
	left32 = std::vector<uint8_t>(digest.begin(), digest.begin() + 32);
	BigInt Il = tools->BytesToBigInt(left32);
	BigInt coB = tools->BytesToBigInt(CURVE_ORDER_BYTES);
	if (Il >= coB) throw InvalidHDKeychainException();

	if (isPrivate()) {
		BigInt k = tools->BytesToBigInt(mKey);
		std::string test = k.str();
		k -= Il;
		test = k.str();
		k %= coB;
		test = k.str();
		if (k == 0) throw InvalidHDKeychainException();

		child_key = tools->BigIntToBytes(k);
		child_key.resize(32);

		child.mKey = child_key;
		child.updatePubkey();
	}
	else {
		throw InvalidHDKeychainException();
	}

	child.mVersion = mVersion;
	child.mDepth = mDepth + 1;
	child.mParentFp = fp();
	child.mChildNum = i;
	child.mChainCode.assign(digest.begin() + 32, digest.end());

	child.mValid = true;
	return child;
}

HDKeychain HDKeychain::getChild(const std::string& path) const
{
	if (path.empty()) throw InvalidHDKeychainPathException();

	std::vector<uint32_t> path_vector;

	size_t i = 0;
	uint64_t n = 0;
	while (i < path.size())
	{
		char c = path[i];
		if (c >= '0' && c <= '9')
		{
			n *= 10;
			n += (uint32_t)(c - '0');
			if (n >= 0x80000000) throw InvalidHDKeychainPathException();
			i++;
			if (i >= path.size()) { path_vector.push_back((uint32_t)n); }
		}
		else if (c == '\'')
		{
			if (i + 1 < path.size())
			{
				if ((i + 2 >= path.size()) || (path[i + 1] != '/') || (path[i + 2] < '0') || (path[i + 2] > '9'))
					throw InvalidHDKeychainPathException();
			}
			n |= 0x80000000;
			path_vector.push_back((uint32_t)n);
			n = 0;
			i += 2;
		}
		else if (c == '/')
		{
			if (i + 1 >= path.size() || path[i + 1] < '0' || path[i + 1] > '9')
				throw InvalidHDKeychainPathException();
			path_vector.push_back((uint32_t)n);
			n = 0;
			i++;
		}
		else
		{
			throw InvalidHDKeychainPathException();
		}
	}

	HDKeychain child(*this);
	for (auto n : path_vector)
	{
		child = child.getChild(n);
	}
	return child;
}

std::string HDKeychain::toString() const
{
	std::stringstream mSS;

	std::vector<uint8_t> cc_b;
    cc_b.resize(mChainCode.size() * 2);
    Botan::hex_encode((char *)cc_b.data(), mChainCode.data(),
        (size_t)mChainCode.size(), false);
	std::string chain_code_str(cc_b.begin(), cc_b.end());

	std::vector<uint8_t> key_b;
    cc_b.resize(mKey.size() * 2);
    Botan::hex_encode((char *)key_b.data(), mKey.data(),
        (size_t)mKey.size(), false);
	std::string key_str(key_b.begin(), key_b.end());

	std::vector<uint8_t> hash_b;
	cc_b.resize(this->hash().size() * 2);
	Botan::hex_encode((char *)hash_b.data(), this->hash().data(),
		(size_t)this->hash().size(), false);
	std::string hash_str(key_b.begin(), key_b.end());

    mSS << "version: " << std::hex << mVersion << std::endl
		<< "depth: " << depth() << std::endl
        << "parent_fp: " << mParentFp << std::endl
        << "child_num: " << mChildNum << std::endl
		<< "chain_code: " << chain_code_str << std::endl
		<< "key: " << key_str << std::endl
		<< "hash: " << hash_str  << std::endl;
	return mSS.str();
}

void HDKeychain::updatePubkey() {
	if (isPrivate()) {
        mPubKey.resize(32);
        Botan::curve25519_basepoint(mPubKey.data(), mKey.data());
	}
	else
	{
		throw std::runtime_error("Cannot do private key derivation on public key.");
	}
}

uint32_t HDKeychain::mPrivVersion = GRIDNET_MAIN_PRIV_VERSION;
uint32_t HDKeychain::mPubVersion = GRIDNET_MAIN_PUB_VERSION;
uint32_t HDKeychain::mPrivVersionTest = GRIDNET_TEST_PRIV_VERSION;
uint32_t HDKeychain::mPubVersionTest = GRIDNET_MAIN_PUB_VERSION;
