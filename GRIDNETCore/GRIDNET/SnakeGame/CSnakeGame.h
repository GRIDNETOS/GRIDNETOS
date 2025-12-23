#pragma once
#include <string>
#include <memory>
#include <mutex>
#include "snailEnums.h"
struct Coin;

struct Player;
class CDTI;
class CTools;
class CTokenPool;
class CTokenPoolBank;

class CSnakeGame : public std::enable_shared_from_this<CSnakeGame>
{
private:
	std::mutex mFieldsGurdian;
	std::mutex mGuardian;
	std::shared_ptr<Coin> mCoin;
	std::shared_ptr<Player> mPlayer1;
	std::shared_ptr<Player> mPlayer2;
	std::shared_ptr<CDTI> mDTIPlayer1;
	std::shared_ptr<CDTI> mDTIPlayer2;
	bool mIsHost;
	int mLocalInput;
	int mEnd = MAX_GAMES / 2;
	Result mResult;
	std::shared_ptr<CTools> mTools;
	uint64_t mBankIDP1, mBankIDP2;
	std::shared_ptr<CTokenPool> mTokenPool;
	

	uint64_t mRewardPerCoin;//1 eaten coin = 1 GBU to be fetched from a Token-Pool
	uint64_t mRewardValueP1;
	uint64_t mRewardValueP2;
	std::shared_ptr<CTransmissionToken> mTTP1, mTTP2;// renewed on each consumption based on total acumulated reward value asigned to player
	
	bool mP1RewardsEnabled, mP2RewardsEnabled;
	

public:
	void setP1RewardsEnabled(bool isIt = true);
	void setP2RewardsEnabled(bool isIt = true);
	uint64_t getRewardValueP1();
	uint64_t getRewardValueP2();
	void setTTP1(std::shared_ptr<CTransmissionToken> tt);
	void setTTP2(std::shared_ptr<CTransmissionToken> tt);
	uint64_t getBankIDP1();
	uint64_t getBankIDP2();
	std::shared_ptr<CTokenPool> getTokenPool();
	bool getP1RewardsEnabled();
	bool getP2RewardsEnabled();

	void incP1ReportedRewardBy(uint64_t value);
	void incP2ReportedRewardBy(uint64_t value);

	std::shared_ptr<CTransmissionToken> getTTP1();
	std::shared_ptr<CTransmissionToken> getTTP2();
	void setRewardPerCoin(uint64_t reward);
	uint64_t getRewardPerCoin();
	std::shared_ptr <CTools> getTools();
	Result getResult();
	bool getIsHost();

	bool init();
	bool getIsMultiplayer();
	void generateC();
	bool gameLoop();
	std::shared_ptr<Coin> getCoin();
	CSnakeGame(std::string player1ID, std::string player2ID);
	static std::shared_ptr<CSnakeGame> makeInstance(std::shared_ptr<CDTI> dti1, std::shared_ptr<CDTI> dti2, bool isHost, std::shared_ptr<CTokenPool> tokenPool=nullptr);
	CSnakeGame(std::shared_ptr<CDTI> dti1, std::shared_ptr<CDTI> dti2, bool isHost, std::shared_ptr<CTokenPool> tokenPool=nullptr);

	void initFields();

	std::shared_ptr<Player>  getPlayer1();
	std::shared_ptr<Player> getPlayer2();
	std::shared_ptr<CDTI> getPlayer1DTI();
	std::shared_ptr<CDTI> getPlayer2DTI();
};
