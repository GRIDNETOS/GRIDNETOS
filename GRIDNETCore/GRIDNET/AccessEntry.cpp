#include "AccessEntry.h"

AEFlags& AEFlags::operator=(const AEFlags& t)
{
    std::memcpy(this, &t, sizeof(AEFlags));
    return *this;
}

void AEFlags::clear()
{
    std::memset(this, 0, sizeof(AEFlags));
  
}
void AEFlags::setDefaults()
{
    std::memset(this, 0, sizeof(AEFlags));
    mRead = true;
    mWrite = true;
}



AEFlags::AEFlags(const AEFlags& sibling)
{
    std::memcpy(this, &sibling, sizeof(AEFlags));
}

AEFlags::AEFlags(bool write , bool read, bool execute , bool assets, bool ownership , bool isDynamic)
{
    mWrite = write;
    mRead = read;
    mExecute = execute;
    mAssets = assets;
    mOwnership = ownership;
    mIsDynamicEntry = isDynamic;
}

AEFlags::AEFlags(uint8_t byte)
{
    std::memcpy(this, &byte, 1);
}

uint64_t AEFlags::getNr()
{
    uint8_t nr = 0;
    std::memcpy(&nr, this, 1);
    return static_cast<uint64_t>(nr);
}

void AEFlags::setIsDynamicEntry(bool isIt)
{
    mIsDynamicEntry = isIt;
}

bool AEFlags::getIsDynamicEntry()
{
    return mIsDynamicEntry;
}

void AEFlags::setRead(bool allowed)
{
    mRead = allowed;
}

void AEFlags::setOwner(bool isIt)
{
    mOwnership = isIt;
}


void AEFlags::setWrite(bool allowed)
{
    mWrite = allowed;
}

void AEFlags::setAssets(bool allowed)
{
    mAssets = true;
}

void AEFlags::setExecute(bool allowed)
{
    mExecute = allowed;
}

void AEFlags::setRemoval(bool allowed)
{
    mRemoval = allowed;
}


bool AEFlags::getRead()
{
    return mRead;
}

bool AEFlags::getRemoval()
{
    return mRemoval;
}

bool AEFlags::getAssets()
{
    return mAssets;
}

bool AEFlags::getWrite()
{
      return mWrite;
}

bool AEFlags::getExecute()
{
    return mExecute;
}

bool AEFlags::getVoting()
{
    return mIsPollPermission;
}

bool AEFlags::getOwnership()
{
    return mOwnership;
}

void AEFlags::setVoting(bool isIt)
{
    mIsPollPermission = isIt;
}


CAccessEntry::CAccessEntry(std::vector<uint8_t> sdID,bool write, bool read, bool execute, bool assets)
{
    initFields();
    mFlags = AEFlags(write, read, execute);
    mSDID = sdID;
}

CAccessEntry::CAccessEntry(const CAccessEntry& sibling)
{
    initFields();
    mFlags = sibling.mFlags;
    mSDID = sibling.mSDID;
}

CAccessEntry& CAccessEntry::operator=(const CAccessEntry& t)
{
    initFields();
    mFlags = t.mFlags;
    mSDID = t.mSDID;
    return *this;
}
bool CAccessEntry::setPermisions(bool write, bool execute, bool read , bool removal, bool assets)
{
    mFlags.setWrite(write);
    mFlags.setExecute(execute);
    mFlags.setRead(read);
    mFlags.setRemoval(removal);
    mFlags.setAssets(assets);
    return true;
}
std::vector<uint8_t> CAccessEntry::getPackedData()
{
    std::lock_guard <std::mutex> lock(mGuardian);

    Botan::DER_Encoder enc;
    std::vector<uint8_t> flagsV(1);
    flagsV[0] = static_cast<uint8_t>(mFlags.getNr());

    enc = enc.start_cons(Botan::ASN1_Tag::SEQUENCE).
        encode(mVersion)
        .start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(flagsV, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mSDID, Botan::ASN1_Tag::OCTET_STRING);

    enc.start_cons(Botan::ASN1_Tag::SEQUENCE);

    if (mPollIDs.size())//notice: if we ever add additional fields after mPollIDs - then this field would become obligatory.
    {
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
        for (uint64_t i = 0; i < mPollIDs.size(); i++)
        {
            enc.encode(mPollIDs[i]);
        }
        enc.end_cons();
    }

    enc.end_cons().end_cons().end_cons();

    std::vector<uint8_t> toRet = enc.get_contents_unlocked();
    std::shared_ptr<CAccessEntry> test = CAccessEntry::instantiate(toRet);
    return toRet;
}

std::shared_ptr<CAccessEntry> CAccessEntry::instantiate(std::vector<uint8_t> packedData)
{
    try {
        std::shared_ptr<CAccessEntry> token = std::make_shared<CAccessEntry>();//new version
        uint64_t sourceVersion = 1;
        Botan::BER_Decoder dec1 = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE);
        Botan::BER_Decoder dec2 =  dec1.decode(sourceVersion).start_cons(Botan::ASN1_Tag::SEQUENCE);
        std::vector<uint8_t> flagsV;

        if (sourceVersion == 1 || sourceVersion == 2)//would be upgraded when saved
        {
            dec2.decode(flagsV, Botan::ASN1_Tag::OCTET_STRING);
            if (flagsV.size() != 1)
                return nullptr;
            token->mFlags = AEFlags(flagsV[0]);

            dec2.decode(token->mSDID, Botan::ASN1_Tag::OCTET_STRING);
        }

        if (sourceVersion == 2 && dec2.more_items())// the following sequence is not obligatory. Notice that it would become - were we to introduce additional fields. 
        {//Polls Extension - a sequence of Polls the Access Entry relates to.
      
            Botan::BER_Decoder dec3 = Botan::BER_Decoder(dec2.get_next_object().value);
            uint64_t bankID = 0;

            while (dec3.more_items())
            {
                dec3.decode(bankID);
                token->mPollIDs.push_back(bankID);
            }
        }

        return token;
    }
    catch (...)
    {
        return nullptr;
    }
}

std::vector<uint8_t> CAccessEntry::getSDID()
{
    return mSDID;
}

void CAccessEntry::initFields()
{
    mVersion = 2;
}

AEFlags CAccessEntry::getAccessFlags()
{
    std::lock_guard <std::mutex> lock(mGuardian);
    return mFlags;
}

void CAccessEntry::setIsDynamic(bool isIt)
{
    std::lock_guard <std::mutex> lock(mGuardian);
    mFlags.setIsDynamicEntry(isIt);
}

bool CAccessEntry::getIsDynamic()
{
    std::lock_guard <std::mutex> lock(mGuardian);
    return mFlags.getIsDynamicEntry();
}

void CAccessEntry::grantOwnership(bool doIt)
{
    std::lock_guard <std::mutex> lock(mGuardian);
    mFlags.setOwner(doIt);
}

void CAccessEntry::grantWrite(bool doIt)
{
    std::lock_guard <std::mutex> lock(mGuardian);
    mFlags.setWrite(doIt);
}

void CAccessEntry::grantRead(bool doIt)
{
    std::lock_guard <std::mutex> lock(mGuardian);
    mFlags.setRead(doIt);
}

void CAccessEntry::grantExecute(bool doIt)
{
    std::lock_guard <std::mutex> lock(mGuardian);
    mFlags.setExecute(doIt);
}

void CAccessEntry::grantAssets(bool doIt)
{
    std::lock_guard <std::mutex> lock(mGuardian);
    mFlags.setAssets(doIt);
}

void CAccessEntry::grantRemoval(bool doIt)
{
    std::lock_guard <std::mutex> lock(mGuardian);
    mFlags.setRemoval(doIt);
}

void CAccessEntry::grantPollAccess(uint64_t pollID)
{
    for (uint64_t i = 0; i < mPollIDs.size(); i++)
    {
        if (mPollIDs[i] == pollID)
        {
            return;
        }
    }

    mPollIDs.push_back(pollID);
}

bool CAccessEntry::removePollAccess(uint64_t pollID)
{
    for (uint64_t i = 0; i < mPollIDs.size(); i++)
    {
        if (mPollIDs[i] == pollID)
        {
            mPollIDs.erase(mPollIDs.begin() + i);
            return true;
        }
       
    }
    return false;
}

bool CAccessEntry::getIsPollAccessGranted(uint64_t pollID)
{
    for (uint64_t i = 0; i < mPollIDs.size(); i++)
    {
        if (mPollIDs[i] == pollID)
        {
            return true;
        }
    }
    return false;
}
