/*
 * @file app.c
 * @brief Main application entrypoint
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

/* ========================= INCLUDES ========================= */
#include <stdint.h>

#include "uart.h"
#include "system.h"
#include "os.h"
#include "print/print.h"

/* ========================= CONSTANTS ========================= */


/* ========================= STATIC DATA ========================= */

/** @brief Sample data in .data section */
static char data_buffer[] = "Hello, .data";

/** @brief Sample data in .rodata section */
static const char rodata_buffer[] = "Hello, .rodata";

/** @brief Sample data in .bss section */
static char bss_buffer[128];

/** @brief Secondary tick counter */
static volatile uint32_t test_tick;

/** @brief PendSV flag to verify it is triggered */
static volatile uint32_t test_pendsv;


/* ========================= FUNCTION DECLARATIONS ========================= */

/* ========================= FUNCTION DEFINITIONS ========================= */

/** @brief Sample task A */
void taskA(void* arg1, void* arg2, void* arg3)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    uint32_t cnt = 0;

    while(1) {
        print_hex(__func__, cnt);
        cnt++;
        sleep(100);
    }
}

/** @brief Sample task A */
void taskB(void* arg1, void* arg2, void* arg3)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    uint32_t cnt = 0;

    while(1) {
        print_hex(__func__, cnt);
        cnt++;
        sleep(1000);
    }
}

/** @brief Register the tasks with the OS */
OS_TASKS_INIT(
    OS_TASK_DEFINE(taskA, 0, 0, 0, OS_LOWEST_PRIO + 1),
    OS_TASK_DEFINE(taskB, 0, 0, 0, OS_LOWEST_PRIO + 1),
);

/**
 * @brief Entrypoint
 */
void main(void)
{

    /* Sample data in literal pool */
    char * buffer = "Hello, literal pool!";

    /* Initialize UART for printing */
    UART_init();

    /* Print from literal pool */
    print(buffer);

    /* Print from .data section, modify, and re-print */
    print(data_buffer);
    data_buffer[0] = 'X';
    print(data_buffer);

    /* Print from .rodata buffer */
    print(rodata_buffer);

    /* Store something in buffer in .bss section and print it */
    bss_buffer[0] = '.';
    bss_buffer[1] = 'b';
    bss_buffer[2] = 's';
    bss_buffer[3] = 's';
    print(bss_buffer);

    /* Kick off scheduling. Will not return */
    scheduler_start();

    print("UNREACHABLE");

    while(1) { ; } 
}
