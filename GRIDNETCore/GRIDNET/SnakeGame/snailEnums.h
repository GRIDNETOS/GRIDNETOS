#pragma once
/**
* Game constants
*/

#define MAX_ROW 20
#define MAX_COLUMN 40
#define PLAYER_SPEED 400000
#define FPS 1000
#define TAIL_SIZE 10
#define MAX_GAMES 10
#define COIN_5_LIMIT 1000
#define COIN_5_OCCURANCE 10

#define END_COLOR "\x1B[0m"
#define RED_COLOR "\x1B[31m"
#define BLUE_COLOR "\x1B[34m"
#define YELLOW_COLOR "\x1B[33m"
#define MAGENTA_COLOR "\x1B[35m"

#define END_BACK "\x1B[0m"
#define RED_BACK "\x1B[41m"
#define BLUE_BACK "\x1B[44m"
#define WHITE_BACK "\x1B[47m"

/**
* Structures
*/


typedef enum Result { NONE, DRAW, PLAYER1_WINS, PLAYER2_WINS } Result;
