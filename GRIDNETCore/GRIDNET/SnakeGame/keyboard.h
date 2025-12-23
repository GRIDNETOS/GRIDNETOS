#pragma once
#ifndef KEYBOARD_H_
#define KEYBOARD_H
#include <memory>
#include <string>
struct Player;
class CSnakeGame;

void parsePlayersInputs(std::shared_ptr<CSnakeGame> game);


#endif // KEYBOARD_H_

