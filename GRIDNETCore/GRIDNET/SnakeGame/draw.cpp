#include "draw.h"
#include <iostream>
#include "coin.h"
#include "keyboard.h"
#include "CSnakeGame.h"
#include "snailEnums.h"
#include "player.h"
#include "DTI.h"
#include "TransmissionToken.h"
#include "Tools.h"

void drawPlayerStatistics(std::shared_ptr<CDTI> dti, std::shared_ptr <Player>  player, int life_lost){
	dti->writeLine("\r\nPlayer: "+ player->name);
	dti->writeLine("Life lost: "+std::to_string(life_lost));
	dti->writeLine("Coins collected: "+ std::to_string(player->max_coin_score));
}

void drawStats(std::shared_ptr <CSnakeGame> game)
{
	std::shared_ptr<Player> p1 = game->getPlayer1();
	std::shared_ptr<Player> p2 = game->getPlayer2();
	std::shared_ptr<CDTI> p1DTI = game->getPlayer1DTI();
	std::shared_ptr<CDTI> p2DTI = game->getPlayer2DTI();

	p1DTI->writeLine("\n",false);
	if(p2DTI!=nullptr)
		p2DTI-> writeLine("\n", false);

	if (game->getIsMultiplayer())
	{
		if (p1->life_score > p2->life_score) {
			p1DTI->writeLine("Player " + p1->name + " wins!");
			if (p2DTI != nullptr) p2DTI->writeLine("Player " + p1->name + " wins!");
		}
		else if (p1->life_score < p2->life_score) {
			p1DTI->writeLine("Player " + p2->name + " wins!");
			if (p2DTI != nullptr) p2DTI->writeLine("Player " + p2->name + " wins!");
		}
		else {
			p1DTI->writeLine("Both players have same result");
			if (p2DTI != nullptr) p2DTI->writeLine("Both players have same result");

		}
		p1DTI->writeLine("\r\nStatistics:");
		if (p2DTI != nullptr) p2DTI->writeLine("\r\nStatistics:");

	}
	drawPlayerStatistics(p1DTI,p1, p2->life_score);
	if(p2DTI!=nullptr)
	drawPlayerStatistics(p2DTI, p1, p2->life_score);

	drawPlayerStatistics(p1DTI,p2, p1->life_score);
	if (p2DTI != nullptr)
	drawPlayerStatistics(p2DTI, p2, p1->life_score);

	p1DTI->writeLine("\n", false);
	if(p2DTI!=nullptr) p2DTI->writeLine("\n", false);
}

bool drawResults(std::shared_ptr <CSnakeGame> game)
{
	//Local Variables - BEGIN
	std::shared_ptr<CDTI> p1DTI = game->getPlayer1DTI();
	std::shared_ptr<CDTI> p2DTI = game->getPlayer2DTI();
	std::shared_ptr<CTools> tools = CTools::getInstance();

	std::string authCode;
	std::string newLine = "\r\n";
	int input = 0;
	//Local Variables - END
	
	if (p2DTI != nullptr)
	{
		switch (game->getResult()) {
		case DRAW:
			p1DTI->writeLine("It is a draw.");
			if (p2DTI != nullptr) p2DTI->writeLine("It is a draw.");
			break;
		case PLAYER1_WINS:
			p1DTI->writeLine(p1DTI->getUserID() + " wins.");
			if (p2DTI != nullptr) p2DTI->writeLine(p1DTI->getUserID() + " wins.");
			//game->getPlayer1DTI()->writeLine("Player 1 wins.");
			break;
		case PLAYER2_WINS:
			p1DTI->writeLine(p2DTI->getUserID() + " wins.");
			if (p2DTI != nullptr) p2DTI->writeLine(p2DTI->getUserID() + " wins.");
			break;
		}
	}

	/*p1DTI->writeLine("Press 'c' to continue..");
	if(p2DTI!=nullptr) p2DTI->writeLine("Press 'c' to continue..");
	uint64_t waitStart = CTools::getInstance()->getTime();
	uint64_t now = 0;
	uint64_t maxWaitTime = 5;
	uint64_t timeLeft = 10;
	uint64_t waitingFor = 0;
	bool Player1Ready = false;
	bool Player2Ready = p2DTI != nullptr?false:true;


	
	while(timeLeft &&!(Player1Ready && Player2Ready)){
		waitingFor = now - waitStart;
		timeLeft = waitingFor > maxWaitTime ? 0 : maxWaitTime - waitingFor;

		now = CTools::getInstance()->getTime();
		p1DTI->flashLine("Waiting for Player.. ["+ std::to_string(timeLeft)+"]",true);
		if(p2DTI!=nullptr) p2DTI->flashLine("Waiting for Player.. [" + std::to_string(timeLeft) + "]", true);
		input = game->getPlayer1DTI()->getRecentKey();
		
		if (!Player1Ready)
		{
			Player1Ready = (p1DTI->getRecentKey() == 'c');
		}

		if (!Player2Ready)
		{
			Player2Ready = (p2DTI->getRecentKey() == 'c');
		}
		Sleep(500);

	}
	
	if (Player1Ready && Player2Ready)
		return true;
		
	else */return false;
}

short drawPlayer( std::shared_ptr <Player>  player, int column, int row){
	int i;
	for(i = 0; i <= player->coin_score; i++){
		if(player->tail[i].x == column && player->tail[i].y == row){
			return 1;
		}
	}
	return 0;
}

void drawField(std::shared_ptr <CSnakeGame> game, int column, int row, std::string& rendering, bool isHost,const bool& wasDoubleCharDrawn)
{
	//Note: local players is always GREEN

	std::shared_ptr<CTools> tools = game->getTools();
	std::shared_ptr<Player> p1 = game->getPlayer1();
	std::shared_ptr<Player> p2 = game->getPlayer2();

	//std::shared_ptr<CDTI> p1DTI = game->getPlayer1DTI();
	//std::shared_ptr<CDTI> p2DTI = game->getPlayer2DTI();
	bool isPlayer1 = isHost;
	eColor::eColor p1Color = isPlayer1 ? eColor::lightGreen:eColor::lightCyan;
	eColor::eColor p2Color = isPlayer1 ? eColor::lightCyan : eColor::lightGreen;
	bool warpSpace = wasDoubleCharDrawn;
	const_cast<bool&>(wasDoubleCharDrawn) = false;

	std::shared_ptr<Coin> coin = game->getCoin();
	/* Draw wall */
	short column_start = (column == 0);
	short column_end =  (column == (MAX_COLUMN - 1));

	if ((row == 0 || row == MAX_ROW - 1) &&  column_start)
	{
		rendering += "\033[1;35m";
	}

	if (row == (MAX_ROW - 1) && column_end) {
		rendering += u8"┛\033[0m";
	}
	else if (row == (MAX_ROW - 1) && column_start) {
		rendering += u8"\033[1;35m┖";
	}
	else if (row == (MAX_ROW - 1) && !column_end)
	{
		rendering += u8"─";
	}
	else if (row == 0 && column_start)
	{
		rendering += u8"\033[1;35m┏";
	}
	else if (row == 0 && column_end)
	{
		rendering += u8"┓\033[0m";
	}
	else if(row == 0 && !column_end){
		rendering += u8"─";
	}
	else if (row == (MAX_ROW - 1) && !column_end) 
	{
			rendering += u8"─";
	}

	else if(column_end && !(row == 0 || row == (MAX_ROW - 1))){
		rendering += u8"\033[38;5;39m│\033[0m ";
	}
	else if (column_start && !(row==0 || row == (MAX_ROW - 1))) {
		rendering += u8"\033[38;5;39m│\033[0m";
	}

	/* Draw players */
	else if (drawPlayer(p1, column, row)) {
		rendering += tools->getColoredString(u8"●", p1Color);

	}
	else if (drawPlayer(p2, column, row)) {
		rendering += tools->getColoredString(u8"●", p2Color);

	}

	/* Draw elements */
	else if((coin->point.x == column) && (coin->point.y == row)){
		eColor::eColor color = coin->add_to_score == 5 ? eColor::blue : eColor::orange;
	
		rendering += tools->getColoredString(u8"💰", color);
		//const_cast<bool&>(wasDoubleCharDrawn) = true;
	}

	/* Draw empty field */
	else if(!warpSpace){
		rendering += " ";
	}
}

void drawResultBar(std::shared_ptr <CSnakeGame> game, std::string& rendering){

	//Local Variables - BEGIN
	std::shared_ptr<Player> p1 = game->getPlayer1();
	std::shared_ptr<Player> p2 = game->getPlayer2();

	std::shared_ptr<CTools> tools = game->getTools();
	int index = 0;
	int start = (MAX_COLUMN / 3 - MAX_GAMES) / 2;
	int end = start + MAX_GAMES;
	int player1_score_end = p1->life_score + start;
	int player2_score_start = MAX_GAMES - p2->life_score + start;
	std::string line;
	uint64_t p1RewardedGBUs = game->getRewardValueP1();
	uint64_t p2RewardedGBUs = game->getRewardValueP2();

	//Local Variables - END

	// center bar
	for(; index < start - 1; index++){
		rendering += "   ";
	}

	// better aligment
	index += 2; // aligment
	if(p1->max_coin_score < 10){
		rendering += " ";
	}
	// draw coins left player
	rendering += tools->getColoredString(u8" 💰" + std::to_string(p1RewardedGBUs == 0 ? p1->max_coin_score: p1RewardedGBUs), eColor::orange);

	// draw bar
	for(;index <= end; index++){
		if(index <= player1_score_end){
			rendering+= (RED_BACK + std::string("  "));
	
		}
		else if(index <= player2_score_start){
			rendering += (WHITE_BACK + std::string("  "));
	
		}
		else{
			rendering += (BLUE_BACK + std::string("  "));
		}
		rendering += (END_BACK + std::string(" "));
		
	}

	// draw coins right player
	if (game->getIsMultiplayer())
	{
		rendering += (std::to_string(p2RewardedGBUs == 0 ? p2->max_coin_score : p2RewardedGBUs) + YELLOW_COLOR + u8"💰" + END_COLOR);
	}
	rendering += "\r\n";
}

void drawMenu(std::shared_ptr <CSnakeGame> game)
{
	//Local Variables - BEGIN
	std::shared_ptr<Player> p1 = game->getPlayer1();
	std::shared_ptr<Player> p2 = game->getPlayer2();
	std::shared_ptr<CDTI> p1DTI = game->getPlayer1DTI();
	std::shared_ptr<CDTI> p2DTI = game->getPlayer2DTI();
	std::shared_ptr<CTools> tools = CTools::getInstance();

	std::string rendering = CTools::getFTNoRendering();//disable pre-rendering for performance
	//Local Variables - END
	
	//bool P1GNCRewarded = game->getP1RewardsEnabled();
	//bool P2GNCRewarded = game->getP2RewardsEnabled();


	if (game->getIsMultiplayer())
	{
		rendering += tools->getColoredString(tools->getColoredString(u8"🐍", eColor::lightGreen) + " Multi-player Mode. ", eColor::lightCyan) + tools->getColoredString("Rewards Multiplier:", eColor::lightPink) + tools->getColoredString(" On",eColor::lightGreen) + "\r\n";
		
		rendering +=( p1->name + " vs " + p2->name);
	}
	else
	{
		rendering += tools->getColoredString(tools->getColoredString(u8"🐍", eColor::lightGreen) + " Single-player Mode. ", eColor::lightCyan) + tools->getColoredString("Rewards Multiplier:", eColor::lightPink) + tools->getColoredString(" Off", eColor::cyborgBlood) + "\r\n";
	}
	rendering +="\r\n";
	
	drawResultBar(game, rendering);
	
	p1DTI->writeLine(rendering, false);
	if(p2DTI!=nullptr)
		p2DTI->writeLine(rendering, false);

}

void drawBoard(std::shared_ptr <CSnakeGame> game, bool isHost)
{
	//Local Variables - BEGIN
	std::shared_ptr<CDTI> p1DTI = game->getPlayer1DTI();
	std::shared_ptr<CDTI> p2DTI = game->getPlayer2DTI();
	int row = 0;
	int column = 0; 
	std::string renderingP2= CTools::getFTNoRendering();
	std::string renderingP1 = CTools::getFTNoRendering();//do NO pre-rendering (performance)
	//Local Variables - END
	bool doubleCharDrawn=false;
	isHost = !game->getIsMultiplayer()|| game->getPlayer1DTI()->getIsServer();

	for(row = 0; row < MAX_ROW; row++)
	{
		for (column = 0; column < MAX_COLUMN; column++)
		{
			if (column == 0&& doubleCharDrawn)
				doubleCharDrawn = false;

			drawField(game,column, row, renderingP1, isHost,  doubleCharDrawn);
		}
		renderingP1 += "\r\n";

	}


	doubleCharDrawn = false;
	row = 0;
	column = 0;

	if (p2DTI != nullptr)
	{
		isHost = !isHost;
		for (row = 0; row < MAX_ROW; row++)
		{
			
			for (column = 0; column < MAX_COLUMN; column++)
			{
				if (column == 0 && doubleCharDrawn)
					doubleCharDrawn = false;

				drawField(game, column, row, renderingP2, isHost, doubleCharDrawn);
			}
			renderingP2 += "\r\n";
		}

		p2DTI->writeLine(renderingP2, false);
	}

	p1DTI->writeLine(renderingP1, false);
}

void draw( std::shared_ptr <CSnakeGame> game, bool isHost){
	
	//Local Variables - BEGIN
	std::shared_ptr<CDTI> p1DTI = game->getPlayer1DTI();
	std::shared_ptr<CDTI> p2DTI = game->getPlayer2DTI();

	uint64_t linebackupP1 = 0;
	uint64_t linebackupP2 = 0;
	uint64_t lineDiffP1 = 0;
	uint64_t lineDiffP2 = 0;
	//Local Variables - END

	//Operational Logic - BEGIN
	/*
	We could have chosen between two ways of doing this:
	1) make client copy world-view from host and render it/provide input to be fetched by server. Each client renders on its own.
	2) make client provide input and make server render the world-view to both screens.

	We've decided to go with the 2nd approach to limit memory-copy operations and avoid necessary synchronization mechanics.
	Still, the server is fully aware of client's Terminal properties (as seen by the screen clearning mechanics below).
	The client calls the draw method, providing server's game instance and whether its a host or not.
	[Todo:CodesInChaos=> kindly consider refactoring these things. It seems to be the only place we don't fully employ OOP.]
	*/

	//Draw game-world taking into account each terminals' state
	 linebackupP1 = p1DTI->getCursorInViewLine();

	 if(p2DTI!=nullptr)
	 linebackupP2 = p2DTI->getCursorInViewLine();

	drawMenu(game);
	drawBoard(game, isHost);

	lineDiffP1 = p1DTI->getCursorInViewLine() - linebackupP1;

	 if(p2DTI!=nullptr)
		 lineDiffP2 = p2DTI->getCursorInViewLine() - linebackupP2;

	p1DTI->moveCursorUP(lineDiffP1);

	if (p2DTI != nullptr)
		p2DTI->moveCursorUP(lineDiffP2);

	//Operational Logic - END
}
