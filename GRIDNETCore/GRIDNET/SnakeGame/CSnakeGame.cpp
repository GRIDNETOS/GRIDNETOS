#include "CSnakeGame.h"
#include "keyboard.h"
#include "coin.h"
#include"draw.h"
#include "player.h"
#include "collider.h"
#include "snailEnums.h"
#include <DTI.h>
#include "scriptengine.h"
#include "Tools.h"
#include "TokenPool.h"
#include "TokenPoolBank.h"
#include "TransmissionToken.h"

uint64_t CSnakeGame::getBankIDP1()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mBankIDP1;
}

uint64_t CSnakeGame::getBankIDP2()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mBankIDP2;
}


std::shared_ptr<CTokenPool> CSnakeGame::getTokenPool()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mTokenPool;
}

uint64_t CSnakeGame::getRewardValueP1()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mRewardValueP1;
}

uint64_t CSnakeGame::getRewardValueP2()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mRewardValueP2;
}
void CSnakeGame::setTTP1(std::shared_ptr<CTransmissionToken> tt)
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	mTTP1 = tt;

	if (mDTIPlayer1 != nullptr)
	{
		mDTIPlayer1->setPlayerTT(tt);
	}

	
}

void CSnakeGame::setTTP2(std::shared_ptr<CTransmissionToken> tt)
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	mTTP1 = tt;

	if (mDTIPlayer2 != nullptr)
	{
		mDTIPlayer2->setPlayerTT(tt);
	}

}

void CSnakeGame::setP1RewardsEnabled(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	 mP1RewardsEnabled = isIt;
}
void CSnakeGame::setP2RewardsEnabled(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	mP2RewardsEnabled = isIt;
}


bool CSnakeGame::getP1RewardsEnabled()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mP1RewardsEnabled;
}
bool CSnakeGame::getP2RewardsEnabled()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mP2RewardsEnabled;
}

void CSnakeGame::incP1ReportedRewardBy(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	mRewardValueP1 += value;
	if (mDTIPlayer1 != nullptr)
	{
		mDTIPlayer1->ringABell();
	}
}

void CSnakeGame::incP2ReportedRewardBy(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	mRewardValueP2 += value;
	if (mDTIPlayer2 != nullptr)
	{
		mDTIPlayer2->ringABell();
	}
}

std::shared_ptr<CTransmissionToken> CSnakeGame::getTTP1()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mTTP1;
}

std::shared_ptr<CTransmissionToken> CSnakeGame::getTTP2()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mTTP2;
}

void CSnakeGame::setRewardPerCoin(uint64_t reward)
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	mRewardPerCoin = reward;
}

uint64_t CSnakeGame::getRewardPerCoin()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mRewardPerCoin;
}

std::shared_ptr<CTools> CSnakeGame::getTools()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mTools;
}

Result CSnakeGame::getResult()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mResult;
}

bool CSnakeGame::getIsHost()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mIsHost;
}


bool  CSnakeGame::init()
{
	//std::lock_guard<std::mutex> lock(mFieldsGurdian);
	std::shared_ptr<CSnakeGame> tGame = shared_from_this();
	
	std::shared_ptr<CDTI> dtiP1 = getPlayer1DTI();
	std::shared_ptr<CDTI> dtiP2 = getPlayer2DTI();
	std::shared_ptr<CTransmissionToken> ttP1,ttP2;
	

	if (mIsHost || !getIsMultiplayer())
	{
		//Update Token Pool usage - BEGIN

		//ony the server instance actually affects the token-pool here false param
		if (dtiP1 != nullptr)
		{
			ttP1 = dtiP1->getPlayerTT();
			dtiP1->clearScreen();
		}

		if (dtiP2 != nullptr)
		{
			ttP2 = dtiP2->getPlayerTT();
			dtiP2->clearScreen();
		}
	
		if (ttP1)
		{//Player1
			if (!mTokenPool->validateTT(ttP1, true))
			{
				return false;
			}
		}

		if (ttP2)
		{//Player2
			if (!mTokenPool->validateTT(ttP2, true))
			{
				return false;
			}
		}
		//Update Token Pool usage - END
		
		if (ttP1 && ttP2)
		{
			if (ttP1->getBankID() == ttP2->getBankID())
			{
				return false;
			}
		}
	}
	
	// Initialization
	initPlayers(tGame);
	generateCoin(tGame);
	//draw(tGame);
	return true;
}

bool  CSnakeGame::getIsMultiplayer()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mDTIPlayer2 != nullptr;
}

void CSnakeGame::generateC()
{
	generateCoin(shared_from_this());
}

/// <summary>
/// Returns true if the Loop is to continue.
/// </summary>
/// <returns></returns>
bool CSnakeGame::gameLoop()
{
	std::shared_ptr<CDTI> dtiP1 = getPlayer1DTI();
	std::shared_ptr<CDTI> dtiP2 = getPlayer2DTI();

	//Local Variables - BEGIN
	char localInput = dtiP1->getRecentKey();
	std::shared_ptr<CSnakeGame> tGame = shared_from_this();
	bool showStats = true;
	//Local Variables - END

	if ((getPlayer1()->life_score <= mEnd) && (getPlayer2()->life_score <= mEnd))
	{
		mLocalInput = dtiP1->getRecentKey();
		// If key is ESC or CTRL^D
		if (mLocalInput == 0x1B || localInput == 0x04) {
			showStats = 0;
			return false;
		}
		// If key is ASCII
		//if (localInput > 0x20 && localInput < 0x7F) {
			parsePlayersInputs(shared_from_this());
		//}
		// Animate field and check for coin collect
		std::this_thread::sleep_for(std::chrono::microseconds(PLAYER_SPEED));
		if (getIsHost() || !getIsMultiplayer())
		{
			//server/game-host side logic- BEGIN
			collectCoin(tGame);
			moveTail(mPlayer1);
			if(getIsMultiplayer())
			moveTail(mPlayer2);

			
				if ((mResult = checkForColisions(mPlayer1, mPlayer2,getIsMultiplayer())) != NONE) {

					if (mDTIPlayer1 != nullptr)
					{
						mDTIPlayer1->ringABell();
					}
					if (mDTIPlayer2 != nullptr)
					{
						mDTIPlayer2->ringABell();
					}
					if (!drawResults(tGame))
						return false;

					resetPlayers(tGame);
					generateCoin(tGame);
				}
			
			
			//server/game-host side logic - END
		}

		//Draw the game-world - BEGIN
		if (getIsMultiplayer() && !getIsHost())
		{
			//in such a case it is the other peer which does the game-mechanics AND it also does rendering to both terminals
			//we simply do NOTHING except making keys available
			std::shared_ptr<CSnakeGame> peerInstance = std::static_pointer_cast<CSnakeGame>(dtiP2->getScriptEngine()->getNativeActiveApp());
			if (peerInstance == nullptr)
				return false;

			if (peerInstance->getResult() != Result::NONE)
				return false;
			//if (peerInstance == nullptr)
			//	return false;//error
			//draw(peerInstance,false); <= the host does all the rendering in both terminals
		}
		else
		{
			//we're either hosting the game or in single-payer mode.
			draw(tGame);
		}
		
		//Draw the game-world - BEGIN
		
	}

	//Show statistics  - BEGIN
	if (showStats) {
		//dtiP1->clearScreen();
		//if (dtiP2 != nullptr)
		//	dtiP2->clearScreen();

		//draw(tGame);
		//drawStats(tGame);
	
//		//return false;
	}
	//Show statistics  - END
	return true;
}

std::shared_ptr<Coin> CSnakeGame::getCoin()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mCoin;
}

CSnakeGame::CSnakeGame(std::string player1ID, std::string player2ID)
{
	initFields();
}

std::shared_ptr<CSnakeGame> CSnakeGame::makeInstance(std::shared_ptr<CDTI> dti1, std::shared_ptr<CDTI> dti2, bool isHost, std::shared_ptr<CTokenPool> tokenPool)
{
	if (dti1 == nullptr)
		return nullptr;

	std::shared_ptr<CSnakeGame> toRet = std::make_shared<CSnakeGame>(dti1, dti2, isHost,  tokenPool);
	return toRet;
}

CSnakeGame::CSnakeGame(std::shared_ptr<CDTI> dti1, std::shared_ptr<CDTI> dti2, bool isHost, std::shared_ptr<CTokenPool> tokenPool)
{
	initFields();
	mTokenPool = tokenPool;
	if (dti1 != nullptr && dti1->getPlayerTT() != nullptr)
	{
		mBankIDP1 = dti1->getPlayerTT()->getBankID();
		setP1RewardsEnabled(true);
	}
	if (dti2 != nullptr && dti2->getPlayerTT() != nullptr)
	{
		mBankIDP2 = dti2->getPlayerTT()->getBankID();
		setP2RewardsEnabled(true);
	}
	mDTIPlayer1 = dti1;
	mDTIPlayer2 = dti2;

	mIsHost = isHost;
	//[todo:CodesInChaos:low] OOP refactor
	mPlayer1->name = dti1->getUserID();
	
	if(dti2!=nullptr)
	mPlayer2->name = dti2->getUserID();
}

void CSnakeGame::initFields()
{
	mP1RewardsEnabled = false;
	mP2RewardsEnabled = false;
	mRewardValueP1 = 0;
	mRewardValueP2 = 0;
	mBankIDP1 = 0;
	mBankIDP2 = 0;
	mRewardPerCoin = 1;
	mCoin = std::make_shared<Coin>();
	mLocalInput = 0;
	mResult = Result::NONE;
	mPlayer1 = std::make_shared<Player>();
	mPlayer2 = std::make_shared<Player>();
	mIsHost = true;
	mTools = CTools::getInstance();
}

std::shared_ptr<Player> CSnakeGame::getPlayer1()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mPlayer1;
}

std::shared_ptr<Player> CSnakeGame::getPlayer2()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mPlayer2;
}

std::shared_ptr<CDTI> CSnakeGame::getPlayer1DTI()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mDTIPlayer1;
}

std::shared_ptr<CDTI> CSnakeGame::getPlayer2DTI()
{
	std::lock_guard<std::mutex> lock(mFieldsGurdian);
	return mDTIPlayer2;
}
