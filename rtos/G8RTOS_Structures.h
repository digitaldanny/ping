/*
 * G8RTOS_Structure.h
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include <stdbool.h>
#include <stdint.h>
#include "msp.h"
#include "G8RTOS_Semaphores.h"

/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - For Lab 2 the TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */

#define MAX_NAME_LENGTH     16

typedef uint32_t threadId_t;

/* Thread control block */
struct tcb
{
    int32_t *sp;            // current tcb stack pointer - MUST BE FIRST ADDRESS
    struct tcb *next;       // next tcb pointer
    struct tcb *prev;       // previous tcb pointer

    semaphore_t *blocked;   // address of the semaphore blocking the thread

    uint8_t asleep;         // boolean to test if the thread has yielded
    uint32_t sleep_count;   // number of SysTicks before thread expects to be woken up

    uint8_t priority;       // 0 is the highest priority, 255 is the lowest
    uint8_t priority_perm;  // permanent priority assigned at init doesn't allow low level threads to starve
    uint32_t age;
    uint32_t starvation_age; // age met before the priority is automatically set to level 10

    bool alive;          // 0 is dead, 1 is alive
    char name[MAX_NAME_LENGTH];
    threadId_t id;     // used to quickly find a specific thread
};

typedef struct tcb tcb_t; // typedef the tcb structure

/*
 *  Periodic Thread Control Block:
 *      - Holds a function pointer that points to the periodic thread to be executed
 *      - Has a period in us
 *      - Holds Current time
 *      - Contains pointer to the next periodic event - linked list
 */
struct ptcb
{
    void (*handler)(void); // function pointer to the periodic thread to be executed
    uint32_t period;    // period of execution
    uint32_t exec_time; // time that the execution needs to begin (compare to system time)
    uint32_t curr_time; // ??
    struct ptcb *next;  // pointer to the next ptcb in the linked list
    struct ptcb *prev;  // pointer to the previous ptcb in the linked list
};

typedef struct ptcb ptcb_t;

/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

tcb_t * CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/




#endif /* G8RTOS_STRUCTURES_H_ */
