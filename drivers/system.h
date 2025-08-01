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
/* =================== MACRO DEFINITIONS ====================== */

#define SYSTEM_OK       0
#define SYSTEM_ERROR    1

/* =================== TYPE DEFINITIONS ======================= */

typedef void (*Tick_Handler)(void);

typedef struct SystemDriver {
    const int (* const TickInit)(int, Tick_Handler);
    const uint64_t (* const GetTicks)(void);
    const void (* const Sleep)(int);
    const void (* const BusySleep)(int);
} SystemDriver;

extern const SystemDriver *Sys_Driver;

/* =================== FUNCTION DECLARATIONS ================== */

/**
 * @brief Configure and enable the system tick
 * @param[in] ms    tick interval in milliseconds
 * @param[in] cb    tick callback
 * 
 * @return SYSTEM_OK on success, SYSTEM_ERROR otherwise
 */
static inline int TICK_init(int ms, Tick_Handler cb)
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
 * @brief Blocking sleep using system tick. Yields current task
 * @param[in] ms    sleep interval in milliseconds
 */
static inline void sleep(int ms)
{
    if(Sys_Driver) {
        Sys_Driver->Sleep(ms);
    }
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

#endif /* __SYSTEM_H__ */
