/*
 * @file uart.h
 * @brief UART driver wrapper
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

#ifndef __UART_H__
#define __UART_H__

/* =================== INCLUDES =============================== */
/* =================== MACRO DEFINITIONS ====================== */

#define UART_OK 0
#define UART_ERROR 1

/* =================== TYPE DEFINITIONS ======================= */

typedef struct UartDriver {
    const int (* const Initialize)(void);
    const int (* const PrintChar)(const char *);
    const int (* const PrintString)(const char *);
} UartDriver;

extern const UartDriver *Uart_Driver;

/* =================== FUNCTION DECLARATIONS ================== */

/**
 * @brief Initialize the UART peripheral
 * 
 * @return UART_OK on success, UART_ERROR otherwise
 */
static inline int UART_init(void)
{
    if(!Uart_Driver) {
        return UART_ERROR;
    }
    
    if(Uart_Driver->Initialize() != 0) {
        return UART_ERROR;
    }
    return UART_OK;
}

/**
 * @brief Print one character to UART
 * @param[in] c   pointer to the character to print
 * 
 * @return UART_OK on success, UART_ERROR otherwise
 */
static inline int UART_print_chr(const char *c)
{
    if(!Uart_Driver) {
        return UART_ERROR;
    }
    
    if(Uart_Driver->PrintChar(c) != 0) {
        return UART_ERROR;
    }
    return UART_OK;
}

/**
 * @brief Print a null-terminated string on UART
 * @param[in] msg   pointer to the string to print
 * 
 * @return UART_OK on success, UART_ERROR otherwise
 */
static inline int UART_print_str(const char* msg)
{
    if(!Uart_Driver) {
        return UART_ERROR;
    }
    
    if(Uart_Driver->PrintString(msg) != 0) {
        return UART_ERROR;
    }
    return UART_OK;
}

#endif /* __UART_H__ */
