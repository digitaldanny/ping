/*
 * G8RTOS_IPC.c
 */

/*********************************************** Defines ******************************************************************************/
#include "G8RTOS_IPC.h"

#define FIFOSIZE 100
#define MAX_NUMBER_OF_FIFOS 6

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/*
 * FIFO struct will hold
 *  - buffer
 *  - head
 *  - tail
 *  - lost data
 *  - current size
 *  - mutex
 */

struct FIFO
{
    uint32_t buffer[FIFOSIZE];       // fifo size
    uint32_t *head;                  // first piece of data passed in
    uint32_t *tail;                  // last piece of data passed in
    uint32_t lost_data;             //  counts the amount of data lost because the buffer was full
    semaphore_t current_size;       //
    semaphore_t mutex;              // doesn't allow more than one consumer to access fifo
};

typedef struct FIFO FIFO_t;

/* Array of FIFOS */
static FIFO_t FIFOs[MAX_NUMBER_OF_FIFOS];

/*********************************************** Data Structures Used *****************************************************************/

/*
 * Initializes FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex)
{
    if (FIFOIndex > MAX_NUMBER_OF_FIFOS-1)
    {
        return -1;
    }
    else
    {
        // fifo head and tail point to the first data item
        FIFOs[FIFOIndex].head = &FIFOs[FIFOIndex].buffer[0];
        FIFOs[FIFOIndex].tail = &FIFOs[FIFOIndex].buffer[0];

        // initialize FIFO semaphores
        G8RTOS_InitSemaphore( &FIFOs[FIFOIndex].current_size, 0 );
        G8RTOS_InitSemaphore( &FIFOs[FIFOIndex].mutex, 1 );

        // init lost data count
        FIFOs[FIFOIndex].lost_data = 0;

        return 0;
    }
}

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param: "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
uint32_t readFIFO(uint32_t FIFOChoice)
{
    // WAIT(current-size) in case fifo contains nothing
    G8RTOS_WaitSemaphore( &FIFOs[FIFOChoice].current_size );

    // WAIT(mutex) in case fifo is being read from another thread
    G8RTOS_WaitSemaphore( &FIFOs[FIFOChoice].mutex );

    // return HEAD value of FIFOs[FIFOCHoice]
    uint32_t return_data = *(FIFOs[FIFOChoice].head);    // dereference the pointer
    FIFOs[FIFOChoice].head++;   // point to next piece of data in the queue

    // wrap back to the beginning if the queue overflowed its max size
    if ( FIFOs[FIFOChoice].head == &FIFOs[FIFOChoice].buffer[FIFOSIZE] )
    {
        // adjust pointer to go to the beginning of the queue
        FIFOs[FIFOChoice].head = &FIFOs[FIFOChoice].buffer[0];
    }

    // SIGNAL(mutex)
    G8RTOS_SignalSemaphore( &FIFOs[FIFOChoice].mutex );
    return return_data;
}

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if ncessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFOChoice, uint32_t Data)
{
    // WAIT(mutex) in case fifo is being read or written to from another thread
     G8RTOS_WaitSemaphore( &FIFOs[FIFOChoice].mutex );

    // check if the current size is too large
    // and return error code if it is. Also
    // overwrite old data into the queue
    if ( FIFOs[FIFOChoice].current_size > FIFOSIZE-1 )
    {
        // overwrite old data here...
        FIFOs[FIFOChoice].head++;   // old data is lost here

        if ( FIFOs[FIFOChoice].head == &FIFOs[FIFOChoice].buffer[FIFOSIZE] )
            FIFOs[FIFOChoice].head = &FIFOs[FIFOChoice].buffer[0];

        // keep track of the lost data
        FIFOs[FIFOChoice].lost_data++;
    }

    // update tail pointer to new data
    *(FIFOs[FIFOChoice].tail) = Data;   // load data into queue
    FIFOs[FIFOChoice].tail++;           // increment tail pointer

    // wrap if the tail has reached the top of the queue
    if ( FIFOs[FIFOChoice].tail == &FIFOs[FIFOChoice].buffer[FIFOSIZE] )
    {
        FIFOs[FIFOChoice].tail = &FIFOs[FIFOChoice].buffer[0];
    }

    // SIGNAL(mutex) in case fifo so other threads can read or write to the fifo
     G8RTOS_SignalSemaphore( &FIFOs[FIFOChoice].mutex );

    // SIGNAL(current_size)
    G8RTOS_SignalSemaphore( &FIFOs[FIFOChoice].current_size );

    // determine if there was an error or not
    if ( FIFOs[FIFOChoice].current_size > FIFOSIZE-1 )
        return -1;
    else
        return 0;
}

