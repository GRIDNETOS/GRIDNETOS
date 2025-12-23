#pragma once
#ifndef COIN_H_
#define COIN_H_
#include <memory>
#include "Point.h"
struct Player;
class CSnakeGame;
struct Coin{
	Point point;
	short add_to_score;
};

// Init


void generateCoin(std::shared_ptr<CSnakeGame> game);
void add_coin_score(std::shared_ptr<CSnakeGame> game, std::shared_ptr <Player> player);
void collectCoin(std::shared_ptr<CSnakeGame> game);

#endif //COIN_H_
