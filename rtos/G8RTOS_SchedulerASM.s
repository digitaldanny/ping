; G8RTOS_SchedulerASM.s
; Holds all ASM functions needed for the scheduler
; Note: If you have an h file, do not have a C file and an S file of the same name

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler, G8RTOS_Scheduler_Priority

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file 
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start: .asmfunc

	cpsid I
	ldr SP, [r0]			; SP <= mem[first address of the TCB_T is the STACK POINTER]

	; Load registers from CurrentlyRunningThread's stack
	pop {r4 - r11}
	pop {r0 - r3}
	pop {r12}
	add SP, SP, #4			; link register isn't useful yet
	pop {LR}				; LR <= PC for thread
	add SP, SP, #4			; current status register is useless

	; enable interrupts
	cpsie I
	bx LR


	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler: .asmfunc

	; LR = 0xFFFFFFF9 --------------
	; Registers R0-R3, R12-R14, and PSR automatically pushed to stack
	; After triggering the PENDSV_HANDLER
	CPSID I					; prevent interrupt during thread switch
	push {r4 - r11}

	; STORE THE STACK POINTER
	ldr r0, RunningPtr
	ldr r1, [r0]
	str SP, [r1]			; store SP into TCB

	push {LR}						; SAME STACK HERE
	BL G8RTOS_Scheduler_Priority	; Calls G8RTOS_Scheduler to get new tcb
	pop {LR}						; LR = 0xFFFFFFF9 restored for exitting interrupt

	;	- Set stack pointer to new stack pointer from new tcb
	ldr r0, RunningPtr
	ldr r1, [r0]
	ldr SP, [r1]		; unload SP from TCB

	;	- Pops registers from thread stack
	pop {r4 - r11}

	cpsie I
	bx LR
	; Registers R0-R3, R12-R15, and PSR automatically popped from stack
	; After returning from the PENDSV_HANDLER

	.endasmfunc
	
	.align
	.end
