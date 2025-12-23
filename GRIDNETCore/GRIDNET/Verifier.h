#pragma once
class CVerifiable;
class CBlockchainManager;
class CBlock;
class CTransactionManager;
class CStateDomain;
#include <mutex>
class CVerifier {
public:
	CVerifier(std::shared_ptr<CBlockchainManager> bm);
	std::shared_ptr<CTransactionManager> getFlowManager();
	void setFlowManager(std::shared_ptr<CTransactionManager> fm);
	std::shared_ptr<CBlockchainManager> getBlockchainManager();
	bool verifyActuateAndArm( CVerifiable &verifiable,
		std::vector<std::shared_ptr<CStateDomain>> &affectedDomainStates,
		std::shared_ptr<CBlock> proposal =0,
		BigInt feesValidatedUpToNow =0,
		bool buildingBlock=false
	);
	bool enterFlow();
	bool exitFlow();
	bool isInFlow();
	void updateTM();


private:


	std::mutex mFieldsGuardian;
	std::recursive_mutex mGuardian;
	bool mInFlow;
	std::shared_ptr<CTransactionManager> mFlowManager;
	std::shared_ptr<CBlockchainManager>mBlockchainManager;
	std::shared_ptr<CTransactionManager> mCurrentTransactionsManager;
};
