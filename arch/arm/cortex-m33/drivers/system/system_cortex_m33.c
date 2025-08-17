/*
 * @file system_cortex_m33.c
 * @brief Cortex-M33 system utilities driver
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

/* =================== INCLUDES =============================== */
#include <stdint.h>
#include "system.h"
#include "os.h"

/* ========================= CONSTANTS ========================= */

// https://developer.arm.com/documentation/100235/0100/The-Cortex-M33-Peripherals/System-timer--SysTick
#define SCS_BASE            (0xE000E000UL)

#define SYSTICK_BASE        (SCS_BASE +  0x0010UL)
#define SYSTICK_CTRL        (volatile uint32_t*)(SYSTICK_BASE + 0x0UL)
#define SYSTICK_RVR         (volatile uint32_t*)(SYSTICK_BASE + 0x4UL)
#define SYSTICK_CVR         (volatile uint32_t*)(SYSTICK_BASE + 0x8UL)
#define SYSTICK_PRIO_MASK   (0xFFUL << 24)
#define PENDSV_PRIO_MASK    (0xFFUL << 16)
#define SYSTICK_PRIO        (0xC0UL << 24)
#define PENDSV_PRIO         (0xD0UL << 16)

#define NVIC_ICSR           (volatile uint32_t*)(SCS_BASE + 0xD04UL)
#define NVIC_SHPR3          (volatile uint32_t*)(SCS_BASE + 0xD20UL)
#define PENDSV_SET          (0x1UL << 28)

/** @brief Debug value in stacks */
#define SENTINEL 0xDEADBEEFUL

/* ========================= FUNCTION DECLARATIONS ========================= */

int STM_TICK_init(int ms, Tick_Callback cb);
int STM_PendSV_init(void);
void STM_Task_Stack_init(task_t *task);
static inline uint32_t STM_Count_Leading_Zeros(uint32_t value);
uint64_t STM_TICK_get(void);
void STM_busy_sleep(int ms);
void STM_PendSV_trigger(void);

/* ========================= STATIC DATA ========================= */

/** @brief System driver vtable */
static SystemDriver drv = {
    &STM_TICK_init,
    &STM_PendSV_init,
    &STM_Task_Stack_init,
    &STM_Count_Leading_Zeros,
    &STM_TICK_get,
    &STM_busy_sleep,
    &STM_PendSV_trigger
};

/** @brief System driver pointer, matching extern in os driver abstraction */
const SystemDriver *Sys_Driver = &drv;

/** @brief Static pointers for ISR callbacks */
static Tick_Callback tick_cb;

/** @brief System tick counter */
static volatile uint64_t systicks;

/** @brief Size of task_t, needed in fetching stack pointers in @ref PendSV_Handler */
static const uint32_t __attribute__((unused)) task_struct_size = sizeof(task_t);

/** @brief Extern linkage to definition of task states */
extern uint32_t task_state_list[];

/** @brief Extern linkage to definition of tasks to run, @ref OS_TASKS_INIT */
extern task_t __tasks[];

/** @brief Extern linkage to definition of task stacks, @ref OS_TASKS_INIT */
extern uint8_t task_stacks[];

/* ========================= FUNCTION DEFINITIONS ========================= */


/** @brief Debugging function, that will be called if a task function tries to return */
void loop_forever(void) 
{
    while(1) {;}
}

/**
 * @brief SysTick ISR
 * @n Increments the system tick count, and calls the tick callback (if set)
 */
void  __attribute__( ( naked ) ) SysTick_Handler(void)
{
    /* Save registers modified by this function (r0-r3 saved in hw) */
    asm("push {r4, r5, r6, lr}");

    /* Load systicks to registers */
    asm("ldr r2, =systicks");
    asm("ldr r3, [r2, #0]");
    asm("ldr r4, [r2, #4]");
    asm("mov r1, #1");

    /* 64-bit add and store back*/
    asm("adds r5, r3, #1");
    asm("adc  r6, r4, #0");
    asm("str  r5, [r2, #0]");
    asm("str  r6, [r2, #4]");

    /* Call systick handler, null-checking the tick_cb pointer */
    asm("ldr r1, =tick_cb");
    asm("ldr r1, [r1]");
    asm("subs r1, #0");
    asm("it ne");
    asm("blxne  r1");

    /* Restore registers */
    asm("pop {r4, r5, r6, lr}");

    /* Return from interrupt */
    asm("bx lr");
}

/** 
 * @brief Perform a context switch.
 *      Stores the context of the currently running task into it's stack
 *      space, and restores the context of the task marked ready, resuming
 *      the execution in the new task's context once the interrupt running
 *      this function returns.
 */
void __attribute__((naked)) PendSV_Handler(void)
{

/* 
 * Steps to perform the context switch:
 *  1. Store the stack pointer of the running task, and the contents of
 *      the Link Register into general purpose registers
 *  2. Store the registers that are not automatically stored by exception
 *      entry in the currently running task's stack
 *  3. Get the task number of the currently running task by counting the
 *      leading zeros on the global task state list's RUNNING entry
 *  4. Set the value of the EJECTED entry to the previous value in RUNNING
 *      entry, and clear the RUNNING entry to mark the current task switched out
 *  5. Find the offset of the currently running task's stack pointer member in the
 *      task structures and store the stack pointer there
 *  6. Copy the value of the NEXT entry in task state list into the RUNNING list,
 *      and clear the NEXT entry to mark the next task to run as being actively run
 *  7. Get the number of the next task to run by counting the leading zeros on the
 *      RUNNING entry of the task state list
 *  8. Load the stack pointer from the task structure of the new running task into a 
 *      general purpose register
 *  9. Load the registers not automatically stored by exception entry from the new
 *      task's stack
 *  10. Restore the CPU stack pointer to the new task's stack, and return from interrupt
 *      with the stored Link Register value
 *
 * Saving and loading the context:
 * 
 * The interrupt entry mechanism of the CPU automatically stores the following registers to the
 *  stack of the task that was running when the interrupt was taken:
 *
 *      0x20 - SP               <SP before the interrupt>
 *      0x1C - xPSR
 *      0x18 - PC
 *      0x14 - LR
 *      0x10 - R12
 *      0x8  - R3
 *      0x8  - R2
 *      0x4  - R1
 *      0x0  - R0               <SP during interrupt entry>
 *
 * As a first step in the context switch interrupt handler, the rest of the registers are also
 *  stored onto the stack:
 *      0x4  - R4
 *      ...  - R5 - R10
 *      0x20 - R11              <SP after storing the context>
 * 
*/

    /* Store stack pointer and link register into a general purpose register */
    asm("mrs r0, msp");                 /* Move from special register MSP to general purpose register r0 */
    asm("mov r1, lr");                  /* Move LR into r1 */

    /* Store registers R4-R11 onto the stack of the currently running task. 
        Store new SP address in R0 */
    asm("stmdb r0!, {r4-r11}");         /* Store multiple, decrement before, write back the address */

    /* Figure out current task number, move it from RUNNING to EJECTED and clear RUNNING */
    asm("mov r6, #0");                  /* Store the number 0 into r6 */
    asm("ldr r2, =task_state_list");    /* Load the task_state_list address into r2 */
    asm("ldr r5, [r2, #12]");           /* Load the value of the RUNNING entry into r5 with an offset to r2 */
    asm("str r5, [r2, #16]");           /* Store value of the RUNNING entry into EJECTED entry */
    asm("str r6, [r2, #12]");           /* Clear the value of the RUNNING entry by writing a zero into it */
    asm("clz r5, r5");                  /* Count leading zeros of the entry to get the active task number */

    /* Store the current stack pointer into the task structure of the running task */
    asm("ldr r3, =__tasks");            /* Load the __tasks structure array starting address into r3 */
    asm("ldr r4, =task_struct_size");   /* Load the address of the size of the task_t structure into r4 */
    asm("ldr r4, [r4]");                /* Load the value of the size of the task_t structure into r4 */
    asm("mul r5, r4");                  /* Multiply the task number by the size of the task_t structure into r5 */
    asm("str r0, [r3, r5]");            /* Store the stack pointer of the currently running task into it's task_t
                                            structure with the computed offset w.r.t the starting address of the
                                            __tasks array. The stack pointer is the first element in the array. */

    /* Figure out next task number with interrupts disabled (as SysTick may alter the NEXT entry) */
    asm("cpsid i");                     /* Disable interrupts */
    asm("ldr r5, [r2, #0]");            /* Load the value of the NEXT entry in the task state list */
    asm("str r5, [r2, #12]");           /* Store the value of the NEXT entry into the RUNNING entry */
    asm("str r6, [r2, #0]");            /* Clear the NEXT entry by writing a zero into it */
    asm("cpsie i");                     /* Enable interrupts */
    asm("clz r5, r5");                  /* Count the leading zeros of the NEXT/RUNNING entry to get the task number */

    /* Load stack pointer of next ready task from struct */
    asm("mul r5, r4");                  /* Multiply the new task number by the size of the task_t structure to... */
    asm("ldr r0, [r3, r5]");            /* Get the offset of and load the new task's stack pointer into register r0 */

    /* Load registers r4-r11 */
    asm("ldmia r0!, {r4-r11}");         /* Load multiple, increment after, write back the address into r0 */

    /* Restore stack pointer */
    asm("msr msp, r0");                 /* Write the new task's stack pointer (after loading context) into 
                                            the CPU's stack register */

    /* Return from interrupt (exc return stored in r1) */
    asm("bx r1");                       /* Branch to the address stored in R1, the Link Register value when
                                            this interrupt was entered. Execution will now continue in the
                                            new task's context */
}

/**
 * @brief Trigger PendSV interrupt
 */
void STM_PendSV_trigger(void)
{
    /* Set PendSV */
    *NVIC_ICSR = PENDSV_SET;

    /* Sync barriers */
    asm("dsb");
    asm("isb");
}


/**
 * @brief SysTick initialization function
 * @n Loads the tick interval register, populates callback, 
 *      and enables the SysTick interrupt
 */
int STM_TICK_init(int ms, Tick_Callback cb)
{
    uint32_t temp;
    /* Populate the SysTick Reload register value, i.e. tick interval. This is
        (system clock freq / 1000) * milliseconds - 1 */
    temp = (0x3D0900UL / 0x3E8UL) * ms - 0x01UL;
    *SYSTICK_RVR = temp;

    /* Set SysTick priority to lowest */
    temp = *NVIC_SHPR3;
    temp &= ~(SYSTICK_PRIO_MASK);
    temp |= SYSTICK_PRIO;
    *NVIC_SHPR3 = temp;

    /* Clear the current counter value */
    *SYSTICK_CVR = 0x0UL;

    /* Set bits CLKSOURCE | TICKINT | ENABLE to enable SysTick */
    *SYSTICK_CTRL = 0x7UL; 

    /* Store the provided callback */
    tick_cb = cb;

    return 0;
}

/**
 * @brief PendSV initialization function
 * @n Sets the priority of PendSV interrupt
 */
int STM_PendSV_init(void)
{
    uint32_t temp;

    /* Set PendSV priority to lower than SysTick*/
    temp = *NVIC_SHPR3;
    temp &= ~(PENDSV_PRIO_MASK);
    temp |= PENDSV_PRIO;
    *NVIC_SHPR3 = temp;

    return 0;
}


/**
 * @brief Getter for tick count
 * @return tick count
 */
uint64_t STM_TICK_get(void)
{
    return systicks;
}


/** TODO: get the interval right
 * @brief Blocking busy sleep
 * @param[in] us    sleep interval in microseconds
 */
void STM_busy_sleep(int ms)
{
    uint32_t i, j;
    /* Loop for some time */
    for(i = 0; i < ms; i++) {
        for(j = 0; j < 0xFFFFUL; j++) { ; }
    }
}


/** @brief Initialize task stack for the first time
 *      as if it was returning from PendSV/context switch 
 *      to bootstrap the operation. See @ref context_switch
 *      for reference of the expected layout
 * @param task  task to initialize the stack of
 */
void STM_Task_Stack_init(task_t *task)
{
    uint32_t i;
    uint32_t* sp;

    sp = task->sp;

    /* A few funny values for debug traces */
    *sp-- = (uint32_t)SENTINEL;
    *sp-- = (uint32_t)SENTINEL;

    /* xPSR a.k.a Program Status Register */
    *sp-- = (uint32_t)0x01000000; /* Thumb bit set */

    /* PC -> function entry point */
    *sp-- = (uint32_t)task->fn;

    /* LR -> return address - set it to loop forever so we can
     catch if a task ever gets here (it shouldn't) */
    *sp-- = (uint32_t)&loop_forever;

    /* R12 -> scratch register - doesn't matter*/
    *sp-- = (uint32_t)0x0C;

    /* R3 -> doesn't matter */
    *sp-- = (uint32_t)0x03;

    /* R2 - R0 -> task entry parameters */
    *sp-- = (uint32_t)task->arg3;
    *sp-- = (uint32_t)task->arg2;
    *sp-- = (uint32_t)task->arg1;

    /* Rest of the general purpose registers (R11 - R4), contents don't matter */
    for(i = 11; i >= 4; i--) {
        *sp-- = i;
    }

    /* Store stack last pointer into struct (revert last decrement) */
    task->sp = ++sp;
}

/**
 * @brief Count leading zeros in asm
 *
 * @param value The unsigned integer to count leading zeros for
 * @return The number of leading zeros (0-32)
 */
static inline uint32_t STM_Count_Leading_Zeros(uint32_t value) {
    uint32_t result;
    
    /* Use the CLZ (count leading zeros) instruction to count leading zeros */
    __asm__ __volatile__ (
        "clz %0, %1"
        : "=r" (result)  /* output operand*/
        : "r" (value)    /* input operand */
        :                /* other registers affected */
    );

    return result;
}
