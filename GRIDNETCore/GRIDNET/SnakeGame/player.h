#pragma once
#ifndef PLAYER_H_
#define PLAYER_H_
#include <memory>
#include <vector>
#include "Point.h"

typedef struct Player {
	std::vector<Point> tail;
	short move_x;
	short move_y;
	int coin_score;
	int max_coin_score;
	int life_score;
	std::string name;
	char mark;
	Player()
	{
		max_coin_score = 0;
		life_score = 0;
		coin_score = 0;
		tail.resize(1);
	}
} Player;

struct Point;


class CSnakeGame;

void addTailChild(std::shared_ptr<Player> player);
void moveTail(std::shared_ptr<Player> player);
void move_player(std::shared_ptr<Player> player, int x, int y);
void movePlayerUp(std::shared_ptr<Player> player);
void movePlayerBottom(std::shared_ptr<Player> player);
void movePlayerRight(std::shared_ptr<Player> player);
void movePlayerLeft(std::shared_ptr<Player> player);
void enter_player_name(std::shared_ptr<Player> player, char *default_name);
void resetPlayer(std::shared_ptr<Player> player, std::shared_ptr<Point> position, std::shared_ptr<Point> move_to);
void resetPlayers(std::shared_ptr <CSnakeGame> game);
void initPlayers(std::shared_ptr <CSnakeGame> game);

#endif
