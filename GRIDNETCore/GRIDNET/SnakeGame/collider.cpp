#include "collider.h"
#include "coin.h"
#include "player.h"

short calculateWallHit(std::shared_ptr<Player> player){
	if(player->tail[0].x >= MAX_COLUMN - 1
	|| player->tail[0].x <= 0
	|| player->tail[0].y >= MAX_ROW - 1
	|| player->tail[0].y <= 0){
		return 1;
	}
	return 0;
}

short calculateTailHit(std::shared_ptr<Player> p1, std::shared_ptr<Player> p2){
	int i = 0;
	int head_x = p1->tail[0].x;
	int head_y = p1->tail[0].y;

	if(p1->mark == p2->mark){
		i = 1;
	}

	for(; i <= p2->coin_score; i++){
		if(p2->tail[i].x == head_x && p2->tail[i].y == head_y){
			return 1;
		}
	}
	return 0;
}

short checkPlayerColisions(std::shared_ptr<Player> player, std::shared_ptr<Player> toPlayer, bool isMultiplayer){

    return calculateWallHit(player) || calculateTailHit(player, player)|| (isMultiplayer&&(
			 calculateTailHit(player, toPlayer) ));
}

/// <summary>
/// Check for colisions between players.
/// </summary>
/// <param name="p1"></param>
/// <param name="p2"></param>
/// <param name="isMultiplayer"></param>
/// <returns></returns>
Result checkForColisions(std::shared_ptr<Player> p1, std::shared_ptr<Player> p2, bool isMultiplayer){
	Result mResult = NONE;
	short player1_lose = checkPlayerColisions(p1, p2, isMultiplayer);
	short player2_lose = checkPlayerColisions(p2, p1, isMultiplayer);


	// Check if it is draw
	if(player1_lose && player2_lose){
		p1->life_score += 1;
		p2->life_score += 1;
		mResult = DRAW;
	}
	// Calculate wall and tail hit for both player
	else if(player2_lose){
		p1->life_score += 1;
		mResult = PLAYER1_WINS;
	}
	else if(player1_lose){
		mResult = PLAYER2_WINS;
		p2->life_score += 1;
	}

    return mResult;
}

/// <summary>
/// Check if player just colided with a coin.
/// </summary>
/// <param name="coin"></param>
/// <param name="player"></param>
/// <returns></returns>
short checkCoinColisions(std::shared_ptr<Coin> coin, std::shared_ptr<Player> player){
	short flag = 0;
	int i= player->coin_score;
	int j = i;
	if (coin->point.x == 0 && coin->point.y == 0)
		return 0;
	for(i = 0; i <= j; i++){
		if(player->tail[i].x == coin->point.x
		&& player->tail[i].y == coin->point.y){
			return 1;
		}
	}
	return 0;
}
