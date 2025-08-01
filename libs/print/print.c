/*
 * @file print.c
 * @brief Implementation of generic debug print operations
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

/* ========================= INCLUDES ========================= */
#include <stdint.h>
#include "uart.h"
#include "print.h"

/* ========================= CONSTANTS ========================= */
/* ========================= FUNCTION DECLARATIONS ============= */
/* ========================= STATIC DATA ======================= */
/* ========================= FUNCTION DEFINITIONS ============== */

/**
 * @brief Prints a null-terminated string and newline
 * @param[in] msg   string to print
 */
void print(const char *msg)
{
    (void)UART_print_str(msg);
    (void)UART_print_str("\r\n");
}
/**
 * @brief Prints a null-terminated string, a hex value, and a newline
 * @param[in] msg   string to print
 * @param[in] value integer to print in hex format
 */
void print_hex(const char *msg, uint32_t value)
{
    uint32_t tmp;
    int32_t i;
    char c;

    (void)UART_print_str(msg);
    (void)UART_print_str("0x");

    for( i = 7; i >= 0; i--) {
        tmp = value >> (i*4) & 0xFUL; 
        if(tmp < 10) {
            c = '0' + (char)tmp;
        } else {
            c = 'A' + (char)(tmp - 10);

        }
        (void)UART_print_chr(&c);
    }

    (void)UART_print_str("\r\n");
}
