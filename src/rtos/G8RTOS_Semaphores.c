/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/
#include "G8RTOS_Semaphores.h"

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
    uint32_t primask = StartCriticalSection();

    *s = value;

    EndCriticalSection(primask);
    __enable_interrupt();
}

// triggers PendSV handler starting context
// switch to next unblocked thread
void StartContextSwitch( void )
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*
 * No longer waits for semaphore
 *  - Decrements semaphore
 *  - Blocks thread is semaphore is unavalaible
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s)
{
    uint32_t primask = StartCriticalSection();

    (*s)--; // decrement semaphore if it is available

    // add blocked thread and force a context switch
    if ( (*s) < 0 )
    {
        CurrentlyRunningThread->blocked = s;    // blocked is given address of the semaphore
        __enable_interrupt();
        StartContextSwitch();
    }

    EndCriticalSection(primask);
    __enable_interrupt();
}

/*
 * Signals the completion of the usage of a semaphore
 *  - Increments the semaphore value by 1
 *  - Unblocks any threads waiting on that semaphore
 * Param "s": Pointer to semaphore to be signaled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
    uint32_t primask = StartCriticalSection();  // ints disabled

    (*s)++;

    // cycle through tcb's until a tcb that is not blocked
    // by this semaphore shows up.
    if ( (*s) <= 0 )
    {
        //temp points to the next thread's tcb
        tcb_t *pt = CurrentlyRunningThread->next;

        // continue cycling until it finds a thread that is
        // not blocked by this semaphore
        while(pt->blocked != s)
        {
            pt = pt->next;
        }

        pt->blocked = 0;
    }

    EndCriticalSection(primask);
    __enable_interrupts();      // ints enabled
    return;
}

/*********************************************** Public Functions *********************************************************************/


