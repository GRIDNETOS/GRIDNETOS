#include <memory>
#include <vector>
#include <mutex>
#include "TokenPool.h"
#include <cmath>
#include "CryptoFactory.h"
#include "TransmissionToken.h"
#include "TokenPoolBank.h"
#include "DTI.h"
#include "Receipt.h"




bool CTokenPool::validate(bool requireSeed)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return ((mMasterSeedHash.size()==32||!requireSeed)  && mOwnerID.size()>0 && mTokenPoolID.size()==32);
}

void CTokenPool::initFields()
{
    mCryptoFactory = nullptr;
    mDimensionsCount = 0;
    mDimensionDepth = 0;
    mTotalValue = 0;
    mVersion = 2;
    mTokenPoolID = CTools::getInstance()->genRandomVector(32);
    mStatus = eTokenPoolStatus::active;
}

CTokenPool::CTokenPool(std::shared_ptr<CCryptoFactory> cf, uint64_t dimensionsCount, std::vector<uint8_t> ownerID, std::vector<uint8_t> transactionID, BigInt valuePerToken, BigInt totalValue, BigInt currentIndex, std::string friendlyID, Botan::secure_vector<uint8_t> seedHash, std::vector<uint8_t> finalHash,  std::vector<uint8_t> currentHash)
{
    initFields();

    mCryptoFactory = cf;
    if (mCryptoFactory == nullptr)
        mCryptoFactory = CCryptoFactory::getInstance();
    mReceiptID = transactionID;
 
    mDimensionsCount = dimensionsCount;
    assertGN(valuePerToken != 0);
    BigInt totalTokensCount = valuePerToken?static_cast<BigInt>(floor<0>(static_cast<BigFloat>(totalValue) / static_cast<BigFloat>(valuePerToken))):0;
    //we'll distribute tokens among the available dimensions (banks)

    //the number of dimensions should be set accordinly with the expected numerosity of pending, parallel 
    //State-Less Blockchain Channel transactions
    mDimensionDepth = dimensionsCount?static_cast<BigInt>(floor<0>(static_cast<BigFloat>(totalTokensCount) / static_cast<BigFloat>(dimensionsCount))):0; 

    mTotalValue = valuePerToken * dimensionsCount * mDimensionDepth;

    mFriendlyID = friendlyID;
    mMasterSeedHash =  seedHash;

    mOwnerID = ownerID;
}

void CTokenPool::setTotalValue(BigInt value)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    mTotalValue = value;
}


eTokenPoolStatus::eTokenPoolStatus CTokenPool::getStatus()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mStatus;
}
uint64_t  CTokenPool::getDimensionsCount()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mDimensionsCount;
}

void CTokenPool::setDimensionsCount(uint64_t count)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    mDimensionsCount = count;
}

void CTokenPool::setStatus(eTokenPoolStatus::eTokenPoolStatus status)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    mStatus = status;
}

bool CTokenPool::getIsMasterSeedHashAvailable()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return (mMasterSeedHash.size() == 32) ? true : false;
}
void  CTokenPool::resetBanks()
{

    for (uint64_t i = 0; i < mBanks.size(); i++)
    {
        mBanks[i]->reset();
    }
}

/// <summary>
/// Validates the Transmission Token.
/// As seen by the current state of the token-pool.
/// MIGHT return 0 and the isValid flag set ONLY if token describes the current state.
/// Returns GBU value of the Token.
/// Return 0 if Token is invalid (or an attempt to cheat).
/// Invalid token should result in the coresponding Token-Pool being banned/blocked.
/// Currently the 'universal' implementation does not punish for double-spend attempts.
/// These attempts would NOT succeed, BUT agent would not be punished for an attempt.
/// </summary>
/// <param name="token"></param>
/// <returns></returns>
BigInt CTokenPool::validateTT(std::shared_ptr<CTransmissionToken> token, bool updateState, const bool&isValid)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    if (token == nullptr)
        return 0;

    //local-variables - BEGIN

    //**WARNING*** <--------------------
    BigInt worthValue = 0; //*UTMOST* importance is for this NOT to change until decision is made and everything IS VERIFIED
    //**WARNING*** <--------------------
    std::shared_ptr<CTokenPoolBank> bank;
    std::vector<uint8_t> startPoint;
    std::vector<uint8_t> checkPoint;
    BigInt iterationsCount = 0;
    std::shared_ptr<CTools> tools = CTools::getTools();

    startPoint = token->getRevealedHash();//that's the secret revealed by this very Transmission Token
    //local-variables - END

    //Operational Logic - BEGIN ----------------------

            //Security-Checks - BEGIN


    if (mStatus != eTokenPoolStatus::active)
        return 0;

    if (mPubKey.size() == 32 && token->getSig().size() > 0)
    {
        if (!CCryptoFactory::getInstance()->verifySignature(token->getSig(), token->getPackedData(false), mPubKey))
            return false;
    }

    if (token->getRevealedHashesCount() == 0)//worthless
        return 0;

    if (token->getValue() > getTotalValue()) //impossible
    {
        return 0;
    }

    //note that the order of checks suits performance optimization
    bank = getBankById(token->getBankID());
    if (bank == nullptr)
        return 0;

    if (bank->getStatus() == eTokenPoolBankStatus::depleted)
        return 0;

    if (token->getValue() > bank->getValueLeft()) //impossible
    {
        return 0;
    }

    if (token->getCurrentDepth() < bank->getCurrentDepth())//possibly a double-spend attempt 
        //(still, LEGIT behaviour IF data arrived through multiple paths though. Recognition of that NoT suported in this universial implementation)
        //Note that doule-spends would NOT succeeed anyway.
    {
       return 0;
    }

    /*
    DOUBLE spends: We CANNOT allow to iterate over part of the dimension which has been already consumed.
    */

    //special case where current depth == token's depth - BEGIN
    if (token->getCurrentDepth() == bank->getCurrentDepth() && tools->compareByteVectors(token->getRevealedHash(), bank->getCurrentHash()))
    {//used in Snake game. allows player to continue gameplay
        const_cast<bool&>(isValid) = true;
        return 0;
    }
    //special case where current depth == token's depth - END
    
            //Security-Checks - END

    //check tokens in between - BEGIN ----------------------
    startPoint = token->getRevealedHash();
    //we'll be verifying hashes from this point up to the current hash of the Token
    

    //IMPORTANT: for the purpose of rewards' calculation, we are interested ONLY in the CURRENT final hash.
    //we DO NOT care about the true final hash AT ALL. It is of paramount importance that the current hash gets updated within the VM at all times.
    //otherwise we would be opening up for double spends

    //thanks to this approach we DO NOT need to care about the reported current depth within the TT as well

    checkPoint = bank->getFinalHash();//gets CURRENT ceiling hash the token-pool interval represented by this token would NEED to end at it.
    //mCurrentHash will be used solely for optimization purposes IF available.
    //the function COULD return the true final hash if FALSE param set.

    //let's go with verifying tokens in between:
    //this should result in the final hash OR the intermediary hash IF available within the Token Pool (optimization)

    //check just in case
    if (startPoint.size() != 32 || checkPoint.size() != 32)
        return 0;

    iterationsCount = token->getRevealedHashesCount();

    BigInt effectiveIterations = token->getCurrentDepth() - bank->getCurrentDepth();
    //we ware NOT interested in the reported hashes being revealed. We compare the reported depth instead (more universal).
    //this allows to properly veirfy tokens who provide the number of revealed hashes from the ceiling value (ex. the Snake game)

    //hashing
    std::vector<uint8_t> currentHash = startPoint;
    for (uint64_t i = 0; i < effectiveIterations; i++)
    {
        currentHash = mCryptoFactory->getSHA2_256Vec(currentHash);
       
    }
    if (!tools->compareByteVectors(currentHash, checkPoint))
    {
        return 0;
    }

    //check if checkpoint reached
    if (!tools->compareByteVectors(currentHash, checkPoint))
    {
        return 0;
    }

    //check tokens in between - END ----------------------
   
    //calculate total value
    worthValue = getSingleTokenValue() * effectiveIterations;

    if (updateState)
    {
        //update pool-usage (internal state only)
        bank->setCurrentHash(token->getRevealedHash());
        bank->setCurrentDepth(token->getCurrentDepth());

        //depletion check
        if (updateState && bank->getCurrentDepth() >= mDimensionDepth)
        {
            bank->setStatus(eTokenPoolBankStatus::depleted);
        }
    }

    //** DECISION MADE - BEGIN **



    //** DECISION MADE - END **

    //checks passed

    //Operational Logic - END ----------------------
    const_cast<bool&>(isValid) = true;
    return worthValue;
}
bool CTokenPool::generateDimensions(bool reportStatus, bool verify, std::shared_ptr<CDTI> dti, std::shared_ptr<CTools> tools )
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    //local variables - BEGIN
    std::vector<uint8_t> bankSeed;
    std::shared_ptr<CTokenPoolBank> bank;
    std::vector<uint8_t> currentHash;
    std::string reportLine;
    BigInt lastReportAt = 0;
    BigInt reportEveryHash = 1000;
    BigInt generatedTokens = 0;
    std::vector<uint8_t> temp(64);
    uint64_t progress = 0;
    //local variables - END

    if (mMasterSeedHash.size() != 0 || mBanks.size() != 0)
        return false;

    //gen masterSeedhash
    currentHash = CTools::getTools()->genRandomVector(32);//no need for side-channel security over here during computations
    mMasterSeedHash = Botan::secure_vector<uint8_t>(currentHash.begin(), currentHash.end());
    std::vector<uint8_t> masterNr(32);
    std::memcpy(masterNr.data(), currentHash.data(), 32);

    mBanks.clear();

    if(tools==nullptr)
    tools = CTools::getInstance();
  

    for (uint64_t a = 0; a < mDimensionsCount; a++)
    {

        //Dimension Switch - begin
        //Note: the dimension's secret is never revealed.
        std::memcpy(temp.data(), masterNr.data(), 32);
        std::memcpy(temp.data() + 32, masterNr.data(), 32);
        masterNr = mCryptoFactory->getSHA2_256Vec(temp);//switch to a new dimension
        bankSeed = mCryptoFactory->getSHA2_256Vec(masterNr);
        //Dimension Switch - end

       // masterNr++;//generate new seed-hash for each bank based of the master-seed-hash
       // bankSeed = masterNr;//tools->BigIntToBytes(masterNr);

        currentHash = bankSeed;

        //let us proceed with token-generation
        for (BigInt i = 0; i < mDimensionDepth; i++)
        {
            currentHash = mCryptoFactory->getSHA2_256Vec(currentHash);
            generatedTokens++;

            //Reporting - BEGIN
            if (reportStatus && generatedTokens > reportEveryHash && ((generatedTokens - lastReportAt) > reportEveryHash))
            {
                lastReportAt = generatedTokens;
                progress = static_cast<uint64_t>(((double)a / (double)mDimensionsCount) * 100);
                reportLine = "Progress: " + std::to_string(progress) + "%";

                if (dti != nullptr)
                {
                    dti->flashLine(reportLine, true);
                }
                else
                {
                    tools->flashLine(reportLine, true, eViewState::GridScriptConsole);
                }
            }
            //Reporting - END
        }
        std::vector<uint8_t> ending = currentHash;
        //verify - begin
        currentHash = bankSeed;
        if (verify)
        {
            for (BigInt i = mDimensionDepth; i > 0; i--)// !tools->compareByteVectors(currentHash, ending) && iterations < (mDimensionDepth + 10))
            {
                currentHash = mCryptoFactory->getSHA2_256Vec(currentHash);
            }
            if (!tools->compareByteVectors(currentHash, ending))
                return false;
        }

        //verify - end

        //CTokenPoolBank(a,shared_from_this(), currentHash, 0, Botan::secure_vector<uint8_t>(bankSeed.begin(), bankSeed.end()));
        bank = std::make_shared<CTokenPoolBank>(a, shared_from_this(), ending, 0, Botan::secure_vector<uint8_t>(bankSeed.begin(), bankSeed.end()));
        mBanks.push_back(bank);
    }

    return true;

}
/// <summary>
/// Retrieves seeding hash for a given dimension (if master-seed-hash available)
/// </summary>
/// <param name="dimensionID"></param>
/// <returns></returns>
Botan::secure_vector<uint8_t> CTokenPool::getSeedingHashForDimension(uint64_t dimensionID)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    //local variables - BEGIN
    Botan::secure_vector<uint8_t> toRet;
    std::vector<uint8_t> bankSeed;
    // BigInt masterNr = 0;
    std::vector<uint8_t> masterNr(32);
    std::vector<uint8_t> temp(64);
    size_t s = 0;
    //local variables - END


    if (mMasterSeedHash.size() != 32)
        return toRet;

    //masterNr = BigInt(mMasterSeedHash);//intepret hash as a number

    std::memcpy(masterNr.data(), mMasterSeedHash.data(), 32);

    for (uint64_t a = 0; a < mDimensionsCount; a++)
    {
        //Dimension Switch - begin
        std::memcpy(temp.data(), masterNr.data(), 32);
        std::memcpy(temp.data() + 32, masterNr.data(), 32);
        masterNr = mCryptoFactory->getSHA2_256Vec(temp);
        bankSeed = mCryptoFactory->getSHA2_256Vec(masterNr);
        //Dimension Switch - end

       // masterNr++;//generate new seed-hash for each bank based of the master-seed-hash
       // bankSeed = masterNr;//CTools::getInstance()->BigIntToBytes(masterNr);
       // s = bankSeed.size();
       
     //   s = bankSeed.size();
        if (a == dimensionID)
            return Botan::secure_vector<uint8_t>(bankSeed.begin(), bankSeed.end());
    }
    return toRet;
}

bool CTokenPool::setMasterSeedHash(Botan::secure_vector<uint8_t>  seed)
{
   
    if(seed.size()!=32)
    return false;
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    Botan::secure_vector<uint8_t> dimSeed;
    mMasterSeedHash = seed;

    for (uint64_t i = 0; i < mBanks.size(); i++)
    {
        dimSeed = getSeedingHashForDimension(i);
        if (!mBanks[i]->setSeedHash(dimSeed))
            return false;
    }
    return true;
}

/// <summary>
/// Gets the Seed Hash(if available);
/// </summary>
/// <returns></returns>
Botan::secure_vector<uint8_t> CTokenPool::getSeedHash()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mMasterSeedHash;
}

/// <summary>
/// Retrieves information in regard to the Multi-Dimensional Token Pool and
// of each of its internal dimensions (value-store banks).
/// </summary>
/// <param name="newLine"></param>
/// <returns></returns>
std::string CTokenPool::getInfo(std::string newLine, bool describeDimensions, bool includeAnsiFormatting)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    //local variables - BEGIN
    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::string toRet= std::string(includeAnsiFormatting?(newLine + CTools::getInstance()->getColoredString("v---------------Token Pool-------------v",eColor::blue) + newLine):"");
    uint64_t valueLeft = 0;
   
    //local variables - END
    toRet   +=  tools->getColoredString("[ID]: ", includeAnsiFormatting?eColor::blue: eColor::none) + tools->base58CheckEncode(mTokenPoolID)
        + (mFriendlyID.size() > 0 ? (newLine +  tools->getColoredString("[Friendly-ID]: ", includeAnsiFormatting?eColor::blue: eColor::none) + mFriendlyID) : "")+ newLine + "Status: ";

    switch (mStatus)
    {
    case eTokenPoolStatus::active:
        toRet += tools->getColoredString("Active", includeAnsiFormatting?eColor::lightGreen: eColor::none);
        break;
    case eTokenPoolStatus::depleted:
        toRet += tools->getColoredString("Depleted", includeAnsiFormatting?eColor::lightPink: eColor::none);
        break;
    case eTokenPoolStatus::banned:
        toRet += tools->getColoredString("Banned", includeAnsiFormatting?eColor::cyborgBlood: eColor::none);
        break;
    default:
        toRet += "Unknown";
        break;
    }

    
    toRet += newLine + std::string(
        mMasterSeedHash.size() == 32 ? ("Master Seed-Hash: " + tools->base58CheckEncode(Botan::unlock(mMasterSeedHash))
            + newLine + "            ^-- [IMPORTANT: do not disclose, store securely]") : ""); 

    toRet += newLine + "[Dimensions' Depth]: " + mDimensionDepth.str()
        + newLine + "[Dimensions' Count]: " + std::to_string(mDimensionsCount)
        + newLine + "[Tokens in all dimensions]: " + (mDimensionDepth * mDimensionsCount).str()
        + newLine + "[Single-Token Value]: " + tools->attoToGNCStr(getSingleTokenValue()) + " GNC " + "(" + getSingleTokenValue().str() + " Atto Units)"
        + newLine + "[Total Initial Value] : " + tools->attoToGNCStr(mTotalValue) + " GNC " + "(" + mTotalValue.str() + " Atto Units)"
        + newLine + "[Total Value Left]: " + tools->attoToGNCStr(getValueLeft()) + " GNC " + "(" + getValueLeft().str() + " Atto Units)";
                //print info for each dimension
                if (describeDimensions && mBanks.size() > 0)
                {
                    toRet += newLine+std::to_string(mBanks.size())+ " Value Banks available."+ newLine;
                    for (uint64_t i = 0; i < mBanks.size(); i++)
                    {
                        toRet += newLine + "- - Bank " + std::to_string(i) + "- - :" + newLine + mBanks[i]->getInfo(newLine) + newLine;
                    }
                }
                

                if (includeAnsiFormatting)
                {
                    toRet += newLine + CTools::getInstance()->getColoredString("^--------------------------------------^",eColor::blue) + newLine;
                }
    return toRet;

}

std::vector<uint8_t> CTokenPool::getOwnerID()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mOwnerID;
}

std::string CTokenPool::getFriendlyID()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mFriendlyID;
}

void CTokenPool::setFriendlyID(std::string id)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    mFriendlyID = id;
}

BigInt CTokenPool::getDimensionDepth()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mDimensionDepth;
}


std::vector<uint8_t> CTokenPool::getReceiptID()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mReceiptID;
}

std::vector<uint8_t> CTokenPool::getID()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mTokenPoolID;
}

std::vector<uint8_t> CTokenPool::getPackedData(bool includeSeed)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

   // if (mBanks.size() != mDimensionsCount || mBanks.size()==0)
      //  return std::vector<uint8_t>(); Note: banks CAN be empty, if empty then it's a request
    Botan::DER_Encoder enc;
    std::shared_ptr<CTools> tools = CTools::getInstance();
   
    enc.start_cons(Botan::ASN1_Tag::SEQUENCE).
        encode(mVersion)
        .start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(mOwnerID, Botan::ASN1_Tag::OCTET_STRING);

    if (includeSeed && mMasterSeedHash.size() == 32)
        enc.encode(mMasterSeedHash, Botan::ASN1_Tag::OCTET_STRING);
    else
        enc.encode(std::vector<uint8_t>(), Botan::ASN1_Tag::OCTET_STRING);

       //encode token-pool info 
        enc.encode(CTools::getInstance()->BigIntToBytes(mDimensionDepth), Botan::ASN1_Tag::OCTET_STRING)
        .encode(mDimensionsCount)
        .encode(tools->BigIntToBytes(mTotalValue), Botan::ASN1_Tag::OCTET_STRING)
        .encode(mReceiptID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mTokenPoolID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(tools->stringToBytes(mFriendlyID), Botan::ASN1_Tag::OCTET_STRING)
        .encode(static_cast<size_t>(mStatus));

        //encode info related to particular banks / dimensions - BEGIN
 
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
   
        for (uint64_t i = 0; i < mBanks.size(); i++)
        {
          
            enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
            enc.encode(mBanks[i]->getFinalHash(false), Botan::ASN1_Tag::OCTET_STRING)//IMPORTANT: use false parameter during serialization
                .encode(mBanks[i]->getCurrentHash(), Botan::ASN1_Tag::OCTET_STRING)
                .encode(tools->BigIntToBytes(mBanks[i]->getCurrentDepth()),  Botan::ASN1_Tag::OCTET_STRING)
            .end_cons();
        }

        //encode info related to particular banks / dimensions - END
   

        enc=enc.end_cons();
        //optional for authenticated TokenPools
        //authenticated token pools support both authenticated Tokens and non-authenticated tokens once the secret has been revealed 
        //both would be accepted. 
        //still , if a token IS authenticated then full-node woouldn't be able to 'steal it away'.
        //usage of authenticated tokens is more expensive due to their bigger size
        if (mPubKey.size() == 32)//optional
            enc = enc.encode(mPubKey, Botan::ASN1_Tag::OCTET_STRING);

       enc.end_cons().end_cons();

    return enc.get_contents_unlocked();
}

/// <summary>
/// Brings a serialize BER-encoded, Multi-Dimensional Token Pool to Life
/// </summary>
/// <param name="packedData"></param>
/// <param name="cf"></param>
/// <returns></returns>
std::shared_ptr<CTokenPool> CTokenPool::instantiate(const std::vector<uint8_t>& packedData, std::shared_ptr<CCryptoFactory> cf, Botan::secure_vector<uint8_t> masterSeed)
{
    //local variables - BEGIN
    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::shared_ptr<CTokenPoolBank> bank;
    std::shared_ptr<CTokenPool> pool;
    std::vector<uint8_t>friendlyID;
    BigInt usageDepth = 0;
    std::vector<uint8_t> finalHash, currentHash;
    size_t status = 0;
    //local variables - END

    try {
        pool = CTokenPool::getNew(cf);
      
        Botan::BER_Decoder dec1 = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE);
        Botan::BER_Decoder dec2 = dec1.decode(pool->mVersion).start_cons(Botan::ASN1_Tag::SEQUENCE);
        std::vector<uint8_t> temp;
        if(pool->mVersion==2)
        {
            dec2.decode(pool->mOwnerID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(pool->mMasterSeedHash, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
            pool->mDimensionDepth =tools->BytesToBigInt(temp);
            dec2.decode(pool->mDimensionsCount);
            dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
            pool->mTotalValue = tools->BytesToBigInt(temp);
            dec2.decode(pool->mReceiptID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(pool->mTokenPoolID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(friendlyID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(status);

            if (masterSeed.size() == 32)
            {
                pool->mMasterSeedHash = masterSeed;
            }

            //decode dimensions/banks
            Botan::BER_Decoder dec3 = Botan::BER_Decoder(dec2.get_next_object().value);
            uint64_t bankID = 0;
            while (dec3.more_items())
            {
                Botan::BER_Decoder dec4 = Botan::BER_Decoder(dec3.get_next_object().value);

                dec4.decode(finalHash, Botan::ASN1_Tag::OCTET_STRING);
                dec4.decode(currentHash, Botan::ASN1_Tag::OCTET_STRING);
                dec4.decode(temp,  Botan::ASN1_Tag::OCTET_STRING);
                usageDepth = tools->BytesToBigInt(temp);

                bank = std::make_shared<CTokenPoolBank>(bankID,pool, finalHash, usageDepth, pool->getSeedingHashForDimension(bankID));
                bank->setCurrentHash(currentHash);//info: both currentHash and final hash is needed if we don't wanna loose information regarding final hash.
                //for pure math we could be just replacing finalHash with the current value
                bank->setPool(pool);
                pool->mBanks.push_back(bank);



                if (CTools::getInstance()->compareByteVectors(currentHash,finalHash))
                    bank->setStatus(eTokenPoolBankStatus::depleted);//serializing this would be a waste of storage; we can infer its value
                bankID++;
            }
            //check for optional public-key
            if (dec2.more_items())
            {
                dec2.decode(pool->mPubKey, Botan::ASN1_Tag::OCTET_STRING);
            }

              dec2.end_cons().end_cons();
            if (friendlyID.size() > 0)
                pool->setFriendlyID(CTools::getTools()->bytesToString(friendlyID));

           
            dec2.verify_end();
            pool->setStatus(static_cast<eTokenPoolStatus::eTokenPoolStatus>(status));

    
        }
        return pool;

    }
    catch (...)
    {
        return nullptr;
    }
}

bool CTokenPool::setPubKey(std::vector<uint8_t> pubKey)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    if (!(pubKey.size() == 0 || pubKey.size() == 32))
        return false;
    mPubKey = pubKey;
    return true;
}

std::vector<uint8_t> CTokenPool::getPubKey()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mPubKey;
}

std::shared_ptr<CTokenPoolBank> CTokenPool::getBankById(uint64_t id)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    for (uint64_t i = 0; i < mBanks.size(); i++)
        if (i == id)
            return mBanks[i];
    return nullptr;
}

/// <summary>
/// Bringgs new TokenPool instance to Life (factory-model)
/// </summary>
/// <param name="cf"></param>
/// <param name="dimensionsCount"></param>
/// <param name="ownerID"></param>
/// <param name="receiptID"></param>
/// <param name="valuePerToken"></param>worthv
/// <param name="totalValue"></param>
/// <param name="currentIndex"></param>
/// <param name="friendlyID"></param>
/// <param name="seedHash"></param>
/// <param name="finalHash"></param>
/// <param name="currentHash"></param>
/// <returns></returns>
std::shared_ptr<CTokenPool> CTokenPool::getNew(std::shared_ptr<CCryptoFactory> cf, uint64_t dimensionsCount, std::vector<uint8_t> ownerID, std::vector<uint8_t> receiptID, BigInt valuePerToken, BigInt totalValue, BigInt currentIndex, std::string friendlyID, Botan::secure_vector<uint8_t> seedHash, std::vector<uint8_t> finalHash, std::vector<uint8_t> currentHash)
{//this way there's always at least one shared_ptr to (this)

   // if (totalValue == 0 || valuePerToken==0 || dimensionsCount ==0)
     //   return nullptr;

    return std::make_shared<CTokenPool>(cf, dimensionsCount, ownerID, receiptID,  valuePerToken,  totalValue, currentIndex, friendlyID, seedHash, finalHash,  currentHash);
}

eTokenPoolBankStatus::eTokenPoolBankStatus CTokenPool::getBankStatus(uint64_t bankID)
{
    //we do not want to return inner bank-objects to avoid state-synchronization between the two
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    std::shared_ptr<CTokenPoolBank> bank = getBankById(bankID);
    if (bank != nullptr)
    {
        return bank->getStatus();
    }
    return eTokenPoolBankStatus::depleted;
}

BigInt CTokenPool::getValueLeftInBank(uint64_t bankID)
{
    //we do not want to return inner bank-objects to avoid state-synchronization between the two
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    std::shared_ptr<CTokenPoolBank> bank = getBankById(bankID);
    if (bank != nullptr)
    {
        return bank->getValueLeft();
    }
    return 0;
}

void CTokenPool::notifyBankStatusChanged(eTokenPoolBankStatus::eTokenPoolBankStatus status)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    bool allBanksUsedUp = true;

    switch (status)
    {
    case eTokenPoolBankStatus::active:
        break;
    case eTokenPoolBankStatus::depleted:
        for (uint64_t i = 0; i<mBanks.size(); i++)
        {
            if (mBanks[i]->getStatus() != eTokenPoolBankStatus::depleted)
                return;
        }
        //all banks depleted, update the status of my own
        setStatus(eTokenPoolStatus::depleted);
        break;
    default:
        break;
    }
}

std::vector<uint8_t> CTokenPool::genTokenWorthValue(uint64_t bankID, BigInt value, BigInt& hashesConsumedCount, bool markUsed)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    std::vector<uint8_t> toRet;

    if (mBanks.size() == 0)
        return toRet;

    if (bankID > mBanks.size() - 1)
        return toRet;

   return mBanks[bankID]->genTokenWorthValue(value, hashesConsumedCount, markUsed);

}

std::shared_ptr<CTransmissionToken> CTokenPool::getTTWorthValue(uint64_t bankID, BigInt value, bool markUsed)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    if (mBanks.size() == 0 || bankID > mBanks.size() - 1)
        return nullptr;
   return  mBanks[bankID]->genTTWorthValue(value, markUsed);
}

BigInt CTokenPool::getSingleTokenValue()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    if (mDimensionDepth == 0 || mDimensionsCount == 0)
        return 0;//no tokens

    return static_cast<BigInt>( floor(static_cast<BigFloat>(mTotalValue)/(static_cast<BigFloat>(mDimensionDepth)* static_cast<BigFloat>(mDimensionsCount))));
}

BigInt CTokenPool::getTotalValue()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mTotalValue;
}

/// <summary>
/// Returns a confirmed, vefiried total value of a Token Pool.
/// Includes the value of already spent assets.
/// </summary>
/// <param name="receipt">Takes receipt of a sacrificial transaction.</param>
/// <returns></returns>
BigInt CTokenPool::getTotalValueVerified(const CReceipt &receipt)
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    //Local Variables - BEGIN
    BigInt txValue = const_cast<CReceipt&>(receipt).getSacrificedValue();
    BigInt tokensCount = mDimensionDepth * mDimensionsCount;
    //Local Variables - END

    //Operational Logic - BEGIN
    if (txValue == 0)
        return 0;

    if (tokensCount == 0)
        return 0;

    if (mTotalValue > txValue)
        return 0;

    // Integer division already truncates towards zero (floors for positive numbers)
    // Avoids expensive BigFloat conversion that can cause Boost multiprecision exceptions
    BigInt confirmedSingleTokenValue = txValue / tokensCount;
    return confirmedSingleTokenValue * tokensCount;
    //Operational Logic - END
}

/// <summary>
/// Gets value left in all banks.
/// </summary>
/// <returns></returns>
BigInt CTokenPool::getValueLeft()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);

    BigInt valueLeft = 0;

    for (uint64_t i = 0; i < mBanks.size(); i++)
    {
        valueLeft+= mBanks[i]->getValueLeft();
    }

    return valueLeft;
}

uint64_t CTokenPool::getVersion()
{
    std::lock_guard <std::recursive_mutex> lock(mGuardian);
    return mVersion;
}

bool operator==(const CTokenPool& c1, const CTokenPool& c2)
{
    std::shared_ptr<CTools>tools = CTools::getInstance();

    //perform deep comparison od immediate member value-fields

    if (!(c1.mDimensionDepth== c2.mDimensionDepth && 
        c1.mDimensionsCount==c2.mDimensionsCount
        && c1.mFriendlyID == c2.mFriendlyID && 
        tools->compareByteVectors(Botan::unlock(c1.mMasterSeedHash) , Botan::unlock(c2.mMasterSeedHash))
        && tools->compareByteVectors(c1.mOwnerID, c2.mOwnerID)
        && tools->compareByteVectors(c1.mReceiptID, c2.mReceiptID) && c1.mStatus == c2.mStatus
        && tools->compareByteVectors(c1.mTokenPoolID , c2.mTokenPoolID)
        && c1.mTotalValue == c2.mTotalValue && c1.mVersion == c2.mVersion && 
        tools->compareByteVectors(c1.mPubKey,c2.mPubKey)
       ))
        return false;

    //perform deep-comparison of member-data structures

    for (uint64_t i = 0; i < c1.mBanks.size(); i++)
    {
        if ((*c1.mBanks[i]) != (*c2.mBanks[i]))
        {
            return false;
        }
    }

    return true;
}

bool operator!=(const CTokenPool& c1, const CTokenPool& c2)
{
    return !(c1 == c2);
}
