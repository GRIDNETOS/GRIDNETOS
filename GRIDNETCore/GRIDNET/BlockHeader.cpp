#pragma once
#include "BlockHeader.h"
#include "SolidStorage.h"
#include "Block.h"
#include "arith_uint256.h"
#include <cmath>
#include "Receipt.h"
#include "transaction.h"
#include "Verifiable.h"
#include "BlockchainManager.h"
#include "enums.h"
#include "StateDomain.h"

/// <summary>
/// Instantiates a Block-Header or also what is called a 'key-block' if the appropriate member field is set.
/// The key-block consists only of a header when serialized. 
/// The reason is to ensure as fast data exchange of these as possible.
/// </summary>
/// <param name="headerBER"></param>
/// <param name="result"></param>
/// <param name="errorInfo"></param>
/// <param name="loadRootsFromSS"></param>
/// <param name="blockchainMode"></param>
/// <returns></returns>
std::shared_ptr<CBlockHeader> CBlockHeader::instantiate(const std::vector<uint8_t>& headerBER, const eBlockHeaderInstantiationResult& result, const std::string& errorInfo, bool loadRootsFromSS, eBlockchainMode::eBlockchainMode blockchainMode)
{

	eBlockHeaderInstantiationResult resultA;
	resultA = eBlockHeaderInstantiationResult::invalidBERData;
	std::vector<uint8_t> temp;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::shared_ptr<CBlockHeader> bh;
	try {
		bh = newBlockHeader(nullptr, blockchainMode, true, resultA);//true for keyBlock is just a stub here; will be set below based on encoded data inside

		if (!bh)
		{
			return nullptr;
		}

		Botan::BER_Decoder dec1 = Botan::BER_Decoder(headerBER);// .decode(bh->mVersion).start_cons(Botan::ASN1_Tag::SEQUENCE);
		Botan::BER_Object obj = dec1.get_next_object();

		bool isKeyBlockHeader = false;
		if (obj.class_tag != Botan::ASN1_Tag::PRIVATE)
			return nullptr;
		if (obj.type_tag == eBERObjectType::eBERObjectType::keyBlockHeader)
			isKeyBlockHeader = true;
		else if (obj.type_tag == eBERObjectType::eBERObjectType::regularBlockHeader)
			isKeyBlockHeader = false;
		else
			return nullptr;//unknown type
		Botan::BER_Decoder dec2 = Botan::BER_Decoder(obj.value);
		dec2.decode(bh->mVersion);
		obj = dec2.get_next_object();

		bh->mIsKeyBlock = isKeyBlockHeader;
		Botan::BER_Decoder dec3 = Botan::BER_Decoder(obj.value);
		switch (bh->mVersion)
		{
		case 1:

			//COMMON FIELDS - BEGIN
			dec3.decode(bh->mHeight);
			dec3.decode(bh->mSolvedAt);
			dec3.decode(bh->mKeyHeight);
			//*WARNING*: KEY-BLOCK-ONLY FIELDS - BEGIN
			if (bh->mIsKeyBlock)
			{
				//these fields would appear only in a key-block as that's the one containing a PoW
				dec3.decode(bh->mNonce);
				dec3.decode(bh->mTarget);
				dec3.decode(bh->mParentKeyBlockID, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(bh->mPublicKey, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mPaidToLeader = tools->BytesToBigInt(temp);
			}

			//*WARNING*: KEY-BLOCK-ONLY FIELDS - END
			dec3.decode(bh->mSignature, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mParentBlockID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mUncleBlocksHash, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mFinalStateRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mExtraData, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mReceiptsRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mVerifiablesRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mNrOfVerifiables);
			dec3.decode(bh->mNrOfReceipts);
			dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			bh->mTotalReward = tools->BytesToBigInt(temp);

			//COMMON FIELDS - END
			//REGULAR BLOCK FIELDS - BEGIN
			if (!bh->mIsKeyBlock)
			{
				//only regular block is the one containing transactions-related data
				dec3.decode(bh->mTransactionsRootID, Botan::ASN1_Tag::OCTET_STRING);

				dec3.decode(bh->mLogsBloom, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mErgLimit = tools->BytesToBigInt(temp);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mErgUsed = tools->BytesToBigInt(temp);

				dec3.decode(bh->mNrOfTransactions);


				//verify the perspectives
				if (!(bh->mFinalStateRootID.size() == 32 && bh->mTransactionsRootID.size() == 32 && bh->mReceiptsRootID.size() == 32))
				{
					const_cast<std::string&>(errorInfo) = "TrieDB perspectives are invalid.";
					const_cast<eBlockHeaderInstantiationResult&>(result) = CBlockHeader::eBlockHeaderInstantiationResult::failure;
					return nullptr;
				}

			}
			//REGULAR BLOCK FIELDS - END

			break;


		case 2:

			//COMMON FIELDS - BEGIN
			dec3.decode(bh->mHeight);
			dec3.decode(bh->mSolvedAt);
			dec3.decode(bh->mKeyHeight);
			//*WARNING*: KEY-BLOCK-ONLY FIELDS - BEGIN
			if (bh->mIsKeyBlock)
			{
				//these fields would appear only in a key-block as that's the one containing a PoW
				dec3.decode(bh->mNonce);
				dec3.decode(bh->mTarget);
				dec3.decode(bh->mParentKeyBlockID, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(bh->mPublicKey, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mPaidToLeader = tools->BytesToBigInt(temp);
			}

			//*WARNING*: KEY-BLOCK-ONLY FIELDS - END
			dec3.decode(bh->mSignature, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mParentBlockID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mUncleBlocksHash, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mInitiallStateRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mFinalStateRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mExtraData, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mReceiptsRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mVerifiablesRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mNrOfVerifiables);
			dec3.decode(bh->mNrOfReceipts);
			dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			bh->mTotalReward = tools->BytesToBigInt(temp);

			//COMMON FIELDS - END
			//REGULAR BLOCK FIELDS - BEGIN
			if (!bh->mIsKeyBlock)
			{
				//only regular block is the one containing transactions-related data
				dec3.decode(bh->mTransactionsRootID, Botan::ASN1_Tag::OCTET_STRING);

				dec3.decode(bh->mLogsBloom, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mErgLimit = tools->BytesToBigInt(temp);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mErgUsed = tools->BytesToBigInt(temp);

				dec3.decode(bh->mNrOfTransactions);


				//verify the perspectives
				if (!(bh->mFinalStateRootID.size() == 32 && bh->mTransactionsRootID.size() == 32 && bh->mReceiptsRootID.size() == 32))
				{
					const_cast<std::string&>(errorInfo) = "TrieDB perspectives are invalid.";
					const_cast<eBlockHeaderInstantiationResult&>(result) = CBlockHeader::eBlockHeaderInstantiationResult::failure;
					return nullptr;
				}

			}
			//REGULAR BLOCK FIELDS - END
			break;

		case 3:
	
			//COMMON FIELDS - BEGIN
			dec3.decode(bh->mHeight);
			dec3.decode(bh->mSolvedAt);
			dec3.decode(bh->mKeyHeight);

			//*WARNING*: KEY-BLOCK-ONLY FIELDS - BEGIN
			if (bh->mIsKeyBlock)
			{
				//these fields would appear only in a key-block as that's the one containing a PoW
				dec3.decode(bh->mNonce);
				dec3.decode(bh->mTarget);
				dec3.decode(bh->mParentKeyBlockID, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(bh->mPublicKey, Botan::ASN1_Tag::OCTET_STRING);
			}
			//*WARNING*: KEY-BLOCK-ONLY FIELDS - END

			// Paid-To-Operator After Tax - BEGIN
			// Notice: this field is relevat to both Key Blocks and Data Blocks
			dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			bh->mPaidToLeader = tools->BytesToBigInt(temp);
			// Paid-To-Operator After Tax - END

			dec3.decode(bh->mSignature, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mParentBlockID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mUncleBlocksHash, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mInitiallStateRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mFinalStateRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mExtraData, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mReceiptsRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mVerifiablesRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mNrOfVerifiables);
			dec3.decode(bh->mNrOfReceipts);
			dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			bh->mTotalReward = tools->BytesToBigInt(temp);

			//COMMON FIELDS - END
			//REGULAR BLOCK FIELDS - BEGIN
			if (!bh->mIsKeyBlock)
			{
				//only regular block is the one containing transactions-related data
				dec3.decode(bh->mTransactionsRootID, Botan::ASN1_Tag::OCTET_STRING);

				dec3.decode(bh->mLogsBloom, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mErgLimit = tools->BytesToBigInt(temp);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mErgUsed = tools->BytesToBigInt(temp);

				dec3.decode(bh->mNrOfTransactions);


				//verify the perspectives
				if (!(bh->mFinalStateRootID.size() == 32 && bh->mTransactionsRootID.size() == 32 && bh->mReceiptsRootID.size() == 32))
				{
					const_cast<std::string&>(errorInfo) = "TrieDB perspectives are invalid.";
					const_cast<eBlockHeaderInstantiationResult&>(result) = CBlockHeader::eBlockHeaderInstantiationResult::failure;
					return nullptr;
				}

			}
			//REGULAR BLOCK FIELDS - END
			break;


		case 4:

			//COMMON FIELDS - BEGIN
			dec3.decode(bh->mHeight);
			dec3.decode(bh->mSolvedAt);
			dec3.decode(bh->mKeyHeight);
			dec3.decode(bh->mCoreVersion); // New in version 4: Core version that produced this block
			//*WARNING*: KEY-BLOCK-ONLY FIELDS - BEGIN
			if (bh->mIsKeyBlock)
			{
				//these fields would appear only in a key-block as that's the one containing a PoW
				dec3.decode(bh->mNonce);
				dec3.decode(bh->mTarget);
				dec3.decode(bh->mParentKeyBlockID, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(bh->mPublicKey, Botan::ASN1_Tag::OCTET_STRING);
			}
			//*WARNING*: KEY-BLOCK-ONLY FIELDS - END

			// Paid-To-Operator After Tax - BEGIN
			// Notice: this field is relevat to both Key Blocks and Data Blocks
			dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			bh->mPaidToLeader = tools->BytesToBigInt(temp);
			// Paid-To-Operator After Tax - END

			dec3.decode(bh->mSignature, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mParentBlockID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mUncleBlocksHash, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mInitiallStateRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mFinalStateRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mExtraData, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mReceiptsRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mVerifiablesRootID, Botan::ASN1_Tag::OCTET_STRING);
			dec3.decode(bh->mNrOfVerifiables);
			dec3.decode(bh->mNrOfReceipts);
			dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			bh->mTotalReward = tools->BytesToBigInt(temp);

			//COMMON FIELDS - END
			//REGULAR BLOCK FIELDS - BEGIN
			if (!bh->mIsKeyBlock)
			{
				//only regular block is the one containing transactions-related data
				dec3.decode(bh->mTransactionsRootID, Botan::ASN1_Tag::OCTET_STRING);

				dec3.decode(bh->mLogsBloom, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mErgLimit = tools->BytesToBigInt(temp);
				dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				bh->mErgUsed = tools->BytesToBigInt(temp);

				dec3.decode(bh->mNrOfTransactions);


				//verify the perspectives
				if (!(bh->mFinalStateRootID.size() == 32 && bh->mTransactionsRootID.size() == 32 && bh->mReceiptsRootID.size() == 32))
				{
					const_cast<std::string&>(errorInfo) = "TrieDB perspectives are invalid.";
					const_cast<eBlockHeaderInstantiationResult&>(result) = CBlockHeader::eBlockHeaderInstantiationResult::failure;
					return nullptr;
				}

			}
			//REGULAR BLOCK FIELDS - END
			break;

		}


		bool gotVirginStateDB;

		//instantiate TrieDBs. Constructors will attempt to load coresponding roots from solid storage; virgin roots will be created on failure.
		/*if (!loadRootsFromSS)
		{
			bh->mStateRootID.clear();
			bh->mTransactionsRootID.clear();
			bh->mReceiptsRootID.clear();
			bh->mVerifiablesRootID.clear();
		}*/

		//no such data within a key-block
		try {
			//Note: loading Roots from cold-storage is pointless here; roots are NOT stored seperately from the Block; they are stored ONLY within the blocks body
			//AND will bo populated on demands with a call to PopulateTries(0
			loadRootsFromSS = false;

			if (bh->mTransactionsDB != nullptr)
				delete bh->mTransactionsDB;
			if (bh->mReceiptsDB != nullptr)
				delete bh->mReceiptsDB;
			if (bh->mVerifiablesDB != nullptr)
				delete bh->mVerifiablesDB;

			bh->mTransactionsDB = new CTrieDB(CGlobalSecSettings::getTransactionsDBID(), CSolidStorage::getInstance(blockchainMode), bh->mTransactionsRootID, false, false, loadRootsFromSS);
			bh->mReceiptsDB = new CTrieDB(CGlobalSecSettings::getReceiptsDBID(), CSolidStorage::getInstance(blockchainMode), bh->mReceiptsRootID, true, false, loadRootsFromSS);
			bh->mVerifiablesDB = new CTrieDB(CGlobalSecSettings::getVerifiablesDBID(), CSolidStorage::getInstance(blockchainMode), bh->mVerifiablesRootID, false, false, loadRootsFromSS);
		}
		catch (std::invalid_argument ex)
		{
			const_cast<std::string&>(errorInfo) = ex.what();
			const_cast<eBlockHeaderInstantiationResult&>(result) = CBlockHeader::eBlockHeaderInstantiationResult::failure;
			return nullptr;
		}
		//TODO: report error on virgin STateDB
		//bool virginStateDB = bh->mStateDB->isEmpty();//STILL TRUE!
		bool virginTransactionsDB = bh->mTransactionsDB->isEmpty();
		bool virginReceiptsDB = bh->mReceiptsDB->isEmpty();
		bool virginVerifiablesDB = bh->mVerifiablesDB->isEmpty();
		if ((bh->mIsKeyBlock ? true : (bh->mTransactionsDB != nullptr && bh->mTransactionsDB != nullptr)) && bh->mVerifiablesDB != nullptr &&
			bh->mTransactionsDB->getRoot() != nullptr && bh->mReceiptsDB->getRoot() != nullptr
			&& bh->mVerifiablesDB != nullptr & bh->mVerifiablesDB->getRoot() != nullptr)
		{
			if (virginTransactionsDB)
				const_cast<eBlockHeaderInstantiationResult&>(result) = CBlockHeader::eBlockHeaderInstantiationResult::OKVriginRoots;
			else
				const_cast<eBlockHeaderInstantiationResult&>(result) = CBlockHeader::eBlockHeaderInstantiationResult::OKRootsFromSS;///this is deprecated. roots are NEVER loaded from ss.
			//all data is always available from within the block. will be populated from block's body in block's instatiation routine.

		}
		else const_cast<eBlockHeaderInstantiationResult&>(result) = CBlockHeader::eBlockHeaderInstantiationResult::failure;



		if (bh->isKeyBlock() && bh->getParentKeyBlockID().size() != 32 && bh->getHeight() != 0)
		{//note: the parentKeyBlock hash/id is needed only for key-blocks (PoW takes it as argument)
			const_cast<eBlockHeaderInstantiationResult&>(result) = CBlockHeader::eBlockHeaderInstantiationResult::parentMissing;
			return nullptr;
		}

		return bh;
	}
	catch (...)
	{
		const_cast<eBlockHeaderInstantiationResult&>(result) = eBlockHeaderInstantiationResult::invalidBERData;
		return nullptr;

	}
}

void CBlockHeader::setParent(std::shared_ptr<CBlock> parent, bool onlyHash)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian); 
											// ^- parental mechanics have its own guardian
	// Operational Logic - BEGIN

	// Null Pointer Case - BEGIN
	if (parent == nullptr)

	{
		{
			std::lock_guard<std::recursive_mutex> lock(mParentGuardian);
			mParentBlock = nullptr;
		}
		return;
	}
	// Null Pointer Case - END

	assertGN(parent != mBlock.lock());

	//assertGN(parent->getHeader()->getPackedData(temp)); // notice: locks both mGuardian and mParentGuardian, on parent

	std::vector<uint8_t> parentBlockHahsh = parent->getID(); // uses optimisation if hash is already pre-computed

	setParentHash(parentBlockHahsh); // locks only mParentGuardian

	if (!onlyHash)
	{
		{
			std::lock_guard<std::recursive_mutex> lock(mParentGuardian);
			mParentBlock = parent;
		}
	}
	return;
	// Operational Logic - END
}


//optimized only for cache formation.
void CBlockHeader::setParentPtr(std::shared_ptr<CBlock> parent)
{
	std::lock_guard<std::recursive_mutex> lock(mParentGuardian);
	mParentBlock = parent;
}



uint64_t CBlockHeader::getNrOfParentsInMem()
{
	std::shared_ptr<CBlock> current = getParentPtr();;
	uint64_t nr = 0;

	while (current != nullptr) {
		current = current->getParentPtr();
		nr++;
	}
	return nr;
}

bool  CBlockHeader::setErgLimit(BigInt ergLimit)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (ergLimit < 10)
		return false;
	this->mErgLimit = ergLimit;
	return true;
}

bool  CBlockHeader::setErgUsed(BigInt ergUsed)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (ergUsed < 10)
		return false;
	this->mErgUsed = ergUsed;
	return true;
}

void CBlockHeader::setTotalBlockReward(BigInt reward, bool updateOnlyEffective)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	
	mTotalRewardEffective = reward;

	if (updateOnlyEffective)
	{
		return;
	}
	
	mTotalReward = reward;
}

BigInt CBlockHeader::getTotalBlockReward(bool getEffectiveValue)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	if (getEffectiveValue && mTotalRewardEffective)
	{
		return mTotalRewardEffective;
	}

	return mTotalReward;
}

bool CBlockHeader::setExtraData(std::vector<uint8_t> data)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (data.size() == 0)
		return false;
	this->mExtraData = data;
	return true;
}



bool CBlockHeader::setLogsBloom(std::vector<uint8_t> bloom)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (bloom.size() < 32)
		return false;

	this->mLogsBloom = bloom;
	return true;
}


/// <summary>
/// Gets packed data for either a keyblock or a regular block-header
/// </summary>
/// <returns></returns>
bool CBlockHeader::getPackedData(std::vector<uint8_t>& data, bool includeSig)
{
	try {
		//Important: for a key-block we encode only the required fields
		//the reason is to make the data transmission of key-blocks as fast as possible
		//additional fields would follow for a regular block
		//Key-block is deserialized using the same mechanisms and contained within a 'regular-architecture'
		//still the ammount of serialized data differs a lot.
		std::scoped_lock lock(mGuardian, mParentGuardian);

		std::shared_ptr<CTools> tools = CTools::getInstance();

		if (!validate(includeSig))
		{
			data = std::vector<uint8_t>();
			return false;
		}
		if (mVersion == 1)
		{
			if (!mIsKeyBlock)
			{//key-block contains no transactions; it might contain receipts and verifiables though (to account for block- and transaction-rewards)
				if (mTransactionsRootID.size() == 0)
					mTransactionsRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mTransactionsDB->getPerspective());
			}
			if (mReceiptsRootID.size() == 0)
				mReceiptsRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mReceiptsDB->getPerspective());
			if (mVerifiablesRootID.size() == 0)
				mVerifiablesRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mVerifiablesDB->getPerspective());
			//}

			Botan::DER_Encoder enc;
			enc.start_cons(static_cast<Botan::ASN1_Tag>(mIsKeyBlock ? eBERObjectType::eBERObjectType::keyBlockHeader : eBERObjectType::eBERObjectType::regularBlockHeader), Botan::ASN1_Tag::PRIVATE);


			enc.encode(mVersion)
				.start_cons(Botan::ASN1_Tag::SEQUENCE)
				.encode(mHeight)
				.encode(mSolvedAt)
				.encode(mKeyHeight);
			if (mIsKeyBlock)
			{
				enc.encode(mNonce)
					.encode(mTarget)
					.encode(mParentKeyBlockID, Botan::ASN1_Tag::OCTET_STRING) // distinct from mParentBlockID
					.encode(mPublicKey, Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mPaidToLeader), Botan::ASN1_Tag::OCTET_STRING);

			}

			if (includeSig)
				enc.encode(mSignature, Botan::ASN1_Tag::OCTET_STRING);
			else
				enc.encode_null();

			enc.encode(mParentBlockID, Botan::ASN1_Tag::OCTET_STRING) // distinct from mParentKeyBlockID
				.encode(mUncleBlocksHash, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mFinalStateRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mExtraData, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mReceiptsRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mVerifiablesRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mNrOfVerifiables)
				.encode(mNrOfReceipts)
				.encode(tools->BigIntToBytes(mTotalReward), Botan::ASN1_Tag::OCTET_STRING);//represents total reward given to leaderS(S!)
			if (!mIsKeyBlock)
			{
				enc.encode(mTransactionsRootID, Botan::ASN1_Tag::OCTET_STRING)
					.encode(mLogsBloom, Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mErgLimit), Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mErgUsed), Botan::ASN1_Tag::OCTET_STRING)
					.encode(mNrOfTransactions);
			}
			enc.end_cons();

			enc.end_cons();
			data = enc.get_contents_unlocked();
			return true;
		}
		else if (mVersion == 2)
		{
			if (!mIsKeyBlock)
			{//key-block contains no transactions; it might contain receipts and verifiables though (to account for block- and transaction-rewards)
				if (mTransactionsRootID.size() == 0)
					mTransactionsRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mTransactionsDB->getPerspective());
			}
			if (mReceiptsRootID.size() == 0)
				mReceiptsRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mReceiptsDB->getPerspective());
			if (mVerifiablesRootID.size() == 0)
				mVerifiablesRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mVerifiablesDB->getPerspective());
			//}

			Botan::DER_Encoder enc;
			enc.start_cons(static_cast<Botan::ASN1_Tag>(mIsKeyBlock ? eBERObjectType::eBERObjectType::keyBlockHeader : eBERObjectType::eBERObjectType::regularBlockHeader), Botan::ASN1_Tag::PRIVATE);


			enc.encode(mVersion)
				.start_cons(Botan::ASN1_Tag::SEQUENCE)
				.encode(mHeight)
				.encode(mSolvedAt)
				.encode(mKeyHeight);
			if (mIsKeyBlock)
			{
				enc.encode(mNonce)
					.encode(mTarget)
					.encode(mParentKeyBlockID, Botan::ASN1_Tag::OCTET_STRING)
					.encode(mPublicKey, Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mPaidToLeader), Botan::ASN1_Tag::OCTET_STRING);

			}

			if (includeSig)
				enc.encode(mSignature, Botan::ASN1_Tag::OCTET_STRING);
			else
				enc.encode_null();

			enc.encode(mParentBlockID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mUncleBlocksHash, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mInitiallStateRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mFinalStateRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mExtraData, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mReceiptsRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mVerifiablesRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mNrOfVerifiables)
				.encode(mNrOfReceipts)
				.encode(tools->BigIntToBytes(mTotalReward), Botan::ASN1_Tag::OCTET_STRING);//represents total reward given to leaderS(S!)
			if (!mIsKeyBlock)
			{
				enc.encode(mTransactionsRootID, Botan::ASN1_Tag::OCTET_STRING)
					.encode(mLogsBloom, Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mErgLimit), Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mErgUsed), Botan::ASN1_Tag::OCTET_STRING)
					.encode(mNrOfTransactions);
			}
			enc.end_cons();

			enc.end_cons();
			data = enc.get_contents_unlocked();
			return true;
		}
		else if (mVersion == 3)
		{
			if (!mIsKeyBlock)
			{//key-block contains no transactions; it might contain receipts and verifiables though (to account for block- and transaction-rewards)
				if (mTransactionsRootID.size() == 0)
					mTransactionsRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mTransactionsDB->getPerspective());
			}
			if (mReceiptsRootID.size() == 0)
				mReceiptsRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mReceiptsDB->getPerspective());
			if (mVerifiablesRootID.size() == 0)
				mVerifiablesRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mVerifiablesDB->getPerspective());
			//}

			Botan::DER_Encoder enc;
			enc.start_cons(static_cast<Botan::ASN1_Tag>(mIsKeyBlock ? eBERObjectType::eBERObjectType::keyBlockHeader : eBERObjectType::eBERObjectType::regularBlockHeader), Botan::ASN1_Tag::PRIVATE);


			enc.encode(mVersion)
				.start_cons(Botan::ASN1_Tag::SEQUENCE)
				.encode(mHeight)
				.encode(mSolvedAt)
				.encode(mKeyHeight);


			// Warning - Key Block Only Fields - BEGIN
			if (mIsKeyBlock)
			{
				enc.encode(mNonce)
					.encode(mTarget)
					.encode(mParentKeyBlockID, Botan::ASN1_Tag::OCTET_STRING)
					.encode(mPublicKey, Botan::ASN1_Tag::OCTET_STRING);
			}
			// Warning - Key Block Only Fields - END

			// Paid-To-Operator After Tax - BEGIN
			// Notice: this is relevant both to key blocks and data blocks.
			enc.encode(tools->BigIntToBytes(mPaidToLeader), Botan::ASN1_Tag::OCTET_STRING);
			// Paid-To-Operator After Tax - END

			if (includeSig)
				enc.encode(mSignature, Botan::ASN1_Tag::OCTET_STRING);
			else
				enc.encode_null();

			enc.encode(mParentBlockID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mUncleBlocksHash, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mInitiallStateRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mFinalStateRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mExtraData, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mReceiptsRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mVerifiablesRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mNrOfVerifiables)
				.encode(mNrOfReceipts)
				.encode(tools->BigIntToBytes(mTotalReward), Botan::ASN1_Tag::OCTET_STRING);//represents total reward given to leaderS(S!)
			if (!mIsKeyBlock)
			{
				enc.encode(mTransactionsRootID, Botan::ASN1_Tag::OCTET_STRING)
					.encode(mLogsBloom, Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mErgLimit), Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mErgUsed), Botan::ASN1_Tag::OCTET_STRING)
					.encode(mNrOfTransactions);
			}
			enc.end_cons();

			enc.end_cons();
			data = enc.get_contents_unlocked();
			return true;
			}
		else if (mVersion == 4)
		{
			if (!mIsKeyBlock)
			{//key-block contains no transactions; it might contain receipts and verifiables though (to account for block- and transaction-rewards)
				if (mTransactionsRootID.size() == 0)
					mTransactionsRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mTransactionsDB->getPerspective());
			}
			if (mReceiptsRootID.size() == 0)
				mReceiptsRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mReceiptsDB->getPerspective());
			if (mVerifiablesRootID.size() == 0)
				mVerifiablesRootID = CCryptoFactory::getInstance()->getSHA2_256Vec(mVerifiablesDB->getPerspective());
			//}

			Botan::DER_Encoder enc;
			enc.start_cons(static_cast<Botan::ASN1_Tag>(mIsKeyBlock ? eBERObjectType::eBERObjectType::keyBlockHeader : eBERObjectType::eBERObjectType::regularBlockHeader), Botan::ASN1_Tag::PRIVATE);


			enc.encode(mVersion)
				.start_cons(Botan::ASN1_Tag::SEQUENCE)
				.encode(mHeight)
				.encode(mSolvedAt)
				.encode(mKeyHeight)
				.encode(mCoreVersion);


			// Warning - Key Block Only Fields - BEGIN
			if (mIsKeyBlock)
			{
				enc.encode(mNonce)
					.encode(mTarget)
					.encode(mParentKeyBlockID, Botan::ASN1_Tag::OCTET_STRING)
					.encode(mPublicKey, Botan::ASN1_Tag::OCTET_STRING);
			}
			// Warning - Key Block Only Fields - END

			// Paid-To-Operator After Tax - BEGIN
			// Notice: this is relevant both to key blocks and data blocks.
			enc.encode(tools->BigIntToBytes(mPaidToLeader), Botan::ASN1_Tag::OCTET_STRING);
			// Paid-To-Operator After Tax - END

			if (includeSig)
				enc.encode(mSignature, Botan::ASN1_Tag::OCTET_STRING);
			else
				enc.encode_null();

			enc.encode(mParentBlockID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mUncleBlocksHash, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mInitiallStateRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mFinalStateRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mExtraData, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mReceiptsRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mVerifiablesRootID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mNrOfVerifiables)
				.encode(mNrOfReceipts)
				.encode(tools->BigIntToBytes(mTotalReward), Botan::ASN1_Tag::OCTET_STRING);//represents total reward given to leaderS(S!)
			if (!mIsKeyBlock)
			{
				enc.encode(mTransactionsRootID, Botan::ASN1_Tag::OCTET_STRING)
					.encode(mLogsBloom, Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mErgLimit), Botan::ASN1_Tag::OCTET_STRING)
					.encode(tools->BigIntToBytes(mErgUsed), Botan::ASN1_Tag::OCTET_STRING)
					.encode(mNrOfTransactions);
			}
			enc.end_cons();

			enc.end_cons();
			data = enc.get_contents_unlocked();
			return true;
		}
		else
			return false;


	}
	catch (...)
	{
		data = std::vector<uint8_t>();
		return false;
	}
}



void CBlockHeader::getPubSig(std::vector<uint8_t>& sig, std::vector<uint8_t>& pub)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	sig = mSignature;
	pub = mPublicKey;
}

std::vector<uint8_t> CBlockHeader::getPubKey()
{
	return mPublicKey;
}

bool CBlockHeader::addTransaction(CTransaction  trans)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mTransactionsDB == nullptr)
	{
		return false;
	}
	//const CTransaction t = *trans;
	//size_t nr = 0;
	//CTransaction *tempTrans = new CTransaction(t);//we need to create a copy as the pointer might become invalid during Trie restructuring.
	trans.clearInterData();//used only internally by DB for partial ID storage. initally it needs to be empty. it will be updated during addition.also cleared when sent on the wire.
	CTrieDB::eResult result = mTransactionsDB->addNodeExt(mTools->bytesToNibbles(trans.getHash()), &trans, true);
	//mTransactionsDB->testTrie(nr);
	mTransactionsRootID = mTransactionsDB->getPerspective();
	mNrOfTransactions++;
	//CTrieNode * tt = mTransactionsDB->findNodeByFullID(mTools->bytesToNibbles(trans.getGUID()));
	//assert(tt!=nullptr && tt->getSubType() == 2);
	if (result != CTrieDB::eResult::failure)
		return true;
	else return false;
}
bool CBlockHeader::addVerifiable(CVerifiable verifiable)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mVerifiablesDB == nullptr)
		return false;
	mNrOfVerifiables++;
	verifiable.clearInterData();
	std::vector<uint8_t> h = verifiable.getHash();
	CTrieDB::eResult res = mVerifiablesDB->addNodeExt(mTools->bytesToNibbles(h), &verifiable, true);
	mVerifiablesRootID = mVerifiablesDB->getPerspective();
	if (res != CTrieDB::eResult::failure)
		return true;
	else return false;
}
bool CBlockHeader::addReceipt(CReceipt receipt)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mReceiptsDB == nullptr)
		return false;
	mNrOfReceipts++;
	receipt.clearInterData();
	CTrieDB::eResult  res = mReceiptsDB->addNodeExt(mTools->bytesToNibbles(receipt.getHash()), &receipt, true);
	mReceiptsRootID = mReceiptsDB->getPerspective();
	if (res != CTrieDB::eResult::failure)
		return true;
	else return false;
}

bool CBlockHeader::getTransaction(std::vector<uint8_t> transactionID, CTransaction& transaction)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//the transaction might contain inter-data (containing partial ID) however it will be cleared on getPackedData()
	if (transactionID.size() == 0)
		return false;

	if (!mTriesLoaded)
		if (!loadTries())
			return false;

	size_t nr = 0;

	CTrieNode* tranTemp = mTransactionsDB->findNodeByFullID(mTools->bytesToNibbles(transactionID));
	if (tranTemp == nullptr) return false;
	//subtype 1 for StateDomains , subtype 2 for transactions , subtype 3 for receipts
	if (!(tranTemp->getType() == 3 && tranTemp->getSubType() == 2))
		return false;
	transaction = *(CTransaction*)tranTemp;
	transaction.clearInterData();

	return true;
}

bool CBlockHeader::getReceipt(std::vector<uint8_t> receiptID, CReceipt& receipt)
{
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<nibblePair> nbID = tools->bytesToNibbles(receiptID);
	CTrieNode* receiptTemp = nullptr;

	if (receiptID.size() == 0)
		return false;

	if (mTriesLoaded == false)
	{
		if (loadTries() == false)
		{
			return false;
		}
	}



	receiptTemp = mReceiptsDB->findNodeByFullID(nbID);

	if (receiptTemp == nullptr)
		return false;
	//subtype 1 for StateDomains , subtype 2 for transactions , subtype 3 for receipts
	if (!receiptTemp->getType() == 3 && receiptTemp->getSubType() == 3)
		return false;
	receipt = *(CReceipt*)receiptTemp;
	receipt.clearInterData();
	return true;
}

bool CBlockHeader::areTriesLoaded(bool verify)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mTriesLoaded)
	{//additional verification
		if (!(mTransactionsRootID.size() == 32 && mReceiptsRootID.size() == 32 && mVerifiablesRootID.size() == 32))
		{
			mTriesLoaded = false;
		}
		else if (verify)
		{
			return (std::memcmp(mTransactionsRootID.data(), mTransactionsDB->getPerspective().data(), 32) == 0
				&& std::memcmp(mVerifiablesRootID.data(), mVerifiablesDB->getPerspective().data(), 32) == 0
				&& std::memcmp(mReceiptsRootID.data(), mReceiptsDB->getPerspective().data(), 32) == 0)
				;
		}
		else
		{
			return true;
		}
	}

	return mTriesLoaded;
}

void CBlockHeader::setTriesLoaded(bool mark)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mTriesLoaded = mark;
}

bool CBlockHeader::loadTries()
{
	eBlockInstantiationResult::eBlockInstantiationResult res;
	if (isKeyBlock())//todo: load tries for key--blocks as well, these are needed for recepits
		return false;
	std::vector<uint8_t> bytes = CSolidStorage::getInstance(mBlockchainMode)->getBlockDataByHash(getHash());

	if (!loadTries(bytes))
	{
		mTriesLoaded = false;
		return mTriesLoaded;
	}
	else
	{
		mTriesLoaded = true;
		return mTriesLoaded;

	}
	//std::shared_ptr<CBlock> b=CBlockchainManager::getInstance(blockchainMode)->getBlockByHash(getHash(), res, true);
}

bool CBlockHeader::loadTries(std::vector<uint8_t> blockBER)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	std::shared_ptr<CBlock> block = mBlock.lock();
	if (block == nullptr)
		return eBlockInfoRetrievalResult::eBlockInfoRetrievalResult::blockBodyUnavailable;

	try {
		if (mBlock.lock() == nullptr && blockBER.size() == 0)
		{
			return false;
		}
		else if (blockBER.size() == 0)
		{
			eBlockInstantiationResult::eBlockInstantiationResult re;
			//try to load the block from Cold Sotrage
			blockBER = CSolidStorage::getInstance(mBlockchainMode)->getBlockDataByHash(block->getID());
			if (blockBER.size() == 0)
				return false;
		}
		bool isKey = false;
		Botan::BER_Object obj;
		size_t version;
		std::vector<uint8_t> temp;
		Botan::BER_Decoder dec1 = Botan::BER_Decoder(blockBER);

		Botan::BER_Object ob = dec1.get_next_object();
		if (ob.class_tag != Botan::ASN1_Tag::PRIVATE)
			assertGN(false);
		if (ob.type_tag == static_cast<Botan::ASN1_Tag>(eBERObjectType::eBERObjectType::keyBlock))
			isKey = true;
		else if (ob.type_tag == static_cast<Botan::ASN1_Tag>(eBERObjectType::eBERObjectType::regularBlock))
			isKey = false;
		else return false;//unknown type



		Botan::BER_Decoder dec2 = Botan::BER_Decoder(ob.value);
		dec2.decode(version);

		std::vector<uint8_t> claimedTransactionRootID = getPerspective(eTrieID::transactions);
		std::vector<uint8_t>claimedReceiptsRootID = getPerspective(eTrieID::receipts);
		std::vector<uint8_t> claimedVerifiablesRootID = getPerspective(eTrieID::verifiables);
		if (!(claimedTransactionRootID.size() == 32 && claimedReceiptsRootID.size() == 32 && claimedVerifiablesRootID.size() == 32))
		{
			mTriesLoaded = false;
			return false;
		}
		switch (version)
		{

		case 1:
			obj = dec2.get_next_object();
			if (obj.type_tag != Botan::ASN1_Tag::OCTET_STRING)
			{
				return false;
			}
			std::vector<uint8_t> headerData = Botan::unlock(obj.value);
			std::shared_ptr<CBlockHeader> header = nullptr;

			for (int i = isKeyBlock() ? 1 : 0; i < 3; i++)//0 - transactions, 1  Receipts , 2 - Verifiables
			{
				obj = dec2.get_next_object();//supposed to be a sequence
				Botan::BER_Decoder dec3(obj.value);

				switch (i)
				{
				case 0:
					//start transactions
					mTransactionsDB->trackKnownNodes();
					while (dec3.more_items())
					{
						temp.clear();
						obj = dec3.get_next_object();
						if (obj.type_tag == Botan::ASN1_Tag::OCTET_STRING)
						{
							temp = Botan::unlock(obj.value);
							CTransaction* t = CTransaction::instantiate(temp, mBlockchainMode);
							if (t == nullptr)
								throw std::invalid_argument("could not instantiate transaction");
							t->clearInterData();
							mTransactionsDB->addNode(mTools->bytesToNibbles(t->getHash()), t, true);
						}
						else break;
					}
					if (std::memcmp(mTransactionsDB->getPerspective().data(), claimedTransactionRootID.data(), 32) != 0)
					{
						return false;
					}
					mTransactionsDB->setIsTrieFullyInRAM(true);
					break;
				case 1:
					//start receipts
					//mReceiptsDB->pruneTrie(true,true);
					mReceiptsDB->trackKnownNodes();
					while (dec3.more_items())
					{
						temp.clear();

						obj = dec3.get_next_object();
						if (obj.type_tag == Botan::ASN1_Tag::OCTET_STRING)
						{
							temp = Botan::unlock(obj.value);

							CReceipt* rec = CReceipt::instantiateReceipt(temp, mBlockchainMode);
							if (rec == nullptr)
								throw std::invalid_argument("could not instantiate receipt");

							std::vector<uint8_t> h = rec->getHash();
							mReceiptsDB->addNode(mTools->bytesToNibbles(h), rec, true);
							CTrieNode* nn = mReceiptsDB->findNodeByFullID(h);
							assertGN(nn != nullptr);

						}
						else break;
					}
					if (std::memcmp(mReceiptsDB->getPerspective().data(), claimedReceiptsRootID.data(), 32) != 0)
					{
						return false;
					}
					mReceiptsDB->setIsTrieFullyInRAM(true);
					break;
				case 2:
					//start verifiables
					mVerifiablesDB->trackKnownNodes();
					while (dec3.more_items())
					{
						temp.clear();

						obj = dec3.get_next_object();
						if (obj.type_tag == Botan::ASN1_Tag::OCTET_STRING)
						{
							temp = Botan::unlock(obj.value);

							CVerifiable* ver = CVerifiable::instantiateVerifiable(temp, mBlockchainMode);
							if (ver == nullptr)
								throw std::invalid_argument("could not instantiate verifiable");
							std::vector<uint8_t> h = ver->getHash();
							mVerifiablesDB->addNode(mTools->bytesToNibbles(h), ver, true);
						}
						else break;
					}
					if (std::memcmp(mVerifiablesDB->getPerspective().data(), claimedVerifiablesRootID.data(), 32) != 0)
					{
						return false;
					}

					mVerifiablesDB->setIsTrieFullyInRAM(true);
					break;
				}

			}

		}
		mTriesLoaded = true;
		return true;
	}
	catch (...)
	{
		mTriesLoaded = false;
		return false;
	}
}

bool CBlockHeader::getVerifiable(std::vector<uint8_t> verifiableID, CVerifiable& verifiable)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (verifiableID.size() == 0)
		return false;
	if (!mTriesLoaded)
		if (!loadTries())
			return false;

	CTrieNode* verifiableTemp = mVerifiablesDB->findNodeByFullID(mTools->bytesToNibbles(verifiableID));
	if (verifiableTemp == nullptr)
		return false;
	//subtype 1 for StateDomains , subtype 2 for transactions , subtype 3 for receipts,subtype 4 for verifiables
	if (!(verifiableTemp->getType() == 3 && verifiableTemp->getSubType() == 4))
		return false;
	verifiable = *static_cast<CVerifiable*>(verifiableTemp);
	verifiable.clearInterData();
	return true;
}

size_t CBlockHeader::getVersion()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mVersion;
}

bool CBlockHeader::solvedNow()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mSolvedAt = mTools->getTime();
	return true;
}
void CBlockHeader::setCachedOperatorID(const std::vector<uint8_t>& id)
{
	std::unique_lock lock(mSharedGuardian);
	mCachedOperatorID = id;
}

std::vector<uint8_t> CBlockHeader::getCachedOperatorID()
{
	std::shared_lock lock(mSharedGuardian);
	return mCachedOperatorID;
}



std::vector<uint8_t> CBlockHeader::getMinersID()
{
	std::vector<uint8_t> id = getCachedOperatorID();

	if (!id.empty())
		return id;

	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	if (mPublicKey.size() == 32)
	{
		CCryptoFactory::getInstance()->genAddress(mPublicKey, id);
		setCachedOperatorID(id);
	}
	else
	{
		//TODO: fetch ID
	}
	return id;
}
/// <summary>
/// Usually root nodes are updates and saved each time new laves are added (when sandBoxMode is not enabled in addNode param).
/// However, when no nodes were added for instance we might need to force root nodes storage.
/// </summary>
/// <returns></returns>
bool CBlockHeader::forceRootNodesStorage()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!mIsKeyBlock)
		return false;
	mTransactionsRootID = mTransactionsDB->getPerspective();
	mReceiptsRootID = mReceiptsDB->getPerspective();
	mVerifiablesRootID = mVerifiablesDB->getPerspective();

	assertGN(mTransactionsDB->forceRootNodeStorage());
	assertGN(mReceiptsDB->forceRootNodeStorage());
	assertGN(mVerifiablesDB->forceRootNodeStorage());
	return true;
}
void CBlockHeader::setPubSig(std::vector<uint8_t> pub, std::vector<uint8_t> sig)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mSignature = sig;
	mPublicKey = pub;
}

void CBlockHeader::setPubKey(std::vector<uint8_t> pub)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mPublicKey = pub;
}
bool CBlockHeader::setBlock(std::shared_ptr<CBlock> block)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	this->mBlock = block;
	return true;
}

//Updates the Block-Header based on data available within the Block Body
eBlockInfoRetrievalResult::eBlockInfoRetrievalResult CBlockHeader::update()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::shared_ptr<CBlock> block = mBlock.lock();
	if (block == nullptr)
		return eBlockInfoRetrievalResult::eBlockInfoRetrievalResult::blockBodyUnavailable;

	mNrOfTransactions = block->getTransactionsIDs().size();
	mNrOfVerifiables = block->getVerifiablesIDs().size();
	mNrOfReceipts = block->getReceiptsIDs().size();
	return eBlockInfoRetrievalResult::eBlockInfoRetrievalResult::OK;

}

CBlockHeader::~CBlockHeader()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mTransactionsDB != nullptr)
		delete mTransactionsDB;
	if (mReceiptsDB != nullptr)
		delete mReceiptsDB;
	if (mVerifiablesDB != nullptr)
		delete mVerifiablesDB;
}



/// <summary>
/// Gets the Core version that produced this block.
/// For blocks with header version < 4, returns 0 since Core version tracking
/// was not implemented in earlier header versions.
/// </summary>
/// <returns>Core version number that produced this block, or 0 for legacy blocks</returns>
size_t CBlockHeader::getCoreVersion()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mVersion < 4) {
		return 0; // Core version tracking not implemented in earlier header versions
	}
	return mCoreVersion;
}

/// <summary>
/// Sets the Core version for this block. This method should generally not be called directly - 
/// use imposeCoreMarking() during block production instead.
/// The setter exists primarily for deserialization purposes.
/// </summary>
/// <param name="version">Core version to set</param>
/// <returns>True if version was set successfully, false if header version < 4</returns>
bool CBlockHeader::setCoreVersion(size_t version)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mVersion < 4) {
		return false; // Cannot set Core version on older header versions
	}
	mCoreVersion = version;
	return true;
}


/// <summary>
/// Marks a newly produced block with the current Core version number.
/// This method should be called only when mining/producing new blocks to ensure proper version tracking.
/// 
/// Security considerations:
/// - Enables version-specific cross-validation of blocks across the network
/// - Prevents replay attacks using blocks from older Core versions
/// - Required for proper handling of hard/soft forks based on Core versions
/// - Ensures blocks can be properly validated against minimum version requirements
/// 
/// [IMPORTANT]: This marking is critical for:
/// 1. Network consistency - all nodes need to know which Core version produced each block
/// 2. Fork handling - enables proper block validation at fork boundaries
/// 3. Version enforcement - supports minimum version requirements at specific heights
/// 4. Attack prevention - blocks produced by outdated Cores can be rejected
/// 5. Automatic Updates notifications - allows Operators to consider updating to new Core release.
/// 
/// Note: Only called during new block production/mining (see CBlock::newBlock with miningNewBlock=true)
/// to maintain an accurate record of which Core version actually produced each block.
/// </summary>
void CBlockHeader::imposeCoreMarking()
{
	mCoreVersion = static_cast<size_t>(CGlobalSecSettings::getVersionNumber());
}

CBlockHeader::CBlockHeader(eBlockchainMode::eBlockchainMode mode, bool isKeyBlock)
{
	mCoreVersion = 0;
	mTotalRewardEffective = 0;
	mPaidToLeaderEffective = 0;
	mTriesLoaded = false;
	mCoreVersion = 0;
	mPaidToLeader = 0;
	mKeyHeight = 0;
	mIsKeyBlock = isKeyBlock; //key-block is header-only it only has a stub body
	mTotalWorkDone = 0;
	this->mCf = nullptr;
	this->mSs = nullptr;
	this->mSs = nullptr;
	mTransactionsDB = nullptr;
	mBlockchainMode = mode;
	mReceiptsDB = nullptr;
	mVerifiablesDB = nullptr;
	mVersion = BLOCK_HEADER_LATEST_VERSION;
	mAvailableLocally = false;
	mHeight = 1;
	mNonce = 0;

	if (mIsKeyBlock)
		mTarget = 1;
	else
		mTarget = 0;

	mSolvedAt = 0;
	mErgLimit = 0;
	mErgUsed = 0;

	mParentBlock = nullptr;
	mNrOfTransactions = 0;
	mNrOfVerifiables = 0;
	mNrOfReceipts = 0;
	mTotalReward = 0;
	mTools = CTools::getTools();
}


CBlockHeader::CBlockHeader(const CBlockHeader& sibling)
{
	mCoreVersion = sibling.mCoreVersion;
	mPaidToLeader = sibling.mPaidToLeader;
	mKeyHeight = sibling.mKeyHeight;
	mIsKeyBlock = sibling.mIsKeyBlock;
	mTotalRewardEffective = sibling.mTotalRewardEffective;
	mPaidToLeaderEffective = sibling.mPaidToLeaderEffective;
	mBlockchainMode = sibling.mBlockchainMode;
	mTotalWorkDone = sibling.mTotalWorkDone;
	mAvailableLocally = sibling.mAvailableLocally;
	mTotalReward = sibling.mTotalReward;
	//TODO: validate integrity of the copy constructor; by comparing hashes of serialized instances
	mVersion = sibling.mVersion;
	mHeight = sibling.mHeight;
	mNonce = sibling.mNonce;
	mTarget = sibling.mTarget;
	mSolvedAt = sibling.mSolvedAt; // time since UNIX epoch

	mSignature = sibling.mSignature;
	mPublicKey = sibling.mPublicKey;

	//uncle blocks
	mUncleBlocksHash = sibling.mUncleBlocksHash;

	//Merkle-Patricie Tries - IDs:
	mInitiallStateRootID = sibling.mInitiallStateRootID;
	mFinalStateRootID = sibling.mFinalStateRootID;
	mTransactionsRootID = sibling.mTransactionsRootID;
	mReceiptsRootID = sibling.mReceiptsRootID;
	mVerifiablesRootID = sibling.mVerifiablesRootID;

	mLogsBloom = sibling.mLogsBloom;
	mExtraData = sibling.mExtraData;

	//erg-costs
	mErgLimit = sibling.mErgLimit;
	mErgUsed = sibling.mErgUsed;


	mParentBlockID = sibling.mParentBlockID;
	mParentBlock = sibling.mParentBlock;
	mParentKeyBlockID = sibling.mParentKeyBlockID;

	mNrOfTransactions = sibling.mNrOfTransactions;
	mNrOfVerifiables = sibling.mNrOfVerifiables;
	mNrOfReceipts = sibling.mNrOfReceipts;
	mTransactionsDB = new CTrieDB(*sibling.mTransactionsDB);
	//sibling.mTransactionsDB->copyTo(mTransactionsDB, true);
	assertGN(std::memcmp(sibling.mTransactionsDB->getPerspective().data(), mTransactionsDB->getPerspective().data(), 32) == 0);
	mReceiptsDB = new CTrieDB(*sibling.mReceiptsDB);
	//sibling.mReceiptsDB->copyTo(mReceiptsDB, true);
	assertGN(std::memcmp(sibling.mReceiptsDB->getPerspective().data(), mReceiptsDB->getPerspective().data(), 32) == 0);
	mVerifiablesDB = new CTrieDB(*sibling.mVerifiablesDB);
	//sibling.mVerifiablesDB->copyTo(mVerifiablesDB, true);
	assertGN(std::memcmp(sibling.mVerifiablesDB->getPerspective().data(), mVerifiablesDB->getPerspective().data(), 32) == 0);

	mSs = sibling.mSs;
	mBlock = sibling.mBlock;
	mTools = sibling.mTools;
	mCf = sibling.mCf;


}

void CBlockHeader::setPackedTarget(uint32_t target)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!isKeyBlock())
		return;
	mTarget = target;
}
CTrieDB* CBlockHeader::getTransactionsDB()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mTransactionsDB;
}
CTrieDB* CBlockHeader::getReceiptsDB()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mReceiptsDB;
}
CTrieDB* CBlockHeader::getVerifiablesDB()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mVerifiablesDB;
}
/// <summary>
/// Instantiates a *NEW* fresh blockHeader based on a given state of the StateDB.
/// TransactionDB and receiptsDB are virgin.
/// </summary>
/// <param name="block"></param>
/// <param name="useTestDB"></param>
/// <param name="result"></param>
/// <param name="rootHash"></param>
/// <param name="allowEmptyStateDB">returns nullptr if was unable to recover desired stateDB from cold storage.</param>
/// <returns></returns>
std::shared_ptr<CBlockHeader> CBlockHeader::newBlockHeader(std::shared_ptr<CBlock> block, eBlockchainMode::eBlockchainMode blockchainMode, bool isKeyBlock, eBlockHeaderInstantiationResult& result)
{
	try {
		if (blockchainMode != eBlockchainMode::TestNet)
			return nullptr;

		//std::lock_guard<std::mutex> lock(mGeneratorLock);

		//[todo (CodesInChaos): NEVER create a shared_ptr from new. Use make_shared instead. Validate in other parts of code.
		std::shared_ptr<CBlockHeader> header = std::make_shared<CBlockHeader>(blockchainMode, isKeyBlock);
		header->mSs = CSolidStorage::getInstance(blockchainMode);
		header->mCf = CCryptoFactory::getInstance();
		header->mSs = CSolidStorage::getInstance(blockchainMode);
		header->mBlock = block;
		header->mBlockchainMode = blockchainMode;


		//transactions and receipts DBs are always new for a new block
		header->mTransactionsDB = new CTrieDB(CGlobalSecSettings::getTransactionsDBID(), header->mSs, CCryptoFactory::getEmptyNodeHash(), true, true, false);
		header->mReceiptsDB = new CTrieDB(CGlobalSecSettings::getReceiptsDBID(), header->mSs, CCryptoFactory::getEmptyNodeHash(), true, true, false);
		header->mVerifiablesDB = new CTrieDB(CGlobalSecSettings::getVerifiablesDBID(), header->mSs, CCryptoFactory::getEmptyNodeHash(), true, true, false);

		header->mTransactionsRootID = CCryptoFactory::getEmptyNodeHash();//header->mTransactionsDB->getPerspective();
		header->mReceiptsRootID = CCryptoFactory::getEmptyNodeHash();//header->mReceiptsDB->getPerspective();
		header->mVerifiablesRootID = CCryptoFactory::getEmptyNodeHash();//header->mVerifiablesDB->getPerspective();

		header->mVersion = BLOCK_HEADER_LATEST_VERSION;
		header->mHeight = 0;
		header->mNonce = 0;
		header->mTarget = 0;
		header->mSolvedAt = 0;
		header->mErgLimit = 0;
		header->mErgUsed = 0;
		header->mParentBlock = 0;
		//header->mIsTestNet = useTestDB;
		header->mNrOfTransactions = 0;
		header->mNrOfVerifiables = 0;
		header->mNrOfReceipts = 0;
		header->mAvailableLocally = false;
		result = OK;
		return header;
	}
	catch(...)
	{
		return nullptr;
	}
}
size_t CBlockHeader::getHeight()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mHeight;
}

void CBlockHeader::setHeight(size_t height)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mHeight = height;
}

void CBlockHeader::setKeyHeight(size_t height)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mKeyHeight = height;

}

size_t CBlockHeader::getKeyHeight()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mKeyHeight;
}



size_t CBlockHeader::getNonce()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mNonce;
}

void CBlockHeader::setNonce(size_t nonce)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!isKeyBlock())
		return;
	mNonce = nonce;
}

void CBlockHeader::setPaidToMiner(BigInt GNCPaid, bool updateOnlyEffective)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	mPaidToLeaderEffective = GNCPaid;

	if (updateOnlyEffective)
	{
		return;
	}

	mPaidToLeader = GNCPaid;
}

BigInt CBlockHeader::getPaidToMiner(bool getEffectiveValue)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	if (getEffectiveValue && mTotalRewardEffective)
	{
		return mPaidToLeaderEffective;
	}

	return mPaidToLeader;
}

size_t CBlockHeader::getPackedTarget()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mTarget;
}

void CBlockHeader::markLocalAvailability(bool available)
{
	mAvailableLocally = available;
}

bool CBlockHeader::validate(bool requireSig)
{
	std::scoped_lock lock(mGuardian, mParentGuardian);
	if (mIsKeyBlock)
	{
		if (mPublicKey.size() != 32)
			return false;

		//if (mTarget == 0)
		//	return false; now Hard Fork Block requires this (which is also a key block)
		if (mHeight > 0 && mParentKeyBlockID.size() != 32)//required for PoW
			return false;
	}
	else
	{

	}
	if (mHeight > 0 && mParentBlockID.size() != 32)
		return false;
	if (requireSig && mSignature.size() == 0)
		return false;

	return true;
}

/// <summary>
/// Gets the parent key-block ID.
/// Each key block has a parent which might be a regular block
/// but also another parent which must be the key-block.
/// 
/// Only key-blocks require this fields to be set(needed for PoW)
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CBlockHeader::getParentKeyBlockID()
{
	std::lock_guard<std::recursive_mutex> lock(mParentGuardian);
	return mParentKeyBlockID;
}
/// <summary>
/// Sets the parent key-block ID.
/// Each key block has a parent which might be a regular block
/// but also another parent which must be the key-block.
/// 
/// Only key-blocks require this field to be set(needed for PoW)
/// </summary>
/// <returns></returns>
bool CBlockHeader::setParentKeyBlockID(std::vector<uint8_t> id)
{
	if (!mIsKeyBlock)
		return false;
	if (id.size() != 32 && mIsKeyBlock)
		return false;

	std::lock_guard<std::recursive_mutex> lock(mParentGuardian);
	mParentKeyBlockID = id;
	return true;
}

bool CBlockHeader::isKeyBlock()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mIsKeyBlock;
}

bool CBlockHeader::isHardForkBlock()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mIsKeyBlock && mNonce == 0 && mTarget == 0;
}

uint64_t CBlockHeader::getTotalDiffField()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mTotalWorkDone;
}

void CBlockHeader::setTotalDiffField(uint64_t diff)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mTotalWorkDone = diff;
}

/// <summary>
/// Returns a Merkle-Proof for a given entity stroed within the Block.
/// The data structure proves in an efficient mannear the existance of a given entity.
/// </summary>
/// <param name=""></param>
/// <param name="trie"></param>
/// <returns></returns>
std::vector<uint8_t> CBlockHeader::getMerkleProof(std::vector<uint8_t> id, eTrieID trie)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!mTriesLoaded)
		if (!loadTries())
			return std::vector<uint8_t>();
	switch (trie)
	{
	case eTrieID::receipts:
		return	mReceiptsDB->getPackedMerklePathTopBottom(id);
		break;
	case eTrieID::transactions:
		return	mTransactionsDB->getPackedMerklePathTopBottom(id);
		break;
	case eTrieID::verifiables:
		return	mVerifiablesDB->getPackedMerklePathTopBottom(id);
		break;
	default:
		return std::vector<uint8_t>();
		break;


	}
}

bool CBlockHeader::getReceiptForVerifiable(std::vector<uint8_t> proofID, eVerifiableType::eVerifiableType verType, CReceipt& rec, bool pruneDBAfter)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	size_t receiptsCount = 0;
	//if (proofID.size() != 32)
		//return false;
	bool found = false;
	if (getReceiptsDB()->getKnownNodeIDs().size() == 0)
	{
		//let's check what's inside
		getReceiptsDB()->trackKnownNodes();
		//size_t rece
		getReceiptsDB()->testTrie(receiptsCount, false, false, eTrieSize::smallTrie);

	}
	//let's look inside in search for a Receipt of a particular Transaction
	std::vector<std::vector<uint8_t>> knownReceipts = getReceiptsDB()->getKnownNodeIDs();
	for (int i = 0; i < knownReceipts.size(); i++)
	{

		CTrieNode* node = getReceiptsDB()->findNodeByFullID(knownReceipts[i]);
		if (node != nullptr)
		{
			CReceipt* recR = static_cast<CReceipt*>(node);
			if (recR->getReceiptType() == eReceiptType::verifiable)
			{
				if (CTools::getTools()->compareByteVectors(proofID, recR->getVerifiableID()))
				{
					rec = *recR;
					rec.clearInterData();
					found = true;
					break;
				}
			}
		}
	}
	if (pruneDBAfter)
		getReceiptsDB()->pruneTrie();
	if (found)
		return true;
	else return false;
}

bool CBlockHeader::getReceiptForTransaction(std::vector<uint8_t> transactionID, CReceipt& rec, bool pruneDBAfter)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	size_t receiptsCount = 0;
	if (transactionID.size() != 32)
		return false;
	bool found = false;
	if (getReceiptsDB()->getKnownNodeIDs().size() == 0)
	{
		//let's check what's inside
		getReceiptsDB()->trackKnownNodes();
		//size_t rece
		getReceiptsDB()->testTrie(receiptsCount, false, false, eTrieSize::smallTrie);

	}
	//let's look inside in search for a Receipt of a particular Transaction
	std::vector<std::vector<uint8_t>> knownReceipts = getReceiptsDB()->getKnownNodeIDs();
	for (int i = 0; i < knownReceipts.size(); i++)
	{

		CTrieNode* node = getReceiptsDB()->findNodeByFullID(knownReceipts[i]);
		if (node != nullptr)
		{
			CReceipt* recR = static_cast<CReceipt*>(node);
			if (recR->getVerifiableID().size() == 32)
			{
				if (std::memcmp(transactionID.data(), recR->getVerifiableID().data(), 32) == 0)
				{
					rec = *recR;
					rec.clearInterData();
					rec.unregister();
					found = true;
					break;
				}
			}
		}
	}


	if (pruneDBAfter)
	{
		mTriesLoaded = false;
		getReceiptsDB()->pruneTrie();
	}

	if (found)
		return true;
	else return false;
}

bool CBlockHeader::isAvailableLocally()
{
	return mAvailableLocally;
}

double   CBlockHeader::getDifficulty()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//arith_uint256  gen_target = gen_target.SetCompact(0x1f00ffff, false);

	//if (getPackedTarget() == 0)
		//setPackedTarget(0x1f00ffff);

	size_t packed = getPackedTarget();
	double toRet = CTools::getTools()->packedTarget2diff(packed);
	if (toRet > 100000000000)
	{
		return 0;//just to prevent overflow attacks on this value
	}
	return toRet;
	//arith_uint256  min_target = min_target.SetCompact(packed, false);
	//return (gen_target / min_target).getdouble();
}

void CBlockHeader::freeTries()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mTransactionsDB != nullptr)
		mTransactionsDB->pruneTrie(true,false,false,false);
	if (mVerifiablesDB != nullptr)
		mVerifiablesDB->pruneTrie(true, false, false, false);
	if (mReceiptsDB != nullptr)
		mReceiptsDB->pruneTrie(true, false, false, false);
	mTriesLoaded = false;
}



size_t CBlockHeader::getSolvedAtTime()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mSolvedAt;
}

void CBlockHeader::setSolvedAtTime(size_t time)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mSolvedAt = time;
}

std::shared_ptr<CBlock> CBlockHeader::getParent(eBlockInstantiationResult::eBlockInstantiationResult& result, bool useColdStorage, bool instantiateTries, bool muteConsole, std::shared_ptr<CBlockchainManager> bm)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian); // block any operations on ths object from taking place
	//    										^-- lock not needed since parent related matters have its own mutex.
	if (!mHeight)
	{
		return nullptr;
	}
	std::shared_ptr<CBlock> parent = nullptr;

	// Operational Logic - BEGIN
	std::lock_guard<std::recursive_mutex> lock(mParentGuardian);// warning: the mutex is acquired also durin cache retrieval below ( see declaration of .mParentGuardian).

	if (mParentBlockID.size() == 0) // this never gets changed
	{
		result = eBlockInstantiationResult::eBlockInstantiationResult::parentMissing;
		return nullptr;
	}
	// Pre-Computed Data Available - BEGIN
	if (mParentBlock != nullptr)
	{
		result = eBlockInstantiationResult::eBlockInstantiationResult::blockFetchedFromCache;
		return mParentBlock;

	}// Pre-Computed Data Available - END
	else {
		//  Retrieve Block - BEGIN

		// Try Cache - BEGIN
		// try to fetch from cache
		if (!bm)
		{
			bm = CBlockchainManager::getInstance(mBlockchainMode,false,true);
		}
		parent = bm->getBlockAtHeight(mHeight - 1);
		if (parent != nullptr)
			return  parent;
		// Try Cache - END
		// - else -
		// Fetch from Cold Storage - BEGIN
		else
			if (mParentBlockID.size() > 0 && useColdStorage)
			{
				if (!muteConsole)
					mTools->logEvent("Fetching block " + mTools->base58CheckEncode(mParentBlockID), "Cold Storage");
				parent = mSs->getBlockByHash(mParentBlockID, result, instantiateTries);
				//mParentBlock = parent; *DO NOT* do this=> this might lead to unwanted circular ascendancies; parents will be assigned and cached EXPLICITLY
				if (result == eBlockInstantiationResult::OK)
				{
					result = eBlockInstantiationResult::eBlockInstantiationResult::blockFetchedFromSS;
				}
				//if (mBlock != nullptr)
				//{
				//parent->setNext(mBlock);// *DO NOT* do this = > this might lead to unwanted circular ascendancies; offspring will be assigned and cached EXPLICITLY
				//}
				return parent;
			}
			else result = eBlockInstantiationResult::eBlockInstantiationResult::Failure;
		// Fetch from Cold Storage - END

		//  Retrieve Block - END
	}
	// Operational Logic - END
	return parent;
}

std::shared_ptr<CBlock> CBlockHeader::getParentPtr()
{
	std::lock_guard<std::recursive_mutex> lock(mParentGuardian);
	return mParentBlock;
}

std::shared_ptr<CBlock> CBlockHeader::getParentPtrK()
{
	return mParentBlock;
}

void CBlockHeader::freeParent()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mParentBlock != nullptr)
	{
		mTools->writeLine("Removing block (" + mTools->base58CheckEncode(mParentBlock->getID()) + ") from Hot Storage");
		mParentBlock.reset();
		//mParentBlock = nullptr;
	}
}


std::vector<uint8_t> CBlockHeader::getUncleBlocksHash()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mUncleBlocksHash;
}



bool CBlockHeader::setTrieRoot(CTrieNode* root, eTrieID id)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	bool result = false;
	switch (id)
	{

	case transactions:
		result = mTransactionsDB->setPerspective(root->getHash());
		this->mTransactionsRootID = mTransactionsDB->getPerspective();
		break;

	case receipts:
		result = mReceiptsDB->setPerspective(root->getHash());
		this->mReceiptsRootID = mReceiptsDB->getPerspective();
		break;
	case verifiables:
		result = mVerifiablesDB->setPerspective(root->getHash());
		this->mVerifiablesRootID = mVerifiablesDB->getPerspective();
		break;
	default:
		return false;
	}


	return result;
}


bool CBlockHeader::setPerspective(std::vector<uint8_t> id, eTrieID trieid, bool isFinal,
	const std::vector<uint8_t>& parentBlockHash)
{
	// Validate input perspective
	if (id.size() != 32) {
		return false;
	}

	// If parent block hash is provided, it must be 32 bytes
	if (!parentBlockHash.empty() && parentBlockHash.size() != 32) {
		return false;
	}

	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	std::shared_ptr<CTools> tools = CTools::getInstance();

	switch (trieid)
	{
	case state:
	{
		// If parent block hash provided, convert Block Perspective to Trie Perspective
		std::vector<uint8_t> triePerspective = id;
		if (!parentBlockHash.empty()) {
			triePerspective = tools->blockPerspectiveToTriePerspective(id, parentBlockHash);
			if (triePerspective.empty()) {
				return false;
			}
		}

		if (isFinal) {
			mFinalStateRootID = triePerspective;
		}
		else {
			mInitiallStateRootID = triePerspective;
		}
	}
	break;

	case transactions:
		// Non-state tries must not provide parent block hash
		if (!parentBlockHash.empty()) {
			return false;
		}
		mTransactionsRootID = id;
		if (mTransactionsDB == nullptr) {
			return false;
		}
		return mTransactionsDB->setPerspective(mTransactionsRootID);

	case receipts:
		if (!parentBlockHash.empty()) {
			return false;
		}
		mReceiptsRootID = id;
		if (mReceiptsDB == nullptr) {
			return false;
		}
		return mReceiptsDB->setPerspective(mReceiptsRootID);

	case verifiables:
		if (!parentBlockHash.empty()) {
			return false;
		}
		mVerifiablesRootID = id;
		if (mVerifiablesDB == nullptr) {
			return false;
		}
		return mVerifiablesDB->setPerspective(mVerifiablesRootID);

	default:
		return false;
	}

	return true;
}




std::vector<uint8_t> CBlockHeader::getPerspective(eTrieID trie, bool getFinal, const std::vector<uint8_t>& parentBlockHash)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	// Get the Trie Perspective based on trie type
	std::vector<uint8_t> triePerspective;
	switch (trie) {
	case state:
		triePerspective = getFinal ? mFinalStateRootID : mInitiallStateRootID;
		break;
	case transactions:
		triePerspective = mTransactionsRootID;
		break;
	case receipts:
		triePerspective = mReceiptsRootID;
		break;
	case verifiables:
		triePerspective = mVerifiablesRootID;
		break;
	default:
		assertGN(false);
		return std::vector<uint8_t>();
	}

	// If no parent block hash provided or invalid size, return pure Trie Perspective
	if (parentBlockHash.empty() || parentBlockHash.size() != 32 || triePerspective.empty()) {
		return triePerspective;
	}

	// Create Block Perspective by XORing with parent hash
	// XOR operation is:
	// 1. Reversible (can recover Trie Perspective if parent hash is known)
	// 2. Deterministic
	// 3. Maintains 32-byte length
	// 4. Changes propagate through entire result
	std::vector<uint8_t> blockPerspective(32);
	for (size_t i = 0; i < 32; i++) {
		blockPerspective[i] = triePerspective[i] ^ parentBlockHash[i];
	}

	return blockPerspective;
}





std::vector<uint8_t> CBlockHeader::getLogsBloom()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mLogsBloom;
}

std::vector<uint8_t> CBlockHeader::getExtraData()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mExtraData;
}

BigInt CBlockHeader::getErgLimit()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mErgLimit;
}

BigInt CBlockHeader::getErgUsed()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mErgUsed;
}


std::vector<CVerifiable*> CBlockHeader::getBeneficiaries()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mBeneficiaries;
}
bool  CBlockHeader::setParentHash(std::vector<uint8_t> blockID)
{
	if (blockID.size() != 32)
		return false;

	std::lock_guard<std::recursive_mutex> lock(mParentGuardian);
	std::shared_ptr<CBlock> block = mBlock.lock();

	mParentBlockID = blockID;
	return true;
}

std::vector<uint8_t> CBlockHeader::getParentID()
{
	std::lock_guard<std::recursive_mutex> lock(mParentGuardian);
	return mParentBlockID;
}

size_t CBlockHeader::getNrOfTransactions()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mNrOfTransactions;
}

size_t CBlockHeader::getNrOfVerifiables()
{
	return mNrOfVerifiables;
}

size_t CBlockHeader::getNrOfReceipts()
{
	return mNrOfReceipts;
}

void CBlockHeader::setNrOfTransactions(size_t nr)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (isKeyBlock())
		return;
	mNrOfTransactions = nr;
}

void CBlockHeader::setNrOfVerifiables(size_t nr)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (isKeyBlock())
		return;
	mNrOfVerifiables = nr;
}

void CBlockHeader::setNrOfReceipts(size_t nr)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mNrOfReceipts = nr;
}

eBlockInfoRetrievalResult::eBlockInfoRetrievalResult CBlockHeader::getRewardInMiningVerifiable(BigInt& reward)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//we need to traverse the list of verifiables to get block reward (excluding ERG fees)
	std::shared_ptr<CBlock> block = mBlock.lock();
	if (block == nullptr)
		return eBlockInfoRetrievalResult::eBlockInfoRetrievalResult::blockBodyUnavailable;

	for (int i = 0; i < block->getVerifiablesIDs().size(); i++)
	{
		CVerifiable ver;
		if (!getVerifiable(block->getVerifiablesIDs()[i], ver))
			continue;
		if (ver.getVerifiableType() == eVerifiableType::minerReward)
			if (ver.getAffectedStateDomains().size() > 0)
				reward = static_cast<BigInt>((ver.getAffectedStateDomains()[0])->getPendingPreTaxBalanceChange());
			else
				reward = 0;
	}
	return eBlockInfoRetrievalResult::eBlockInfoRetrievalResult::OK;
}

std::vector<uint8_t> CBlockHeader::getHash(bool allowedCashed)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	if (allowedCashed && (mHashTemp.empty() == false))
	{
		return mHashTemp;
	}

	std::vector<uint8_t> temp;
	assertGN(getPackedData(temp));
	mHashTemp = mCf->getSHA2_256Vec(temp);
	return mHashTemp;
}
