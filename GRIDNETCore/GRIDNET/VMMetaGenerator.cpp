#include "VMMetaGenerator.h"
#include "VMMetaGenerator.h"
#include "QRIntent.h"
#include "TokenPool.h"
#include "SearchResults.hpp"
#include "Receipt.h"
#include "BlockHeader.h"
#include "BlockDesc.hpp"
#include "TransactionDesc.hpp"
#include "DomainDesc.hpp"
std::mutex CVMMetaGenerator::sGuardian;
uint64_t CVMMetaGenerator::sRecentReqID = 0;

CVMMetaGenerator::CVMMetaGenerator()
{
    mInSection = false;
    mVersion = 1;
    mSectionsVersion = 1;
    mSectionsCount = 0;
    mTotalEntriesCount = 0;
   mFinalized = false;
   mCurrentSectionType = eVMMetaSectionType::unknown;
}

bool CVMMetaGenerator::finalize()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (mFinalized || mSectionsCount==0 || mInSection)
        return false;
    mEncoder.end_cons();//sections container
    mFinalized = true;
    return true;
}

/// <summary>
/// Single meta-data package can contain multiple sections.
//  Section CAN contain multiple compatible entries.
/// </summary>
/// <param name="sType"></param>
/// <returns></returns>
bool CVMMetaGenerator::beginSection(eVMMetaSectionType::eVMMetaSectionType sType, std::vector<uint8_t> metaData)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (getInSection())
        endSection();// to support cases in which caller does not know there's a section ongoing; doesn't hurt to end it here

    if (mSectionsCount == 0)
    {
        //META-HEADER BEGIN
        mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);//sections-wrapper-main,top-most sequence
        mEncoder.encode(mVersion);//meta-generator version used to encode VM-meta-data
        //META-HEADER END
    }
    mSectionsCount++;
    setInSection();
    setCurrentSectionType(sType);

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(mSectionsVersion);//header
    mEncoder.encode(static_cast<size_t>(mCurrentSectionType));
    mEncoder.encode(metaData, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);

    return true;
}

/// <summary>
/// Ends a meta-data Section.
/// </summary>
/// <returns></returns>
bool CVMMetaGenerator::endSection()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!mInSection)
        return false;

    mEncoder.end_cons().end_cons();
    mInSection = false;
    return true;
}

bool CVMMetaGenerator::addRAWGridScriptCmd(std::string cmd, eVMMetaCodeExecutionMode::eVMMetaCodeExecutionMode mode, uint64_t reqID,  uint64_t windowID, uint64_t appID, std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::requests)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::GridScriptCode));
    mEncoder.encode(static_cast<size_t>(reqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(mode));
    mEncoder.encode(static_cast<size_t>(windowID));
    mEncoder.encode(mTools->stringToBytes(cmd), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


bool CVMMetaGenerator::addGridScriptResult(eVMMetaProcessingResult::eVMMetaProcessingResult status, std::vector<uint8_t> BERMetaData, uint64_t inRespToReqID, std::string txt,  uint64_t appID,  std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::notifications)
        return false;
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::GridScriptCodeResponse));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(status));
    mEncoder.encode(mTools->stringToBytes(txt), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(BERMetaData, Botan::ASN1_Tag::OCTET_STRING);//[todo:vega4]:update web-ui to handle
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


bool CVMMetaGenerator::addTerminalData(std::vector<uint8_t> data1, std::vector<uint8_t> data2, eTerminalDataType::eTerminalDataType dType, uint64_t inRespToReqID, uint64_t appID , std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    std::vector<uint8_t> t;

    switch (dType)
    {
    case eTerminalDataType::input:
        t.push_back(0);
        break;
    case eTerminalDataType::output:
        t.push_back(1);
        break;
    case eTerminalDataType::windowDimensions:
        t.push_back(2);
        break;
    default:
        break;
    }

    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::notifications)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::terminalData));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(t, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(data1, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(data2, Botan::ASN1_Tag::OCTET_STRING);///when sending  window-dimensions these two vectors contain numbers; the width and height respectively
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}

bool CVMMetaGenerator::addAppData(std::vector<uint8_t> data1, std::vector<uint8_t> data2, uint64_t inRespToReqID, uint64_t appID , std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    std::vector<uint8_t> t;

    /*switch (dType)
    {
    case eTerminalDataType::input:
        t.push_back(0);
        break;
    case eTerminalDataType::output:
        t.push_back(1);
        break;
    case eTerminalDataType::windowDimensions:
        t.push_back(2);
        break;
    default:
        break;
    }*/
    t.push_back(0);

    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::notifications)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::appNotification));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(t, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(data1, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(data2, Botan::ASN1_Tag::OCTET_STRING);///when sending  window-dimensions these two vectors contain numbers; the width and height respectively
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


bool CVMMetaGenerator::addFileListingEntry(std::string fileName, uint64_t timestamp, std::string accessRights, eDFSElementType::eDFSElementType eType, uint64_t inRespToReqID, uint64_t appID , std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::directoryListing)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eType));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(mTools->stringToBytes(fileName), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(mTools->stringToBytes(accessRights), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(timestamp);
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}
// Statistics - BEGIN
bool CVMMetaGenerator::addTXProofEntry(std::shared_ptr<CReceipt> receipt, std::shared_ptr<CTransaction> tx, std::shared_ptr<CBlockHeader> header, const std::string& address, const BigSInt& value, uint64_t inRespToReqID, uint64_t appID, std::vector<uint8_t> vmID)
{
    if (!receipt ||( receipt->getReceiptType() == eReceiptType::transaction && !tx))
    {
        return false;
    }
    // todo: make sure the mobile app supports cases when either receipt or tx are unknown (block/genesis rewards)
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);

    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;
    if (!header)
        return false;

    std::vector<uint8_t> packedHeader, packedReceipt, addressB, tranasctionB;

    if (receipt)
    {
        packedReceipt = receipt->getPackedData();
    }
    if (!header->getPackedData(packedHeader))
    {
        return false;
    }

    if (tx)
    {
        tranasctionB = tx->getPackedData();
        addressB = mTools->stringToBytes(address);
    }
    else
    {
        addressB = mTools->stringToBytes("Reward");
    }

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::txProof));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(packedReceipt, Botan::ASN1_Tag::OCTET_STRING); // 0 - Receipt
    mEncoder.encode(tranasctionB, Botan::ASN1_Tag::OCTET_STRING); // 1 - TX
    mEncoder.encode(packedHeader, Botan::ASN1_Tag::OCTET_STRING);// 2 - Block Header
    mEncoder.encode(addressB, Botan::ASN1_Tag::OCTET_STRING); // 3 - other peer's wallet address
    mEncoder.encode(mTools->BigSIntToBytes(value), Botan::ASN1_Tag::OCTET_STRING); // 4 - amount of associated value transfer (if any)

    mEncoder.encode(std::vector<uint8_t>(), Botan::ASN1_Tag::OCTET_STRING);// 5 - Reserved for Merkle Proof
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}

bool CVMMetaGenerator::addSearchResults(std::shared_ptr<CSearchResults> results, uint64_t inRespToReqID, uint64_t appID, std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);

    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;
    if (!results)
        return false;

    std::vector<uint8_t> packedResults;

    if (!results->getPackedData(packedResults))
    {
        return false;
    }

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::searchResults));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);

    // Main Payload - BEGIN
    mEncoder.encode(packedResults, Botan::ASN1_Tag::OCTET_STRING); // 0 - Receipt
    // Main Payload - END

    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}

bool CVMMetaGenerator::addLivenessInfo(eLivenessState::eLivenessState livenessState, uint64_t inRespToReqID, uint64_t appID,  std::vector<uint8_t>& vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);
    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::livenessInfo));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    // Main Payload - BEGIN
    mEncoder.encode(static_cast<uint64_t>(livenessState));
    // Main Payload - END
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}
bool CVMMetaGenerator::addBlockchainHeight(const uint64_t height, const uint64_t keyHeight, const uint64_t inRespToReqID, const uint64_t appID,  const std::vector<uint8_t>& vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);
    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::blockchainHeight));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    // Main Payload - BEGIN
    mEncoder.encode(height);
    mEncoder.encode(keyHeight);
    // Main Payload - END
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}

bool CVMMetaGenerator::addUSDTPrice(const BigInt& usdtPrice, const uint64_t inRespToReqID, const  uint64_t appID, const std::vector<uint8_t>& vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);
    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;

    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::vector<uint8_t> serializedPrice = tools->BigIntToBytes(usdtPrice);

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::usdtPrice));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    // Main Payload - BEGIN
    mEncoder.encode(serializedPrice, Botan::ASN1_Tag::OCTET_STRING);
    // Main Payload - END
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}
// Statistics - END


// In CVMMetaGenerator.cpp
bool CVMMetaGenerator::addTransactionInfo(const std::shared_ptr<CTransactionDesc>& transactionDesc,
    const uint64_t inRespToReqID,
    const uint64_t appID,
    const std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);
    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;
    if (!transactionDesc)
        return false;

    std::vector<uint8_t> packedTransaction;
    if (!transactionDesc->getPackedData(packedTransaction))
    {
        return false;
    }

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::transactionDetails));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    // Main Payload - BEGIN
    mEncoder.encode(packedTransaction, Botan::ASN1_Tag::OCTET_STRING);
    // Main Payload - END
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


bool CVMMetaGenerator::addMarketDepth(const BigInt& totalMarketCap,
    const uint64_t inRespToReqID,
    const uint64_t appID,
    const std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);
    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;

    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::vector<uint8_t> serializedMarketCap = tools->BigIntToBytes(totalMarketCap);

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::marketDepth));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    // Main Payload - BEGIN
    mEncoder.encode(serializedMarketCap, Botan::ASN1_Tag::OCTET_STRING);
    // Main Payload - END
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}

// In CVMMetaGenerator.cpp
bool CVMMetaGenerator::addTransactionDailyStats(const std::vector<uint64_t>& dates,
    const std::vector<uint64_t>& counts,
    const uint64_t inRespToReqID,
    const uint64_t appID,
    const std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);
    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;

    if (dates.size() != counts.size())
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::transactionDailyStats));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    // Main Payload - BEGIN
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    for (const auto& date : dates)
    {
        mEncoder.encode(static_cast<size_t>(date));
    }
    mEncoder.end_cons();
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    for (const auto& count : counts)
    {
        mEncoder.encode(static_cast<size_t>(count));
    }
    mEncoder.end_cons();
    // Main Payload - END
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


// In CVMMetaGenerator.cpp
bool CVMMetaGenerator::addBlockInfo(const std::shared_ptr<CBlockDesc>& blockDesc,
    const uint64_t inRespToReqID,
    const uint64_t appID,
    const std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);
    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;
    if (!blockDesc)
        return false;

    std::vector<uint8_t> packedBlock;
    if (!blockDesc->getPackedData(packedBlock))
    {
        return false;
    }

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::blockDetails));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    // Main Payload - BEGIN
    mEncoder.encode(packedBlock, Botan::ASN1_Tag::OCTET_STRING);
    // Main Payload - END
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}

bool CVMMetaGenerator::addBlockchainStatus(const BigInt& totalSupply,
    const uint64_t height,
    const uint64_t keyHeight,
    const uint64_t inRespToReqID,
    const uint64_t appID,
    const std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);
    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;

    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::vector<uint8_t> serializedTotalSupply = tools->BigIntToBytes(totalSupply);

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::blockchainStatus));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    // Main Payload - BEGIN
    mEncoder.encode(serializedTotalSupply, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(height);
    mEncoder.encode(keyHeight);
    // Main Payload - END
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


bool CVMMetaGenerator::addDomainInfo(const std::shared_ptr<CDomainDesc>& domainDesc,
    const uint64_t inRespToReqID,
    const uint64_t appID,
    const std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::statistics);
    if (mCurrentSectionType != eVMMetaSectionType::statistics)
        return false;
    if (!domainDesc)
        return false;

    std::vector<uint8_t> packedDomain;
    if (!domainDesc->getPackedData(packedDomain))
    {
        return false;
    }

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::domainDetails));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    // Main Payload - BEGIN
    mEncoder.encode(packedDomain, Botan::ASN1_Tag::OCTET_STRING);
    // Main Payload - END
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}
bool CVMMetaGenerator::addFileContent(std::string fileName, const std::vector<uint8_t> &data,  eDataType::eDataType dType, uint64_t inRespToReqID, uint64_t appID , std::vector<uint8_t> vmID, std::vector<uint8_t> NFTMeta)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::fileContents);
    if (mCurrentSectionType != eVMMetaSectionType::fileContents)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eDFSElementType::eDFSElementType::fileContent));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(dType));//protocol specific entry. note that DFS has additionally VM MetaData specific sections and Elements!
    mEncoder.encode(mTools->stringToBytes(fileName), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(data, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(NFTMeta, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


bool CVMMetaGenerator::addStateLessChannelElement(std::string name, eStateLessChannelsElementType::eStateLessChannelsElementType eType, std::vector<uint8_t> serializedData, uint64_t inRespToReqID, uint64_t appID , std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::stateLessChannels)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::StateLessChannelsElement));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));//protocol depicted
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eType));//protocol specific entry
    mEncoder.encode(mTools->stringToBytes(name), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(serializedData, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


bool CVMMetaGenerator::addNotification(eNotificationType::eNotificationType eType, std::string title, std::string msg, uint64_t inRespToReqID, uint64_t appID , std::vector<uint8_t> vmID)
{
    return addNotification(eType, title, mTools->stringToBytes(msg), appID);
}

bool CVMMetaGenerator::addConsensusTaskNotification(std::shared_ptr<CConsensusTask> task, uint64_t processID, std::vector<uint8_t> vmID, uint64_t inRespToReqID)
{
    if (!task)
        return false;

    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        beginSection(eVMMetaSectionType::notifications);

    if (mCurrentSectionType != eVMMetaSectionType::notifications)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::consensusTask));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(processID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(task->getPackedData(), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


bool CVMMetaGenerator::addNotification(eNotificationType::eNotificationType eType, std::string title, std::vector<uint8_t> data, uint64_t inRespToReqID, uint64_t appID , std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::notifications)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::notification));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eType));//notificaiton specific field
    mEncoder.encode(title.size() > 0 ? mTools->stringToBytes(title) : std::vector<uint8_t>(), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(data, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}


/// <summary>
/// The data response emchanics are data-type agnostic.
/// The requestor needs to keep track of requests IDs and associate these with responses. That's by design. Thus requsts IDs need to be uniqeue.
/// </summary>
/// <param name="reqID"></param>
/// <param name="dataFields"></param>
/// <param name="appID"></param>
/// <returns></returns>
uint64_t CVMMetaGenerator::addDataResponse(const std::vector <eDataType::eDataType>& dataTypes, const std::vector <std::vector<uint8_t>>& dataFields, uint64_t reqID, uint64_t appID , const std::vector<uint8_t>& vmID )
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (!getInSection())
        beginSection(eVMMetaSectionType::notifications);

    if (mCurrentSectionType != eVMMetaSectionType::notifications)
        return false;


    if (encodeTypedRAWEntry(eVMMetaEntryType::dataResponse, dataTypes, dataFields, reqID, appID))
    {
        mTotalEntriesCount++;
        return reqID;
    }

    return 0;
}



/// <summary>
/// Requests IDs are generated and kept track of by the Generator.
/// </summary>
/// <param name="eType"></param>
/// <param name="title"></param>
/// <param name="msg"></param>
/// <param name="defaultValue"></param>
/// <param name="expiresInSec"></param>
/// <param name="appID"></param>
/// <returns></returns>
uint64_t CVMMetaGenerator::addDataRequest(eDataRequestType::eDataRequestType eType, std::string title, std::string msg, std::vector<uint8_t> defaultValue, uint64_t expiresInSec, uint64_t appID , std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mTotalEntriesCount++;
    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::requests)
        return false;

    uint64_t reqID = genReqID();
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::dataRequest));
    mEncoder.encode(static_cast<size_t>(reqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    /*Request cannot be formulated in a response to another request.
    Thus:
   + in case of non-request entries the above field represents ID of a request which we're fulfilling right now.
   + in case of a request-field -> the above represent Request-ID
    */
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eType));//request specific field
    mEncoder.encode(defaultValue, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(static_cast<size_t>(expiresInSec));
    mEncoder.encode(mTools->stringToBytes(title), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(mTools->stringToBytes(msg), Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.end_cons();
    mEncoder.end_cons();

    return reqID;
}

uint64_t CVMMetaGenerator::addGenTokenPoolRequest(std::shared_ptr<CTokenPool> tp, uint64_t appID, uint64_t reqID, std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    
   
    if (tp == nullptr)
        return false;

    //Preludium - BEGIN

    bool endSectionL = false;
    bool createSection = false;


    if (!getInSection())
    {
        createSection = true;
    }
    else
    {
        if (mCurrentSectionType != eVMMetaSectionType::requests)
        {
            createSection = true;
            endSectionL = true;
        }
    }

    if (endSectionL)
    {
        endSection();
    }
    if (createSection)
    {
        beginSection(eVMMetaSectionType::requests);
    }

    //Preludium - END

    
    if(reqID==0)
    reqID = genReqID();

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::dataRequest));
    mEncoder.encode(static_cast<size_t>(reqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    /*Request cannot be formulated in a response to another request.
    Thus:
   + in case of non-request entries the above field represents ID of a request which we're fulfilling right now.
   + in case of a request-field -> the above represent Request-ID
    */
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eDataRequestType::tokenPoolGeneration));//request specific field
    mEncoder.encode(tp->getPackedData(), Botan::ASN1_Tag::OCTET_STRING); //default value field re-used
    mEncoder.encode(static_cast<size_t>(0));////expires-in field
    mEncoder.encode("", Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode("", Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return reqID;
}

bool CVMMetaGenerator::encodeRAWEntry(eVMMetaEntryType::eVMMetaEntryType eType, std::vector <std::vector<uint8_t>> dataFields, uint64_t inRespToReqID, uint64_t appID , std::vector<uint8_t> vmID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (!getInSection())
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eType));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);

    for (int i = 0; i < dataFields.size(); i++)
    {
        mEncoder.encode(dataFields[i], Botan::ASN1_Tag::OCTET_STRING);
    }

    mEncoder.end_cons();
    mEncoder.end_cons();

    return true;
}


bool CVMMetaGenerator::encodeTypedRAWEntry(eVMMetaEntryType::eVMMetaEntryType eType, std::vector <eDataType::eDataType> basicDataTypes,  std::vector <std::vector<uint8_t>> dataFields, uint64_t inRespToReqID, uint64_t appID, std::vector<uint8_t> vmID)
{

    if (basicDataTypes.size() != dataFields.size())
        return false;

    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (!getInSection())
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eType));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(appID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);

    for (int i = 0; i < dataFields.size(); i++)
    {
        mEncoder.encode(static_cast<size_t>(basicDataTypes[i]));
        mEncoder.encode(dataFields[i], Botan::ASN1_Tag::OCTET_STRING);
    }

    mEncoder.end_cons();
    mEncoder.end_cons();

    return true;
}


uint64_t CVMMetaGenerator::getTotalEntriesCount()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mTotalEntriesCount;
}




bool CVMMetaGenerator::addVMStatus(eVMStatus::eVMStatus status, SE::vmFlags &flags, uint64_t processID, std::vector<uint8_t>  vmID, std::vector<uint8_t> terminalID, std::vector<uint8_t> conversationID, uint64_t inRespToReqID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::notifications)
        return false;

    std::vector<uint8_t> flagsBytes(1);
    flagsBytes[0] = reinterpret_cast<uint8_t&>(flags);

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::VMStatus));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(processID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(mTools->getSigBytesFromNumber(status), Botan::ASN1_Tag::OCTET_STRING);//encoded as  bytes to save on storage
    mEncoder.encode(flagsBytes, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(terminalID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(conversationID, Botan::ASN1_Tag::OCTET_STRING);

    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}

bool CVMMetaGenerator::addThreadOperationStatus(eThreadOperationType::eThreadOperationType oType,   std::vector<uint8_t>  vmID, uint64_t processID, std::vector<uint8_t> terminalID, std::vector<uint8_t> conversationID, uint64_t inRespToReqID)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (!getInSection())
        return false;
    if (mCurrentSectionType != eVMMetaSectionType::notifications)
        return false;

    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(static_cast<size_t>(eVMMetaEntryType::threadOperation));
    mEncoder.encode(static_cast<size_t>(inRespToReqID));
    mEncoder.encode(static_cast<size_t>(processID));
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
    mEncoder.encode(mTools->getSigBytesFromNumber(oType), Botan::ASN1_Tag::OCTET_STRING);//encoded as  bytes to save on storage
    mEncoder.encode(vmID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(terminalID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.encode(conversationID, Botan::ASN1_Tag::OCTET_STRING);
    mEncoder.end_cons();
    mEncoder.end_cons();
    mTotalEntriesCount++;
    return true;
}

void CVMMetaGenerator::reset()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mEncoder = Botan::DER_Encoder();
    mInSection = false;
    mCurrentSectionType = eVMMetaSectionType::unknown;
    mSectionsCount = 0;
    mTotalEntriesCount = 0;
    mFinalized = false;
    mData.clear();

}
// Stats - BEGIN


uint64_t CVMMetaGenerator::addDataResponse(const BigInt& value, uint64_t reqID, uint64_t appID, const std::vector<uint8_t>& vmID)
{
    std::vector<eDataType::eDataType> dataTypes = { eDataType::BigInt };
    std::vector<std::vector<uint8_t>> dataFields = { mTools->BigIntToBytes(value) };
    return addDataResponse(dataTypes, dataFields, reqID, appID, vmID);
}

uint64_t CVMMetaGenerator::addDataResponse(uint64_t value, uint64_t reqID, uint64_t appID, const std::vector<uint8_t>& vmID)
{
    std::vector<eDataType::eDataType> dataTypes = { eDataType::unsignedInteger };
    std::vector<uint8_t> bytes = mTools->getSigBytesFromNumber(value);
    std::vector<std::vector<uint8_t>> dataFields = { bytes };
    return addDataResponse(dataTypes, dataFields, reqID, appID, vmID);
}

uint64_t CVMMetaGenerator::addDataResponse(double value, uint64_t reqID, uint64_t appID, const std::vector<uint8_t>& vmID)
{
    std::vector<eDataType::eDataType> dataTypes = { eDataType::doublee };
    std::vector<std::vector<uint8_t>> dataFields = { mTools->doubleToByteVector(value) };
    return addDataResponse(dataTypes, dataFields, reqID, appID, vmID);
}

uint64_t CVMMetaGenerator::addDataResponse(const std::vector<std::string>& values, uint64_t reqID, uint64_t appID, const std::vector<uint8_t>& vmID)
{
    std::vector<eDataType::eDataType> dataTypes(values.size(), eDataType::bytes);
    std::vector<std::vector<uint8_t>> dataFields;
    for (const auto& value : values) {
        dataFields.push_back(mTools->stringToBytes(value));
    }
    return addDataResponse(dataTypes, dataFields, reqID, appID, vmID);
}

uint64_t CVMMetaGenerator::addDataResponse(const std::vector<double>& values, uint64_t reqID, uint64_t appID, const std::vector<uint8_t>& vmID)
{
    std::vector<eDataType::eDataType> dataTypes(values.size(), eDataType::doublee);
    std::vector<std::vector<uint8_t>> dataFields;
    for (const auto& value : values) {
        dataFields.push_back(mTools->doubleToByteVector(value));
    }
    return addDataResponse(dataTypes, dataFields, reqID, appID, vmID);
}

uint64_t CVMMetaGenerator::addDataResponse(const std::vector<BigInt>& values, uint64_t reqID, uint64_t appID, const std::vector<uint8_t>& vmID)
{
    std::vector<eDataType::eDataType> dataTypes(values.size(), eDataType::BigInt);
    std::vector<std::vector<uint8_t>> dataFields;
    for (const auto& value : values) {
        dataFields.push_back(mTools->BigIntToBytes(value));
    }
    return addDataResponse(dataTypes, dataFields, reqID, appID, vmID);
}

uint64_t CVMMetaGenerator::addDataResponse(const std::string& value, uint64_t reqID, uint64_t appID, const std::vector<uint8_t> vmID)
{
    std::vector<eDataType::eDataType> dataTypes = { eDataType::bytes };
    std::vector<std::vector<uint8_t>> dataFields = {mTools->stringToBytes(value)};
    return addDataResponse(dataTypes, dataFields, reqID, appID, vmID);
}

// Stats - END

std::vector<uint8_t> CVMMetaGenerator::getData()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    std::vector<uint8_t> toRet;

    if (mData.size() > 0)
        return mData;

    if(mSectionsCount==0)
        return toRet;

    if (mInSection)
        endSection();

    finalize();

    mData =mEncoder.get_contents_unlocked();//Encoder gets reset after first invocation; need to buffer the result
    toRet = mData;
     return toRet;
}


uint64_t CVMMetaGenerator::genReqID()
{
    std::lock_guard lock(sGuardian);
    return ++sRecentReqID;
}

bool CVMMetaGenerator::getInSection()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mInSection;
}

void CVMMetaGenerator::setInSection(bool inSection)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mInSection = inSection;
}

void CVMMetaGenerator::setCurrentSectionType(eVMMetaSectionType::eVMMetaSectionType sType)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mCurrentSectionType = sType;
}


