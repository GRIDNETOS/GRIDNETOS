#pragma once
#ifndef DRAW_H_
#define DRAW_H_
#include <memory>
#include "snailEnums.h"
class CSnakeGame;
struct Player;

void draw(std::shared_ptr <CSnakeGame> game, bool isHost = true);
//void draw_menu_player(std::shared_ptr <Player> player);
void drawMenu(std::shared_ptr <CSnakeGame> game);
void drawBoard(std::shared_ptr <CSnakeGame> game, bool isHost=true);
void drawField(std::shared_ptr <CSnakeGame> game, int column, int row, std::string& rendering, bool isHost = true, const bool& wasDoubleCharDrawn = false);
short drawPlayer( std::shared_ptr <Player> player, int column, int row);
void drawResultBar( std::shared_ptr <CSnakeGame> game, std::string & rendering);
bool drawResults(std::shared_ptr <CSnakeGame> game);
void drawStats( std::shared_ptr <CSnakeGame> game);
void drawPlayerStatistics(std::shared_ptr<CDTI> dti, std::shared_ptr <Player> player, int life_lost);

#endif // DRAW_H_
