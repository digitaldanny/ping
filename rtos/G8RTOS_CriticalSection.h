/*
 * G8RTOS_CriticalSection.h
 *
 */

#ifndef G8RTOS_CRITICALSECTION_H_
#define G8RTOS_CRITICALSECTION_H_

/*
 * Starts a critical section
 * 	- Saves the state of the current PRIMASK (I-bit)
 * 	- Disables interrupts
 * Returns: The current PRIMASK State
 */
extern int32_t StartCriticalSection();

/*
 * Ends a critical Section
 * 	- Restores the state of the PRIMASK given an input
 * Param "IBit_State": PRIMASK State to update
 */
extern void EndCriticalSection(int32_t IBit_State);


#endif /* G8RTOS_CRITICALSECTION_H_ */
