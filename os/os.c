/*
 * @file os.c
 * @brief Operating system / scheduler implementation
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

/* =================== INCLUDES =============================== */
#include <stdint.h>

#include "os.h"
#include "system.h"
#include "print/print.h"

/* =================== EXTERN DEFINITIONS ===================== */

/** @brief Task stack upper limit from linker script */
extern uint32_t __TaskStackTop;

/** @brief Task stack lower limit from linker script */
// extern uint32_t __TaskStackLimit;

/** @brief Extern linkage to definition of tasks to run, @ref OS_TASKS_INIT */
extern task_t __tasks[];

/** @brief Number of tasks defined, including idle task, @ref OS_TASKS_INIT */
extern const uint32_t __tasks_count;

/** @brief Extern linkage to definition of task stacks, @ref OS_TASKS_INIT */
extern uint8_t task_stacks[];

/* =================== MACRO DEFINITIONS ====================== */

#ifdef OS_DEBUG
#define DBG_PRINT(x) (print(x))
#define DBG_PRINT_HEX(x, y) (print_hex(x, y))
#else
#define DBG_PRINT(x)
#define DBG_PRINT_HEX(x, y) 
#endif /* OS_DEBUG */

/** @brief Number of task states in @ref task_state_e */
#define NUM_TASK_STATES 5

/** @brief Special value indicating a thread is not actively sleeping */
#define OS_NOSLEEP 0xFFFFFFFFFFFFFFFF

/** @brief Convert task number to a bit in @ref task_state_list; MSB = task 0 */
#define TASK_NUM_TO_BIT(x) (1 << (31UL - x))

/** @brief Convert task number to pointer to beginning of task stack. Idle
 *      task stack size is different, and it's the last task, this accounts for that
 */
#define TASK_NUM_TO_INITIAL_SP(tasknum) (                                       \
    (tasknum == (__tasks_count - 1))                                            \
    ? &task_stacks[((tasknum - 1) * TASK_STACK_SIZE) + IDLE_STACK_SIZE - 0x4UL] \
    : &task_stacks[(tasknum * TASK_STACK_SIZE) - 0x4UL]                         \
)


/* =================== TYPE DEFINITIONS ========================== */

/** @brief Enum representing the state of the task in scheduler */
typedef enum Task_State {
    NEXT    = 0,    /** @brief Task is selected to be executed next */
    READY   = 1,    /** @brief Task is ready to be executed */
    PENDING = 2,    /** @brief Task is sleeping or pending other synchronization */
    RUNNING = 3,    /** @brief Task is executing */
    EJECTED = 4     /** @brief Task has just been context switched out */
} task_state_e;


/* =================== STATIC DATA =============================== */

/** @brief An array of 32-bit numbers, where each bit represents a task in that state */
volatile uint32_t task_state_list[NUM_TASK_STATES] = {0};

/* =================== FUNCTION DECLARATIONS ===================== */

void schedule(void);

/* ========================= FUNCTION DEFINITIONS ========================= */

/** @brief The idle task, this can be overridden in application code.
 *          The idle task runs, when no other task is ready to run
 * @param arg1  unused
 * @param arg2  unused
 * @param arg3  unused
 */
__attribute__((weak)) void idle_task(void* arg1, void* arg2, void* arg3)
{
    while(1) {
#ifdef OS_DEBUG
        print(__func__);
        busysleep(10);
        yield();
#else
        asm("wfi");
#endif /* OS_DEBUG */
    }
}


/**
 * @brief Start the scheduler. Initializes task data, and calls
 *      the first task. This should never return!
 */
void scheduler_start(void)
{
    uint32_t i;

    if(__tasks_count > MAX_NUM_TASKS) {
        print("ERROR: task count may never exceed MAX_NUM_TASKS");
        return;
    }

    DBG_PRINT("================ SCHEDULER START =================");
    DBG_PRINT_HEX(" == > Number of tasks : ", __tasks_count);

    /* Initialize scheduler related members of the task structures, and the task stacks */
    for(i = 0; i < __tasks_count; i++) {
        /* Set task initial stack pointer */
        __tasks[i].sp = (uint32_t*)TASK_NUM_TO_INITIAL_SP(i);
        
        /* Mark the task as not sleeping */
        __tasks[i].wakeup_time = OS_NOSLEEP;
        
        /* Mark the first task running, and others ready */
        if(i > 0) {
            task_state_list[READY] |= TASK_NUM_TO_BIT(i);
        } else {
            task_state_list[RUNNING] |= TASK_NUM_TO_BIT(i);
        }

        /* Initialize the task stack */
        TaskStackInit(&__tasks[i]);
    }


    /* Initialize the PendSV interrupt that will handle context switches */
    PendSV_init();

    /* Initialize the System Tick for keeping time, enabling sleep() */
    TICK_init(1, &schedule);

    /* Flush the cache after configuring the interrupts */
    asm("dsb");
    asm("isb");

    DBG_PRINT("================= OS START =================="); 

    /* Kick off the OS by calling the first task */
    __tasks[0].fn(__tasks[0].arg1, __tasks[0].arg2, __tasks[0].arg3);
}

/** @brief Run the scheduler; check if a task has become ready to run
 *          and schedule it if needed
 */
void schedule(void)
{
    uint32_t task, selected;
    uint32_t pending, original_pending;
    uint32_t curr, cur_prio, candidates, next;
    uint64_t ticks;

    /* Check the previously running task */
    if(task_state_list[EJECTED]) {
        task = CountLeadingZeros(task_state_list[EJECTED]);
        
        /* Check if task wakeup time has been set */
        if(__tasks[task].wakeup_time != OS_NOSLEEP) {
            
            /* Mark task PENDING if it is */
            task_state_list[PENDING] |= task_state_list[EJECTED];
        } else {

            /* Mark task READY if it is not */
            task_state_list[READY] |= task_state_list[EJECTED];
        }
        task_state_list[EJECTED] = 0;
    }

    /* Exit early if nothing to do */
    if(!task_state_list[PENDING]) {
        return;
    }

    /* Get list of pending tasks */
    original_pending = task_state_list[PENDING];
    pending = original_pending;

    /* Get current ticks */
    ticks = TICK_get();

    /* Check if any task should move from PENDING to READY */
    do {
        /* Get the next PENDING task */
        task = CountLeadingZeros(pending);

        /* Check if task sleep is done */ 
        if(ticks > __tasks[task].wakeup_time) {
            /* Clear wakeup time, mark task ready */
            __tasks[task].wakeup_time = OS_NOSLEEP;
            task_state_list[PENDING] &= ~(TASK_NUM_TO_BIT(task));
            task_state_list[READY] |= TASK_NUM_TO_BIT(task);
        }
        /* Clear bit to not check this task again */
        pending &= ~(TASK_NUM_TO_BIT(task));
    } while(pending);

    /* Check if a task was moved from PENDING to READY */
    if(task_state_list[PENDING] != original_pending) {

        /* Get parameters of the running task */
        curr = CountLeadingZeros(task_state_list[RUNNING]);
        selected = curr;
        candidates = task_state_list[READY];
        cur_prio = __tasks[curr].prio;

        /* Check if there's a task with same or higher priority marked as ready */
        do {
            next = CountLeadingZeros(candidates);
            if(__tasks[next].prio >= cur_prio) {
                selected = next;
                break;
            }
            candidates &= ~(TASK_NUM_TO_BIT(next));
        } while(candidates);

        /* If a new task was selected, mark it as NEXT and trigger a context switch */
        if(selected != curr) {
            task_state_list[NEXT] = TASK_NUM_TO_BIT(selected);
            task_state_list[READY] &= ~(TASK_NUM_TO_BIT(selected));
            (void)PendSV_trigger();
        }
    }
}

/** @brief Yield from a task. Selects the next
 *      task to be executed, if one is available with
 *      the same or higher priority, otherwise returns to
 *      the caller. If a new task is selected, a context
 *      switch will be performed
 */
void yield(void)
{
    uint32_t tasknum;
    uint32_t nexttask;
    uint32_t current_prio;
    uint32_t candidate;
    uint32_t candidates = 0;

    DBG_PRINT_HEX("----> yield from: ", CountLeadingZeros(task_state_list[RUNNING]));

    /* Clean up any previously switched out task */
    if(task_state_list[EJECTED]) {
        tasknum = CountLeadingZeros(task_state_list[EJECTED]);
        
        /* Check if task wakeup time has been set */
        if(__tasks[tasknum].wakeup_time != OS_NOSLEEP) {
            
            /* Mark task PENDING if it is */
            task_state_list[PENDING] |= task_state_list[EJECTED];
        } else {

            /* Mark task READY if it is not */
            task_state_list[READY] |= task_state_list[EJECTED];
        }
        task_state_list[EJECTED] = 0;
    }

    /* Get the list of tasks ready to be run */
    candidates = task_state_list[READY];

    /* Return from yield if no other task is ready to run */
    if(!candidates) {
        return;
    }
    
    /* Get the current task number */
    tasknum = CountLeadingZeros(task_state_list[RUNNING]);
    
    /* Check if there's another same or higher prio task waiting */
    nexttask = tasknum;
    current_prio = __tasks[tasknum].prio;

    do {
        /* Get next task candidate from list of ready tasks */
        candidate = CountLeadingZeros(candidates);
        
        /* Select the task to be run if it has the same or higher priority */
        if(__tasks[candidate].prio >= current_prio) {
            nexttask = candidate;
            break;
        }

        /* Clear the task already checked from list of candidates */
        candidates &= ~(TASK_NUM_TO_BIT(candidate));
    
    } while(candidates);

    /* Check if the best candidate is this task */
    if(nexttask == tasknum) {
        if(__tasks[tasknum].wakeup_time == OS_NOSLEEP) {
            /* No available candidate, and current task is not sleeping, return */
            return;
        }

        /* Pick the next ready task if current task is sleeping (yield called from sleep).
            This should be the idle task */
        nexttask = CountLeadingZeros(task_state_list[READY]);
    }

    /* Mark the selected task as next */
    task_state_list[NEXT] = TASK_NUM_TO_BIT(nexttask);
    task_state_list[READY] &= ~(TASK_NUM_TO_BIT(nexttask));

    /* Trigger PendSV to context switch */
    (void)PendSV_trigger();
}

/**
 * @brief Blocking sleep using system tick. Yields current task
 * @param[in] ms    sleep interval in milliseconds
 */
void sleep(int ms)
{
    uint32_t task;
    /* Get running task number */
    task = CountLeadingZeros(task_state_list[RUNNING]);

    DBG_PRINT_HEX("----> sleep from: ", CountLeadingZeros(task_state_list[RUNNING]));

    /* Mark the wakeup time */
    __tasks[task].wakeup_time = TICK_get() + (uint64_t)ms;
    
    /* Call yield to switch to the next task */
    yield();
}

