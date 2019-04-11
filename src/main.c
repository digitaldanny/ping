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
playerType  myPlayerType = None;    // undefined to avoid launching threads
uint8_t     GameInitMode = 1;       // determines if the buttons are used as game controls or menu navigation



#ifdef MAIN

// ================================== MAIN PROGRAM ===============================
void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    // Initialize and launch RTOS
    G8RTOS_Init();
    buttons_init();
    LCD_Init(TP_DISABLE);

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
            //G8RTOS_AddThread( &JoinGame, 0, 0xFFFFFFFF, "JOIN_GAME_______" ); // lowest priority
            break;
        }
    }

    // Erase the main menu before beginning the game
    writeMainMenu(MENU_BG_COLOR);

    // launch RTOS
    G8RTOS_Launch_Priority();
}

// ================================ INTERRUPT SERVICE ROUTINES ===========================
void PORT4_IRQHandler( void )
{

#ifndef BUTTON_BUG
    // B0 = UP (BIT4), B1 = RIGHT (BIT5) -------------------------
    if ( (P4->IFG & BIT4 || P4->IFG & BIT5) && GameInitMode == 1 )
    {
        NVIC_DisableIRQ(PORT4_IRQn);
        myPlayerType = Host;
        GameInitMode = 0;
    }
#endif

#ifdef BUTTON_BUG
    // Daughter board buttons B2, B3 need to be resoldered.
    // Until then, use this configuration...
    // B0 = UP (BIT4), B1 = RIGHT (BIT5) -------------------------
    if ( (P4->IFG & BIT4) && GameInitMode == 1 )
    {
        NVIC_DisableIRQ(PORT4_IRQn);
        NVIC_DisableIRQ(PORT5_IRQn);
        myPlayerType = Host;
        GameInitMode = 0;
    }
    else if ( (P4->IFG & BIT5) && GameInitMode == 1 )
    {
        NVIC_DisableIRQ(PORT4_IRQn);
        NVIC_DisableIRQ(PORT5_IRQn);
        myPlayerType = Client;
        GameInitMode = 0;
    }

    // determine the next game mode
    else if ( (P4->IFG & BIT4))
    {
        NVIC_DisableIRQ(PORT4_IRQn);
        NVIC_DisableIRQ(PORT5_IRQn);
        nextState = NextGame;
    }
    else if ( (P4->IFG & BIT5))
    {
        NVIC_DisableIRQ(PORT4_IRQn);
        NVIC_DisableIRQ(PORT5_IRQn);
        nextState = EndGame;
    }
#endif

    // B0 = UP (BIT4), B1 = RIGHT (BIT5) --------------------------

    P4->IFG &= ~(BIT4 | BIT5);
}

void PORT5_IRQHandler( void )
{

    // MAIN MENU NAVIGATION MODE --------------------------------------
    if ( (P5->IFG & BIT4 || P5->IFG & BIT5) && GameInitMode == 1 )
    {
        NVIC_DisableIRQ(PORT5_IRQn);
        NVIC_DisableIRQ(PORT4_IRQn);
        myPlayerType = Client;
        GameInitMode = 0;
    }

    // B2 = DOWN (BIT4), B3 = LEFT (BIT5) ------------------------

    P5->IFG &= ~(BIT4 | BIT5);
}
#endif
