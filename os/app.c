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
static uint32_t test_tick;

/* ========================= FUNCTION DECLARATIONS ========================= */

/* ========================= FUNCTION DEFINITIONS ========================= */

/**
 * @brief System tick callback
 * @n Implements a secondary tick count for now
 */
void tick_callback(void) {
    test_tick += 1;
}

/**
 * @brief Entrypoint
 */
void main(void)
{
    uint64_t ticks = 0;
    uint64_t prevticks = 0;

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

    /* Initialize system tick with 1s interval, and the callback */
    TICK_init(1000, &tick_callback);

    /* Loop forever and print current tick count. Proves both that the
        internal tick count is incremented, and that the custom callback
        is being run */
    while(1) {
        ticks = TICK_get();
        if(ticks != prevticks) {
            prevticks = ticks;
            print_hex("Tick count: ", (uint32_t)test_tick);

        }
    }

    while(1) { ; } 
}