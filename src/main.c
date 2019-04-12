#define MAIN
#define BUTTON_BUG

// To run game with different routers, go to the
// cc3100_usage.h and sl_common.h header files and
// adjust the preprocessor directive.

/*
 *
 *  main.c
 *
 *  UPDATES :
 *  3/29/2019   : File initialization.
 *  4/2/2019    : Connect to game as Host or Client.
 *	4/10/2019	: LCD and LED semaphores initialized.
 *
 *  TODO    :
 *  ~  Remove BUTTON_BUG directive once Daniel's board has been
 *      resoldered.
 *
 */

#include "G8RTOS.h"
#include "cc3100_usage.h"
#include "LCD_empty.h"
#include "Game.h"

// MACROS --------------------------------------------
#define TP_ENABLE   1
#define TP_DISABLE  0

// GLOBALS -------------------------------------------

#ifdef MAIN

// ================================== MAIN PROGRAM ===============================
void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    // Initialize and launch RTOS
    G8RTOS_Init();
    buttons_init();
    LCD_Init(TP_DISABLE);

    G8RTOS_AddAperiodicEvent_Priority(&ButtonPress, 0, PORT4_IRQn);
    G8RTOS_AddAperiodicEvent_Priority(&ButtonPress, 0, PORT5_IRQn);

    // write the menu text
    writeMainMenu(MENU_TEXT_COLOR);

    // Initialize semaphores
    G8RTOS_InitSemaphore(&CC3100_SEMAPHORE, 1);
    G8RTOS_InitSemaphore(&GAMESTATE_SEMAPHORE, 1);
	G8RTOS_InitSemaphore(&LCDREADY, 1);
    G8RTOS_InitSemaphore(&LEDREADY, 1);    
  
    // write the menu text
    writeMainMenu(MENU_TEXT_COLOR);

    // Do not initialize any of the threads until the user decides
    // whether to host the game or to be on the client side.
    while (1)
    {
        if (myPlayerType == Host)
        {
            // Initialize HOST-side threads
            G8RTOS_AddThread( &CreateGame, 0, 0xFFFFFFFF, "CREATE_GAME_____" ); // lowest priority
            break;
        }

        else if ( myPlayerType == Client )
        {
            // Initialize CLIENT-side threads
            G8RTOS_AddThread( &JoinGame, 0, 0xFFFFFFFF, "JOIN_GAME_______" ); // lowest priority
            break;
        }
    }

    // Erase the main menu before beginning the game
    writeMainMenu(MENU_BG_COLOR);

    // launch RTOS
    G8RTOS_Launch_Priority();
}

#endif
