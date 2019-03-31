#define MAIN

#include "G8RTOS.h"
#include "LCD_empty.h"
#include "threads.h"

#define TP_ENABLE   1
#define TP_DISABLE  0

#ifdef MAIN
void main(void)
{

    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    // Initialize and launch RTOS
    G8RTOS_Init();
    LCD_Init(TP_DISABLE);

    // Add threads
    G8RTOS_AddThread( &IdleThread, 255, 0xFFFFFFFF, "IDLE____________" ); // lowest priority

    // launch RTOS
    G8RTOS_Launch_Priority();
}
#endif
