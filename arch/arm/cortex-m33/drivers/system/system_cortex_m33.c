/*
 * @file system_cortex_m33.c
 * @brief Cortex-M33 system utilities driver
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

/* =================== INCLUDES =============================== */
#include <stdint.h>
#include "system.h"

/* ========================= CONSTANTS ========================= */
// https://developer.arm.com/documentation/100235/0100/The-Cortex-M33-Peripherals/System-timer--SysTick
#define SCS_BASE            (0xE000E000UL)

#define SYSTICK_BASE        (SCS_BASE +  0x0010UL)
#define SYSTICK_CTRL        (volatile uint32_t*)(SYSTICK_BASE + 0x0UL)
#define SYSTICK_RVR         (volatile uint32_t*)(SYSTICK_BASE + 0x4UL)
#define SYSTICK_CVR         (volatile uint32_t*)(SYSTICK_BASE + 0x8UL)
#define SYSTICK_PRIO_MASK   (0xFFUL << 24)
#define SYSTICK_PRIO        (0xC0UL << 24)

#define NVIC_ICSR           (volatile uint32_t*)(SCS_BASE + 0xD04UL)
#define NVIC_SHPR3          (volatile uint32_t*)(SCS_BASE + 0xD20UL)
#define PENDSV_SET          (0x1UL << 28)

/* ========================= FUNCTION DECLARATIONS ========================= */

int STM_TICK_init(int ms, Tick_Callback cb);
int STM_PendSV_init(PendSV_Callback cb);
uint64_t STM_TICK_get(void);
void STM_sleep(int ms);
void STM_busy_sleep(int ms);
void STM_PendSV_trigger(void);

/* ========================= STATIC DATA ========================= */

/** @brief System driver vtable */
static SystemDriver drv = {
    &STM_TICK_init,
    &STM_PendSV_init,
    &STM_TICK_get,
    &STM_sleep,
    &STM_busy_sleep,
    &STM_PendSV_trigger
};

/** @brief System driver pointer, matching extern in os driver abstraction */
const SystemDriver *Sys_Driver = &drv;

/** @brief Static pointers for ISR callbacks */
static Tick_Callback tick_cb;
static PendSV_Callback psv_cb;

/** @brief System tick counter */
static volatile uint64_t systicks;

/* ========================= FUNCTION DEFINITIONS ========================= */

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
 * @brief PendSV ISR
 * @n Calls the PendSV ISR callback (if set)
 */
void __attribute__( ( naked ) ) PendSV_Handler(void)
{
    /* Save registers modified by this function (r0-r3 saved in hw) */
    asm("push {lr}");

    /* Call pendsv handler, null-checking the psv_cb pointer */
    asm("ldr r0, =psv_cb");
    asm("ldr r0, [r0]");

    asm("subs r0, #0");
    asm("it ne");
    asm("blxne  r0");

    asm("pop {lr}");

    /* Return from interrupt */
    asm("bx lr");
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
 * @n Populates the pointer to PendSV ISR callback
 */
int STM_PendSV_init(PendSV_Callback cb)
{
    psv_cb = cb;

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

/** TODO:
 * @brief Blocking sleep using system tick. Yields current task
 * @param[in] ms    sleep interval in milliseconds
 */
void STM_sleep(int ms)
{
    /** TODO: implement */
    uint32_t i;
    /* Loop for some time */
    for(i = 0; i < 0xFFFF; i++) { ; }
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