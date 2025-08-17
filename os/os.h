/*
 * @file os.h
 * @brief Header file for the operating system implementation
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

#ifndef __KANTO_OS_H__
#define __KANTO_OS_H__

/* =================== INCLUDES =============================== */
#include <stdint.h>

/* =================== MACRO DEFINITIONS ====================== */

/** @brief Lowest possible priority for task. This is the idle task's priority,
 * actual tasks should define a value OS_LOWEST_PRIO + N */
#define OS_LOWEST_PRIO  0

/** @brief The size of the stack allocated for each task. Override to adjust */
#ifndef TASK_STACK_SIZE
#define TASK_STACK_SIZE 0x400UL
#endif

#ifndef IDLE_STACK_SIZE
#define IDLE_STACK_SIZE 0x100UL
#endif

/** @brief Enable logging */
// #define OS_DEBUG

#define MAX_NUM_TASKS 32

/* =================== TYPE DEFINITIONS ========================== */


/** @brief Task entrypoint function template */
typedef void (*TaskEntry_Handler)(void*, void*, void*);

/** @brief The task data structure, representing one task and it's context */
typedef struct Task_t {
    
    /** @brief Stack pointer to allow context switch */
    uint32_t *sp;           
    

    /** @brief First argument for task entrypoint */
    void* arg1;
    
    /** @brief Second argument for task entrypoint */ 
    void* arg2;

    /** @brief Third argument for task entrypoint */   
    void* arg3;


    /** @brief Task entry point */
    TaskEntry_Handler fn;   


    /** @brief Task priority. Higher number is higher priority */
    const uint32_t prio;

    /** @brief Task stack size */
    const uint32_t stack_sz;

    /** @brief Point in time when task should be awoken */
    uint64_t wakeup_time;
} task_t;

/* =================== EXTERN DEFINITIONS ======================== */

__attribute__((weak)) void idle_task(void*, void*, void*);

/* =================== HELPER MACROS ============================= */

/** 
 * @brief Define a task to be run by the OS. 
 * @param entry     task entry function
 * @param a1        task entry function 1st argument
 * @param a2        task entry function 2nd argument
 * @param a3        task entry function 3rd argument
 * @param prio      task priority
 */
#define OS_TASK_DEFINE(entry, a1, a2, a3, priority)             \
{                                                               \
    .fn = entry,                                                \
    .arg1 = a1,                                                 \
    .arg2 = a2,                                                 \
    .arg3 = a3,                                                 \
    .prio = priority,                                           \
    .stack_sz = TASK_STACK_SIZE                                 \
}

/** @brief Define the OS idle task */
#define OS_IDLE_TASK_DEFINE                                     \
{                                                               \
    .fn = idle_task,                                            \
    .arg1 = 0,                                                  \
    .arg2 = 0,                                                  \
    .arg3 = 0,                                                  \
    .prio = OS_LOWEST_PRIO,                                     \
    .stack_sz = IDLE_STACK_SIZE,                                \
}

/** @brief Define the tasks to be run by OS. Initialize with @ref OS_TASK_DEFINE */
#define OS_TASKS_INIT(...)                                      \
task_t __tasks[] = {                                            \
    __VA_ARGS__                                                 \
    OS_IDLE_TASK_DEFINE                                         \
};                                                              \
                                                                \
const uint32_t __tasks_count = (                                \
    sizeof(__tasks) /                                           \
    sizeof(task_t)                                              \
);                                                              \
                                                                \
uint8_t __attribute__(( section(".task_stacks") )) task_stacks[ \
    ((sizeof(__tasks) / sizeof(task_t) - 1) * TASK_STACK_SIZE)  \
    + IDLE_STACK_SIZE                                           \
];


/* =================== FUNCTION DECLARATIONS ===================== */

void scheduler_start(void);
void yield(void);
void sleep(int ms);


#endif /* __KANTO_OS_H__ */