/*
 * @file system.h
 * @brief System driver wrapper
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

/* =================== INCLUDES =============================== */
#include <stdint.h>
#include "os.h"
/* =================== MACRO DEFINITIONS ====================== */

#define SYSTEM_OK       0
#define SYSTEM_ERROR    1

/* =================== TYPE DEFINITIONS ======================= */

/** @brief SysTick ISR callback function pointer */
typedef void (*Tick_Callback)(void);

/** @brief Abstract System Driver vtable definition */
typedef struct SystemDriver {
    const int (* const TickInit)(int, Tick_Callback);
    const int (* const PendSVInit)(void);
    const void (* const TaskStackInit)(task_t *);
    const uint32_t (* const CountLeadingZeros)(uint32_t);
    const uint64_t (* const GetTicks)(void);
    const void (* const BusySleep)(int);
    const void (* const PendSVTrigger)(void);
} SystemDriver;

/** @brief Pointer to SystemDriver implementation */
extern const SystemDriver *Sys_Driver;

/* =================== FUNCTION DEFINITIONS ================== */

/**
 * @brief Configure and enable the system tick
 * @param[in] ms    tick interval in milliseconds
 * @param[in] cb    tick callback
 * 
 * @return SYSTEM_OK on success, SYSTEM_ERROR otherwise
 */
static inline int TICK_init(int ms, Tick_Callback cb)
{
    if(!Sys_Driver || !cb) {
        return SYSTEM_ERROR;
    }
    
    if(Sys_Driver->TickInit(ms, cb) != 0) {
        return SYSTEM_ERROR;
    }
    return SYSTEM_OK;
}

/**
 * @brief Get number of ticks since startup
 * 
 * @return number of ticks
 */
static inline uint64_t TICK_get(void)
{
    if(!Sys_Driver) {
        return (uint64_t)0;
    }

    return Sys_Driver->GetTicks();
}

/**
 * @brief Set a callback for the PendSV interrupt
 * @param[in] cb    callback
 * 
 * @return SYSTEM_OK on success, SYSTEM_ERROR otherwise
 */
static inline int PendSV_init()
{
    if(!Sys_Driver) {
        return SYSTEM_ERROR;
    }
    
    if(Sys_Driver->PendSVInit() != 0) {
        return SYSTEM_ERROR;
    }
    return SYSTEM_OK;
}

/**
 * @brief Trigger the PendSV interrupt
 */
static inline int PendSV_trigger()
{
    if(!Sys_Driver) {
        return SYSTEM_ERROR;
    }
    
    Sys_Driver->PendSVTrigger();

    return SYSTEM_OK;
}

/**
 * @brief Blocking busy sleep
 * @param[in] us    sleep interval in microseconds
 */
static inline void busysleep(int us)
{
    if(Sys_Driver) {
        Sys_Driver->BusySleep(us);
    }
}

/**
 * @brief Perform architecture specific task 
 *  stack initialization
 * @param task  pointer to task whose stack shall be initialized
 */
static inline void TaskStackInit(task_t *task)
{
    if(Sys_Driver) {
        Sys_Driver->TaskStackInit(task);
    }
}

/**
 * @brief Count leading zeroes on a 32-bit number
 * @param value  The value to count leading zeros on
 * @return The number or leading zeroes (0-32)
 */
static inline uint32_t CountLeadingZeros(uint32_t value)
{
    if(Sys_Driver) {
        return Sys_Driver->CountLeadingZeros(value);
    }
    return 0;
}


#endif /* __SYSTEM_H__ */
