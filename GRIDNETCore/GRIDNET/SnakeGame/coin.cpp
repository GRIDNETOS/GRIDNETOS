#include "coin.h"
#include "player.h"
#include "collider.h"
#include <ctime>
#include "Point.h"
#include "snailEnums.h"
#include "CSnakeGame.h"
#include "TransmissionToken.h"
#include "TokenPool.h"

void generateCoin(std::shared_ptr<CSnakeGame> game){
	int add_to_score;
	static int coin_occurance = COIN_5_OCCURANCE;
	std::shared_ptr<Coin> coin = game->getCoin();
	// Define type of coin
	srand((unsigned)time(NULL));
	add_to_score = rand() % COIN_5_LIMIT;
	coin->add_to_score = (add_to_score < coin_occurance) ? 5 : 1;
	if(coin->add_to_score == 1){
		coin_occurance += COIN_5_OCCURANCE / 2;
	} else {
		coin_occurance = COIN_5_OCCURANCE;
	}

	srand((unsigned)time(NULL));
	coin->point.x = 1 + rand() % (MAX_COLUMN - 3);
	srand((unsigned)time(NULL));
	coin->point.y = 1 + rand() % (MAX_ROW - 3);
}

void addCoinTailScore(std::shared_ptr<CSnakeGame> game,std::shared_ptr<Player> player){
	std::shared_ptr<Coin> coin = game->getCoin();
   player->coin_score += coin->add_to_score;
   player->max_coin_score += coin->add_to_score;
    addTailChild(player);
}

/// <summary>
/// The function is responsible for collecting points (and possibly) crypto-rewards - if available.
/// If available, the function queries the available Multi-Dimensional Token-Pool for a Transmission Token.
/// The token is assigned to player as the current one and will be shown to him/her by the end of the Game.
/// </summary>
/// <param name="game"></param>
void collectCoin(std::shared_ptr<CSnakeGame> game){
    short collected = 0;
	std::shared_ptr<Player> p1 = game->getPlayer1();
	std::shared_ptr<Player> p2 = game->getPlayer2();

	std::shared_ptr<CTransmissionToken> ttP1;
	std::shared_ptr<CTransmissionToken> ttP2;

	std::shared_ptr<Coin> coin = game->getCoin();
    // First player
    if(checkCoinColisions(coin,p1) == 1){

		if (game->getP1RewardsEnabled())
		{
			ttP1 = game->getTokenPool()->getTTWorthValue(game->getBankIDP1(),game->getRewardPerCoin(),true);
			
			if (ttP1 != nullptr)
			{
				ttP1->setRevealedHashesCount(ttP1->getCurrentDepth());
				ttP1->setValue(game->getTokenPool()->getSingleTokenValue() * ttP1->getCurrentDepth());
				game->incP1ReportedRewardBy(game->getRewardPerCoin());
				game->setTTP1(ttP1);
				

			}
			else
			{
				//todo: notify about depletion
			}
		}

        addCoinTailScore(game, p1);
        collected = 1;
    }

    // Second player
	if (game->getIsMultiplayer())
	{
		if (checkCoinColisions(coin, p2) == 1) {

			if (game->getP2RewardsEnabled())
			{
				ttP2 = game->getTokenPool()->getTTWorthValue(game->getBankIDP2(), game->getRewardPerCoin(), true);

				if (ttP2 != nullptr)
				{
					ttP2->setRevealedHashesCount(ttP2->getCurrentDepth());
					ttP2->setValue(game->getTokenPool()->getSingleTokenValue() * ttP2->getCurrentDepth());
					game->incP2ReportedRewardBy(game->getRewardPerCoin());
					game->setTTP2(ttP2);
				}
				else
				{
					//todo: notify about depletion
				}
			}

			game->incP2ReportedRewardBy(game->getRewardPerCoin());
			addCoinTailScore(game, p2);
			collected = 1;
		}
	}
	
    // Generate new coin if coin was collected
	if(collected > 0){
		generateCoin(game);
	}
}
