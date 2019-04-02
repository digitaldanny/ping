/*
 * G8RTOS_Scheduler.h
 */

#ifndef G8RTOS_SCHEDULER_H_
#define G8RTOS_SCHEDULER_H_

#include <stdbool.h>
#include <stdint.h>
#include "G8RTOS_Structures.h"
#include "G8RTOS_CriticalSection.h"
#include "BSP.h"

/*********************************************** Sizes and Limits *********************************************************************/
#define MAX_THREADS     26
#define MAXPTHREADS     1
#define STACKSIZE       256
#define OSINT_PRIORITY  7

#define DONT_STARVE_PRIORITY    10
#define DONT_STARVE_AGE         100
#define DEF_QUANT_COUNT         1
/*********************************************** Sizes and Limits *********************************************************************/

/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
extern uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

// Error codes for the scheduler
typedef enum
{
    NO_ERROR                    = 0,
    THREAD_LIMIT_REACHED        = -1,
    NO_THREADS_SCHEDULED        = -2,
    THREADS_INCORRECTLY_ALIVE   = -3,
    THREAD_DOES_NOT_EXIST       = -4,
    CANNOT_KILL_LAST_THREAD     = -5,
    IRQn_INVALID                = -6,
    HWI_PRIORITY_INVALID        = -7
} sched_err_code_t;

/* Points to next and previous tcb's when using priority operation */
typedef struct OrganizedPriorityObject
{

    uint32_t empty_slot;
    tcb_t* current;
    tcb_t* next;
    tcb_t* prev;
    sched_err_code_t err;

} OrganizedPriorityObject_t;

/*
 * Initializes variables and hardware for G8RTOS usage
 */
void G8RTOS_Init();

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes Systick Timer
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch();
sched_err_code_t G8RTOS_Launch_Priority();

void G8RTOS_Scheduler(void);
void G8RTOS_Scheduler_Priority(void);


threadId_t G8RTOS_GetThreadId();
sched_err_code_t G8RTOS_KillThread( threadId_t id );
sched_err_code_t G8RTOS_KillSelf();

/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
OrganizedPriorityObject_t FindEmptyTcb(uint8_t priority);    // helper func
sched_err_code_t G8RTOS_AddThread__Def_Starvation(void (*threadToAdd)(void), uint8_t priority, char * name);  // helper func
sched_err_code_t G8RTOS_AddThread( void (*threadToAdd)(void), uint8_t priority, uint32_t starvation_age, char * name );
/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
sched_err_code_t G8RTOS_AddAperiodicEvent_Priority(void (*PthreadToAdd)(void), uint32_t priority, IRQn_Type IRQn);
sched_err_code_t G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period);


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS);

/*********************************************** Public Functions *********************************************************************/

#endif /* G8RTOS_SCHEDULER_H_ */
