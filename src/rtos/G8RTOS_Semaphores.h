/*
 * G8RTOS_Semaphores.h
 */

#ifndef G8RTOS_SEMAPHORES_H_
#define G8RTOS_SEMAPHORES_H_

/*********************************************** Datatype Definitions *****************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "G8RTOS_CriticalSection.h"

/*
 * Semaphore typedef
 */
typedef int32_t semaphore_t;

/*********************************************** Datatype Definitions *****************************************************************/
#include "G8RTOS_Structures.h"


/*********************************************** Public Functions *********************************************************************/

void StartContextSwitch( void );

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value);

/*
 * Waits for a semaphore to be available (value greater than 0)
 * 	- Decrements semaphore when available
 * 	- Spinlocks to wait for semaphore
 * Param "s": Pointer to semaphore to wait on
 */
void G8RTOS_WaitSemaphore(semaphore_t *s);

/*
 * Signals the completion of the usage of a semaphore
 * 	- Increments the semaphore value by 1
 * Param "s": Pointer to semaphore to be signalled
 */
void G8RTOS_SignalSemaphore(semaphore_t *s);

/*********************************************** Public Functions *********************************************************************/


#endif /* G8RTOS_SEMAPHORES_H_ */
