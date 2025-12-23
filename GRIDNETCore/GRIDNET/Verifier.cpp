#include <vector>
#include <stdafx.h>
#include "Verifier.h"
#include "Verifiable.h"
#include "BlockchainManager.h"
#include "block.h"

#include "StateDomain.h"
#include "enums.h"
#include "GenesisRewards.h"
#include "CStateDomainManager.h"
#include "LinkContainer.h"
CVerifier::CVerifier(std::shared_ptr<CBlockchainManager> bm)
{
 assertGN(bm != NULL);
	mBlockchainManager = bm;
	mCurrentTransactionsManager = mBlockchainManager->getLiveTransactionsManager();
	mInFlow = false;
}

std::shared_ptr<CTransactionManager>  CVerifier::getFlowManager()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mFlowManager;
}

void CVerifier::setFlowManager(std::shared_ptr<CTransactionManager> fm)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mFlowManager = fm;
}


std::shared_ptr<CBlockchainManager> CVerifier::getBlockchainManager()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mBlockchainManager;
}
bool  CVerifier::verifyActuateAndArm(CVerifiable& verifiable,
	std::vector<std::shared_ptr<CStateDomain>>& affectedDomainStates,
	std::shared_ptr<CBlock> proposal,
	BigInt totalBlockRewardValidUptilNow, // to keep building up rewards issued as part of this block
	bool buildingBlock // so that we know when block is prepared vs verified
)
{
	// RAII Logging - BEGIN

	/*
		Important: totalBlockRewardValidUptillNow protects against abstract reward values. Ensures that rewards are verified locally.
		IF operators wants less reward - that's not an issue at all. Note however that this 'affects' Leader B ( receives 60% from what Operator A did).
	*/

	updateTM();

	// Local Verifiables - BEGIN
	CStateDomain* domain = nullptr;
	std::vector<uint8_t> perspective;
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector <uint8_t> minedBlockID;
	bool ok; double expectedDifficulty;
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();

	BigInt feesReward = 0;
	int minerRewards = 0;
	std::vector<uint8_t> rewardedSD, pubKey, sig, validVeneficiary, proof;
	std::vector<uint8_t> claimedBeneficiary;
	//CVerifiable ver;
	std::vector<uint8_t> hash, actualHash;
	//	BigInt pastEpochReward = 0;
	CGenesisRewards rewards(mBlockchainManager->getCryptoFactory(), mBlockchainManager->getTools(), mBlockchainManager->getMode());
	std::string factFileContent;
	std::shared_ptr<CTools> tools = mBlockchainManager->getTools();
	CVerifiable ver;
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
	std::stringstream debugLog;
	CProcessVerifiablesRAII loggerRAII(debugLog, tools);
#endif
	std::vector<std::shared_ptr<CStateDomain>> domains;
	std::shared_ptr<CBlockHeader> kbh, dbh1, dbh2;
	std::shared_ptr<CBlock> actualKeyBlock;
	std::vector<uint8_t> minersID;
	std::vector<uint8_t>value;
	std::shared_ptr<CLinkContainer> lc;

	BigInt totalFees = 0;
	BigInt reportedTotalBlockReward = 0;
	bool isCheckpointed = proposal->getIsCheckpointed();
	std::shared_ptr<CTransactionManager> flowManager = getFlowManager();
	// Local Verifiables - BEGIN

	// Operational Logic - BEGIN

#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
	debugLog << tools->getColoredString("[ Verify Actuate And Arm ]", eColor::orange) <<  
		"  called for  " << tools->getVerifiableTypeStr(verifiable.getVerifiableType()) << " in a " << std::string(isCheckpointed == false ? "Non - " : "") << "Checkpointed Block"
		<<"\n";
#endif

	assertGN(flowManager);
	switch (verifiable.getVerifiableType())
	{
		//RIGHT HERE we are only VERIFYING; not processing.
	case eVerifiableType::proofOfFraud:
		if (!verifiable.verifySignature())
			return false;
		//The faulty block cannot be 'in the future'.
		//The faulty key-block needs to be within the main-chain
		//otherwise we would be generating money out-of-the-air.
		//and this could affect the incentives.
		//the key-block header is going to stay within the proof-of-fraud; still this introduces some redundancy
		//considering the current reward-mechanics i.e. key-block could be simply retrieved by its height.

		if (!verifiable.getPoFProof(kbh, dbh1, dbh2, mBlockchainManager->getMode()))
			return false;//unable to instantiate block-headers from within the Proof-of-Fraud; all 3 are needed. kbh contains public key and PoW (spam protection)

		actualKeyBlock = mBlockchainManager->getKeyBlockAtKeyBlockHeight(kbh->getKeyHeight(), false);

		if (actualKeyBlock == nullptr)
			return false;

		if (!mBlockchainManager->getTools()->compareByteVectors(actualKeyBlock->getHeader()->getHash(), kbh->getHash()))
			return false;//the block is different in the current main-chain;sry..


		//check if a Link for the current PoF was not created already within the current Flow
		//meaning the PoF for the very same Fraud was already processed thus we need to pass on the verifiable
		lc = std::make_shared< CLinkContainer>(tools->getProofOfFraudID(kbh->getHash()), value, eLinkType::eLinkType::PoFIDtoReceiptID);
		if (flowManager->isLinkInFlow(lc))
			return false;

		//1) check if the miner was not already penalized for this fraud
		//i.e. check if a Link with the Proof-of-Fraud IS is present - this will suffice.
		if (mBlockchainManager->getSolidStorage()->loadLink(tools->getProofOfFraudID(kbh->getHash()), value, eLinkType::eLinkType::PoFIDtoReceiptID))
			return false;//already processed and reported by somebody else; sry; <= redundant PoF spotted when verifying a verifiable
		//redundant fraud might be detected when
		//1) Verifying received Verifiable during block-formation - in such a case the verifiable would be committed
		//2) When receding a data-block, verifying the block it might turn out that it is fraudulent and thus a PoF would be created and broadcast


		//2) issue a penalty - i.e.no need to check if the asset-lock-time period expired.
		//even if the lock-time expired, we'll deduct assets from miner's account IF available (small chances), but doest cost us more than a few more CPU cycles.
		totalFees = kbh->getPaidToMiner();
		if (totalFees == 0)
			return false;//there's no point

		minersID = kbh->getMinersID();
		domain = mBlockchainManager->getStateDomainManager()->findByID(minersID);

		if (domain == nullptr)
			return false;//as far as we're concerned in the current chain the miner DOES NOT EXIST
		//we DO NOT process anything backwards, we do not re-process what already stored within the chain, thus the PoF is INVALID as 
		//far as we're concerned.
		else  
		{
			verifiable.arm();//  Arm the Varifiable explicitly (instead of relying on unitary armed balance changes the way mining rewards do).
			//					todo: Refactor this to rely on pending balance changes mechanics as well.

			return true;//do not check leader's balance; even if there's nothing to deduct etc.
		}
		//we want to mark the PoF as already processed; maybe in the future we'll implement some additional rewards for those who reported in the past
		//etc. etc.
		//The Proof-of-Fraud was decided to be VALID
		break;

	case eVerifiableType::GenesisRewards:
		if (!proposal->isGenesis())
			return false;//genesis-verifiable allowed only in Genesis Block

		//GENESIS-REWARD BEGIN
		tools->writeLine(tools->getColoredString("Processing Genesis Rewards..", eColor::lightCyan));
		factFileContent = CTools::getTools()->bytesToString(verifiable.getProof());

		tools->writeLine("Attempting to validate and parse the Genesis JSON facts-file..");
		//verify hash

		if (CGlobalSecSettings::getBase58GenesisFactFileHash().size() > 0)
		{

			if (!tools->base58CheckDecode(CGlobalSecSettings::getBase58GenesisFactFileHash(), hash))
				return false;

			actualHash = mBlockchainManager->getCryptoFactory()->getSHA2_256Vec(tools->stringToBytes(factFileContent));
			if (!tools->compareByteVectors(hash, actualHash))
			{
				tools->writeLine(tools->getColoredString("Invalid hash of the Genesis facts-file!", eColor::cyborgBlood));
				return false;
			}
			else
			{
				tools->writeLine(tools->getColoredString("Integrity of the Genesis facts-file VERIFIED!", eColor::lightGreen));
			}

		}
		//GENESIS-REWARD END
		if (!rewards.parse(factFileContent))
		{
			tools->logEvent("Invalid semantics of the Genesis facts-file. Won't be processed..", eLogEntryCategory::VM, 1, eLogEntryType::failure);
			return false;
		}

		if (!rewards.getGenesisStateDomains(domains))
		{
			tools->logEvent("Invalid semantics of the Genesis facts-file. Won't be processed..", eLogEntryCategory::VM, 1, eLogEntryType::failure);
			return false;
		}

		tools->writeLine("The retrieval was successful. (" + std::to_string(domains.size()) + " winners found )");

		if (!rewards.getGenesisStateDomains(affectedDomainStates))
			return false;

		verifiable.arm();//  Arm the Varifiable explicitly (instead of relying on unitary armed balance changes the way mining rewards do).
		//					todo: Refactor this to rely on pending balance changes mechanics as well.
		return true;


		break;

	case eVerifiableType::minerReward:
	{
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
		debugLog << tools->getColoredString("[ Verifying Operator's Reward ]\n", eColor::orange);
#endif
		// [ IMPORTANT ]: -  this verifiable needs to be the last one processed.
		// A Priori: verifiable.getAffectedStateDomains()[0]->getPendingBalanceChange() needs to equal 'myReward',
		//           which is the reward for both Epochs.
		// 
		// While it's contained only in key-blocks, 
		// it describes rewards for data-blocks (through past Epoch mechanics), as well as, key-blocks.

		minedBlockID = verifiable.getProof(); // subject

		eBlockInstantiationResult::eBlockInstantiationResult res;

		if (verifiable.getAffectedStateDomains().size() != 1) {
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
			debugLog << tools->getColoredString("[ ERROR ] Invalid number of affected state domains\n", eColor::cyborgBlood)
				<< " - Expected: 1\n"
				<< " - Actual: " << verifiable.getAffectedStateDomains().size() << "\n";
#endif
			return false; //only a single beneficiary is to receive the main mining reward
		}

		int minerRewards = 0;
		for (int i = 0; i < proposal->getVerifiablesCount(); i++)
			if (proposal->getVerifiable(proposal->getVerifiablesIDs()[i], ver))
				if (ver.getVerifiableType() == eVerifiableType::minerReward)
					minerRewards++;

		if (minerRewards > 1) {
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
			debugLog << tools->getColoredString("[ ERROR ] Multiple miner rewards detected\n", eColor::cyborgBlood)
				<< " - Number of rewards found: " << minerRewards << "\n"
				<< " - Only one miner reward verifiable is allowed per block\n";
#endif
			return false; //there can be only one
		}

		// IMPORTANT: all the accomulation of verified rewards is to be performed during either block formation of processing (verification).
		// here, only a copy is provided. 

		// Operator's Reward Amount Validation Logic - BEGIN
		// Gist: does the localy computed amount match fields found in both block's header and within of the Verifiable?
		//if (!isCheckpointed) {
			/*
				Notice that checkpoints override checks below.
				If checkpointed, block would be re-procedded accoring to CURRENT issuance rules.
				Reason: on 1/13/2025 we've fixed issuance of 90GNC rewards. Previously Operators received less.
						we could this not trust data present in verifiabels (the pending balance change fields of verifiabels)
						since we wanted to make Operators whole (operators who have been mining prior to 1/13/2025) as well.

						In the future problems might arise when we decide (for whatever the reason to nerf Operators' rewards).
						In such a case we might need to trust fields in verifiables from the past and proceed accordingly.
			*/



		// If the block is not checkpointed, we apply the validation rules.
		// We validate the integrity of pending domain balance changes as reported by the verifiables.
		// Note: Values within verifiables do not affect the outcome in this scenario.

		// [SECURITY]: Ensure the value in Miner's Verifiable matches the total block reward in the block header.
		//             Both values are compared and cross-validated against the locally computed total reward.

		// Retrieve the total block reward from the block's header
		BigInt totalBlockRewardInHeader = proposal->getHeader()->getTotalBlockReward(); // Note: this includes Tax; there's a dedicated Tax-exclusive header-field mPaidToMiner

		// Validate the pending balance change against the locally computed total block reward so far

		// [ SECURITY ]: pendingBalanceChange == totalBlockRewardValidUptilNow needs to hold even for checkpointed blocks.
		//				 If in the future we change rewards this needs to happen thorugh a custom key-block height-based compatibility layer
		//				 right above, which would affect the totalBlockRewardValidUptilNow for it to match totalBlockRewardValidUptilNow the value found in Verifiable /block at  a given height.
		//			     For any non checkpointed block it is always checked for the amount in header and Verifiable (pending change assigned in a serialized Verifiable)
		//				 to match the dynamically, locally computed amount.
		// 
		// [ Exception ]: only once have we overriden all rewards issuance for them to be compatible with Docs (90GNC for 2 Epochs).
		//			      This affected all Key Blocks right from the Genesis Block. Luckily, the change was about increasing supply on accounts rather than decreasing ( otherwise) prior
		//				  transfers would have failed.
		//
		auto affectedStateDomains = verifiable.getAffectedStateDomains();
		if (!affectedStateDomains.empty()) { // Ensure there are affected domains to process
			BigSInt pendingBlockRewardBeforeTax = totalBlockRewardValidUptilNow;

#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
			debugLog << " - Original Operator's Reward (in Verifiable): " << tools->formatGNCValue(affectedStateDomains[0]->getPendingPreTaxBalanceChange()) << "\n"
				<< " - Original Operator's Reward (in header): " << tools->formatGNCValue(totalBlockRewardInHeader) << "\n"
				<< " - Effective Reward (before Tax): " << tools->formatGNCValue(pendingBlockRewardBeforeTax) << "\n";
#endif

			
			if (pendingBlockRewardBeforeTax != totalBlockRewardValidUptilNow) {
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
				debugLog << tools->getColoredString("[ ERROR ] Pending (before-Tax) block reward mismatch\n", eColor::cyborgBlood)
					<< " - Block: " << tools->formatGNCValue(totalBlockRewardValidUptilNow) << "\n"
					<< " - Actual: " << tools->formatGNCValue(pendingBlockRewardBeforeTax) << "\n";
#endif
				return false;
			}

			// Post Tax Verification - BEGIN
			BigInt tax = 0;

			//Calculate Tax - BEGIN 
			double taxRate = CGlobalSecSettings::getMiningTax(proposal->getHeader()->getKeyHeight());
			if (pendingBlockRewardBeforeTax > 101) // avoid issues with 1%-100% rounding
			{
				if (taxRate >= 0.0 && taxRate <= 1.0) {
					long long taxFrac = static_cast<long long>(std::llround(taxRate * 1000000.0));
					BigInt bigFrac(taxFrac);
					BigInt bigDen(1000000);

					// Cast pendingBalanceChange to BigInt for calculation
					BigInt pendingBalanceBI = static_cast<BigInt>(pendingBlockRewardBeforeTax);
					tax = (pendingBalanceBI * bigFrac) / bigDen;
				}
			}

			BigInt effectiveRewardAfterTAX = static_cast<BigInt>(pendingBlockRewardBeforeTax) - tax;
			//Calculate Tax - END
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
			debugLog << " - Effective Operator's Reward (after Tax): " << tools->formatGNCValue(effectiveRewardAfterTAX) << "\n"
				<< " - Applied Tax: " << tools->formatGNCValue(tax) << " Rate: " << tools->doubleToString(taxRate, 2) << "\n";
#endif


			// Integrity Checks - BEGIN
			// For any blocks issues after first Hard Fork in 2025 the below checks need to succeed.
			// For earlier blocks - Operators' Rewards are issued according to NEW heuristics (yes, we override values found in Verifiables AND block headers).
			// If ever, in the future, we find ourselves in need of modifying Operators' Rewards at Key Height KX- we would include a compatibility layer, right above.
			// The Compatibility layer would issue rewards
			/*
			*    1) According to existing rules up to Key Height KX
			*    2) keep in mind that Values in Verifiables and block headers are to be assumed as valid only after the first Hard Fork in 2025 (90GNC per key-block).
			*    3) earlier there might be values such as 20GNC per block (right after Genesis Block)- we've overridden this.
			*    4) so in short - only after the 2025 Hard Fork - can rewards be issued according to block headers and Verifiables.
			*    5) IMPORTANT:  always re-compute values locally. NEVER trust member fields of headers / verifiables.
			*/
			if (!isCheckpointed)
			{

				if (effectiveRewardAfterTAX != proposal->getHeader()->getPaidToMiner()) {
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
					debugLog << tools->getColoredString("[ ERROR ] Post-tax reward mismatch\n", eColor::cyborgBlood)
						<< " - Header: " << tools->formatGNCValue(proposal->getHeader()->getPaidToMiner()) << "\n"
						<< " - Effective: " << tools->formatGNCValue(effectiveRewardAfterTAX) << "\n";
#endif
					return false;
				}

				// Non Checkpointed Procesing - BEGIN
				BigInt headerPaid = proposal->getHeader()->getPaidToMiner();
				BigInt headerTotalReward = proposal->getHeader()->getTotalBlockReward();
				BigSInt verifiableValue = affectedStateDomains[0]->getPendingPreTaxBalanceChange();

#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
				debugLog << tools->getColoredString("[ Performing Operator Rewards Integrity Checks ]\n", eColor::orange)
					<< " - Header Paid to Miner: " << tools->formatGNCValue(headerPaid) << "\n"
					<< " - Header Total Reward: " << tools->formatGNCValue(headerTotalReward) << "\n"
					<< " - Verifiable Value: " << tools->formatGNCValue(verifiableValue) << "\n"
					<< " - Computed Pending Balance Change: " << tools->formatGNCValue(pendingBlockRewardBeforeTax) << "\n"
					<< " - Computed Effective Reward After Tax: " << tools->formatGNCValue(effectiveRewardAfterTAX) << "\n";
#endif
				// Integrity Checks - BEGIN
				if (
					(pendingBlockRewardBeforeTax != headerTotalReward)
					|| (headerPaid != effectiveRewardAfterTAX)
					|| (verifiableValue != headerTotalReward)
					)
				{
				
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
					debugLog << tools->getColoredString("[ ERROR ] Block Rewards Integrity checks failed\n", eColor::cyborgBlood)
						<< "Mismatch Details:\n";

					if (pendingBlockRewardBeforeTax != headerTotalReward) {
						debugLog << " - Total Reward Mismatch:\n"
							<< "   Expected (Header): " << tools->formatGNCValue(headerTotalReward) << "\n"
							<< "   Actual (Computed): " << tools->formatGNCValue(pendingBlockRewardBeforeTax) << "\n";
					}

					if (headerPaid != effectiveRewardAfterTAX) {
						debugLog << " - Effective Reward Mismatch:\n"
							<< "   Expected (Header Paid): " << tools->formatGNCValue(headerPaid) << "\n"
							<< "   Actual (After Tax): " << tools->formatGNCValue(effectiveRewardAfterTAX) << "\n";
					}

					if (verifiableValue != headerTotalReward) {
						debugLog << " - Verifiable Value Mismatch:\n"
							<< "   Expected (Header Before Tax): " << tools->formatGNCValue(headerTotalReward) << "\n"
							<< "   Actual (Verifiable Before Tax): " << tools->formatGNCValue(verifiableValue) << "\n";
					}

					debugLog << tools->getColoredString("[ Key Height: " + std::to_string(proposal->getHeader()->getKeyHeight()) + " ]\n", eColor::orange);
#endif
					return false;
					
				}


				// Ensure the reported total block reward in the verifiable matches the header value
				if (totalBlockRewardInHeader != totalBlockRewardValidUptilNow) {
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
					debugLog << tools->getColoredString("[ ERROR ] Total block reward mismatch\n", eColor::cyborgBlood)
						<< " - Header value: " << tools->formatGNCValue(totalBlockRewardInHeader) << "\n"
						<< " - Computed value: " << tools->formatGNCValue(totalBlockRewardValidUptilNow) << "\n";
#endif
					return false;
				}
				// Integrity Checks - END

#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
				
					debugLog << tools->getColoredString("[ Arming Balance Change ]\n", eColor::orange)
						<< " - Operator Address: " << tools->bytesToString(affectedStateDomains[0]->getAddress()) << "\n"
						<< " - Prior Balance Change Value: " << tools->formatGNCValue(affectedStateDomains[0]->getPendingPreTaxBalanceChange()) << "\n"
						<< " - Arming Balance Change To: " << tools->formatGNCValue(totalBlockRewardValidUptilNow) << "\n";

					verifiable.armBalanceChange(affectedStateDomains[0]->getAddress(), totalBlockRewardValidUptilNow);

					debugLog << " - Armed Status: " << (verifiable.isArmed() ? tools->getColoredString("ARMED", eColor::lightGreen) : tools->getColoredString("NOT ARMED", eColor::cyborgBlood)) << "\n"
						<< " - Armed Value Confirmed: " << tools->formatGNCValue(verifiable.getArmedValue(affectedStateDomains[0]->getAddress())) << "\n"
						<< tools->getColoredString("[ SUCCESS ] Block Rewards Integrity checks passed\n", eColor::lightGreen);
				

#endif
				// Non Checkpointed Procesing - END
			}
			else
			{// Checkpointed Procesing - BEGIN (integrity checks disabled)
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
				debugLog << tools->getColoredString("[ Arming ] non-conditional Checkpointed "+ (proposal->getHeader()->isKeyBlock() ? tools->getColoredString("Key", eColor::orange):
					tools->getColoredString("Data", eColor::blue)) + " Block Balance Change\n", eColor::ghostWhite)
					<< " - Operator Address: " << tools->bytesToString(affectedStateDomains[0]->getAddress()) << "\n"
					<< " - Original Balance Change Value: " << tools->formatGNCValue(affectedStateDomains[0]->getPendingPreTaxBalanceChange()) << "\n"
					<< " - About to Arm Balance Change To: " << tools->formatGNCValue(totalBlockRewardValidUptilNow) << "\n";
#endif

				// IMPORTANT: we employ the concept of 'armed' (i.e. runtime actuated pending balance changes).
				//           These armed values are runtime only and do NOT affect CVerifiable's persistent values (and thus its image) in any way.
				verifiable.armBalanceChange(affectedStateDomains[0]->getAddress(), totalBlockRewardValidUptilNow);// introduced in 2025 Hard Fork

#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
				debugLog << " - Armed Status: " << (verifiable.isArmed() ? tools->getColoredString("ARMED", eColor::lightGreen) : tools->getColoredString("NOT ARMED", eColor::cyborgBlood)) << "\n"
					<< " - Armed Value Confirmed: " << tools->formatGNCValue(verifiable.getArmedValue(affectedStateDomains[0]->getAddress())) << "\n"
					<< tools->getColoredString("[ SUCCESS ] Balance Change Armed for Checkpointed Block\n", eColor::lightGreen);
#endif

				// so the below would be WRONG.
				//affectedStateDomains[0]->setPendingBalanceChange(totalBlockRewardValidUptilNow); // introduced in 2025 Hard Fork


				// during that Hard Fork we've overriden all prior Operators' Rewards to strictly adhere to official documentation.

				// ACTUATE Pending Reward - BEGIN

				// Hard Fork Backwards Compatibilty Support - BEGIN
				// here, introduce any changes based on Key Block Height


				// [ REWARD ACTUATION TEMPLATE ] - BEGIN
				/*
				uint64_t keyHeight = proposal->getHeader()->getKeyHeight();
				if (keyHeight < HARD_FORK_POONT)
				{
					// another Operators' Reward
				}
				else if (keyHeight < PRIOR_HARD_FORK_POINT)
				{
					// another Operators' Reward
				}
				*/
				// [ REWARD ACTUATION TEMPLATE ] - END

				// Hard Fork Backwards Compatibilty Support - END



				// ACTUATE Pending Reward - END

				

				// Checkpointed Procesing - END
			}

		

			// Integrity Checks - END

			// Post Tax Verification - END
		}
		else {
#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
			debugLog << tools->getColoredString("[ ERROR ] No affected state domains found\n", eColor::cyborgBlood);
#endif
			return false;
		}

		

#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
		//else {
		//	debugLog << tools->getColoredString("[ INFO ] Skipping validation for checkpointed block\n", eColor::lightCyan);
		//}
#endif
		// Operator's Reward Amount Validation Logic - END

#if ENABLE_FLOW_DETAILED_LOGS_VERIFIER == 1
		debugLog << tools->getColoredString("[ SUCCESS ] Operator reward verification completed\n", eColor::lightGreen);
#endif

		return true;
	}

	break;

	case eVerifiableType::uncleBlock:
		break;
	case eVerifiableType::dataPropagation:
		break;
	case eVerifiableType::powerTransit:
		break;
	case eVerifiableType::powerGeneration:
		break;
	}


	// Operational Logic - END
	return false;
}


bool CVerifier::enterFlow()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mInFlow)
		return false;
	mInFlow = true;
	mCurrentTransactionsManager = getFlowManager();
	assertGN(mCurrentTransactionsManager != nullptr);
	return true;
}

bool CVerifier::exitFlow()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!mInFlow)
		return false;
	mCurrentTransactionsManager = mBlockchainManager->getLiveTransactionsManager();
	mInFlow = false;
	return true;
}

bool CVerifier::isInFlow()
{
	return mInFlow;
}

void CVerifier::updateTM()
{
	if (mCurrentTransactionsManager == NULL)
	{
		if (mInFlow)
			mCurrentTransactionsManager = getFlowManager();
		else
			mCurrentTransactionsManager = mBlockchainManager->getLiveTransactionsManager();
	}
}


