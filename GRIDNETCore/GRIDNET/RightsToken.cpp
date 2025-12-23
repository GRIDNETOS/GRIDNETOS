#include "RightsToken.h"
#include "AccessEntry.h"
#include "AccessToken.h"
#include "EffectiveRights.h"
#include "Poll.h"
#include "TrieNode.h"
#include "CGlobalSecSettings.h"
void CSecDescriptor::initFields()
{
    mVersion = 3;
    mFlags.setDefaults();
}

void CSecDescriptor::setImplicitOwner(const std::vector<uint8_t>& ownerID)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mImplicitOwner = ownerID;
}

std::vector<uint8_t> CSecDescriptor::getImplicitOwner()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mImplicitOwner;
}

bool CSecDescriptor::getIsDefault()
{
    CRTFlags defFlags;
    defFlags.setDefaults();
    bool isIt = false;
    std::lock_guard<std::mutex> lock(mGuardian);
    if (mFlags != defFlags)
        return false;
    if (mOwner.size())
        return false;
    if(mExtensionBytes.size())
        return false;
    if (mAccessEntries.size())
        return false;
    if (mPollExt)
        return false;

    return true;
}

std::shared_ptr<CPollFileElem> CSecDescriptor::getPollExt()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mPollExt;
}

void  CSecDescriptor::setPollExt(std::shared_ptr<CPollFileElem> pollExt)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mPollExt = pollExt;
}

std::vector<uint8_t> CSecDescriptor::getOwner()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mOwner;
}

void CSecDescriptor::setOwner(std::vector<uint8_t> id)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mOwner = id;
}

std::vector<uint8_t> CSecDescriptor::getExtensionBytes()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mExtensionBytes;
}

void CSecDescriptor::setExtensionBytes(std::vector<uint8_t> bytes)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mExtensionBytes = bytes;
}

bool CSecDescriptor::getEveryoneRead()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mFlags.getEveryoneRead();
}

bool CSecDescriptor::getEveryoneWrite()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mFlags.getEveryoneWrite();
}

bool CSecDescriptor::getEveryoneExecute()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mFlags.getEveryoneExecute();
}

CSecDescriptor::CSecDescriptor()
{
    initFields();
}

CSecDescriptor::CSecDescriptor(bool sysOnly)
{
    initFields();
    mFlags.setSysOnly(sysOnly);
}

CSecDescriptor& CSecDescriptor::operator=(const CSecDescriptor& t)
{
    mVersion = t.mVersion;
    //Extensions - BEGIN
    if (t.mPollExt)
    {
        mPollExt = std::make_shared<CPollFileElem>(*t.mPollExt);
    }
    //Extensions - END

    mOwner = std::vector<uint8_t>(t.mOwner.begin(), t.mOwner.end());
    mExtensionBytes = std::vector<uint8_t>(t.mExtensionBytes.begin(), t.mExtensionBytes.end());

    mAccessEntries.clear();//deep copy
    for (uint64_t i = 0; i < t.mAccessEntries.size(); i++)
        mAccessEntries.push_back(t.mAccessEntries[i]);
    mFlags = t.mFlags;

    return *this;
}


CSecDescriptor::CSecDescriptor(const CSecDescriptor& sibling)
{
    mVersion = sibling.mVersion;
    mFlags = sibling.mFlags;
    if (sibling.mPollExt)
    {
        mPollExt = std::make_shared<CPollFileElem>(*sibling.mPollExt);
    }
    mOwner = std::vector<uint8_t>(sibling.mOwner.begin(), sibling.mOwner.end());
    mImplicitOwner = sibling.mImplicitOwner;
    mExtensionBytes = std::vector<uint8_t>(sibling.mExtensionBytes.begin(), sibling.mExtensionBytes.end());
    mAccessEntries.clear();//deep copy
    for (uint64_t i = 0; i < sibling.mAccessEntries.size(); i++)
        mAccessEntries.push_back(sibling.mAccessEntries[i]);
}

CSecDescriptor::CSecDescriptor(std::shared_ptr<CSecDescriptor> sibling)
{
    if (sibling)
    {
        if (sibling->mPollExt)
        {
            mPollExt = std::make_shared<CPollFileElem>(*sibling->mPollExt);
        }
        mImplicitOwner = sibling->mImplicitOwner;
        mOwner = std::vector<uint8_t>(sibling->mOwner.begin(), sibling->mOwner.end());
        mExtensionBytes = std::vector<uint8_t>(sibling->mExtensionBytes.begin(), sibling->mExtensionBytes.end());
        mVersion = sibling->mVersion;
        mFlags = sibling->mFlags;
        mAccessEntries.clear();//deep copy

        for (uint64_t i = 0; i < sibling->mAccessEntries.size(); i++)
            mAccessEntries.push_back(sibling->mAccessEntries[i]);
    }
    else
    {
        //use defaults
        initFields();
    }

}

bool CSecDescriptor::setCRTFlags(CRTFlags flags)
{
    mFlags = flags;
    if (!flags.getHasACL())
        mAccessEntries.clear();
    return true;
}

std::vector<uint8_t> CSecDescriptor::getPackedData()
{
    std::lock_guard <std::mutex> lock(mGuardian);

    Botan::DER_Encoder enc;
    std::vector<uint8_t> flagsV(1);
    flagsV[0] = static_cast<uint8_t>(mFlags.getNr());

    //Serialize Extensions - BEGIN
        //Polls Extension - BEGIN

    if (mPollExt)// || any other extension. Keep order and insert dummy extensions in order even if prior not in use!
    {//DO NOT attempt to serialize if extension not in use to assure backwards-compatibility (in terms of the resulting
        //image of this very element).
        Botan::DER_Encoder encExt;
        encExt = encExt.start_cons(Botan::ASN1_Tag::SEQUENCE);
        //all serialized extensions follow as OCTET_STRINGs, in order.
        encExt = encExt.encode(mPollExt->getPackedData(), Botan::ASN1_Tag::OCTET_STRING);
        encExt = encExt.end_cons();
        mExtensionBytes = encExt.get_contents_unlocked();
    }
        //Polls Extension - END
    //Serialize Extensions - END
    enc = enc.start_cons(Botan::ASN1_Tag::SEQUENCE).
        encode(mVersion)
        .encode(flagsV, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mExtensionBytes, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mOwner, Botan::ASN1_Tag::OCTET_STRING)
        .start_cons(Botan::ASN1_Tag::SEQUENCE);

    for (uint64_t i = 0; i < mAccessEntries.size(); i++)
    {
        enc = enc.encode(mAccessEntries[i]->getPackedData(), Botan::ASN1_Tag::OCTET_STRING);
    }

     enc.end_cons().end_cons();

    return enc.get_contents_unlocked();
}

std::shared_ptr<CSecDescriptor> CSecDescriptor::instantiate(const std::vector<uint8_t>& packedData)
{
    if (packedData.size() == 0)
        return nullptr;
    try {
        std::shared_ptr<CSecDescriptor> token = std::make_shared<CSecDescriptor>();//new version
        uint64_t sourceVersion=1;
        Botan::BER_Decoder dec1 = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE);
        dec1.decode(sourceVersion);
        std::vector<uint8_t> flagsV;
        bool sysOnlyV1 = false;
        std::vector<uint8_t> sdID;
        size_t status = 0;

        if (sourceVersion == 1)//would be upgraded when saved
        {
            dec1.decode(sysOnlyV1);

            if (sysOnlyV1)
                token->mFlags.setSysOnly();

            Botan::BER_Decoder dec2 = dec1.start_cons(Botan::ASN1_Tag::SEQUENCE);

            std::vector<uint8_t> accessToken;
            while (dec2.more_items())
            {
                dec2.decode(sdID, Botan::ASN1_Tag::OCTET_STRING);
                std::shared_ptr<CAccessEntry> ae = std::make_shared<CAccessEntry>(sdID, true);
                token->mAccessEntries.push_back(ae);
            }
        }
        else
            if (sourceVersion == 2)
            {
                dec1.decode(flagsV, Botan::ASN1_Tag::SEQUENCE);
                if (flagsV.size() != 1)
                    return nullptr;
                token->mFlags = CRTFlags(flagsV[0]);
           
                Botan::BER_Decoder dec2 = dec1.start_cons(Botan::ASN1_Tag::SEQUENCE);

               
                while (dec2.more_items())
                {
                    dec2.decode(sdID, Botan::ASN1_Tag::OCTET_STRING);
                    std::shared_ptr<CAccessEntry> ae = std::make_shared<CAccessEntry>(sdID, true);
                    token->mAccessEntries.push_back(ae);
                }
            }
            else
                if (sourceVersion == 3)
                {
                    dec1.decode(flagsV, Botan::ASN1_Tag::OCTET_STRING);

                    if (flagsV.size() != 1)
                        return nullptr;
                    token->mFlags = CRTFlags(flagsV[0]);

                    dec1.decode(token->mExtensionBytes, Botan::ASN1_Tag::OCTET_STRING);
                    uint64_t extInd = 0;
                    std::vector<uint8_t> extTemp;
                    if (token->mExtensionBytes.size())
                    {
                       
                        //Extensions Support - BEGIN
                        Botan::BER_Decoder decExt = Botan::BER_Decoder(token->mExtensionBytes).
                            start_cons(Botan::ASN1_Tag::SEQUENCE);
                                //now we expect each and every extension (even if not in use)
                                // to be present within, in order.
                                while (decExt.more_items())
                                {
                                    //fetch bytes comprising the extension
                                    decExt.decode(extTemp, Botan::ASN1_Tag::OCTET_STRING);

                                    //interpret these bytes,- instantiate the appropriate Extension Object.
                                    switch (extInd)
                                    {
                                    case 0:
                                        //Position 0 - Polls Extension - BEGIN
                                        token->mPollExt = CPollFileElem::instantiate(extTemp);
                                        //Position 0 - Polls Extension - END
                                        break;
                                    default:
                                        //unsupported extension (by local instance).
                                       break;// continue and  let it read all the serialized arrays _ this break does NOT break the outer loop
                                    }
                          
                                    ++extInd;
                                }
                                decExt.verify_end();
                        //Extensions Support - END
                    }
                    dec1.decode(token->mOwner, Botan::ASN1_Tag::OCTET_STRING);

                    Botan::BER_Decoder dec2 = dec1.start_cons(Botan::ASN1_Tag::SEQUENCE);

                    while (dec2.more_items())
                    {
                        dec2.decode(sdID, Botan::ASN1_Tag::OCTET_STRING);
                        std::shared_ptr<CAccessEntry> ae = CAccessEntry::instantiate(sdID);
                        if (ae == nullptr)
                            return nullptr;//should never happen 
                        token->mAccessEntries.push_back(ae);
                    }
                }
        
        return token;
    }
   
    catch (...)
    {
        return nullptr;
    }
}

std::shared_ptr<CSecDescriptor> CSecDescriptor::genSysOnlyDescriptor()
{
    return std::make_shared<CSecDescriptor>(true);
}

void CSecDescriptor::setVersion(size_t version)
{
    std::lock_guard <std::mutex> lock(mGuardian);
    mVersion = version;
}

size_t CSecDescriptor::getVersion()
{
    std::lock_guard <std::mutex> lock(mGuardian);
    return mVersion;
}

std::vector <std::shared_ptr<CAccessEntry>> CSecDescriptor::getACEEntries(bool includeDynamic, std::shared_ptr<CStateDomainManager> sdm)
{
    std::vector <std::shared_ptr<CAccessEntry>> toRet, dynamic;
    dynamic = getDynamicACEEntries(sdm);
    std::lock_guard <std::mutex> lock(mGuardian);
    toRet.insert(toRet.begin(), mAccessEntries.begin(), mAccessEntries.end());

    if (includeDynamic && sdm)
    {
      
        toRet.insert(toRet.end(), dynamic.begin(), dynamic.end());
    }

    return toRet;
}

std::vector<std::shared_ptr<CAccessEntry>> CSecDescriptor::getDynamicACEEntries(std::shared_ptr<CStateDomainManager> sdm)
{
    std::vector<std::shared_ptr<CAccessEntry>> toRet;
    bool isObjectOwner = false;
    std::shared_ptr<CAccessEntry> ace;
    //Polling Extension - BEGIN

    std::shared_ptr<CPollElem> poll;
    std::shared_ptr<CPollFileElem>  pollExtElem = getPollExt();
    std::shared_ptr<CTools> tools = CTools::getInstance();


    if (pollExtElem)
    {//active poll overrides the corresponding privileges that would otherwise stem from ACLs (including ownership).
        bool pollBasedAccessGranted = false;//DEFAULT: REJECT (*IMPORTANT*) - COULD be granted down below - and then revoked 
        //even further down below..

        //Local Variables - BEGIN
        std::shared_ptr<CPollElem> poll;
        PollElemFlags flags;
        std::vector<uint8_t> winnerID;
        BigInt winningScore = 0;
        std::vector<uint8_t> perspective;
        uint64_t cost = 0;


        //Local Variables - END

        //now we check each and every 'votable' permission for a corresponding active poll.

        //override ACL/ownership-based assets' spendings privilege access - BEGIN
        poll = pollExtElem->getPollWithLocalID(eSystemPollID::spendingsAcessRightsPoll);
        if (poll)
        {
            flags = poll->getFlags();

            if (flags.getIsActive())//an inactive poll is oblivious to our processing.
            {
                //check if a winner was elected
                if (poll->checkTrigger(sdm, winnerID, winningScore))
                {
                    bool ACEFound = false;
                    for (uint64_t i = 0; i < toRet.size(); i++)
                    {
                        if (tools->compareByteVectors(toRet[i]->getSDID(), winnerID))
                        {
                            ACEFound = true;
                            toRet[i]->grantAssets();
                        }
                    }

                    if (!ACEFound)
                    {
                        std::shared_ptr< CAccessEntry> ae = std::make_shared<CAccessEntry>(winnerID);
                        ae->setIsDynamic(true);
                        ae->grantAssets();
                        toRet.push_back(ae);
                    }
                }

            }
        }
            poll = pollExtElem->getPollWithLocalID(eSystemPollID::writeAccessRightsPoll);
            if (poll)
            {
                flags = poll->getFlags();

                if (flags.getIsActive())//an inactive poll is oblivious to our processing.
                {
                    //check if a winner was elected
                    if (poll->checkTrigger(sdm, winnerID, winningScore))
                    {
                        bool ACEFound = false;
                        for (uint64_t i = 0; i < toRet.size(); i++)
                        {
                            if (tools->compareByteVectors(toRet[i]->getSDID(), winnerID))
                            {
                                ACEFound = true;
                                toRet[i]->grantWrite();
                            }
                        }

                        if (!ACEFound)
                        {
                            std::shared_ptr< CAccessEntry> ae = std::make_shared<CAccessEntry>(winnerID);
                            ae->setIsDynamic(true);
                            ae->grantWrite();
                            toRet.push_back(ae);
                        }
                    }

                }
            }

            poll = pollExtElem->getPollWithLocalID(eSystemPollID::ownershipChangeAccessRightsPoll);
            if (poll)
            {
                flags = poll->getFlags();

                if (flags.getIsActive())//an inactive poll is oblivious to our processing.
                {
                    //check if a winner was elected
                    if (poll->checkTrigger(sdm, winnerID, winningScore))
                    {
                        bool ACEFound = false;
                        for (uint64_t i = 0; i < toRet.size(); i++)
                        {
                            if (tools->compareByteVectors(toRet[i]->getSDID(), winnerID))
                            {
                                ACEFound = true;
                                toRet[i]->grantOwnership(true);
                            }
                        }

                        if (!ACEFound)
                        {
                            std::shared_ptr< CAccessEntry> ae = std::make_shared<CAccessEntry>(winnerID);
                            ae->setIsDynamic(true);
                            ae->grantOwnership(true);
                            toRet.push_back(ae);
                        }
                    }

                }
            }
            poll = pollExtElem->getPollWithLocalID(eSystemPollID::executeAccessRightsPoll);
            if (poll)
            {
                flags = poll->getFlags();

                if (flags.getIsActive())//an inactive poll is oblivious to our processing.
                {
                    //check if a winner was elected
                    if (poll->checkTrigger(sdm, winnerID, winningScore))
                    {
                        bool ACEFound = false;
                        for (uint64_t i = 0; i < toRet.size(); i++)
                        {
                            if (tools->compareByteVectors(toRet[i]->getSDID(), winnerID))
                            {
                                ACEFound = true;
                                toRet[i]->grantExecute(true);
                            }
                        }

                        if (!ACEFound)
                        {
                            std::shared_ptr< CAccessEntry> ae = std::make_shared<CAccessEntry>(winnerID);
                            ae->setIsDynamic(true);
                            ae->grantExecute(true);
                            toRet.push_back(ae);
                        }
                    }

                }
            }
            poll = pollExtElem->getPollWithLocalID(eSystemPollID::removalAccessRightsPoll);
            if (poll)
            {
                flags = poll->getFlags();

                if (flags.getIsActive())//an inactive poll is oblivious to our processing.
                {
                    //check if a winner was elected
                    if (poll->checkTrigger(sdm, winnerID, winningScore))
                    {
                        bool ACEFound = false;
                        for (uint64_t i = 0; i < toRet.size(); i++)
                        {
                            if (tools->compareByteVectors(toRet[i]->getSDID(), winnerID))
                            {
                                ACEFound = true;
                                toRet[i]->grantRemoval(true);
                            }
                        }

                        if (!ACEFound)
                        {
                            std::shared_ptr< CAccessEntry> ae = std::make_shared<CAccessEntry>(winnerID);
                            ae->setIsDynamic(true);
                            ae->grantRemoval(true);
                            toRet.push_back(ae);
                        }
                    }

                }
            }
        

        //override object-removal access - END


    }
    //Polling Extension - END
    return toRet;
}

/// <summary>
/// Owner will be charged for addition of /alteration of the data-structure accordingly 
/// by looking at the difference in size of the serialized data-structure
/// </summary>
/// <param name="stateDomainID"></param>
/// <returns></returns>
bool CSecDescriptor::addACE(std::shared_ptr<CAccessEntry> ACE)
{
    if (ACE == nullptr)
        return false;

    std::lock_guard <std::mutex> lock(mGuardian);
    std::shared_ptr<CTools> tools = CTools::getTools();
    bool found = false;
    for (uint64_t i = 0; i < mAccessEntries.size(); i++)
    {
        if (tools->compareByteVectors(ACE->getSDID(), mAccessEntries[i]->getSDID()))
        {
            mAccessEntries[i] = ACE;
            break;
        }
    }
    if (!found)
        mAccessEntries.push_back(ACE);

     return true;
}
bool CSecDescriptor::removeACEAtIndex(uint64_t ind)
{
    if (mAccessEntries.size() == 0)
        return false;
    if (ind > mAccessEntries.size() - 1)
        return false;
    mAccessEntries.erase(mAccessEntries.begin() + ind);
    return true;

}

/// <summary>
/// Calculates Effective Access Rights, by comparing security descriptor associated with the object and the provided Access Token.
/// The Access Token encapsulates manifestation of the requested Access Rights.
/// </summary>
/// <param name="accessToken"></param>
/// <param name="sdm"></param>
/// <param name="updateSecDesc"></param>
/// <returns></returns>
std::shared_ptr<CEffectiveRights> CSecDescriptor::getEffectiveRights(std::shared_ptr<CAccessToken> accessToken, std::shared_ptr<CStateDomainManager> sdm, const bool& updateSecDesc)
{
    std::shared_ptr<CPollFileElem>  pollExtElem = getPollExt();
    std::shared_ptr<CAccessEntry> ace = getACEforEntity(accessToken->getEntityID());;//agent-specific ACE entry (might be non-existent).

    if (!accessToken)
        return std::make_shared<CEffectiveRights>();
    else
    {
        //Overwatch - BEGIN
        if (CGlobalSecSettings::isAnOverwatch(accessToken->getEntityID()))
        {
            return std::make_shared<CEffectiveRights>(true,true,true,true,true,true,true);
        }
        //Overwatch - END
    }
    std::vector<uint8_t> friendlyID = CTools::getInstance()->stringToBytes(accessToken->getFriendlyID());
    bool isSysOnly = getIsSysOnly();//before locking the mutex!
    std::shared_ptr<CTools> tools = CTools::getTools();
    bool isObjectOwner = false;
    const_cast<bool&>(updateSecDesc) = false;
    std::lock_guard<std::mutex> lock(mGuardian);
    std::vector<uint64_t> accessedPollIDs = accessToken->getAccessedPollIDs();
    RequestedPermissions reqPerm = accessToken->getRequestedPermissions();
    //Check POSIX-style flags - BEGIN
    std::shared_ptr<CEffectiveRights> toRet = std::make_shared<CEffectiveRights>(mFlags.getEveryoneRead(), mFlags.getEveryoneWrite(), mFlags.getEveryoneExecute());
    //Check POSIX-style flags - END

    if (mOwner.size())
    {//explicit owner always had priority over an implicit owner
        if (tools->compareByteVectors(accessToken->getEntityID(), mOwner))
        {
            isObjectOwner = true;
        }
    }
    else if(mImplicitOwner.size())
    {//Implicit owner can now be tried.
        if (tools->compareByteVectors(accessToken->getEntityID(), mImplicitOwner))
        {
            isObjectOwner = true;
        }
    }

    //Force Abort Flags - BEGIN
    //if any of these flags is set, it would abort any further processing of the corresponding permission.
    bool readForceAbort = false;
    bool spendingForceAbort = false;
    bool writeForceAbort = false;
    bool ownershipForceAbort = false;
    bool executeForceAbort = false;
    bool removalForceAbort = false;
    bool votingForceAbort = false;
    //Force Abort Flags - END

    /*
    * The processing proceeds in the following order:
    * 1) System Only Objects security check - only System can alter objects labeled as 'system only.
    * Processing of security sub-system extensions - BEGIN (extensions have precedence over ACL and MAY have precedence even over
      ownership privilege if an active poll is associated with the corresponding privilege).
    * 2) Polling Extension - may override any permission if a poll is associated with the corresponding permission.
    * 3) Ownership Extension - only owner can modify ACLs/ACEs, change the ownership or associate Polls with objects.
    * Processing of security sub-system extensions - END
      4) CRT flags - i.e. everyone Reads, everyone Writes, everyone executes.
      5) Explicit ACL entries - i.e. user specific Access Control Entries (ACEs).
    */

    //Polling Extension - BEGIN

    //Voting Permission - BEGIN
    //Notice: here, it's 'only' about security/permissions not about factual/functional possibility of issuing a vote - since the poll might not exist at all
    //which is not supposed to be checked here for.
    
    //if (reqPerm.getIsVoting())
    //{
        bool votingGranted = true;// by default anyone can vote. Support of system objects would proceed down below later on.
        std::shared_ptr<CPollElem> poll;
        //file owner always can vote.
        if (isObjectOwner)
        {
            votingGranted = true;
        }
    
        ace = getACEforEntity(accessToken->getEntityID());// does an ACE entry already exist for this agent?

        //check for permission - BEGIN
        if (ace)
        {
            for (uint64_t i = 0; i < accessedPollIDs.size() && !votingForceAbort; i++)
            {
                if (pollExtElem)
                {
                    poll = pollExtElem->getPollWithLocalID(accessedPollIDs[i]);

                    if (poll && !ace->getIsPollAccessGranted(accessedPollIDs[i]))
                    {
                        //The poll exists, the poll requires only select users to vote (even object owner excluded).
                        //Then, if we cannot vote on any of the selected polls, do not grant voting permission.
                        votingForceAbort = true;
                    }
                }
            }
        }
        //check for permission - END

        if (votingGranted)
        {
            //grant access - BEGIN
            if (ace)
            {
                ace->grantAssets(true);
            }
            else
            {//if not then create one.
                ace = std::make_shared<CAccessEntry>(accessToken->getEntityID());//use default permissions.
                ace->grantAssets(true);//grant the extra ones.
                 // const_cast<bool&>(updateSecDesc) = true;//IMPORTANT: the security descriptor NEEDS to be updated (in cold storage) if the results are
                //taken into account during further processing (i.e. if results are exercised outside of this function ex. assets truly being spent).
            }
            //grant access - END
        }
    //}
    //Voting Permission - END
   
    //Yet again: the extension WOULD override standard processing of ACLs if corresponding poll is found active.


    //Sys-Only Object Support - BEGIN
    if (accessToken->getIsSystem())
        return  std::make_shared<CEffectiveRights>(true, true, true, false, true, true, false);//system can do everything - besides voting.
    else
    {
        if (isSysOnly)//if accessing identity is not system but the object is sys-only.
            return std::make_shared<CEffectiveRights>(true, false, true, false, false, false, false);//allow only to read the data.
    }
    //Sys-Only Object Support - END

    if (pollExtElem)
    {//active poll overrides the corresponding privileges that would otherwise stem from ACLs (including ownership).
        bool pollBasedAccessGranted = false;//DEFAULT: REJECT (*IMPORTANT*) - COULD be granted down below - and then revoked 
        //even further down below..

        //Local Variables - BEGIN
        std::shared_ptr<CPollElem> poll;
        PollElemFlags flags;
        std::vector<uint8_t> winnerID;
        BigInt winningScore = 0;
        std::vector<uint8_t> perspective;
        uint64_t cost = 0;
    

        //Local Variables - END

        //now we check each and every 'votable' permission for a corresponding active poll.

        //override ACL/ownership-based assets' spendings privilege access - BEGIN
        poll = pollExtElem->getPollWithLocalID(eSystemPollID::spendingsAcessRightsPoll);
        if (reqPerm.getIsSpending())
        {
            flags = poll->getFlags();

            if (flags.getIsActive())//an inactive poll is oblivious to our processing.
            {
                //check if a winner was elected
                if (poll->checkTrigger(sdm, winnerID, winningScore))
                {
                    poll->firePostTriggerActions();
                    //someone just gained 'assets-spendings privilege' access as a result of voting..

                    //now, check if the entity gaining access is the one who actually won..

                    if (tools->compareByteVectors(accessToken->getEntityID(), winnerID))
                    {
                        //Process Triggers - BEGIN
                       
                          //apply the privilege through a dedicated ACL entry.

                        ace = getACEforEntity(accessToken->getEntityID());// does an ACE entry already exist for this agent?
                        if (ace)
                        {
                            ace->grantAssets(true);
                        }
                        else
                        {//if not then create one.
                            ace = std::make_shared<CAccessEntry>(accessToken->getEntityID());//use default permissions.
                            ace->grantAssets(true);//grant the extra ones.
                           const_cast<bool&>(updateSecDesc) = true;//IMPORTANT: the security descriptor NEEDS to be updated (in cold storage) if the results are
                           //taken into account during further processing (i.e. if results are exercised outside of this function ex. assets truly being spent).
                        }
                        const_cast<bool&>(updateSecDesc) = true;
                        //Process Triggers - END

                    }
                    else
                    {
                        toRet->setCanSpend(false);
                        spendingForceAbort = true;
                    }
                }
                else
                {//poll-triggering conditions were not met.
                    toRet->setCanSpend(false);
                    spendingForceAbort = true;
                }
                //NOTICE: we return immediately if requested permission was not agreed upon by the corresponding active poll.
                //In such a case processing results of consecutive polls do NOT matter.
                //That enforces caller to request the smallest set of required permissions.

            }
        }

        //override ACL/ownership-based  assets' spendings privilege access - END

        //override ACL/ownership-based write access - BEGIN
        poll = pollExtElem->getPollWithLocalID(eSystemPollID::writeAccessRightsPoll);
        if (poll && reqPerm.getIsWrite())
        {
            flags = poll->getFlags();

            if ( flags.getIsActive())//an inactive poll is oblivious to our processing.
            {
                //check if a winner was elected
                if (poll->checkTrigger(sdm, winnerID, winningScore))
                {
                    poll->firePostTriggerActions();
                    //someone just gained 'write' access as a result of voting..

                    //now, check if the entity gaining access is the one who actually won..

                    if (tools->compareByteVectors(accessToken->getEntityID(), winnerID))
                    {
                        //Process Triggers - BEGIN

                          //apply the privilege through a dedicated ACL entry.

                        ace = getACEforEntity(accessToken->getEntityID());// does an ACE entry already exist for this agent?
                        if (ace)
                        {
                            ace->grantAssets(true);
                        }
                        else
                        {//if not then create one.
                            ace = std::make_shared<CAccessEntry>(accessToken->getEntityID());//use default permissions.
                            ace->grantWrite(true);//grant the extra ones.
                            const_cast<bool&>(updateSecDesc) = true;//IMPORTANT: the security descriptor NEEDS to be updated (in cold storage) if the results are
                            //taken into account during further processing (i.e. if results are exercised outside of this function ex. assets truly being spent).
                        }
                        const_cast<bool&>(updateSecDesc) = true;
                        //Process Triggers - END
                    }
                    else
                    {
                        toRet->setCanWrite(false);
                        writeForceAbort = true;
                    }
                }
                else
                {//poll-triggering conditions were not met.
                    toRet->setCanWrite(false);
                    writeForceAbort = true;
                }
                //NOTICE: we return immediately if requested permission was not agreed upon by the corresponding active poll.
                //In such a case processing results of consecutive polls do NOT matter.
                //That enforces caller to request the smallest set of required permissions.

            }
        }

        //override ACL/ownership-based write access - END

        //override ownership-change access - BEGIN
        poll = pollExtElem->getPollWithLocalID(eSystemPollID::ownershipChangeAccessRightsPoll);
        if (poll && reqPerm.getIsOwnerChange())
        {
            flags = poll->getFlags();

            if (flags.getIsActive())//an inactive poll is oblivious to our processing.
            {
                //check if a winner was elected
                if (poll->checkTrigger(sdm, winnerID, winningScore))
                {
                    poll->firePostTriggerActions();
                    //someone just gained 'ownership-change' access as a result of voting..

                    //now, check if the entity gaining access is the one who actually won..

                    if (tools->compareByteVectors(accessToken->getEntityID(), winnerID))
                    {
                        //Process Triggers - BEGIN

                          //proclaim winner as owner through  a dedicated entry within the security descriptor.

                            mOwner = winnerID;
                            const_cast<bool&>(updateSecDesc) = true;//IMPORTANT: the security descriptor NEEDS to be updated (in cold storage) if the results are
                            //taken into account during further processing (i.e. if results are exercised outside of this function ex. assets truly being spent).
                       
                            const_cast<bool&>(updateSecDesc) = true;
                        //Process Triggers - END
                    }
                    else
                    {
                        toRet->setIsOwner(false); //todo: introduce a flag which (when set) would not cause the current ownership-relationship being neglected.
                        ownershipForceAbort = true;
                    }
                }
                else
                {//poll-triggering conditions were not met.
                    toRet->setIsOwner(false);
                    ownershipForceAbort = true;
                }
                //NOTICE: we return immediately if requested permission was not agreed upon by the corresponding active poll.
                //In such a case processing results of consecutive polls do NOT matter.
                //That enforces caller to request the smallest set of required permissions.

            }
        }
        //override ownership-change access - END

        //override execution access - BEGIN
        poll = pollExtElem->getPollWithLocalID(eSystemPollID::executeAccessRightsPoll);
        if (poll && reqPerm.getIsExec())
        {
            flags = poll->getFlags();

            if ( flags.getIsActive())//an inactive poll is oblivious to our processing.
            {
                //check if a winner was elected
                if (poll->checkTrigger(sdm, winnerID, winningScore))
                {
                    poll->firePostTriggerActions();
                    //someone just gained 'execution access' access as a result of voting..

                    //now, check if the entity gaining access is the one who actually won..

                    if (tools->compareByteVectors(accessToken->getEntityID(), winnerID))
                    {
                        //Process Triggers - BEGIN

                        //apply the privilege through a dedicated ACL entry.

                        ace = getACEforEntity(accessToken->getEntityID());// does an ACE entry already exist for this agent?
                        if (ace)
                        {
                            ace->grantAssets(true);
                        }
                        else
                        {//if not then create one.
                            ace = std::make_shared<CAccessEntry>(accessToken->getEntityID());//use default permissions.
                            ace->grantExecute(true);//grant the extra ones.
                            const_cast<bool&>(updateSecDesc) = true;//IMPORTANT: the security descriptor NEEDS to be updated (in cold storage) if the results are
                            //taken into account during further processing (i.e. if results are exercised outside of this function ex. assets truly being spent).
                        }
                        const_cast<bool&>(updateSecDesc) = true;
                        //Process Triggers - END
                    }
                    else
                    {
                        toRet->setCanExecute(false);
                        executeForceAbort = true;
                    }
                }
                else
                {//poll-triggering conditions were not met.
                    toRet->setCanExecute(false);
                    executeForceAbort = true;
                }
                //NOTICE: we return immediately if requested permission was not agreed upon by the corresponding active poll.
                //In such a case processing results of consecutive polls do NOT matter.
                //That enforces caller to request the smallest set of required permissions.

            }
        }
        //override execution access - END

        //override object-removal access - BEGIN
        poll = pollExtElem->getPollWithLocalID(eSystemPollID::removalAccessRightsPoll);
        if (poll && reqPerm.getIsRemoval())
        {
            flags = poll->getFlags();

            if (flags.getIsActive())//an inactive poll is oblivious to our processing.
            {
                //check if a winner was elected
                if (poll->checkTrigger(sdm, winnerID, winningScore))
                {
                    poll->firePostTriggerActions();
                    //someone just gained 'execution access' access as a result of voting..

                    //now, check if the entity gaining access is the one who actually won..

                    if (tools->compareByteVectors(accessToken->getEntityID(), winnerID))
                    {
                        //Process Triggers - BEGIN

                          //apply the privilege through a dedicated ACL entry.

                        ace = getACEforEntity(accessToken->getEntityID());// does an ACE entry already exist for this agent?
                        if (ace)
                        {
                            ace->grantRemoval(true);
                        }
                        else
                        {//if not then create one.
                            ace = std::make_shared<CAccessEntry>(accessToken->getEntityID());//use default permissions.
                            ace->grantAssets(true);//grant the extra ones.
                            const_cast<bool&>(updateSecDesc) = true;//IMPORTANT: the security descriptor NEEDS to be updated (in cold storage) if the results are
                            //taken into account during further processing (i.e. if results are exercised outside of this function ex. assets truly being spent).
                        }
                        const_cast<bool&>(updateSecDesc) = true;
                        //Process Triggers - END
                    }
                    else
                    {
                        toRet->setCanRemove(false);
                        removalForceAbort = true;
                    }
                }
                else
                {//poll-triggering conditions were not met.
                    toRet->setCanRemove(false);
                    removalForceAbort = true;
                }
                //NOTICE: we return immediately if requested permission was not agreed upon by the corresponding active poll.
                //In such a case processing results of consecutive polls do NOT matter.
                //That enforces caller to request the smallest set of required permissions.

            }
        }
        //override object-removal access - END


    }
    //Polling Extension - END

    //Ownership Extension Support - BEGIN

    if (!ownershipForceAbort)
    {
        
        if (mOwner.size())//owner can do anything.
        { //explicit owner has PRIORITY.
            if (CTools::getInstance()->compareByteVectors(accessToken->getEntityID(), mOwner) ||
                CTools::getInstance()->compareByteVectors(accessToken->getEntityID(), friendlyID))
            {
                return std::make_shared<CEffectiveRights>(true, true, true, true, true, true);
            }
        }
        else {
            //otherwise, validate implicit ownership.
            if (mImplicitOwner.size() && tools->compareByteVectors(accessToken->getEntityID(), mImplicitOwner))
            {
                return std::make_shared<CEffectiveRights>(true, true, true, true, true, true);
            }
        }

    }
    //Ownership Extension Support - END

    //CRT Flags - BEGIN 
    if (!readForceAbort)
    {
        if (mFlags.getEveryoneRead())
        {
            toRet->setCanRead(true);
        }
    }

    if (!writeForceAbort)
    {
        if (mFlags.getEveryoneWrite())
        {
            toRet->setCanWrite(true);
        }
    }
    if (!executeForceAbort)
    {
        if (mFlags.getEveryoneExecute())
        {
            toRet->setCanExecute(true);
        }
    }
    //consider: everyoneCanRemove
    //CRT Flags - END 

    //Support of explicit ACL entries - BEGIN
    //the below entries may only EXTEND upon what is granted through the default (POSIX-style permissions).



    if (ace)
    {
        if (!readForceAbort && ace->getAccessFlags().getRead())
        {
            toRet->setCanRead(true);
        }
        if (!executeForceAbort && ace->getAccessFlags().getExecute())
        {
            toRet->setCanExecute(true);
        }
        if (!writeForceAbort && ace->getAccessFlags().getWrite())
        {
            toRet->setCanWrite(true);
        }
        if (!removalForceAbort && ace->getAccessFlags().getRemoval())
        {
            toRet->setCanRemove(true);
        }
        if (!spendingForceAbort && ace->getAccessFlags().getAssets())
        {
            toRet->setCanSpend(true);
        }
        if (!votingForceAbort && ace->getAccessFlags().getVoting())
        {
            toRet->setCanVote(true);
        }
    }


    //Support of explicit ACL entries - END

    return toRet;
}
std::shared_ptr<CAccessEntry> CSecDescriptor::getACEforEntity(const std::vector<uint8_t> &sdID)
{
    std::shared_ptr<CAccessEntry> ae;
    std::shared_ptr<CTools> tools = CTools::getInstance();

    for (uint64_t i = 0; i < mAccessEntries.size(); i++)
    {
        if (tools->compareByteVectors(mAccessEntries[i]->getSDID(), sdID))
        {
            return mAccessEntries[i];
        }
    }
    return nullptr;
}

std::shared_ptr<CAccessEntry> CSecDescriptor::getACEforEntityEx(const std::vector<uint8_t>& sdID)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return getACEforEntity(sdID);
}
/// <summary>
/// UTMOST IMPORTANCE: always call verifySecurityTokenEx() whenever object changes BEFORE calling getIsAllowed(). That is because the generated Access Token
/// is ONLY valid in the scope of an object passed to verifySecurityTokenEx(); (for example the mIsOwner flag is set. Re-factor: remove any state-flags from access token).
/// Important: remember to update the security descriptor if function sets updateSecDesc.
/// Important 2: getIsAllowed() is no good on the grounds of its own for validating effective permissions to an object.
///              That is because it does NOT take into account execution scope. It makes decisions based on the explicit ACE entries.
///              ex. It wouldn't grant write permissions to a file when executing from a terminal. For this use verifySecurityTokenEx().
///              verifySecurityTokenEx() takes an accessToken which MAY be provided with the set of requested permissions through setRequestedPermissions().
/// </summary>
/// <param name="accessToken"></param>
/// <param name="updateSecDesc"></param>
/// <param name="write"></param>
/// <param name="execute"></param>
/// <param name="read"></param>
/// <param name="chown"></param>
/// <param name="remove"></param>
/// <returns></returns>
bool CSecDescriptor::getIsAllowed(std::shared_ptr<CAccessToken> accessToken, std::shared_ptr<CStateDomainManager> sdm, bool &updateSecDesc)
{



    /* !!! --v
             Extremely Important: parameter 'accessToken' can ONLY by the token returned from previous invocation of verifySecurityTokenEx().
             That is because only such token is guaranteed to contain a validated and authenticated caller's identity (and there are multiple authentication methods0.
       !!! --^
    */

    //Brief: we compute effective permissions, then we compare effective permissions with the set of requested ones.
    //We perform a binary multiplication between the two binary vectors and return false if there's no exact match.
    //In fact we do not do binary operations so not to rely on the underlying implementation (way slower but it doesn't hurt).

    //Operational Logic - BEGIN
    RequestedPermissions reqPerm = RequestedPermissions(true,true,true,false,false,false); //gen defaults

    if (accessToken)
    {//if an explicit set of permissions is being requested.
        reqPerm = accessToken->getRequestedPermissions();

    }
    std::shared_ptr <CEffectiveRights> effective = getEffectiveRights(accessToken, sdm, updateSecDesc);//handles both System and Overwatch privilages (as well).

    if (!effective)
        return false;// abort (should never happen).

    //check if ALL requested permissions are satisfied - BEGIN
    //substitute of a binary AND operation between the requested and effective permissions.


   //EXTREME ATTENTION - the below checks need to account for ALL permissions as we do a default accept.
   if (reqPerm.getIsExec() && !effective->getCanExecute())
       return false;
   if (reqPerm.getIsRead() && !effective->getCanRead())
       return false;
   if (reqPerm.getIsRemoval() && !effective->getCanRemove())
       return false;
   if (reqPerm.getIsSpending() && !effective->getCanSpend())
       return false;
   if (reqPerm.getIsWrite() && !effective->getCanWrite())
       return false;
   if (reqPerm.getIsOwnerChange() && !effective->getIsOwner())
       return false;

   //Voting - BEGIN
   if (reqPerm.getIsVoting())
   {
      
   }
   //Voting - END

    //check if ALL requested permissions are satisfied - END

   return true;
   //Operational Logic - FALSE

}

//do NOT use directly


/// <summary>
/// Returns an Access Control Entry regarding State-Domain described by the provided State-Domain-ID (sdID).
/// </summary>
/// <param name="sdID"></param>
/// <returns></returns>
std::shared_ptr<CAccessEntry> CSecDescriptor::getAccessEntry(std::vector<uint8_t> sdID)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    if (sdID.size() == 0)
        return nullptr;

    std::shared_ptr<CTools> tools = CTools::getTools();
    for (uint64_t i = 0; i < mAccessEntries.size(); i++)
    {
        if (tools->compareByteVectors(sdID, mAccessEntries[i]->getSDID()))
        {
            return mAccessEntries[i];
        }
    }
    return nullptr;

}
bool CSecDescriptor::getIsSysOnly()
{
    std::lock_guard <std::mutex> lock(mGuardian);
    return mFlags.getIsSysOnly();
}
void CSecDescriptor::setIsSysOnly(bool isIt)
{
    std::lock_guard <std::mutex> lock(mGuardian);
    mFlags.setSysOnly(isIt);
     if (isIt)
         mAccessEntries.clear();
}

CRTFlags& CRTFlags::operator=(const CRTFlags& t)
{
    std::memcpy(this, &t, sizeof(CRTFlags));
    return *this;
}

bool CRTFlags::operator==(const CRTFlags& t)
{
    return getNr() == t.getNr();
}
bool CRTFlags::operator!=(const CRTFlags& t)
{
    return !(getNr() == t.getNr());
}

void CRTFlags::clear()
{
    std::memset(this, 0, sizeof(CRTFlags));
}
void CRTFlags::setDefaults()
{
    std::memset(this, 0, sizeof(CRTFlags));
    mEveryoneExecute = true;
    mEveryoneRead = true;
}


CRTFlags::CRTFlags(const CRTFlags& sibling)
{
    std::memcpy(this, &sibling, sizeof(CRTFlags));
}

CRTFlags::CRTFlags()
{
    clear();
}

CRTFlags::CRTFlags(uint8_t byte)
{
    std::memcpy(this, &byte, sizeof(CRTFlags));
}

void CRTFlags::setSysOnly(bool isIt)
{
    mSysOnly = isIt;
}

bool CRTFlags::getIsSysOnly()
{
    return mSysOnly;
}

void CRTFlags::setHasACL(bool hasIt)
{
    mHasACL = hasIt;
}

bool CRTFlags::getHasACL()
{
    return mHasACL;
}

void CRTFlags::setEveryoneWrite(bool hasIt)
{
    mEveryoneWrite = hasIt;
}

bool CRTFlags::getEveryoneWrite()
{
    return mEveryoneWrite;
}

void CRTFlags::setEveryoneRead(bool hasIt)
{
    mEveryoneRead = hasIt;
}

bool CRTFlags::getEveryoneRead()
{
    return mEveryoneRead;
}

void CRTFlags::setEveryoneExecute(bool hasIt)
{
    mEveryoneExecute = hasIt;
}

bool CRTFlags::getEveryoneExecute()
{
    return mEveryoneExecute;
}



 uint64_t CRTFlags::getNr() const
{
    uint8_t nr = 0;
    std::memcpy(&nr, this, 1);
    return static_cast<uint64_t>(nr);
}


