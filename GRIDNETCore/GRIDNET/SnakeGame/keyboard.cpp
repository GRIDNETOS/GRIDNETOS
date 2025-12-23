#include "keyboard.h"
#include "player.h"
#include <string>
#include <memory>
#include "CSnakeGame.h"
#include "DTI.h"


void parsePlayersInputs(std::shared_ptr<CSnakeGame> game)
{
	//Fetch Players' details - BEGIN
	std::shared_ptr<Player> player1 = game->getPlayer1();
	std::shared_ptr<Player> player2 = game->getPlayer2();

	std::shared_ptr<CDTI> player1DTI = game->getPlayer1DTI();
	std::shared_ptr<CDTI> player2DTI = game->getPlayer2DTI();
	char inputPlayer2 = '0';

	char inputPlayer1 = player1DTI->getRecentKey();
	if (player2DTI != nullptr)
	 inputPlayer2 = player2DTI->getRecentKey();
	//Fetch Players' details - END

	//Handle Players' Inputs - BEGIN

	//Player 1 Input - BEGIN
	switch(inputPlayer1)
	{
		case 'w':
            movePlayerUp(player1);
			break;
		case 'a':
            movePlayerLeft(player1);
			break;
		case 's':
            movePlayerBottom(player1);
			break;
		case 'd':
            movePlayerRight(player1);
			break;
	}
	//Player 1 Input - END

	//Player 2 Input - BEGIN
	if (player2DTI != nullptr)
	switch (inputPlayer2)
	{
	case 'w':
		movePlayerUp(player2);
		break;
	case 'a':
		movePlayerLeft(player2);
		break;
	case 's':
		movePlayerBottom(player2);
		break;
	case 'd':
		movePlayerRight(player2);
		break;
	}
	//Player 2 Input - END

	//Handle Players' Inputs - END
}
