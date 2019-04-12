#define MULTI
#define GAMESTATE
#define HANDSHAKE2

// PREPROCESSOR DIRECTIVES :
// SINGLE   : Use this configuration if debugging with one board. CreateGame doesn't
//             wait to read data from the client.
// MULTI    : Primary mode allows both boards to communicate with each other.

/*
 * Game.c
 *
 *  Created     : 4/2/2019
 *  Last Edit   : 4/9/2019
 *
 *  UPDATES     :
 *  4/2/2019    : Initialized threads and game functions.
 *  4/6/2019    : Boards can send/receive UDP packets
 *  4/7/2019    : Boards send/receive packets more efficiently
 *  4/8/2019    : Collisions, animations, main menu, and kill ball
 *  4/10/2019   : All branch merge bugs squashed, wifi connection stabilized
 *  4/10/2019   : changed priority for drawing to 10 whenever a draw thread is
 *              : removed uint8_t infront of score_str
 *              : fixed killthread where past blocked info was not cleared
 *              : moved around semaphores
 *              :  changed priority for drawing to 10 whenever a draw thread is added
 *
 *  TODO        :
 *
 */

#include "Game.h"

// ======================       GLOBALS            ==========================
GameState_t gamestate;
GameState_t packet;
SpecificPlayerInfo_t client_player;
uint8_t playerCount = 2;
uint8_t ballCount = 0;
PrevBall_t previousBalls[MAX_NUM_OF_BALLS];
gameNextState nextState = NA;   // set next game state to NA
int16_t displacement = 160;


// ======================      SEMAPHORES          ==========================
semaphore_t CC3100_SEMAPHORE;
semaphore_t GAMESTATE_SEMAPHORE;
semaphore_t LCDREADY;
semaphore_t LEDREADY;

// ======================     GAME FUNCTIONS       ==========================

void addHostThreads(){
    G8RTOS_AddThread( &GenerateBall, 20, 0xFFFFFFFF,          "GENERATE_BALL___" );
    G8RTOS_AddThread( &DrawObjects, 10, 0xFFFFFFFF,           "DRAW_OBJECTS____" );
    G8RTOS_AddThread( &ReadJoystickHost, 20, 0xFFFFFFFF,      "READ_JOYSTICK___" );
    G8RTOS_AddThread( &MoveLEDs, 20, 0xFFFFFFFF,              "MOVE_LEDS_______" );
    G8RTOS_AddThread( &IdleThread, 255, 0xFFFFFFFF,           "IDLE____________" );

    #ifdef MULTI
    G8RTOS_AddThread( &ReceiveDataFromClient, DEFAULT_PRIORITY, 0xFFFFFFFF, "RECEIVE_DATA____" );
    G8RTOS_AddThread( &SendDataToClient, DEFAULT_PRIORITY, 0xFFFFFFFF,      "SEND_DATA_______" );
    #endif
}

void addClientThreads(){
    G8RTOS_AddThread( &ReadJoystickClient, DEFAULT_PRIORITY, 0xFFFFFFFF,    "READ_JOYSTICK___" );
    G8RTOS_AddThread( &SendDataToHost, DEFAULT_PRIORITY, 0xFFFFFFFF,        "SEND_DATA_______" );
    G8RTOS_AddThread( &ReceiveDataFromHost, DEFAULT_PRIORITY, 0xFFFFFFFF,   "RECEIVE_DATA____" );
    G8RTOS_AddThread( &DrawObjects, 10, 0xFFFFFFFF,                         "DRAW_OBJECTS____" );
    G8RTOS_AddThread( &MoveLEDs, 20, 0xFFFFFFFF,                            "MOVE_LEDS_______" );
    G8RTOS_AddThread( &IdleThread, 255, 0xFFFFFFFF,                         "IDLE____________" );
}

// This function copies over a gamestate into a new
// packet to be sent over Wi-Fi.
void fillPacket ( GameState_t * gs, GameState_t * packet )
{
    packet->gameDone = gs->gameDone;
    packet->numberOfBalls = gs->gameDone;
    packet->player = gs->player;
    packet->winner = gs->winner;

    // copy over all arrays in the gamestate packet
    for (int i = 0; i < 2; i++)
    {
        packet->LEDScores[i] = gs->LEDScores[i];
        packet->overallScores[i] = gs->LEDScores[i];
        packet->players[i] = gs->players[i];
    }

    for (int i = 0; i < MAX_NUM_OF_BALLS; i++)
    {
        packet->balls[i] = gs->balls[i];
    }
}

// This function copies over a packet into the global gamestate
// using the fillPacket function to minimize program mem usage.
void emptyPacket( GameState_t * gs, GameState_t * packet )
{
    fillPacket( packet, gs );
}

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
    LCD_Text(0, 16, "B2 -> CLIENT", Color);
    srand(time(0));  // generated for later use
}

// Any animations or text used for the game menu is displayed with this function
void writeGameMenuHost( uint16_t Color )
{
    LCD_Clear(LCD_BLACK);

    // signal semaphore
    G8RTOS_WaitSemaphore(&LCDREADY);

    LCD_Text(MAX_SCREEN_X/2 - 7*8, MAX_SCREEN_Y/2-8, "B0 -> Next Game", Color);
    LCD_Text(MAX_SCREEN_X/2 - 7*8, MAX_SCREEN_Y/2+8, "B2 -> End Game", Color);

    // signal semaphore
    G8RTOS_SignalSemaphore(&LCDREADY);
}

// Any animations or text used for the game menu is displayed with this function
void writeGameMenuClient( uint16_t Color )
{
    LCD_Clear(LCD_BLACK);

    // signal semaphore
    G8RTOS_WaitSemaphore(&LCDREADY);

    LCD_Text(MAX_SCREEN_X/2 - 8*8, MAX_SCREEN_Y/2-4, "Waiting for Host", Color);

    // signal semaphore
    G8RTOS_SignalSemaphore(&LCDREADY);
}

/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole()
{
    if ( getLocalIP() == HOST_IP_ADDR ) return Host;
    else                                return Client;
}

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player)
{
    int16_t yCenter = 0;

    // determine the player's Y offset based on whether it should be on the top or bottom of the screen
    if (player->position == BOTTOM) yCenter = ARENA_MAX_Y - PADDLE_WID_D2 - PADDLE_OFFSET;
    if (player->position == TOP)    yCenter = ARENA_MIN_Y + PADDLE_WID_D2 + PADDLE_OFFSET;
  
    G8RTOS_WaitSemaphore(&LCDREADY);

    LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2, player->currentCenter + PADDLE_LEN_D2,
                      yCenter - PADDLE_WID_D2, yCenter + PADDLE_WID_D2, player->color);

    G8RTOS_SignalSemaphore(&LCDREADY);


}

/*
 * Updates player's paddle based on current and new center
 *
 * NOTE: Only updates the portion of the paddle that changes on the screen. All
 *      common areas are ignored in order to save processor time.
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer)
{
    int16_t yCenter = 0;

    // only update the player if it's center moved
    if ( prevPlayerIn->Center != outPlayer->currentCenter )
    {

        // determine the player's Y offset based on whether it should be on the top or bottom of the screen
        if (outPlayer->position == BOTTOM) yCenter = ARENA_MAX_Y - PADDLE_WID_D2 - PADDLE_OFFSET;
        if (outPlayer->position == TOP)    yCenter = ARENA_MIN_Y + PADDLE_WID_D2 + PADDLE_OFFSET;

        // Calculate the COMMON area's length
        int16_t center_diff = abs(prevPlayerIn->Center - outPlayer->currentCenter);

        int16_t starting_old_data_window;
        int16_t starting_new_data_window;

        // If the old player data is to the left, erase that data.
        if ( prevPlayerIn->Center - outPlayer->currentCenter < 0 )
        {
            // Calulate the starting x position for the UNCOMMON OLD player's area on the LEFT side
            starting_old_data_window = prevPlayerIn->Center - PADDLE_LEN_D2;

            // Calculate the starting x position for the UNCOMMON NEW player's area on the RIGHT side
            starting_new_data_window = outPlayer->currentCenter + PADDLE_LEN_D2 - center_diff;
        }

        // If the old player data is to the right, erase that data.
        else if ( prevPlayerIn->Center - outPlayer->currentCenter > 0 )
        {
            // Calulate the starting x position for the UNCOMMON OLD player's area on the RIGHT side
            starting_old_data_window = prevPlayerIn->Center + PADDLE_LEN_D2 - center_diff;

            // Calculate the starting x position for the UNCOMMON NEW player's area on the LEFT side
            starting_new_data_window = outPlayer->currentCenter - PADDLE_LEN_D2;
        }

        G8RTOS_WaitSemaphore(&LCDREADY);
      
        // erase the UNCOMMON old player position first
        LCD_DrawRectangle(starting_old_data_window, starting_old_data_window + center_diff,
                          yCenter - PADDLE_WID_D2, yCenter + PADDLE_WID_D2, BACK_COLOR);

        // draw the UNCOMMON updated player position
        LCD_DrawRectangle(starting_new_data_window, starting_new_data_window + center_diff,
                          yCenter - PADDLE_WID_D2, yCenter + PADDLE_WID_D2, outPlayer->color);

        // wrapping the data update doesn't allow the players to update twice
        // before erasing the original
        prevPlayerIn->Center = outPlayer->currentCenter;

        G8RTOS_SignalSemaphore(&LCDREADY);
    }
}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor)
{

    G8RTOS_WaitSemaphore(&LCDREADY);

    // erase the old ball first
    LCD_DrawRectangle(previousBall->CenterX, previousBall->CenterX + BALL_SIZE,
                      previousBall->CenterY, previousBall->CenterY + BALL_SIZE, BACK_COLOR);

    // before erasing the original
    previousBall->CenterX = currentBall->currentCenterX;
    previousBall->CenterY = currentBall->currentCenterY;
    // draw the new ball next
    LCD_DrawRectangle(currentBall->currentCenterX, currentBall->currentCenterX + BALL_SIZE,
                      currentBall->currentCenterY, currentBall->currentCenterY + BALL_SIZE, outColor);

    // wrapping the data update doesn't allow the balls to update twice


    G8RTOS_SignalSemaphore(&LCDREADY);
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
    playerCount = 2;
    ballCount = 0;
  
    // draw the map boundaries
    // This is the only thread running, so semaphores are not required.
    LCD_Clear(BACK_COLOR);
    LCD_DrawRectangle(ARENA_MIN_X-1, ARENA_MIN_X, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);
    LCD_DrawRectangle(ARENA_MAX_X, ARENA_MAX_X+1, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);
  
    char score_str[3] = {0, 0, 0};

    // draw the current score of the players
    snprintf( score_str, 3, "%.2u", gamestate.overallScores[0] );
    LCD_Text(0, MAX_SCREEN_Y - 16 - 1, score_str, PLAYER_RED);

    snprintf( score_str, 3, "%.2u", gamestate.overallScores[1] );
    LCD_Text(0, 0, score_str, PLAYER_BLUE);
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
    // 1. Initialize general players complete in InitBoard
    // player 1 initializations...
    GeneralPlayerInfo_t * tempPlayer;
    tempPlayer = &gamestate.players[0];
    tempPlayer->color = PLAYER_RED;
    tempPlayer->currentCenter = MAX_SCREEN_X / 2;
    tempPlayer->position = BOTTOM;

    // player 2 initializations...
    tempPlayer = &gamestate.players[1];
    tempPlayer->color = PLAYER_BLUE;
    tempPlayer->currentCenter = MAX_SCREEN_X / 2;
    tempPlayer->position = TOP;

    // 2. Establish connection with client
    P2->OUT &= ~(BIT0 | BIT1 | BIT2); // initialize led's off
    P2->DIR |= (BIT0 | BIT1 | BIT2); // set R.G.B direction

#ifdef MULTI
    initCC3100(Host); // connect to the network

    // 3. Try to receive packet from the client until return SUCCESS
#ifndef HANDSHAKE2
    uint8_t handshake = 'X';  // either 'H' or 'C' to show message sent or received from Host or Client.
    while( handshake != 'C' )
    {
        ReceiveData((_u8*)&handshake, sizeof(handshake));
    }

    // STATIC INLINE THIS ---------------------------------------------------------------------------------

    // once the synchronization byte has been received,
    // the host is ready to receive the client's ip so it
    // can begin sending messages back to it. Because the bytes
    // are sent MSB first and MSB begins at highest address of
    // gamestate.player.IP_address, receieved data must first
    // be stored in a buffer and reorganized.
    ReceiveData( (_u8*)&gamestate.player.IP_address     , 4); // ip address
    ReceiveData( (_u8*)&gamestate.player.displacement   , 2); // ip address
    ReceiveData( (_u8*)&gamestate.player.playerNumber   , 1);
    ReceiveData( (_u8*)&gamestate.player.ready          , 1);
    ReceiveData( (_u8*)&gamestate.player.joined         , 1);
    ReceiveData( (_u8*)&gamestate.player.acknowledge    , 1);

    // 4. Acknowledge client to tell them they joined the game.
    handshake = 'H';
    SendData( (_u8*)&handshake, gamestate.player.IP_address, sizeof(handshake) );

    // Wait for client to sync with host by acknowledging that
    // it received the host message.
    while( handshake != 'C' )
    {
        ReceiveData((_u8*)&handshake, sizeof(handshake));
    }
#endif
#ifdef HANDSHAKE2
    while( ReceiveData((_u8*)&gamestate.player, sizeof(gamestate.player)) < 0 );

    // 4. Acknowledge client to tell them they joined the game.
    gamestate.player.joined = true;
    SendData( (_u8*)&gamestate, gamestate.player.IP_address, sizeof(gamestate) );

    // Wait for client to sync with host by acknowledging that
    // it received the host message.
    do
    {
        ReceiveData((uint8_t*)&gamestate.player, sizeof(gamestate.player));
    } while ( gamestate.player.acknowledge == false );

#endif
#endif

    GREEN_ON; // use LED to indicate WiFi connection as HOST

    // 5. Initialize the board (draw arena, players, scores)
    InitBoardState();
    addHostThreads();

    // 6. Add GenerateBall, DrawObjects, ReadJoystickHost, SendDataToClient
    //      ReceiveDataFromClient, MoveLEDs (low priority), Idle
    addHostThreads();

    // 7. Kill self.
    G8RTOS_KillSelf();
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient()
{
#ifdef PACKETS

    while(1)
    {
        // 1. Fill packet for client
        fillPacket(&gamestate, &packet);

        // 2. Send packet
        G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);
        SendData( (uint8_t*)&packet, packet.player.IP_address, sizeof(packet) );
        G8RTOS_SignalSemaphore(&CC3100_SEMAPHORE);

        // 3. Check if the game is done. Add endofgamehost thread if done.
        if ( gamestate.gameDone == true )
            G8RTOS_AddThread(EndOfGameHost, 0, 0xFFFFFFFF, "END_OF_GAME_HOST");

        sleep(5);
    }
#endif
#ifdef GAMESTATE
    while(1)
    {

        // 2. Send packet
        G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);
        SendData( (uint8_t*)&gamestate, gamestate.player.IP_address, sizeof(gamestate) );
        G8RTOS_SignalSemaphore(&CC3100_SEMAPHORE);

        // 3. Check if the game is done. Add endofgamehost thread if done.
        // This has to be wrapped in the semaphores because the gameDone
        // could otherwise be changed immediately after the data transfer
        // and the client wouldn't know the game ended.
        if ( gamestate.gameDone == true )
            G8RTOS_AddThread(EndOfGameHost, 0, 0xFFFFFFFF, "END_OF_GAME_HOST");

        sleep(5);
    }
#endif
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{
#ifdef PACKETS
    int16_t result = -1;
    while(1)
    {
        // if the response is greater than 0, valid data was returned
        // to the gamestate. If not, no valid data was returned and
        // thread is put to sleep to avoid deadlock.
        do
        {
            G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);
            result = ReceiveData( (uint8_t*)&packet.player, sizeof(packet.player));
            G8RTOS_SignalSemaphore(&CC3100_SEMAPHORE);
            sleep(1); // avoid deadlock

        } while ( result < 0 );

        // If data was actually sent, empty the packet into the gamestate
        emptyPacket(&gamestate, &packet);

        // update the player's center
        gamestate.players[1].currentCenter = gamestate.player.displacement;

        // allows the draw thread to update and assign new previous values
        sleep(5);
    }
#endif
#ifdef GAMESTATE
    while(1)
    {
        // if the response is greater than 0, valid data was returned
        // to the gamestate. If not, no valid data was returned and
        // thread is put to sleep to avoid deadlock.
        G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);
        ReceiveData( (uint8_t*)&gamestate.player, sizeof(gamestate.player));
        G8RTOS_SignalSemaphore(&CC3100_SEMAPHORE);

        sleep(2);
    }
#endif
}

/*
 * Generate Ball thread
 */
// Adds another ball thread. Sleeps proportional to the number of balls
// currently in play.
void GenerateBall()
{
    while(1)
    {
        // wake up the next available ball in the array if
        // the max number of balls have not been generated.
        if ( ballCount < MAX_NUM_OF_BALLS )
        {
            G8RTOS_AddThread(&MoveBall, 10, 0xFFFFFFFF, "MOVE_BALL_______");
            ballCount++;
        }
        sleep(ballCount * BALL_GEN_SLEEP);
    }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost()
{
    int16_t avg = 0;
    int16_t joystick_x = 0;
    int16_t joystick_y = 0;
    int16_t displacement = 0;

    while(1)
    {
        // moving average of the joystick inputs
        GetJoystickCoordinates(&joystick_x, &joystick_y);
        avg = (avg + joystick_x + JOYSTICK_BIAS_HOST) >> 1;

        // use the joystick controls to increase and decrease
        // the player velocity
//        switch(avg) {
//        case -10000 ... -6000   : displacement = 10;   break;
//        case -5999 ... -4000    : displacement = 5;    break;
//        case -3999 ... -2000    : displacement = 3;    break;
//        case -1999 ... -750     : displacement = 1;    break;
//        case -749  ... 750      : displacement = 0;     break;
//        case 751   ... 2000     : displacement = -1;     break;
//        case 2001  ... 4000     : displacement = -3;     break;
//        case 4001  ... 6000     : displacement = -5;     break;
//        case 6001  ... 10000    : displacement = -10;    break;
//        default                 : displacement = 0;     break;
//        }

        // The switch statement was causing about 500 ms of lag
        displacement = -(avg >> 9);
        sleep(10);  // sleep before updating host's position to make it fair for client

        // UPDATE JOYSTICK HOST ----------------------------------------------
        // move the player's center
        gamestate.players[0].currentCenter += displacement;

        // player center is too far to the left - limit it.
        if ( gamestate.players[0].currentCenter - PADDLE_LEN_D2 <= ARENA_MIN_X )
        {
            gamestate.players[0].currentCenter = ARENA_MIN_X + PADDLE_LEN_D2 + 1;
        }

        // player center is too far to the right - limit it.
        else if ( gamestate.players[0].currentCenter + PADDLE_LEN_D2 > ARENA_MAX_X - 1 )
        {
            gamestate.players[0].currentCenter = ARENA_MAX_X - PADDLE_LEN_D2 - 1;
        }


        // UPDATE JOYSTICK CLIENT --------------------------------------------
        // Once the host receives valid data from the client, it is allowed to
        // increment the player position.
        // G8RTOS_WaitSemaphore(&GAMESTATE_SEMAPHORE);
        // update the player's center
        gamestate.players[1].currentCenter += gamestate.player.displacement;

        // player center is too far to the left - limit it.
        if ( gamestate.players[1].currentCenter - PADDLE_LEN_D2 <= ARENA_MIN_X )
        {
            gamestate.players[1].currentCenter = ARENA_MIN_X + PADDLE_LEN_D2 + 1;
        }

        // player center is too far to the right - limit it.
        else if ( gamestate.players[1].currentCenter + PADDLE_LEN_D2 > ARENA_MAX_X - 1 )
        {
            gamestate.players[1].currentCenter = ARENA_MAX_X - PADDLE_LEN_D2 - 1;
        }
        // G8RTOS_SignalSemaphore(&GAMESTATE_SEMAPHORE);

        sleep(5);
    }
}

/*
 * SUMMARY: Thread to move a single ball
 *
 * DESCRIPTION: This thread is responsible for moving the ball at
 *              a constant velocity, handling collisions, and
 *              incrementing the score for whichever player scored.
 */
void MoveBall()
{
#define JAKES_VERSION
    // initialize the ball properties for the ball
    // that will be used in this thread.

#ifdef DANS_VERSION
    Ball_t * ball;
    Ball_t new_ball; // used in while loop..
    for (int i = 0; i < MAX_NUM_OF_BALLS; i++)
    {
        ball = &gamestate.balls[i];
        if ( ball->alive == 0 )
        {
            ball->alive = 1;
            ball->color = LCD_WHITE;
            ball->currentCenterX = BALL_SIZE * (rand() % (ARENA_MAX_X/BALL_SIZE)) + ARENA_MIN_X;
            ball->currentCenterY = BALL_SIZE * (rand() % (ARENA_MAX_Y/BALL_SIZE));
            ballCount++;
            break;
        }
    }

    int16_t xvel = rand() % MAX_BALL_SPEED + 1;
    int16_t yvel = rand() % MAX_BALL_SPEED + 1;

    while(1)
    {
        // adjust velocity based on accelerometer here..

        // collision detection if reaching the left or right side borders
        // UPDATE TO USE MINKOWSKI ALGORITHM.
        if ( ball->currentCenterX + BALL_SIZE >= ARENA_MAX_X
                || ball->currentCenterX <= ARENA_MIN_X      )
        {
            xvel *= -1; // go the opposite direction
        }

        // collision detection and color change if the ball hits a player

        // player scores

        // attempt to move ball in current direction
        new_ball.currentCenterX += xvel;
        new_ball.currentCenterY += yvel;

        // update ball on screen
        UpdateBallOnScreen(ball, &new_ball, new_ball.color);
        *ball = new_ball;
        sleep(35);
    }
#endif

#ifdef JAKES_VERSION
    Ball_t * ball;
    PrevBall_t * previousBall;

    for (int i = 0; i < MAX_NUM_OF_BALLS; i++)
    {
        ball = &gamestate.balls[i];
        previousBall = &previousBalls[i];
        if ( ball->alive == 0 )
        {
            previousBall->CenterX = 0;
            previousBall->CenterY = 120;
            ball->alive = 1;
            ball->kill = 0;
            ball->color = LCD_WHITE;
            ball->currentCenterX = BALL_SIZE * (rand() % ((ARENA_MAX_X - ARENA_MIN_X - BALL_SIZE)/BALL_SIZE)) + ARENA_MIN_X + 1;
            ball->currentCenterY = BALL_SIZE * (rand() % (((ARENA_MAX_Y - 80 - ARENA_MIN_Y)/BALL_SIZE)) + 10);
            break;
        }
    }

    int16_t xvel = rand() % MAX_BALL_SPEED + 1;
    int16_t yvel = rand() % MAX_BALL_SPEED + 1;

    if(ball->currentCenterY > 120){
        yvel = -yvel;
    }

    while(1){

        // test if hitting the right or left side wall
        if((xvel > 0 && ball->currentCenterX + xvel + BALL_SIZE + 1 >= ARENA_MAX_X) ||
           (xvel < 0 && ball->currentCenterX + xvel - 1 <= ARENA_MIN_X)){
            xvel = -xvel;
        }

        // test if hitting the top or bottom paddle
        if(((yvel > 0 && ball->currentCenterY + yvel + BALL_SIZE + 1 >= ARENA_MAX_Y - PADDLE_WID) &&
            (ball->currentCenterX >= gamestate.players[0].currentCenter - PADDLE_LEN_D2 - PADDLE_BUFFER)  &&
            (ball->currentCenterX <= gamestate.players[0].currentCenter + PADDLE_LEN_D2 + BALL_SIZE + PADDLE_BUFFER)) ||
           ((yvel < 0 && ball->currentCenterY + yvel - 1 <= ARENA_MIN_Y + PADDLE_WID) &&
            (ball->currentCenterX >= gamestate.players[1].currentCenter - PADDLE_LEN_D2 - PADDLE_BUFFER)  &&
            (ball->currentCenterX <= gamestate.players[1].currentCenter + PADDLE_LEN_D2 + BALL_SIZE + PADDLE_BUFFER))){

            // reverse the y direction
            yvel = -yvel;

            // change color and ball directions for player 2
            if(yvel > 0){
                ball->color = PLAYER_BLUE;
                if(ball->currentCenterX < gamestate.players[1].currentCenter - PADDLE_LEN_D2>>1){
                    // left 1/4 of client side
                    xvel -= 1;
                    if(xvel > 0){
                        yvel += 1;
                    }
                    else{
                        yvel -= 1;
                    }
                }
                else if(ball->currentCenterX > gamestate.players[1].currentCenter + PADDLE_LEN_D2>>1){
                    // right 1/4 of client side
                    xvel += 1;
                    if(xvel < 0){
                        yvel += 1;
                    }
                    else{
                        yvel -= 1;
                    }
                }
                if(yvel == 0){  yvel = 1;}
            }

            // change color and ball directions for player 1
            else{
                ball->color = PLAYER_RED;
                if(ball->currentCenterX < gamestate.players[0].currentCenter - PADDLE_LEN_D2>>1){
                    // left 1/4 of host side
                    xvel += 1;
                    if(xvel > 0){
                        yvel += 1;
                    }
                    else{
                        yvel -= 1;
                    }
                }
                else if(ball->currentCenterX > gamestate.players[0].currentCenter + PADDLE_LEN_D2>>1){
                    // right 1/4 of host side
                    xvel -= 1;
                    if(xvel < 0){
                        yvel += 1;
                    }
                    else{
                        yvel -= 1;
                    }
                }
                if(yvel == 0){  yvel = -1;}
            }
        }
        else if((yvel > 0 && ball->currentCenterY + yvel + BALL_SIZE + 1 >= ARENA_MAX_Y - PADDLE_WID) ||
                (yvel < 0 && ball->currentCenterY + yvel <= ARENA_MIN_Y + PADDLE_WID)){

            // kill this ball, and score the point
            // set ball to dead
            ball->alive = 1;

            // score points
            if(yvel > 0 && (ball->color == LCD_BLUE || ball->color == LCD_RED)){
                gamestate.LEDScores[1] += 1;
            }
            else if(ball->color == LCD_BLUE || ball->color == LCD_RED){
                gamestate.LEDScores[0] += 1;
            }
            if(gamestate.LEDScores[0] == 8 || gamestate.LEDScores[1] == 8){
              
   #ifdef SINGLE
                G8RTOS_AddThread( &EndOfGameHost, 0, 0xFFFFFFFF,          "KillAll___" );
   #endif
              gamestate.gameDone = true;
            }

            ball->kill = 1;

            G8RTOS_KillSelf();

        }

        ball->currentCenterX       += xvel;
        ball->currentCenterY       += yvel;

        sleep(35);
    }

#endif

}


/*
 * End of game for the host
 */
void EndOfGameHost()
{
    // wait for semaphores
    G8RTOS_WaitSemaphore(&LCDREADY);
    G8RTOS_WaitSemaphore(&LEDREADY);
    G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);

    G8RTOS_KillAllOthers();
  
    // force semaphores to reset here..
  // shouldn't be required, but fixes semaphore block bug
    G8RTOS_InitSemaphore(&LCDREADY, 1);
    G8RTOS_InitSemaphore(&LEDREADY, 1);
    G8RTOS_InitSemaphore(&CC3100_SEMAPHORE, 1);

    // determine winner
    if(gamestate.LEDScores[0] == 8){
        //host wins, increment score and color screen
        gamestate.overallScores[0] += 1;
        LCD_Clear(LCD_RED);
    }
    else if(gamestate.LEDScores[1] == 8){
        //client wins, increment score and color screen
        gamestate.overallScores[1] += 1;
        LCD_Clear(LCD_BLUE);
    }
    gamestate.LEDScores[0] = 0;
    gamestate.LEDScores[1] = 0;
    for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
        gamestate.balls[i].alive = 0;
        gamestate.balls[i].kill = 0;
    }

    ballCount = 0;
    playerCount = 2;

    // 1. Initialize players' general parameters
    // player 1 initializations...
    GeneralPlayerInfo_t * tempPlayer;
    tempPlayer = &gamestate.players[0];
    tempPlayer->color = PLAYER_RED;
    tempPlayer->currentCenter = MAX_SCREEN_X / 2;
    tempPlayer->position = BOTTOM;

    // player 2 initializations...
    tempPlayer = &gamestate.players[1];
    tempPlayer->color = PLAYER_BLUE;
    tempPlayer->currentCenter = MAX_SCREEN_X / 2;
    tempPlayer->position = TOP;

    setLedMode_lp3943( RED, 0x0000);
    setLedMode_lp3943( BLUE, 0x0000);

    // delay for 1 secondish
    for (long long i = 0; i < 1000000; i++);

    writeGameMenuHost(LCD_WHITE);
    nextState = NA;   // set next game state to NA
    buttons_init();
    while(nextState == NA);

    // send response to the client
    gamestate.gameDone = false;

    if(nextState == EndGame){ // if the game should end here, notify the client to KillSelf

        // thanks for playing
        // reset leds on board
        gamestate.winner = true;    // this notifies client kill all threads
        SendData((uint8_t*)&gamestate, gamestate.player.IP_address, sizeof(gamestate));
        G8RTOS_KillSelf();
    }
    else // if the game shouldn't end here, notify the client to restart
    {
        gamestate.winner = false;   // this notifies client to restart the board
        SendData((uint8_t*)&gamestate, gamestate.player.IP_address, sizeof(gamestate));
    }


    GREEN_ON; // use LED to indicate WiFi connection as HOST

    // 5. Initialize the board (draw arena, players, scores)
    InitBoardState();

    // 6. Add GenerateBall, DrawObjects, ReadJoystickHost, SendDataToClient
    //      ReceiveDataFromClient, MoveLEDs (low priority), Idle
    addHostThreads();

    // 7. Kill self.
    G8RTOS_KillSelf();
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
#ifdef MULTI
void JoinGame()
{
    initCC3100(Client); // connect to the network

    // 1. Set initial SpecificPlayerInfo_t strict attributes ( getLocalIP() ).
    client_player.IP_address = getLocalIP();
    client_player.acknowledge = false;
    client_player.displacement = 0;
    client_player.joined = false;
    client_player.playerNumber = playerCount;
    client_player.ready = false;

    // hardware initialization
    P2->OUT &= ~(BIT0 | BIT1 | BIT2); // initialize led's off
    P2->DIR |= (BIT0 | BIT1 | BIT2); // set R.G.B direction

#ifndef HANDSHAKE2
    // 2. Send player data into the host.
    // 3. Try to send player packet to host until host acknowledges
    //    that it received the player information.
    uint8_t handshake = 'X';  // either 'H' or 'C' to show message sent or received from Host or Client.
    while( handshake != 'H' )
    {
        handshake = 'C';
        SendData(       (_u8*)&handshake, HOST_IP_ADDR, 1               );   // start synchronization
        SendData(       (_u8*)&player, HOST_IP_ADDR, sizeof(player)     );   // send MSB first of all player data
        ReceiveData(    (_u8*)&handshake, sizeof(handshake)             );   // check if host ackknowledges
    }

    // Sync with host by telling it that client has received the
    // previous message
    handshake = 'C';
    SendData( (_u8*)&handshake, HOST_IP_ADDR, 1 );
#endif
#ifdef HANDSHAKE2

    // wait for the host to receive message and notify
    // client that they joined the game.
    do
    {
        SendData(       (uint8_t*)&client_player, HOST_IP_ADDR, sizeof(client_player) );   // start handshake
        ReceiveData(    (uint8_t*)&gamestate, sizeof(gamestate) );   // check if host acknowledges
    } while( gamestate.player.joined == false );

    // 4. Acknowledge client to tell them they have received
    // the message about joining the game and the game can begin.
    client_player.acknowledge = true;
    SendData( (uint8_t*)&client_player, HOST_IP_ADDR, sizeof(client_player) );

#endif

    // 4. If you've joined the game, acknowledge you've joined to the host
    //      and show connection through LED.
    BLUE_ON;

    // 5. Initialize board state, semaphores, and add the following threads..
    //      ReadJoystickClient, SendDataToHost, ReceiveDataFromHost, DrawObjects,
    //      MoveLEDs, Idle
    InitBoardState();
  
    addClientThreads();

    // 6. Kill self.
    G8RTOS_KillSelf();
}
#endif

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost()
{
#ifdef PACKETS

    int16_t result = -1;

    while(1)
    {
        do
        {
            // 1. Receive packet from the host
            G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);
            result = ReceiveData( (_u8*)&packet, sizeof(packet));
            G8RTOS_SignalSemaphore(&CC3100_SEMAPHORE);
        } while ( result < 0 );

        // 2. Update the gamestate information if it is not being
        //      used by another thread currently.
        emptyPacket(&gamestate, &packet);

        // 3. Check if the game is done. Add EndOfGameHost thread if done.
        if ( gamestate.gameDone == true )
            G8RTOS_AddThread(EndOfGameClient, 0, 0xFFFFFFFF, "END_GAME_CLIENT_");

        // allows update draw thread to run and update previous balls and players
        sleep(5);
    }
#endif
#ifdef GAMESTATE

    while(1)
    {

        // 1. Receive packet from the host
        G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);
        ReceiveData( (_u8*)&gamestate, sizeof(gamestate));
        G8RTOS_SignalSemaphore(&CC3100_SEMAPHORE);

        // 3. Check if the game is done. Add EndOfGameHost thread if done.
        if ( gamestate.gameDone == true )
            G8RTOS_AddThread(EndOfGameClient, 0, 0xFFFFFFFF, "END_GAME_CLIENT_");

        sleep(2);
    }
#endif
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{
#ifdef PACKETS

    while(1)
    {
        fillPacket(&gamestate, &packet);

        // Send data to host in this order. Corrects issue with
        // Host receiving displacement inside of IP_address.
        G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);
        SendData( (_u8*)&packet.player, HOST_IP_ADDR, sizeof(packet.player));  // forcing order
        G8RTOS_SignalSemaphore(&CC3100_SEMAPHORE);

        sleep(2);
    }
#endif
#ifdef GAMESTATE

    while(1)
    {
        G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);
        SendData( (_u8*)&client_player, HOST_IP_ADDR, sizeof(client_player) );
        G8RTOS_SignalSemaphore(&CC3100_SEMAPHORE);

        sleep(5);
    }
#endif
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient()
{
    int16_t avg = 0;
    int16_t joystick_x = 0;
    int16_t joystick_y = 0;

    while(1)
    {
        // moving average of the joystick inputs
        GetJoystickCoordinates(&joystick_x, &joystick_y);
        avg = (avg + joystick_x + JOYSTICK_BIAS_CLIENT) >> 1;

        // use the joystick controls to increase and decrease
        // the player velocity
        // switch(avg) {
        // case -10000 ... -6000   : displacement = 3;   break;
        // case -5999 ... -4000    : displacement = 2;    break;
        // case -3999 ... -2000    : displacement = 1;    break;
        // case -1999 ... -750     : displacement = 1;    break;
        // case -749  ... 750      : displacement = 0;     break;
        // case 751   ... 2000     : displacement = -1;     break;
        // case 2001  ... 4000     : displacement = -1;     break;
        // case 4001  ... 6000     : displacement = -2;     break;
        // case 6001  ... 10000    : displacement = -3;    break;
        // default                 : displacement = 0;     break;
        // }

        // The switch statement was causing about 500 ms of lag
        displacement = -(avg >> 9);

        // move the player's center
        client_player.displacement = displacement;

        sleep(10);
    }
}

/*
 * End of game for the client
 */
void EndOfGameClient()
{
    while(1)
    {
        // wait for semaphores
        G8RTOS_WaitSemaphore(&LCDREADY);
        G8RTOS_WaitSemaphore(&LEDREADY);
        G8RTOS_WaitSemaphore(&CC3100_SEMAPHORE);

        G8RTOS_KillAllOthers();

        G8RTOS_InitSemaphore(&LEDREADY, 1);
        G8RTOS_InitSemaphore(&LCDREADY, 1);
        G8RTOS_InitSemaphore(&CC3100_SEMAPHORE, 1);

        // determine winner
        if(gamestate.LEDScores[0] == 8){
            //host wins, increment score and color screen
            gamestate.overallScores[0] += 1;
            LCD_Clear(LCD_RED);
        }
        else if(gamestate.LEDScores[1] == 8){
            //client wins, increment score and color screen
            gamestate.overallScores[1] += 1;
            LCD_Clear(LCD_BLUE);
        }
        gamestate.LEDScores[0] = 0;
        gamestate.LEDScores[1] = 0;
        for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
            gamestate.balls[i].alive = 0;
            gamestate.balls[i].kill = 0;
        }

        ballCount = 0;
        playerCount = 2;

        setLedMode_lp3943( RED, 0x0000);
        setLedMode_lp3943( BLUE, 0x0000);

        // delay for 1 secondish
        for (long long i = 0; i < 1000000; i++);

        writeGameMenuClient(LCD_WHITE);

        // WAIT TO RECEIVE MESSAGE FROM CLIENT HERE..
        do
        {
            ReceiveData((uint8_t*)&gamestate, sizeof(gamestate));
        } while ( gamestate.gameDone == true );

        // defines the client player's starting center value because we
        // treat the displacement as a center value.
        client_player.displacement = gamestate.players[1].currentCenter;

        if(gamestate.winner == true){

            // thanks for playing
            // reset leds on board
            G8RTOS_KillSelf();
        }


        BLUE_ON; // use LED to indicate WiFi connection as HOST

        // 5. Initialize the board (draw arena, players, scores)
        InitBoardState();

        // 6. Add GenerateBall, DrawObjects, ReadJoystickHost, SendDataToClient
        //      ReceiveDataFromClient, MoveLEDs (low priority), Idle
        addClientThreads();

        // 7. Kill self.
        G8RTOS_KillSelf();
    }
}

// ===================== COMMON COMMON COMMON COMMON =========================
/*
 * Thread to draw all the objects in the game
 */
void DrawObjects()
{
    // store all previous locations of the players to see if
    // it needs to be updated.
    PrevPlayer_t prevPlayers[MAX_NUM_OF_PLAYERS];
    for (int i = 0; i < MAX_NUM_OF_PLAYERS; i++)
    {
        prevPlayers[i].Center = -1; // load fake values to determine first run
    }

    while(1)
    {
        // Draw players --------------------
        for (int i = 0; i < playerCount; i++)
        {
            // This player is on its first run. Draw
            // the entire paddle.
            if ( prevPlayers[i].Center == -1 ) {
                DrawPlayer( &gamestate.players[i] );
                prevPlayers[i].Center = gamestate.players[i].currentCenter;
            }
            // if this player has already been drawn, only
            // update the parts that need to be redrawn.
            else
            {
                // G8RTOS_WaitSemaphore(&GAMESTATE_SEMAPHORE);
                UpdatePlayerOnScreen( &prevPlayers[i], &gamestate.players[i]);
                // G8RTOS_SignalSemaphore(&GAMESTATE_SEMAPHORE);
            }
        }

        // Draw the ping pong balls ----------
        // STATE MACHINE..
        for(int i = 0; i < MAX_NUM_OF_BALLS; i++){

            // IF AT ORIGIN, THEN OFFSET Y TO NON OCCUPIED SPACE
            if(previousBalls[i].CenterX < ARENA_MIN_X){
                previousBalls[i].CenterY = 120;
            }
            // ALIVE && !KILL = REDRAW STATE
            if(gamestate.balls[i].alive && !gamestate.balls[i].kill){
                UpdateBallOnScreen(&previousBalls[i], &gamestate.balls[i], gamestate.balls[i].color);
            }

            // ALIVE && KILL = KILL HOST STATE
            else if(gamestate.balls[i].alive && gamestate.balls[i].kill){
                UpdateBallOnScreen(&previousBalls[i], &gamestate.balls[i], LCD_BLACK);
                gamestate.balls[i].alive = 0;
                ballCount--;
            }

            // !ALIVE && KILL = KILL CLIENT STATE
            // Because our sleep is 20 ms which is very high, our while loop
            // doesn't run again before the packet is sent. This allows the
            // client to enter this state before the host enters this state
            // and delete the ball on the client side.
            else if ( !gamestate.balls[i].alive && gamestate.balls[i].kill )
            {
                UpdateBallOnScreen(&previousBalls[i], &gamestate.balls[i], LCD_BLACK);
                gamestate.balls[i].kill = 0;
            }

            // !ALIVE && !KILL = NULL STATE
            // else do nothing .. (ALL 4 STATES ARE USED WITH THESE 2 BOOLEANS)
        }

        // Refresh rate --------------------
        sleep(20);
    }
}

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs()
{
    setLedPwm_lp3943( RED, 0xFF );
    setLedPwm_lp3943( BLUE, 0xFF );

    while(1){
        uint8_t hostScore = gamestate.LEDScores[0];
        uint8_t clientScore = gamestate.LEDScores[1];
        uint8_t hostLEDS = 0;
        uint8_t clientLEDS = 0;
        for(int i = 0; i < hostScore; i++){
            hostLEDS = (hostLEDS<<1) + 1;
        }
        for(int i = 0; i < clientScore; i++){
            clientLEDS = (clientLEDS>>1) + 0x80;
        }
        uint16_t REDdata = (hostLEDS<<8);
        uint16_t BLUEdata = clientLEDS;

        G8RTOS_WaitSemaphore(&LEDREADY);

        setLedMode_lp3943( RED, REDdata);
        setLedMode_lp3943( BLUE, BLUEdata);

        G8RTOS_SignalSemaphore(&LEDREADY);

        sleep(100);
    }
}

/*
 * Idle thread to avoid deadlocks and RTOS end
 */
void IdleThread() { while(1); }
