#pragma once
#include "contact.h"
#include "TrieLeafNode.h"
#include "enums.h"

/// <summary>
/// Enables core-level accountability.
/// 
/// Accountability provided at the core level (the top-most State Domain) needs to be verified by each full-node.
/// CVerifiable can be used to either represent rewards OR penalties.
/// mProofID provides an identifier of a verifiable object/action. ex. ID an uncle block.
/// 
/// Verifiables are verified by CVerifier.
/// 
/// Verifiables are included inside of a block.
/// </summary>

class CStateDomain;
class CVerifiable : public CTrieLeafNode
{
private:
	std::vector<uint8_t> mSignature;//optional
	std::vector<uint8_t> mPubKey;//optional

	uint64_t mVersion;
	std::vector<uint8_t> mGUID;
	std::recursive_mutex mGuardian;
	uint64_t mErgPrice ;
	BigInt mErgLimit ;
	size_t mReceivedAt;//not serialized
	
	
	eVerifiableType::eVerifiableType mVerifiableType;
	//uint64_t mBalanceChange;
	void basicInit();
	//bool mIsPositive = false;
	std::vector<uint8_t> mProof;//provides proof that can be verified by each node ex. ID of an uncle block in case of block rewards
	//std::vector<std::vector<uint8_t>> mStateDomainIDs;//affected stateDomains
	std::vector<std::shared_ptr<CStateDomain>> mAffectedDomains;

	/**
	 * Non-serialized map of overrides for a domain's pending balance.
	 * Key: domain address (std::vector<uint8_t>)
	 * Value: the new "armed" pending balance change (BigSInt)
	 */
	std::map<std::vector<uint8_t>, BigSInt> mArmedBalanceOverrides;
	bool mIsArmed;
public:
	/// <summary>
	/// Explicitly arms a Verifiable.
	/// </summary>
	void arm();
	/**
	* Introduce a new method that allows external code to set an override
	* for the pending balance of a particular domain. These overrides
	* are used only at runtime, never serialized and thus not affect image/ hash of CVerifiable object.
	*
	* @param domainID The domain's address (ID)
	* @param newValue The new pending balance change to be used at runtime
	*/
	void armBalanceChange(const std::vector<uint8_t>& domainID, const BigSInt& newValue);

	std::vector<uint8_t> getPubKey();
	void setPubKey(std::vector<uint8_t> pubKey);
	std::vector<uint8_t> getSig();
	void setSignature(std::vector<uint8_t> sig);

	CVerifiable& operator = (const CVerifiable &t);
	uint64_t getErgPrice();
	BigInt getErgLimit();
	void setERGPrice(uint64_t price);
	void setERGLimit(BigInt limit);

	ColdStorageProperties ColdProperties;
	size_t getReceivedAt();
	void setReceivedAt(size_t time);
	bool isVerified();
	void markAsVerified();
	bool genID();
	std::vector<uint8_t> getGUID();
	bool prepare(bool store,bool includeSig=true);
	bool setPoFProof(std::vector<uint8_t> kbBody, std::vector<uint8_t> bh1Body, std::vector<uint8_t> bh2Body);
	bool getPoFProof(std::shared_ptr<CBlockHeader>& kbHeader, std::shared_ptr<CBlockHeader>& bh1, std::shared_ptr<CBlockHeader>& bh2, eBlockchainMode::eBlockchainMode blockchainMode);
	bool getPoFProof(std::vector<uint8_t>&kbBody, std::vector<uint8_t>&bh1Body, std::vector<uint8_t>&bh2Body);
	static CVerifiable * instantiateVerifiable(std::vector<uint8_t> serializedData, eBlockchainMode::eBlockchainMode mode);
	static CVerifiable * genNode(CTrieNode **node, bool useTestStorageDB = false);
	CVerifiable(const CVerifiable&);
	eVerifiableType::eVerifiableType getVerifiableType();
	std::vector<uint8_t> getProof();
	std::vector<uint8_t> getPackedData(bool includeSig=true);
	bool verifySignature(std::vector<uint8_t> publicKey=std::vector<uint8_t>());
	bool sign(Botan::secure_vector<uint8_t> privKey);
	CVerifiable();
	bool setGUID(std::vector<uint8_t> GUID);
	uint64_t getVersion();

	CVerifiable( eVerifiableType::eVerifiableType type, std::vector<uint8_t> proofID = std::vector<uint8_t>());
	bool addBalanceChange(std::vector<uint8_t> sdID,BigSInt value);



	/**
	 * Modified method that returns the list of affected state domains.
	 * By default, returns the "armed" (overridden) balances if any overrides exist.
	 * If `useEffectiveIfAvailable` is set to false, it returns the *original*
	 * serialized mAffectedDomains with no overrides applied.
	 *
	 * @param useEffectiveIfAvailable If true (default),
	 *                                the pending balance override is used if present.
	 * @return Vector of CStateDomain pointers
	 */
	std::vector<std::shared_ptr<CStateDomain>> getAffectedStateDomains(
		bool useEffectiveIfAvailable = true
	);

	void setProof(std::vector<uint8_t> proofID);
/// <summary>
/// Checks if any domain in this verifiable has armed (runtime) balance changes
/// </summary>
/// <returns>true if there are any armed balance changes, false otherwise</returns>
	bool isArmed();
	/// <summary>
/// Gets the armed (runtime) balance value for a specific domain if it exists
/// </summary>
/// <param name="domainID">The domain address to check for armed value</param>
/// <returns>The armed balance value if found, or the original balance value if not armed</returns>
	BigSInt getArmedValue(const std::vector<uint8_t>& domainID);


	
};
