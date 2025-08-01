/*
 * @file uart_cortex_m33.c
 * @brief STM32 U5 UART simple driver implementation
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

/* =================== INCLUDES =============================== */
#include <stdint.h>
#include "uart.h"

/* ========================= CONSTANTS ========================= */

/* STM32U545XX UART registers, see 
 * https://www.st.com/resource/en/reference_manual/rm0456-stm32u5-series-armbased-32bit-mcus-stmicroelectronics.pdf
 */
static volatile uint32_t * const UART0_CR1_REG      = (uint32_t*)0x40013800;    /* Control Register 1 */
static volatile uint32_t * const UART0_BRR_REG      = (uint32_t*)0x4001380C;    /* Baud Rate Register */
static volatile uint32_t * const UART0_TDR_REG      = (uint32_t*)0x40013828;    /* TX Data Register */
static volatile uint32_t * const UART0_ISR_REG      = (uint32_t*)0x4001381C;    /* Interrupt Status Register */

/* STM32U545XX RCC registers */
#define RCC_REG_BASE_ADDR (uint32_t)0x46020C00
static volatile uint32_t * const RCC_AHB2ENR1_REG   = (uint32_t*)(RCC_REG_BASE_ADDR + 0x08C);   /* AHB2 periph. clock ENable Reg. 1 */
static volatile uint32_t * const RCC_APB2ENR_REG    = (uint32_t*)(RCC_REG_BASE_ADDR + 0x0A4);   /* APB2 periph. clock ENable Reg. */
static volatile uint32_t * const RCC_CCIPR1_REG     = (uint32_t*)(RCC_REG_BASE_ADDR + 0x0E0);   /* Periph. Independent Clk. Conf. Reg. 1 */

/* STM32U545XX GPIOA registers */
#define GPIOA_REG_BASE_ADDR (uint32_t)0x42020000
// static volatile uint32_t * const GPIOA_ODR_REG      = (uint32_t*)(GPIOA_REG_BASE_ADDR + 0x14);  /* GPIOA Output Data Register */
static volatile uint32_t * const GPIOA_MODER_REG    = (uint32_t*)(GPIOA_REG_BASE_ADDR + 0x00);  /* GPIOA MODE Register */
static volatile uint32_t * const GPIOA_OSPEEDR_REG  = (uint32_t*)(GPIOA_REG_BASE_ADDR + 0x08);  /* GPIOA Output SPEED Register */
static volatile uint32_t * const GPIOA_PUPDR_REG    = (uint32_t*)(GPIOA_REG_BASE_ADDR + 0x0C);  /* GPIOA Pull-Up/Pull-Down Register */
static volatile uint32_t * const GPIOA_AFRH_REG     = (uint32_t*)(GPIOA_REG_BASE_ADDR + 0x24);  /* GPIOA Alternate Function Register High */


/* RCC register bits */
#define USART_CLK_EN    (uint32_t)(1 << 14)                                         /* USART1 Clock enable bit */
#define GPIOA_CLK_EN    (uint32_t)(1 << 0)                                          /* GPIOA Clock enable bit */

/* GPIOA register bits */
#define USART_TX_PIN    9                                                           /* USART1 TX pin PA9 */
#define USART_RX_PIN    10                                                          /* USART1 RX pin PA10 */

#define GPIO_AF_MODE    0x2                                                         /* IO pin configuration in MODER = Alternate function mode */
#define GPIO_MODE_MASK  0x3                                                         /* Bitmask for IO pin configuration in MODER */
#define USART_TX_PIN_MODER_SHIFT    (USART_TX_PIN * 2)                              /* Bit shift for setting TX pin MODER value */
#define USART_RX_PIN_MODER_SHIFT    (USART_RX_PIN * 2)                              /* Bit shift for setting RX pin MODER value */
#define USART_TX_PIN_MODER_AF       (uint32_t)(GPIO_AF_MODE << (USART_TX_PIN * 2))  /* Register value for setting TX pin to AF mode */
#define USART_RX_PIN_MODER_AF       (uint32_t)(GPIO_AF_MODE << (USART_RX_PIN * 2))  /* Register value for setting RX pin to AF mode */

#define HIGH_SPEED  0x2                                                             /* IO pin configuration in OSPEEDR = High speed */
#define SPEED_MASK  0x3                                                             /* Bitmask for IO pin configuration in OSPEEDR */
#define USART_TX_PIN_OSPEEDR_SHIFT    (USART_TX_PIN * 2)                            /* Bit shift for setting TX pin OSPEEDR value */
#define USART_RX_PIN_OSPEEDR_SHIFT    (USART_RX_PIN * 2)                            /* Bit shift for setting RX pin OSPEEDR value */
#define USART_TX_PIN_OSPEEDR_SPD    (uint32_t)(HIGH_SPEED << (USART_TX_PIN * 2))    /* Register value for setting TX pin to High Speed mode */
#define USART_RX_PIN_OSPEEDR_SPD    (uint32_t)(HIGH_SPEED << (USART_RX_PIN * 2))    /* Register value for setting RX pin to High Speed mode */

#define PULLUP 0x1                                                                  /* IO pin configuration in PUPDR = Pull-Up */
#define PULL_MASK 0x3                                                               /* Bitmask for IO pin configuration in PUPDR */
#define USART_TX_PIN_PUPDR_SHIFT    (USART_TX_PIN * 2)                              /* Bit shift for setting TX pin PUPDR value */
#define USART_RX_PIN_PUPDR_SHIFT    (USART_RX_PIN * 2)                              /* Bit shift for setting RX pin PUPDR value */
#define USART_TX_PIN_PUPDR          (uint32_t)(PULLUP << (USART_TX_PIN * 2))        /* Register value for setting TX pin to Pull-Up mode */
#define USART_RX_PIN_PUPDR          (uint32_t)(PULLUP << (USART_RX_PIN * 2))        /* Register value for setting RX pin to Pull-Up mode */

#define GPIO_AF7_USART1 0x7                                                         /* Alternate function selection in AFRH = AF7 = USART */
#define GPIO_AF_MASK    0xF                                                         /* Bitmask for alternate function selection in AFRH */
#define USART_TX_PIN_AFR_SHIFT      ((USART_TX_PIN - 8) * 4)                        /* Bit shift for setting TX pin AFRH value */
#define USART_RX_PIN_AFR_SHIFT      ((USART_RX_PIN - 8) * 4)                        /* Bit shift for setting RX pin AFRH value */
#define USART_TX_PIN_AFR_AF7        (uint32_t)(GPIO_AF7_USART1 << USART_TX_PIN_AFR_SHIFT)   /* Reg. value for setting TX pin to USART AF */
#define USART_RX_PIN_AFR_AF7        (uint32_t)(GPIO_AF7_USART1 << USART_RX_PIN_AFR_SHIFT)   /* Reg. value for setting RX pin to USART AF */

/* UART CTRL register control bits */
#define USART_CR1_UE    (uint32_t)(1 << 0)          /* Usart Enable bit */
#define USART_CR1_TE    (uint32_t)(1 << 3)          /* Transmit Enable bit */
#define UART_ENABLE (USART_CR1_TE | USART_CR1_UE)   /* UE + TE */

/* UART ISR register flags */
#define USART_ISR_TXE   (uint32_t)(1 << 7)          /* TX buffer Empty */
#define USART_ISR_TC    (uint32_t)(1 << 6)          /* Transmit Complete */

/* Easier mnemonics */
#define UART_DATA_REGISTER      (UART0_TDR_REG)
#define UART_CONTROL_REGISTER   (UART0_CR1_REG)
#define GPIOA_CLK_ENABLE_REG    (RCC_AHB2ENR1_REG)
#define UART1_CLK_ENABLE_REG    (RCC_APB2ENR_REG)

/* ========================= FUNCTION DECLARATIONS ========================= */

int STM_UART_init(void);
int STM_UART_printc(const char *c);
int STM_UART_printstr(const char *msg);

/* ========================= STATIC DATA ========================= */


static const UartDriver drv = {
    &STM_UART_init,
    &STM_UART_printc,
    &STM_UART_printstr
};
const UartDriver *Uart_Driver = &drv;

/* ========================= FUNCTION DEFINITIONS ========================= */

/**
 * @brief Enable GPIOA peripheral clock in RCC
 */
void gpioa_clock_enable(void)
{
    uint32_t temp;

    /* Enable GPIOA clock */
    temp = *GPIOA_CLK_ENABLE_REG;
    temp |= GPIOA_CLK_EN;
    *GPIOA_CLK_ENABLE_REG = temp;
}

/**
 * @brief Enable USART1 peripheral clock in RCC
 */
void usart1_clock_enable(void)
{
    uint32_t temp;

    /* Enable UART1 clock */
    temp = *UART1_CLK_ENABLE_REG;
    temp |= USART_CLK_EN;
    *UART1_CLK_ENABLE_REG = temp;
}

/**
 * @brief Initialize the USART1 peripheral
 * @note  Configures clock sources, pinmuxes, and enables the peripheral
 * 
 * @return 0 on success
 */
int STM_UART_init(void)
{
    uint32_t temp;

    /* Enable GPIOA and USART1 clocks */
    gpioa_clock_enable();
    usart1_clock_enable();

    /* set UART clock source to PCLK2 */
    temp = *RCC_CCIPR1_REG;
    temp &= ~(0x3);
    temp |= 0x0;
    *RCC_CCIPR1_REG = temp;

    /* configure AF (AFR reg) */
    temp = *GPIOA_AFRH_REG;
    temp &= ~((GPIO_AF_MASK << USART_TX_PIN_AFR_SHIFT) |(GPIO_AF_MASK << USART_RX_PIN_AFR_SHIFT));
    temp |= (USART_TX_PIN_AFR_AF7 | USART_RX_PIN_AFR_AF7);
    *GPIOA_AFRH_REG = temp;

    /* configure IO direction (MODER reg) */
    temp = *GPIOA_MODER_REG;
    temp &= ~((GPIO_MODE_MASK << USART_TX_PIN_MODER_SHIFT) | (GPIO_MODE_MASK << USART_RX_PIN_MODER_SHIFT));
    temp |= (USART_TX_PIN_MODER_AF | USART_RX_PIN_MODER_AF);
    *GPIOA_MODER_REG = temp;

    /* configure IO speed (OSPEEDR reg) */
    temp = *GPIOA_OSPEEDR_REG;
    temp &= ~((SPEED_MASK << USART_TX_PIN_OSPEEDR_SHIFT) | (SPEED_MASK << USART_RX_PIN_OSPEEDR_SHIFT));
    temp |= (USART_TX_PIN_OSPEEDR_SPD | USART_RX_PIN_OSPEEDR_SPD);
    *GPIOA_OSPEEDR_REG = temp;

    /* configure pull-up (PUPDR reg) */
    temp = *GPIOA_PUPDR_REG;
    temp &= ~((PULL_MASK << USART_TX_PIN_PUPDR_SHIFT) | (PULL_MASK << USART_RX_PIN_PUPDR_SHIFT));
    temp |= (USART_TX_PIN_PUPDR | USART_RX_PIN_PUPDR);
    *GPIOA_PUPDR_REG = temp;

    /* Set baudrate (4MHz / 0x22 = 115200bps). TODO: figure out how on earth to do this automatically */
    *UART0_BRR_REG = 0x22;

    /* Enable UART */
    *UART_CONTROL_REGISTER = USART_CR1_UE | USART_CR1_TE;

    return 0;
}


/**
 * @brief Prints one character to USART1 output
 * 
 * @return 0 on success, -1 on error
 */
int STM_UART_printc(const char *c)
{
    /* Null-check parameters */
    if(!c) {
        return -1;
    }

    /* Busy-loop until TXE (Transmit buffer empty flag) is set */
    while((*UART0_ISR_REG & USART_ISR_TXE) == 0) { ; }

    /* Move byte to UART Transmit Data Register */
    *UART_DATA_REGISTER = *c;

    /* Busy-loop until TC (Transmit Complete flag) is set */
    while((*UART0_ISR_REG & USART_ISR_TC) == 0) { ; }

    return 0;
}

/**
 * @brief Prints a null-terminated string to USART1 output
 * 
 * @return 0 on success
 */
int STM_UART_printstr(const char *msg)
{
    
    /* Loop until null character is reached */
    while(*msg != '\0') {

        /* Busy-loop until TXE (Transmit buffer empty flag) is set */
        while((*UART0_ISR_REG & USART_ISR_TXE) == 0) { ; }

        /* Move byte to UART Transmit Data Register */
        *UART_DATA_REGISTER = (*msg++);
    }

    /* Busy-loop until TC (Transmit Complete flag) is set */
    while((*UART0_ISR_REG & USART_ISR_TC) == 0) { ; }

    return 0;
}