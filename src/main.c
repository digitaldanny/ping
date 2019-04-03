#define MAIN

// To run game with different routers, go to the
// cc3100_usage.h and sl_common.h header files and
// adjust the preprocessor directive.

/*
 *
 *  main.c
 *
 *  UPDATES :
 *  3/29/2019   : File initialization.
 *  4/2/2019    : Connect to hotspot as Host or Client.
 *
 */

#include "G8RTOS.h"
#include "cc3100_usage.h"
#include "LCD_empty.h"
#include "Game.h"
#include "threads.h"

#define TP_ENABLE   1
#define TP_DISABLE  0

#ifdef MAIN
void main(void)
{

    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    // Initialize and launch RTOS
    G8RTOS_Init();
    initCC3100(Host);
    LCD_Init(TP_DISABLE);

    // Add threads
    G8RTOS_AddThread( &IdleThread, 255, 0xFFFFFFFF, "IDLE____________" ); // lowest priority

    // launch RTOS
    G8RTOS_Launch_Priority();
}
#endif
