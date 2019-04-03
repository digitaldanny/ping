/*
 * Game.c
 *
 *  Created     : 4/2/2019
 *  Last Edit   : 4/2/2019
 *
 *  UPDATES     :
 *  4/2/2019    : Initialized threads and game functions.
 *
 *  TODO        :
 *  1. Update player displacement/ready on initialization in JoinGame.
 *  2. Does the player.acknowledge need to be updated in JoinGame?
 *  3. Port number may be incorrect in the header configurations
 *  4. Does it matter what the CreateGame sends back to client when acknowledging?
 *  5. Initialize board in CreateGame and JoinGame
 *  6. What is the difference between gamestate.gameDone and gamestate.winner?
 *
 */

#include "Game.h"

// ======================       GLOBALS            ==========================
GameState_t gamestate;
uint8_t playerCount = 0;

// ======================      SEMAPHORES          ==========================
semaphore_t CLIENT_PLAYERDATA_SEMAPHORE;

// ======================     GAME FUNCTIONS       ==========================

// initialize interrupts for input buttons B0, B1, B2, B3
// on ports 4 and 5.
void buttons_init(void)
{
    // B0 = P4.4, B1 = P4.5
    P4->DIR &= ~(BIT4 | BIT5);
    P4->IFG &= ~(BIT4 | BIT5);      // P4.4 IFG cleared
    P4->IE  |= BIT4 | BIT5;         // Enable interrupt on P4.4
    P4->IES |= BIT4 | BIT5;         // high-to-low transition
    P4->REN |= BIT4 | BIT5;         // Pull-up resister
    P4->OUT |= BIT4 | BIT5;         // Sets res to pull-up
    NVIC_EnableIRQ(PORT4_IRQn);     // enable interrupts on PORT4
    __NVIC_SetPriority(PORT4_IRQn, 0);

    // B2 = P5.4, B3 = P5.5
    P5->DIR &= ~(BIT4 | BIT5);
    P5->IFG &= ~(BIT4 | BIT5);      // P4.4 IFG cleared
    P5->IE  |= BIT4 | BIT5;         // Enable interrupt on P4.4
    P5->IES |= BIT4 | BIT5;         // high-to-low transition
    P5->REN |= BIT4 | BIT5;         // Pull-up resister
    P5->OUT |= BIT4 | BIT5;         // Sets res to pull-up
    NVIC_EnableIRQ(PORT5_IRQn);     // enable interrupts on PORT4
    NVIC_SetPriority(PORT5_IRQn, 0);
}

// Any animations or text used for the main menu is displayed with this function
void writeMainMenu( uint16_t Color )
{
    LCD_Text(0, 0, "B0 -> HOST", Color);
    LCD_Text(0, 16, "B2 -> HOST", Color);
}

/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole()
{

}

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player)
{

}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer)
{

}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor)
{

}

/*
 * Initializes and prints initial game state
 */
void InitBoardState()
{
    // initialize balls
    Ball_t * tempBall;
    for (int i = 0; i < MAX_NUM_OF_BALLS; i++)
    {
        tempBall = &gamestate.balls[i];
        tempBall->alive = 0;
        tempBall->color = LCD_WHITE;
        tempBall->currentCenterX = 0;
        tempBall->currentCenterY = 0;
    }

    // initialize game states
    gamestate.gameDone = 0;
    gamestate.numberOfBalls = 0;
    gamestate.winner = 0;

    // draw the map boundaries
    LCD_Clear(LCD_BLACK);
    LCD_DrawRectangle(ARENA_MIN_X-1, ARENA_MIN_X, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);
    LCD_DrawRectangle(ARENA_MAX_X, ARENA_MAX_X+1, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);

    char score_str[3] = {0, 0, 0};

    // draw the current score of the players
    snprintf( score_str, 2, "%d", gamestate.overallScores[0] );
    LCD_Text(0, MAX_SCREEN_Y - 16 - 1, (uint8_t*)score_str, PLAYER_RED);

    snprintf( score_str, 2, "%d", gamestate.overallScores[1] );
    LCD_Text(0, 0, (uint8_t*)score_str, PLAYER_BLUE);
}

// ====================== HOST HOST HOST HOST HOST ==========================
/*
 * SUMMARY: Thread for the host to create a game.
 *
 * DESCRIPTION:
 * 1. Initialize players
 * 2. Establish connection with client (use LED to indicate WiFi conn.)
 * 3. Try to receive packet from the client. (while)
 * 4. Acknowledge client once the client joins.
 * 5. Initialize the board (draw arena, players, scores)
 * 6. Add GenerateBall, DrawObjects, ReadJoystickHost, SendDataToClient
 *      ReceiveDataFromClient, MoveLEDs (low priority), Idle
 * 7. Kill self.
 *
 * NOTE: Player initializations will need to be updated to count up to 3 if
 *       the game is updated to be a 4 player game.
 */
void CreateGame()
{
    // declare variables
    GeneralPlayerInfo_t * tempPlayer;

    // 1. Initialize players' general parameters
    // player 1 initializations...
    tempPlayer = &gamestate.players[0];
    tempPlayer->color = PLAYER_RED;
    tempPlayer->currentCenter = MAX_SCREEN_X / 2;
    tempPlayer->position = BOTTOM;
    playerCount++;

    // player 2 initializations...
    tempPlayer = &gamestate.players[1];
    tempPlayer->color = PLAYER_BLUE;
    tempPlayer->currentCenter = MAX_SCREEN_X / 2;
    tempPlayer->position = TOP;

    // initialize number of points for the players. Don't include this
    // in the initBoard function so that we can use that function to
    // restart the game without clearing the scores.
    gamestate.overallScores[0] = 0;
    gamestate.overallScores[1] = 0;

    // 2. Establish connection with client
    P2->OUT &= ~(BIT0 | BIT1 | BIT2); // initialize led's off
    P2->DIR |= (BIT0 | BIT1 | BIT2); // set R.G.B direction

    // 3. Try to receive packet from the client until return SUCCESS == 0
    int test = 5;
    ReceiveData((_u8*)&test, sizeof(test));
    while( ReceiveData((_u8*)&gamestate.player, sizeof(gamestate.player)) );

    // 4. Acknowledge client by telling them they joined the game.
    gamestate.player.joined = true;
    SendData( (_u8*)&gamestate.player.joined ,
              gamestate.player.IP_address    ,
              sizeof(gamestate.player.joined) );

    GREEN_ON; // use LED to indicate WiFi connection as HOST

    // 5. Initialize the board (draw arena, players, scores)
    InitBoardState();

    // 6. Add GenerateBall, DrawObjects, ReadJoystickHost, SendDataToClient
    //      ReceiveDataFromClient, MoveLEDs (low priority), Idle
    G8RTOS_AddThread( &GenerateBall, DEFAULT_PRIORITY, 0xFFFFFFFF,          "GENERATE_BALL" );
    G8RTOS_AddThread( &DrawObjects, DEFAULT_PRIORITY, 0xFFFFFFFF,           "DRAW_OBJECTS" );
    G8RTOS_AddThread( &ReadJoystickHost, DEFAULT_PRIORITY, 0xFFFFFFFF,      "READ_JOYSTICK" );
    G8RTOS_AddThread( &SendDataToClient, DEFAULT_PRIORITY, 0xFFFFFFFF,      "SEND_DATA" );
    G8RTOS_AddThread( &ReceiveDataFromClient, DEFAULT_PRIORITY, 0xFFFFFFFF, "RECEIVE_DATA" );
    G8RTOS_AddThread( &MoveLEDs, 254, 0xFFFFFFFF,                           "MOVE_LEDS" );
    G8RTOS_AddThread( &IdleThread, 255, 0xFFFFFFFF,                         "IDLE" );

    // 7. Kill self.
    G8RTOS_KillSelf();
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient()
{
    while(1)
    {

    }
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{
    while(1)
    {

    }
}

/*
 * Generate Ball thread
 */
void GenerateBall()
{
    while(1)
    {

    }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost()
{
    while(1)
    {

    }
}

/*
 * Thread to move a single ball
 */
void MoveBall()
{
    while(1)
    {

    }
}

/*
 * End of game for the host
 */
void EndOfGameHost()
{
    while(1)
    {

    }
}

// ==================== CLIENT CLIENT CLIENT CLIENT ==========================
/*
 * SUMMARY: Thread for client to join game.
 *
 * DESCRIPTION:
 * 1. Set initial SpecificPlayerInfo_t strict attributes ( getLocalIP() ).
 * 2. Send player into the host.
 * 3. Wait for server response.
 * 4. If you've joined the game, acknowledge you've joined to the host
 *      and show connection through LED.
 * 5. Initialize board state, semaphores, and add the following threads..
 *      ReadJoystickClient, SendDataToHost, ReceiveDataFromHost, DrawObjects,
 *      MoveLEDs, Idle
 * 6. Kill self.
 */
void JoinGame()
{
    // 1. Set initial SpecificPlayerInfo_t strict attributes ( getLocalIP() ).
    SpecificPlayerInfo_t player;
    player.IP_address = getLocalIP();
    player.acknowledge = false;
    player.displacement = 0;
    player.joined = false;
    player.playerNumber = playerCount;
    player.ready = false;
    playerCount++;

    // 2. Send player data into the host.
    SendData( (_u8*)&player, player.IP_address, sizeof(player) );

    // hardware initialization
    P2->OUT &= ~(BIT0 | BIT1 | BIT2); // initialize led's off
    P2->DIR |= (BIT0 | BIT1 | BIT2); // set R.G.B direction

    // 3. Wait for server response showing that it acknowledges the new player
    while( ReceiveData( (_u8*)&player.joined, sizeof(player.joined)) );

    // 4. If you've joined the game, acknowledge you've joined to the host
    //      and show connection through LED.
    BLUE_ON;

    // 5. Initialize board state, semaphores, and add the following threads..
    //      ReadJoystickClient, SendDataToHost, ReceiveDataFromHost, DrawObjects,
    //      MoveLEDs, Idle
    InitBoardState();
    G8RTOS_InitSemaphore(&CLIENT_PLAYERDATA_SEMAPHORE, 1);

    G8RTOS_AddThread( &ReadJoystickClient, DEFAULT_PRIORITY, 0xFFFFFFFF,    "READ_JOYSTICK" );
    G8RTOS_AddThread( &SendDataToHost, DEFAULT_PRIORITY, 0xFFFFFFFF,        "SEND_DATA" );
    G8RTOS_AddThread( &ReceiveDataFromHost, DEFAULT_PRIORITY, 0xFFFFFFFF,   "RECEIVE_DATA" );
    G8RTOS_AddThread( &DrawObjects, DEFAULT_PRIORITY, 0xFFFFFFFF,           "DRAW_OBJECTS" );
    G8RTOS_AddThread( &MoveLEDs, 244, 0xFFFFFFFF,                           "MOVE_LEDS" );
    G8RTOS_AddThread( &IdleThread, 255, 0xFFFFFFFF,                         "IDLE" );

    // 6. Kill self.
    G8RTOS_KillSelf();
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost()
{
    while(1)
    {

    }
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{
    while(1)
    {

    }
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient()
{
    while(1)
    {

    }
}

/*
 * End of game for the client
 */
void EndOfGameClient()
{
    while(1)
    {

    }
}

// ===================== COMMON COMMON COMMON COMMON =========================
/*
 * Thread to draw all the objects in the game
 */
void DrawObjects()
{
    while(1)
    {

    }
}

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs()
{
    while(1)
    {

    }
}

/*
 * Idle thread to avoid deadlocks and RTOS end
 */
void IdleThread() { while(1); }
