#include "threads.h"

/*
 * threads.c
 *
 *  Created on: Mar 30, 2019
 *      Author: Daniel Hamilton
 *
 *      Description : N/A
 *
 *      UPDATES     :
 *      3/30/2019   : Idle thread and file initialization
 *
 *      TODO        :
 *      1. ??
 */

// Idle thread set to lowest priority. Use to make sure
// the RTOS never exits "G8RTOS_Launch_Priority()" function.
// Also used to make sure deadlocks never happen.
void IdleThread ( void ) { while(1); }
