#include "player.h"
#include "CSnakeGame.h"
#include "Point.h"
#include "snailEnums.h"

void addTailChild(std::shared_ptr<Player> player){
	int score = player->coin_score;
	Point *p_last, p_before;
	if ((player->tail.size() - 1) < score)
		player->tail.resize(score + 5);

	p_last = &(player->tail[score]);
	p_before = player->tail[score - 1];

	p_last->x = p_before.x;
	p_last->y = p_before.y;
}

void moveTail(std::shared_ptr<Player> player){
	int i = player->coin_score;
	for(; i > 0; i--){
		player->tail[i].x = player->tail[i-1].x;
		player->tail[i].y = player->tail[i-1].y;
	}
	player->tail[0].x += player->move_x;
	player->tail[0].y += player->move_y;
}

void move_player(std::shared_ptr<Player> player, int x, int y){
	if(x != 0 && player->move_x == -x){
		return;
	} else if(y != 0 && player->move_y == -y){
		return;
	}
	player->move_x = x;
	player->move_y = y;
}

void movePlayerUp(std::shared_ptr<Player> player){
    move_player(player, 0, -1);
}

void movePlayerBottom(std::shared_ptr<Player> player){
    move_player(player, 0, 1);
}

void movePlayerRight(std::shared_ptr<Player> player){
    move_player(player, 1, 0);
}

void movePlayerLeft(std::shared_ptr<Player> player){
    move_player(player, -1, 0);
}

void resetPlayer(std::shared_ptr<Player> player, Point position, Point move_to){
	player->tail = std::vector<Point>(TAIL_SIZE);// *sizeof(Point));
	player->tail[0].x = position.x;
	player->tail[0].y = position.y;
	player->move_x = move_to.x;
	player->move_y = move_to.y;
	player->coin_score = 0;
}

void resetPlayers(std::shared_ptr <CSnakeGame> game){
	std::shared_ptr<Player> p1 = game->getPlayer1();
	std::shared_ptr<Player> p2 = game->getPlayer2();
	Point position, move_to;

	position.x = 1;
	position.y = 1;

	move_to.x = 1;
	move_to.y = 0;

	resetPlayer(p1, position, move_to);

	position.x = MAX_COLUMN - 2;
	position.y = MAX_ROW - 2;

	move_to.x = -1;
	move_to.y = 0;

	resetPlayer(p2, position, move_to);
}

void initPlayers(std::shared_ptr <CSnakeGame> game){
	resetPlayers(game);

	std::shared_ptr<Player> player1 = game->getPlayer1();
	std::shared_ptr<Player> player2 = game->getPlayer1();

	player1->mark = 'A';
	player1->max_coin_score = 0;
	player1->life_score = 0;

	player2->mark = 'B';
	player1->max_coin_score = 0;
	player1->life_score = 0;

}

