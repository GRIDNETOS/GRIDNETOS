#pragma once
#include <memory>
#include "enums.h"
#include "IManager.h"
#include <mutex>
#include "oclengine.h"
#include "WorkManager.h"
class CBlockchainManager;
class CGRIDNET :public std::enable_shared_from_this<CGRIDNET>, public IManager
{
private:
	static std::mutex sInstanceGuardian;
	static std::shared_ptr<CGRIDNET> sInstance;
	static std::shared_ptr<CGRIDNET> sInstanceHodler;
	static std::shared_ptr<CTools> sTools;
	
	static std::mutex sWorkManagerGuardian;
	static std::mutex sToolsGuardian;
	static std::shared_ptr<COCLEngine> sOCLEngine;
	static std::shared_ptr<CWorkManager> mWorkManager;
	static void showStatusBar(bool showIt = true);
	static bool getShowStatusBar();
	 std::mutex mStatusGuardian;
	std::shared_ptr<CBlockchainManager> mLocalTestBlockchainManager;

	std::shared_ptr<CBlockchainManager>  mLiveBlockchainManager;
	std::shared_ptr<CBlockchainManager>  mLiveSandBoxBlockchainManager;

	std::shared_ptr<CBlockchainManager>  mTestNetBlockchainManager;
	std::shared_ptr<CBlockchainManager>  mTestNetSandBoxBlockchainManager;
	eManagerStatus::eManagerStatus mStatus;
	eManagerStatus::eManagerStatus mStatusChange;
	std::mutex mStatusChangeGuardian;
	std::mutex mShuttingDownGuardian;
	bool mShuttingDown;
	std::mutex mFieldsGuardian;
	std::vector<uint8_t> mModesPendingInitialization;
	std::vector<uint8_t> mSelfImage;
public:
	std::vector<uint8_t> getSelfy();
	void setSelfy(const std::vector <uint8_t> &img);
	bool getIsShuttingDown();
	void setIsShuttingDown(bool isIt = true);
	static std::shared_ptr<CGRIDNET> getInstance();
	static std::shared_ptr<CTools> getTools();
	static std::shared_ptr<CWorkManager> getWorkManager();
	std::shared_ptr<CBlockchainManager>  getBlockchainManager(eBlockchainMode::eBlockchainMode blockchainMode,bool initializeIfNeeded=false);
	bool shutdown(bool doExit=true);
	bool initialize(eBlockchainMode::eBlockchainMode blockchainMode);
	bool runOneTimeMiningTest();
	bool runLocalTests();
	CGRIDNET();
	~CGRIDNET();

	void markModeForInitialization(eBlockchainMode::eBlockchainMode mode);
	bool getIsModeToBeOperational(eBlockchainMode::eBlockchainMode mode);
	// Inherited via IManager
	virtual void stop() override;

	virtual void pause() override;

	virtual void resume() override;

	virtual eManagerStatus::eManagerStatus getStatus() override;

	virtual void setStatus(eManagerStatus::eManagerStatus status) override;


	// Inherited via IManager
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status) override;

	virtual eManagerStatus::eManagerStatus getRequestedStatusChange() override;

};