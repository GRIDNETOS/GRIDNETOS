#include "TokenPoolBank.h"
#include "TokenPool.h"
#include "cryptofactory.h"
#include "TransmissionToken.h"


void CTokenPoolBank::initFields()
{
    mCurrentDepth = 0;
    mPreCachedSeedHashDepth = 0;
    mCurrentIndex = 0;
    mStatus = eTokenPoolBankStatus::active;
}

void CTokenPoolBank::reset()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    mCurrentDepth = 0;
    mCurrentIndex = 0;
    mCurrentFinalHash.clear();
}

bool CTokenPoolBank::setSeedHash(Botan::secure_vector<uint8_t> seed)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    if (seed.size() != 32)
        return false;
    mSeedHash = seed;
    return true;
}

bool CTokenPoolBank::setPreCachedSeedHash(std::vector<uint8_t> seed, BigInt depth)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    if (seed.size() != 32)
        return false;
    mPreCachedSeedHash = seed;
    mPreCachedSeedHashDepth = depth;
    return true;
}
BigInt CTokenPoolBank::getPreCachedSeedHashDepth()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mPreCachedSeedHashDepth;
}
eTokenPoolBankStatus::eTokenPoolBankStatus CTokenPoolBank::getStatus()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mStatus;
}

CTokenPoolBank::CTokenPoolBank(uint64_t bankID ,std::shared_ptr<CTokenPool> pool, std::vector<uint8_t> finalHash, BigInt currentDepth, Botan::secure_vector<uint8_t> seedHash)
{
    initFields();
    mID = bankID;
    mPool = pool;
    mFinalHash = finalHash;
    mCurrentDepth = currentDepth;
    mSeedHash = seedHash;
}



/// <summary>
/// Returns textual information regarding the TokenPoolBank.
/// </summary>
/// <param name="newLine"></param>
/// <returns></returns>
std::string CTokenPoolBank::getInfo(std::string newLine)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    //Local Variables - BEGIN
    std::string toRet;
    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::string statusStr;
    //Local Variables - END

    switch (mStatus)
    {
    case eTokenPoolBankStatus::active:
        statusStr = "Active";
        break;
    case eTokenPoolBankStatus::depleted:
        statusStr = "Depleted";
        break;
    default:
        break;
    }
    if (auto pool = mPool.lock())
    {
        toRet += newLine + "Status: " + statusStr + std::string(
            mSeedHash.size() == 32 ? ("Seed-Hash: " + tools->base58CheckEncode(Botan::unlock(mSeedHash))
                + newLine + "            ^-- [IMPORTANT: do not disclose") : "");

        toRet += newLine + "Final-Hash: " + tools->base58CheckEncode(mFinalHash)
            + newLine + "Tokens-Count: " +pool->getDimensionDepth().str()
            + newLine + "Single-Token Value: " + pool->getSingleTokenValue().str()
            + newLine + "Value-Left: " + getValueLeft().str() + " GBUs"
            + newLine + "Total-Value: " + getTotalValue().str() + " GBUs"
            + newLine + "Current Usage-Depth:  " + mCurrentDepth.str()
            + newLine + "^--------------------------------------^" + newLine;
        return toRet;
    }
    return toRet;
}


void CTokenPoolBank::setPool(std::weak_ptr<CTokenPool> pool)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    mPool = pool;
}

void CTokenPoolBank::setStatus(eTokenPoolBankStatus::eTokenPoolBankStatus status)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    mStatus = status;
    //notify the Token-Pool itself about status change
    //so that it can update the status of itself if needed
    if (auto pool = mPool.lock())
    {
        pool->notifyBankStatusChanged(status);
    }
}


std::vector<uint8_t> CTokenPoolBank::getFinalHash(bool currentOne)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    if (!currentOne || mCurrentFinalHash.size() != 32)
        return mFinalHash;
    else return mCurrentFinalHash;//bug: for serialization this should ALWAYS be returning the final hash (solution: call with currentOne set to FALSE) during serialization
}

/// <summary>
/// Index is the current index counting from the Seed-Hash (index 0).
/// Depth is counted from the mFinalHash(depth 0, max(index)).
/// function takes use of mPreCachedSeedHash IF available.
/// To return hash at a given depth WHICH IS NOT in between (mCurrentHash or mPreCachedSeedHash) and mFinalHash, mSeedHash is REQUIRED.
/// </summary>
/// <param name="index"></param>
/// <returns></returns>
std::vector<uint8_t> CTokenPoolBank::getHashAtDepth(BigInt depth, bool updateState)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    std::shared_ptr< CCryptoFactory> crypto = CCryptoFactory::getInstance();

    //Local variables - BEGIN
    std::vector<uint8_t> currentHash;
    BigInt currentDepth = mCurrentDepth;
    BigInt currentIndex =0; 
    //Local variables - END

    if (auto pool = mPool.lock())
    {
        currentIndex  = pool->getDimensionDepth() - currentDepth;

        if (depth == 0)
            return mFinalHash;

        if (depth > pool->getDimensionDepth())//request is 'out of scope'
            return std::vector<uint8_t>();

     

        //first, check if the requested sub-chain has been revealed already
        if (depth <= currentDepth && mCurrentFinalHash.size() == 32)
        {//if so, no need for secrets
            currentHash = mCurrentFinalHash;
            currentDepth = mCurrentDepth;
        }
        else //then try to use a pre-cached seed-hash if available for the requested depth
            //we try this first as it's an optimization (by limiting the search-space)
            if (depth <= mPreCachedSeedHashDepth && mPreCachedSeedHash.size() == 32)//we're not in posession of the required secret
            {//the pre-cached secret might have been provided by the mobile app
               //the mobile app shall provide just enough for the services to remain of enough quality (vidoes playing etc)
               //while not risking too much of the user's assets
                currentHash = mPreCachedSeedHash;
                currentDepth = mPreCachedSeedHashDepth;
            }
            else if (mSeedHash.size() == 32)
            {//finally fallback to seed-hash for this dimension, if available
                currentHash = Botan::unlock(mSeedHash);
                currentDepth = pool->getDimensionDepth();
                currentIndex = 0;
            }
            else return std::vector<uint8_t>();//we cannot deliver

            //let us retrieve the hash-value based on known data                                                                                        
        for (BigInt i = currentDepth; i > depth; i--)
        {
            currentHash = crypto->getSHA2_256Vec(currentHash);
        }

        //update bank's utilization - the token-range is assumed as used-up
        if (updateState && depth > mCurrentDepth)
        {
            mCurrentDepth = depth;
            mCurrentFinalHash = currentHash;

            if (getValueLeft() == 0)
                setStatus(eTokenPoolBankStatus::depleted);
        }
        
    }
    return currentHash;
}

/// <summary>
/// Returns next-hash from the perspective of the current usage-index
/// </summary>
/// <param name="markUsed"></param>
/// <returns></returns>
std::vector<uint8_t> CTokenPoolBank::getNextHash(bool markUsed)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    return  getHashAtDepth(mCurrentDepth + 1, markUsed);

}

/// <summary>
/// Would generate a Token worth given Value (GBUs). Returns only the LAST hash of such Token. all the intermediary hashes
/// will be assumed as used-up.hashesConsumedCount indicated the number of hasheses used. Returns an empty vector on failure
/// ex. when no sufficient sesources cumulated within the Token-Pool.
/// </summary>
/// <param name="value"></param>
/// <param name="hashesConsumedCount"></param>
/// <param name="markUsed"></param>
/// <returns></returns>
std::vector<uint8_t> CTokenPoolBank::genTokenWorthValue(BigInt value, BigInt& hashesToBeConsumedCount, bool markUsed)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    std::vector<uint8_t> toRet;

    if (value == 0 ||  value > getValueLeft())
        return toRet;

    if (auto pool = mPool.lock())
    {
        BigInt tokenValue = pool->getSingleTokenValue();
        if(tokenValue==0)
            return toRet;
        //token-value in each bank is the same

        //test the below
        hashesToBeConsumedCount = static_cast<uint64_t>(1 + ((static_cast<BigFloat>(value) - 1) / static_cast<BigFloat>(tokenValue))); // if value != 0 ; gets ceiling of division

        //the following would return an empty vector if value exceeds Pool-size
        toRet =  getHashAtDepth(mCurrentDepth + hashesToBeConsumedCount, markUsed);
    }
    return toRet;

}

std::shared_ptr<CTransmissionToken> CTokenPoolBank::genTTWorthValue(BigInt value, bool markUsed)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    BigInt hashesToBeConsumedCount = 0;

    std::vector<uint8_t> hash = genTokenWorthValue(value, hashesToBeConsumedCount, markUsed);
    BigInt expectedDepth = markUsed ? mCurrentDepth:(mCurrentDepth + hashesToBeConsumedCount);
    if (hash.size() != 32)
        return nullptr;

    if (auto pool = mPool.lock())
    {
        std::shared_ptr<CTransmissionToken> tt = std::make_shared<CTransmissionToken>(mID, value, hash, hashesToBeConsumedCount, expectedDepth, std::vector<uint8_t>(), pool->getID());
            return tt;
    }

    return nullptr;
}

void CTokenPoolBank::setCurrentHash(std::vector<uint8_t> currentHash)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    mCurrentFinalHash = currentHash;
}

std::vector<uint8_t>  CTokenPoolBank::getCurrentHash()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mCurrentFinalHash;
}

BigInt CTokenPoolBank::getCurrentDepth()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mCurrentDepth;
}

void CTokenPoolBank::setCurrentDepth(BigInt index)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    mCurrentDepth = index;
}

BigInt CTokenPoolBank::getValueLeft()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    BigInt dimensionDepth = 0;
    if (mStatus == eTokenPoolBankStatus::depleted)
        return 0;

    if (auto pool = mPool.lock())
    {
      dimensionDepth = pool->getDimensionDepth();

      if (dimensionDepth == 0 ||  mCurrentDepth == dimensionDepth)
          return 0;

        return (dimensionDepth  - mCurrentDepth) * pool->getSingleTokenValue();
    }

    return 0;

}

/// <summary>
/// Retrieves the initial total value (including spent assets) within this token pool.
/// </summary>
/// <returns></returns>
BigInt CTokenPoolBank::getTotalValue()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    if (auto pool = mPool.lock())
    {
        return pool->getDimensionDepth() * pool->getSingleTokenValue();
    }
    return 0;
}

bool operator==(const CTokenPoolBank& c1, const CTokenPoolBank& c2)
{
    std::shared_ptr<CTools>tools = CTools::getInstance();
    if(!(c1.mCurrentDepth == c2.mCurrentDepth && tools->compareByteVectors(c1.mCurrentFinalHash, c2.mCurrentFinalHash)
    && c1.mCurrentIndex == c2.mCurrentIndex && tools->compareByteVectors(c1.mFinalHash, c2.mFinalHash)
    && tools->compareByteVectors(c2.mPreCachedSeedHash ,c2.mPreCachedSeedHash) && c1.mPreCachedSeedHashDepth == c2.mPreCachedSeedHashDepth
    && tools->compareByteVectors(Botan::unlock(c1.mSeedHash) , Botan::unlock(c2.mSeedHash)) && c1.mStatus == c2.mStatus))
    return false;

    return true;
}

bool operator!=(const CTokenPoolBank& c1, const CTokenPoolBank& c2)
{
    return !(c1 == c2);
}
