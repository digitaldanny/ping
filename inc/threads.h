/*
 * threads.h
 *
 *  Created on: Mar 30, 2019
 *      Author: Daniel Hamilton
 */

#ifndef THREADS_H_
#define THREADS_H_

// Idle thread set to lowest priority. Use to make sure
// the RTOS never exits "G8RTOS_Launch_Priority()" function.
// Also used to make sure deadlocks never happen.
void IdleThread ( void );

#endif /* THREADS_H_ */
