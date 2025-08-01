/*
 * @file print.h
 * @brief Header file for generic debug print operations
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

#ifndef __PRINT_H__
#define __PRINT_H__

/* ========================= INCLUDES ========================= */
#include <stdint.h>

/* =================== FUNCTION DECLARATIONS ================== */

void print(const char *msg);
void print_hex(const char *msg, uint32_t value);

#endif /* __PRINT_H__ */
