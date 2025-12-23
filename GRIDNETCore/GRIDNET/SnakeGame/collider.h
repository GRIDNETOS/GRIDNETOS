#pragma once
#include <memory>
#include "snailEnums.h"

struct Player;
struct Coin;

short calculateTailHit(std::shared_ptr<Player> p1, std::shared_ptr<Player> p2);
short calculateWallHit(std::shared_ptr<Player> player);
short checkPlayerColisions(std::shared_ptr<Player> player, std::shared_ptr<Player> toPlayer, bool isMultiplayer);
Result checkForColisions(std::shared_ptr<Player> p1, std::shared_ptr<Player> p2, bool isMultiplayer);
short checkCoinColisions(std::shared_ptr<Coin> coin, std::shared_ptr<Player> player);
