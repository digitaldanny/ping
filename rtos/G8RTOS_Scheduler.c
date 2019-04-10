#define ORIG

/*
 *  Preprocessor Scheduler Selection
 *  ------------------------------------------------------------------------
 *
 *  ORIG -  Original RTOS sorts through each thread to determine highest
 *          priority. Also incorporates aging to make sure that low priority
 *          threads will not starve.
 *
 *  OPT  -  Automatically sorts the priority of threads on AddThread.
 *          Manages highest priority thread with head thread pointer.
 *
 * -------------------------------------------------------------------------
 */

/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/
#include "G8RTOS_Scheduler.h"

/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

// ID counter to produce unique thread ID's
static uint16_t IDCounter;

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
#ifdef OPT
tcb_t * head;
#endif

extern tcb_t * CurrentlyRunningThread;
ptcb_t * CurrentlyRunningPeriodicThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as invdividual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
static ptcb_t Pthread[MAXPTHREADS];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPthreads;

/* Holds the current time for the whole System */
uint32_t SystemTime;

extern void PendSV_Handler(void);

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(void)
{
    uint32_t clockSysFreq = ClockSys_GetSysFreq();  // find system clock frequency*************
    SysTick_Config(clockSysFreq/1000);                // configure interrupt to 1ms
    SysTick_enableInterrupt();                      // enable interrupt at normal level priority
};

/*
 * Chooses the next thread to run.
 * Lab 2 Scheduling Algorithm:
 * 	- Simple Round Robin: Choose the next running thread by selecting the currently running thread's next pointer
 * 	- Check for sleeping and blocked threads
 */
void G8RTOS_Scheduler(void)
{
    do
    {
        CurrentlyRunningThread = CurrentlyRunningThread->next;  // loop until a thread that is not blocked is found
    } while( CurrentlyRunningThread->asleep || CurrentlyRunningThread->blocked );
}

// Scheduling algorithm using priority instead of round robin
#if defined ORIG
void G8RTOS_Scheduler_Priority(void)
{
    uint8_t max_priority = 255;
    tcb_t* tempThreadPtr = CurrentlyRunningThread;
    tcb_t* tempCurrentPtr = CurrentlyRunningThread;

    // search for the highest priority thread out of
    // all the threads that are not asleep or blocked.
    // if there are no higher or equal priority threads
    // unblocked or awake, do not perform a switch.
    // NOTE: Original thread won't run unless there are no other
    // threads of equal or greater priority.
    // for ( unsigned int i = 0; i < NumberOfThreads; i++ )
    uint16_t tempCount = 0;
    do
    {
        // round robin run the highest priority threads-------------
        if (    tempThreadPtr->priority <= max_priority &&
                tempThreadPtr->alive == true &&
                tempThreadPtr->asleep == 0 && tempThreadPtr->blocked == 0 )
        {
            max_priority = tempThreadPtr->priority;
            CurrentlyRunningThread = tempThreadPtr;

            // Because only one thread should ever change its priority due to starvation
            // at a time, I can immediately run the starved thread and reset it to orig priority
            if ( CurrentlyRunningThread->priority < CurrentlyRunningThread->priority_perm )
            {
                CurrentlyRunningThread->priority = CurrentlyRunningThread->priority_perm;
                break;
            }
        }

        // age the thread so it doesn't starve----------------------
        else
        {
            tempThreadPtr->age++;

            if (tempThreadPtr->age == tempThreadPtr->starvation_age)
            {
                tempThreadPtr->age = 0;
                tempThreadPtr->priority = DONT_STARVE_PRIORITY;
            }
        }

        tempThreadPtr = tempThreadPtr->next;
        tempCount++;
    } while ( tempThreadPtr != tempCurrentPtr && tempCount < NumberOfThreads+1);
}
#endif


/*
 * SysTick Handler
 * The Systick Handler now will increment the system time,
 * set the PendSV flag to start the scheduler,
 * and be responsible for handling sleeping and periodic threads
 */
void SysTick_Handler()
{

    // increment the system time
    SystemTime++;

    // check each ptcb to see if the system time is equal to the periodic
    // count. If it is, run the Pthread
    for (int i = 0; i < NumberOfPthreads; i++)
    {
        CurrentlyRunningPeriodicThread = CurrentlyRunningPeriodicThread->next;

        // run the periodic function if it meets the correct system time
        if ( CurrentlyRunningPeriodicThread->exec_time == SystemTime )
        {
            CurrentlyRunningPeriodicThread->exec_time = SystemTime + CurrentlyRunningPeriodicThread->period;
            CurrentlyRunningPeriodicThread->handler();
        }
    }

    // check each tcb to see if system time equals sleep-count. Wake up.
    // Because this goes to the number of threads, the linked list should
    // wrap all the way back around to the beginning.
    // NOTE: CONTEXT NEVER SWITCHES HERE
    for (int i = 0; i <  NumberOfThreads; i++)
    {
        // check the next thread's data
        CurrentlyRunningThread = CurrentlyRunningThread->next;

        // clear the sleep statement if it is the correct time
        if ( CurrentlyRunningThread->sleep_count == SystemTime )
        {
            CurrentlyRunningThread->asleep = 0x0;
            CurrentlyRunningThread->sleep_count = 0x0;
        }
    }

    // trigger the PendSV interrupt to switch context
    // after running periodic threads + waking threads
    StartContextSwitch();
}

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
    // variable init ------------------------------------
    IDCounter = 0;
    SystemTime = 0;         // initialize system time to 0
    NumberOfThreads = 0;    // set the number of threads to 0
    NumberOfPthreads = 0;
    BSP_InitBoard();        // initialize all hardware on the board

    // move interrupt vector into SRAM ------------------
    uint32_t newVTORTable = 0x20000000;
    memcpy( (uint32_t*)newVTORTable, (uint32_t*)SCB->VTOR, 57*4); //57 int vectors to copy
    SCB->VTOR = newVTORTable;
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch()
{
    CurrentlyRunningPeriodicThread = &Pthread[0];     // set the first periodic event even if there is no data yet
    CurrentlyRunningThread = &threadControlBlocks[0]; // set the first tcb as the currently running thread
    InitSysTick();

    // Set SysTick and PendSV to lowest priority
    NVIC_SetPriority(SysTick_IRQn, 7);  // set SysTick to low priority
    NVIC_SetPriority(PendSV_IRQn, 7);   // set PendSV to low priority

    G8RTOS_Start(&threadControlBlocks[0]); // load the first thread and jump to program + interrupts enabled
    return -1;      // scheduler failed
}

/*
 * Starts G8RTOS Scheduler
 *  - Initializes the Systick
 *  - Sets Context to first thread using PRIORITY scheduling
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
sched_err_code_t G8RTOS_Launch_Priority()
{
    if (NumberOfThreads == 0)
    {
        return NO_THREADS_SCHEDULED;
    }
    else
    {
        ptcb_t* maxPThread = &Pthread[0];

#ifdef ORIG
        uint8_t max_thread_priority = 255;
        tcb_t* tempThread = &threadControlBlocks[0];
        tcb_t* maxThread = &threadControlBlocks[0];

        // search for highest priority background threads
        for ( int i = 0; i < NumberOfThreads; i++ )
        {
            if ( tempThread->priority < max_thread_priority )
            {
                max_thread_priority = tempThread->priority;
                maxThread = tempThread;
            }

            tempThread = tempThread->next;
        }
#endif
#ifdef OPT
        tcb_t* maxThread = head;
#endif

        // set the currently running threads based on priority
        CurrentlyRunningPeriodicThread = maxPThread;
        CurrentlyRunningThread = maxThread;
        InitSysTick();

        // Set SysTick and PendSV to lowest priority
        NVIC_SetPriority(SysTick_IRQn, 7);  // set SysTick to low priority
        NVIC_SetPriority(PendSV_IRQn, 7);   // set PendSV to low priority

        G8RTOS_Start(maxThread);  // load the first thread and jump to program + interrupts enabled
        return THREAD_LIMIT_REACHED;
    }
}

// KillThread function takes in a threadId and removes it from the linked list
// of threads
sched_err_code_t G8RTOS_KillThread( threadId_t id )
{
    uint32_t primask = StartCriticalSection();

    // variable declaration
    sched_err_code_t err = NO_ERROR;
    tcb_t* tempTcb = CurrentlyRunningThread;

    // Cannot kill a thread if it is the only one available
    // because the OS will end
    if ( NumberOfThreads == 1 )
        err = CANNOT_KILL_LAST_THREAD;

    // if enough threads exist to kill this one, continue
    else
    {
        // search for requested thread ID + set error code
        for (int i = 0; i < NumberOfThreads; ++i)
        {
            // id was found
            if ( tempTcb->id == id && tempTcb->alive )
            {
                err = NO_ERROR;
                break;
            }

            // id was not found
            else
            {
                err = THREAD_DOES_NOT_EXIST;
                tempTcb = tempTcb->next;
            }
        }

        // if the thread exists, continue.. otherwise,  exit
        if ( err != THREAD_DOES_NOT_EXIST )
        {
            // stops this tcb from being used again.
            // force to be the lowest priority so the
            // PendSV handler is allowed to switch if there
            // are no other high priority threads to kill
            tempTcb->priority = 255;    // min priority to allow thread deletion
            tempTcb->asleep = 0;
            tempTcb->id = 0;
            tempTcb->alive = false;
            NumberOfThreads--;

            // if this is the head pointer, redefine the
            // head pointer
#ifdef OPT
            if (tempTcb == head)
                head = head->next;
#endif

            // update next + previous tcb pointers to point at each other
            tempTcb->next->prev = tempTcb->prev;
            tempTcb->prev->next = tempTcb->next;
        }
    }

    EndCriticalSection(primask);
    __enable_interrupts();

    if (tempTcb == CurrentlyRunningThread) StartContextSwitch();

    return err;
}

void G8RTOS_KillAllOthers()
{
    uint32_t primask = StartCriticalSection();
    tcb_t currentThread = *CurrentlyRunningThread;
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if ( currentThread.id != threadControlBlocks[i].id && threadControlBlocks[i].name[0] != 'I'){
            sched_err_code_t err = G8RTOS_KillThread(threadControlBlocks[i].id);
        }
    }

    EndCriticalSection(primask);
}

// a thread calls the KillSelf function to send its own ID to the KillThread
// function
sched_err_code_t G8RTOS_KillSelf()
{
    sched_err_code_t err = G8RTOS_KillThread( CurrentlyRunningThread->id );
    return err;
}

#ifdef ORIG
// Function used in adding priority threads into the linked
// list with descending order of priority (0 .. 255)
// Returns an "INDEX" of in the TCB linked list where the new
// thread should be inserted.
OrganizedPriorityObject_t FindEmptyTcb(uint8_t priority)
{
    OrganizedPriorityObject_t org;
    tcb_t* tempTcb;
    tcb_t* tempPrev;
    tcb_t* tempNext;
    uint32_t empty_slot = 0;
    org.err = NO_ERROR;

    // determine index of the linked list's tail
    int max_index = NumberOfThreads;
    if (NumberOfThreads > 0)
    {
        tempTcb = &threadControlBlocks[max_index-1]; // temp = tail
        tempNext = tempTcb->next;
        tempPrev = tempTcb;

        // search for threads of this priority starting from the tail (low priority)
        // and working toward the head (higher priority) until finding the thread of
        // the correct priority. No need to search all the way through the list because
        // there will never be a case where a thread with lower priority gets put first.
        int count = 0;
        while( priority < tempTcb->priority )
        {
            tempTcb = tempTcb->prev; // move up the list toward the head
            tempNext = tempTcb->next;
            tempPrev = tempTcb;

            // if this is true, a new thread was added with
            // the highest priority
            count++;
            if ( count > NumberOfThreads-1 ) break;
        }

        // search for the first dead slot in the thread control block to place
        // the new tcb in
        for (unsigned int i = 0; i < MAX_THREADS; i++)
        {
            if ( !threadControlBlocks[i].alive ) // if dead
            {
                empty_slot = i;
                tempTcb = &threadControlBlocks[i];
                org.err = NO_ERROR;
                break;
            }
            else
                org.err = THREAD_LIMIT_REACHED;
        }
    }

    // if the priority DOES exist, add it to the end of that list of threads.
    // if this is the first time the priority is entering, add to the end of the
    // width of items in the linked list with the same priority.
    org.empty_slot = empty_slot;
    org.current = tempTcb;
    org.next = tempNext;
    org.prev = tempPrev;

    return org;
}
#endif

// Returns the currently running thread's thread ID
threadId_t G8RTOS_GetThreadId()
{
    return CurrentlyRunningThread->id;
}

// Default thread adder starts thread with starvation age of 100 and
// quantum count of 1
sched_err_code_t G8RTOS_AddThread__Def_Starvation(void (*threadToAdd)(void), uint8_t priority, char * name)
{
    sched_err_code_t ret = G8RTOS_AddThread( threadToAdd, priority, DONT_STARVE_AGE, name);
    return ret;
}

/*
 * Adds threads to G8RTOS Scheduler
 *  - Checks if there are stil available threads to insert to scheduler
 *  - Initializes the thread control block for the provided thread
 *  - Initializes the stack for the provided thread to hold a "fake context"
 *  - Sets stack tcb stack pointer to top of thread stack
 *  - Sets up the next and previous tcb pointers in a round robin fashion
 *  - Starvation age is the number of SysTicks the thread has not run before
 *      temporary priority is auto boosted to high priority (currently level 10).
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
sched_err_code_t G8RTOS_AddThread(    void (*threadToAdd)(void), uint8_t priority,
                         uint32_t starvation_age, char * name          )
{
    int32_t primask = StartCriticalSection();

    // - Checks if there are still available threads to insert to scheduler
    if (NumberOfThreads < MAX_THREADS)
    {
        // organize the TCB array
        OrganizedPriorityObject_t org = FindEmptyTcb(priority);

        uint32_t priorityIndex = 0;
        if ( org.err == NO_ERROR )
            priorityIndex = org.empty_slot;
        else
            return org.err;

        // - Initializes the stack for the provided thread to hold a "fake context"
        threadStacks[priorityIndex][STACKSIZE - 1] = THUMBBIT;      // thumb bit
        threadStacks[priorityIndex][STACKSIZE - 2] = (uint32_t)threadToAdd;  // PC value for the function
        threadStacks[priorityIndex][STACKSIZE - 3] = 0x14141414;    // R14
        threadStacks[priorityIndex][STACKSIZE - 4] = 0x12121212;    // R12
        threadStacks[priorityIndex][STACKSIZE - 5] = 0x03030303;    // R3    -- function parameters
        threadStacks[priorityIndex][STACKSIZE - 6] = 0x02020202;    // R2    -- function parameters
        threadStacks[priorityIndex][STACKSIZE - 7] = 0x01010101;    // R1    -- function parameters
        threadStacks[priorityIndex][STACKSIZE - 8] = 0x00000000;    // R0    -- function parameters
        threadStacks[priorityIndex][STACKSIZE - 9] = 0x11111111;    // R11
        threadStacks[priorityIndex][STACKSIZE - 10] = 0x10101010;   // R10
        threadStacks[priorityIndex][STACKSIZE - 11] = 0x09090909;   // R9
        threadStacks[priorityIndex][STACKSIZE - 12] = 0x08080808;   // R8
        threadStacks[priorityIndex][STACKSIZE - 13] = 0x07070707;   // R7
        threadStacks[priorityIndex][STACKSIZE - 14] = 0x06060606;   // R6
        threadStacks[priorityIndex][STACKSIZE - 15] = 0x05050505;   // R5
        threadStacks[priorityIndex][STACKSIZE - 16] = 0x04040404;   // R4

        // - Sets tcb stack pointer to top of thread stack + priority
        tcb_t* tempTCB = &threadControlBlocks[priorityIndex];
        tempTCB->sp = &threadStacks[priorityIndex][STACKSIZE - 16];
        tempTCB->priority = priority;
        tempTCB->priority_perm = priority;
        tempTCB->alive = true;
        tempTCB->age = 0;
        tempTCB->starvation_age = starvation_age;
        tempTCB->id = ((IDCounter++) << 16) | (uint32_t)tempTCB;

        // assign the thread's name
        for (int i = 0; i < MAX_NAME_LENGTH; i++)
            tempTCB->name[i] = *(name + i); // could produce bad data -- possibly adjust

        // - Sets up the next and previous tcb pointers in circular linked list format
        if (NumberOfThreads == 0)   // First control block creation
        {
            // initialize next/previous for the current(first) control block
            threadControlBlocks[priorityIndex].next = &threadControlBlocks[priorityIndex];
            threadControlBlocks[priorityIndex].prev = &threadControlBlocks[priorityIndex];
#ifdef OPT
            head = &threadControlBlocks[priorityIndex];
#endif
        }
        else
        {
            // initialize the next/previous for the current block
            threadControlBlocks[priorityIndex].next = org.next;
            threadControlBlocks[priorityIndex].prev = org.prev;

#ifdef OPT
            // if the new tcb's priority is less than equal to the
            // head's priority, this is the new head.
            if ( org.current->priority <= head->priority )
                head = org.current;
#endif

            // adjust the next block
            org.next->prev = org.current;

            // adjust the previous block
            org.prev->next = org.current;
        }

        NumberOfThreads++;  // - prepare for the next thread

        EndCriticalSection(primask);
        __enable_interrupts();
        return NO_ERROR;           // no error while adding threads
    }
    else
    {
        EndCriticalSection(primask);
        __enable_interrupts();
        return THREAD_LIMIT_REACHED; // error code returned
    }
}

sched_err_code_t G8RTOS_AddAperiodicEvent_Priority(void (*PthreadToAdd)(void), uint32_t priority, IRQn_Type IRQn)
{
    uint32_t primask = StartCriticalSection();
    sched_err_code_t err = NO_ERROR;

    if ( IRQn < PSS_IRQn || IRQn > PORT6_IRQn )
        err = IRQn_INVALID;
    else
    {
        if ( priority > 6 )
            err = HWI_PRIORITY_INVALID;
        else
        {
            // Interrupt vector functions.
            // using FPU_IRQn because this unit is never
            // enabled, so it shouldn't be a problem to use it.
            __NVIC_SetVector(IRQn, (uint32_t)PthreadToAdd);
            __NVIC_SetPriority(IRQn, priority);
            __NVIC_EnableIRQ(IRQn);
        }
    }

    EndCriticalSection(primask);
    __enable_interrupt();
    return err;
}

/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
sched_err_code_t G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period)
{
    sched_err_code_t err = NO_ERROR;
    uint32_t primask = StartCriticalSection();

    // only adds threads if there is room in the linked list
    if (NumberOfPthreads < MAXPTHREADS)
    {
        // assign parameter values
        Pthread[NumberOfPthreads].handler = PthreadToAdd;
        Pthread[NumberOfPthreads].period = period;
        Pthread[NumberOfPthreads].exec_time = period + SystemTime + NumberOfPthreads;

        // handle circular linked list organization
        if (NumberOfPthreads == 0)   // First control block creation
        {
            // initialize next/previous for the current(first) control block
            Pthread[NumberOfPthreads].next = &Pthread[NumberOfPthreads];
            Pthread[NumberOfPthreads].prev = &Pthread[NumberOfPthreads];
        }
        else
        {
            // initialize next/previous for the current block
            Pthread[NumberOfPthreads].next = &Pthread[0];
            Pthread[NumberOfPthreads].prev = &Pthread[NumberOfPthreads - 1];

            // update next from the previous block
            Pthread[NumberOfPthreads - 1].next = &Pthread[NumberOfPthreads];

            // update previous from the original block
            Pthread[0].prev = &Pthread[NumberOfPthreads];
        }

        NumberOfPthreads++;
        err = NO_ERROR;
    }
    else
    {
        err = THREAD_LIMIT_REACHED;
    }

    EndCriticalSection(primask);
    __enable_interrupt();
    return err;
}


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    CurrentlyRunningThread->sleep_count = SystemTime + durationMS;
    CurrentlyRunningThread->asleep = 0x1;
    StartContextSwitch();
	while(CurrentlyRunningThread->asleep);
}

/*********************************************** UPDATES *********************************************************************/

#ifdef OPT
// Function used in adding priority threads into the linked
// list with descending order of priority (0 .. 255)
// Returns an "INDEX" of in the TCB linked list where the new
// thread should be inserted.
OrganizedPriorityObject_t FindEmptyTcb(uint8_t priority)
{
    OrganizedPriorityObject_t org;
    tcb_t* tempTcb;
    tcb_t* tempPrev;
    tcb_t* tempNext;
    uint32_t empty_slot = 0;
    org.err = NO_ERROR;

    // determine index of the linked list's tail
    // int max_index = NumberOfThreads;
    if (NumberOfThreads > 0)
    {
        // UPDATE: Highest priority is the head. No need to search through the entire list
        // --------------------------------------------------------------------------

        // head is defined in the AddThread function when NumOfThreads == 0
        tempTcb = head;
        tempPrev = head->prev;

        for (int i = 0; i < NumberOfThreads; i++)
        {
            // places the new thread at the beginning of the linked lists
            // AddThread handles if this is the NEW HEAD thread
            if ( priority <= tempTcb->priority )
            {
                // the returned NextThread should be the current head of
                // the priority group. The PrevThread should be the head's
                // previous still.
                tempNext = tempTcb;
                tempPrev = tempTcb->prev;
                break;
            }

            // if the thread has not found its priority group, continue to
            // move up the linked list towards the lowest priority groups.
            else if ( priority > tempTcb->priority && i == NumberOfThreads-1 )
            {
                // if I've reached the end of the FOR loop, the new
                // thread is the lowest priority thread. Add the thread
                // here.
                tempNext = head;
                tempPrev = head->prev;
                break;
            }

            tempTcb = tempTcb->next;
        }

        // --------------------------------------------------------------------------
        // UPDATE END:

        // search for the first dead slot in the thread control block to place
        // the new tcb in. Also update the CurrentThread to return.
        for (unsigned int i = 0; i < MAX_THREADS; i++)
        {
            if ( !threadControlBlocks[i].alive ) // if dead
            {
                empty_slot = i;
                tempTcb = &threadControlBlocks[i];
                org.err = NO_ERROR;
                break;
            }
            else
                org.err = THREAD_LIMIT_REACHED;
        }
    }

    // if the priority DOES exist, add it to the end of that list of threads.
    // if this is the first time the priority is entering, add to the end of the
    // width of items in the linked list with the same priority.
    org.empty_slot = empty_slot;
    org.current = tempTcb;
    org.next = tempNext;
    org.prev = tempPrev;

    return org;
}
#endif

#if defined OPT
// Scheduling algorithm using priority instead of round robin
// NOTE: **Update to include aging and dynamic quantum time later**
void G8RTOS_Scheduler_Priority(void)
{
    tcb_t* tempThreadPtr = head;

    for (int i = 0; i < NumberOfThreads; i++)
    {
        // Jump through the linked list if the temp thread cannot be run.
        if ( tempThreadPtr->blocked > 0 || tempThreadPtr->asleep == 1 )
        {
            tempThreadPtr = tempThreadPtr->next;
        }

        // If there are multiple threads of this priority, determine which
        // thread should be run next. I do not need to specify less than equal
        // because I am already starting at the head.
        else if (   tempThreadPtr->priority == CurrentlyRunningThread->priority
                &&  tempThreadPtr->priority == tempThreadPtr->next->priority    )
        {
            // old the first of this priority. Cannot be blocked because
            // it made it past the first if statement.
            tcb_t* remember_me = tempThreadPtr;
            while (1)
            {
                tempThreadPtr = tempThreadPtr->next;

                // if unblocked and awake, allow thread to run. This cannot be
                // running the same thing as CurrentlyRunningThread because it
                // forces a thread switch every iteration.
                if ( tempThreadPtr->blocked == 0 && tempThreadPtr->asleep == 0 )
                    break;

                // if the thread reaches the end of its priority list, go to
                // the beginning of the list and run. List will continue iterating
                // on the next SysTick.
                else if ( tempThreadPtr->priority != tempThreadPtr->next->priority )
                {
                    tempThreadPtr = remember_me;
                    break;
                }
            }

            break;
        }

        // if there is only one thread available of this priority, run it now
        else break;
    }

    CurrentlyRunningThread = tempThreadPtr;
}
#endif
