#ifndef GAME_H_
#define GAME_H_

/*********************************************** Includes ********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "G8RTOS.h"
#include "cc3100_usage.h"
#include "LCD_empty.h"
#include "time.h"
#include "math.h"

/*********************************************** Includes ********************************************************************/

/*********************************************** Externs ********************************************************************/

/* Semaphores here */ 
extern semaphore_t CC3100_SEMAPHORE;
extern semaphore_t GAMESTATE_SEMAPHORE;

/*********************************************** Externs ********************************************************************/

// CONFIGURATION MACROS -------------------------------------------------------------
#define MENU_TEXT_COLOR     LCD_YELLOW
#define MENU_BG_COLOR       LCD_BLACK

/*********************************************** Global Defines ********************************************************************/
#define RED_ON              P2->OUT |= BIT0
#define RED_OFF             P2->OUT &= ~BIT0
#define GREEN_ON            P2->OUT |= BIT1
#define GREEN_OFF           P2->OUT &= ~BIT1
#define BLUE_ON             P2->OUT |= BIT2
#define BLUE_OFF            P2->OUT &= ~BIT2

#define MAX_NUM_OF_PLAYERS  2
#define MAX_NUM_OF_BALLS    8
#define BALL_GEN_SLEEP      2500 // 2.5 second increments increasing linearly

#define DEFAULT_PRIORITY    15
#define AGING_PRIORITY      10

// This game can actually be played with 4 players... a little bit more challenging, but doable! 
#define NUM_OF_PLAYERS_PLAYING 2

/* Size of game arena */
#define ARENA_MIN_X                  40
#define ARENA_MAX_X                  280
#define ARENA_MIN_Y                  0
#define ARENA_MAX_Y                  240

/* Size of objects */
#define PADDLE_OFFSET                1
#define PADDLE_LEN                   64
#define PADDLE_LEN_D2                (PADDLE_LEN >> 1)
#define PADDLE_WID                   4
#define PADDLE_WID_D2                (PADDLE_WID >> 1)
#define BALL_SIZE                    4
#define BALL_SIZE_D2                 (BALL_SIZE >> 1)

/* Centers for paddles at the center of the sides */
#define PADDLE_X_CENTER              MAX_SCREEN_X >> 1

/* Edge limitations for player's center coordinate */
#define HORIZ_CENTER_MAX_PL          (ARENA_MAX_X - PADDLE_LEN_D2)
#define HORIZ_CENTER_MIN_PL          (ARENA_MIN_X + PADDLE_LEN_D2)

/* Constant enters of each player */
#define TOP_PLAYER_CENTER_Y          (ARENA_MIN_Y + PADDLE_WID_D2)
#define BOTTOM_PLAYER_CENTER_Y       (ARENA_MAX_Y - PADDLE_WID_D2)

/* Edge coordinates for paddles */
#define TOP_PADDLE_EDGE              (ARENA_MIN_Y + PADDLE_WID)
#define BOTTOM_PADDLE_EDGE           (ARENA_MAX_Y - PADDLE_WID)

/* Amount of allowable space for collisions with the sides of paddles */
#define WIGGLE_ROOM                  2
#define JOYSTICK_BIAS                0

/* Value for velocities from contact with paddles */
#define _1_3_PADDLE                  11

/* Defines for Minkowski Alg. for collision */
#define WIDTH_TOP_OR_BOTTOM          ((PADDLE_LEN + BALL_SIZE) >> 1) + WIGGLE_ROOM
#define HEIGHT_TOP_OR_BOTTOM         ((PADDLE_WID + BALL_SIZE) >> 1) + WIGGLE_ROOM

/* Edge limitations for ball's center coordinate */
#define HORIZ_CENTER_MAX_BALL        (ARENA_MAX_X - BALL_SIZE_D2)
#define HORIZ_CENTER_MIN_BALL        (ARENA_MIN_X + BALL_SIZE_D2)
#define VERT_CENTER_MAX_BALL         (ARENA_MAX_Y - BALL_SIZE_D2)
#define VERT_CENTER_MIN_BALL         (ARENA_MIN_Y + BALL_SIZE_D2)

/* Maximum ball speed */
#define MAX_BALL_SPEED               7
#define MIN_BALL_SPEED               3

/* Background color - Black */
#define BACK_COLOR                   LCD_BLACK

/* Offset for printing player to avoid blips from left behind ball */
#define PRINT_OFFSET                10

/* Used as status LEDs for Wi-Fi */
#define BLUE_LED BIT2
#define RED_LED BIT0

/* Enums for player colors */
typedef enum
{
    PLAYER_RED = LCD_RED,
    PLAYER_BLUE = LCD_BLUE,
    PLAYER_GREEN = LCD_GREEN,
    PLAYER_YELLOW = LCD_YELLOW
}playerColor;

/* Enums for player numbers */
typedef enum
{
    BOTTOM = 0,
    TOP = 1,
    RIGHT = 2,
    LEFT = 3
}playerPosition;

/*********************************************** Global Defines ********************************************************************/

/*********************************************** Data Structures ********************************************************************/
/*********************************************** Data Structures ********************************************************************/
#pragma pack ( push, 1)
/*
 * Struct to be sent from the client to the host
 */
typedef struct
{
    uint32_t IP_address;
    int16_t displacement;
    uint8_t playerNumber;
    bool ready;
    bool joined;
    bool acknowledge;
} SpecificPlayerInfo_t;

/*
 * General player info to be used by both host and client
 * Client responsible for translation
 */
typedef struct
{
    int16_t currentCenter;
    uint16_t color;
    playerPosition position;
} GeneralPlayerInfo_t;

/*
 * Struct of all the balls, only changed by the host
 */
typedef struct
{
    int16_t currentCenterX;
    int16_t currentCenterY;
    uint16_t color;
    bool alive;
} Ball_t;

/*
 * Struct to be sent from the host to the client
 */
typedef struct
{
    SpecificPlayerInfo_t player;
    GeneralPlayerInfo_t players[MAX_NUM_OF_PLAYERS];
    Ball_t balls[MAX_NUM_OF_BALLS];
    uint16_t numberOfBalls;
    bool winner;
    bool gameDone;
    uint8_t LEDScores[2];
    uint8_t overallScores[2];
} GameState_t;
#pragma pack ( pop )

/*
 * Struct of all the previous ball locations, only changed by self for drawing!
 */
typedef struct
{
    int16_t CenterX;
    int16_t CenterY;
}PrevBall_t;

/*
 * Struct of all the previous players locations, only changed by self for drawing
 */
typedef struct
{
    int16_t Center;
}PrevPlayer_t;
/*********************************************** Data Structures ********************************************************************/

/*********************************************** Game Functions *********************************************************************/

// This function copies over a gamestate into a new
// packet to be sent over Wi-Fi.
void fillPacket ( GameState_t * gs, GameState_t * packet );

// This function copies over a packet into the global gamestate
// using the fillPacket function to minimize program mem usage.
void emptyPacket( GameState_t * gs, GameState_t * packet );

// initialize interrupts for input buttons B0, B1, B2, B3
// on ports 4 and 5.
void buttons_init(void);

// Any animations or text used for the main menu is displayed
// with this function
void writeMainMenu( uint16_t Color );

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame();

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost();

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost();

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient();

/*
 * End of game for the client
 */
void EndOfGameClient();

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame();

/*
 * Thread that sends game state to client
 */
void SendDataToClient();

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient();

/*
 * Generate Ball thread
 */
void GenerateBall();

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost();

/*
 * Thread to move a single ball
 */
void MoveBall();

/*
 * End of game for the host
 */
void EndOfGameHost();

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread();

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects();

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs();

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole();

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player);

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer);

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor);

/*
 * Initializes and prints initial game state
 */
void InitBoardState();

/*********************************************** Public Functions *********************************************************************/


#endif /* GAME_H_ */
