/*
 * @file led_cortex_m33.c
 * @brief Cortex-M33 LED utilities driver, TODO
 *
 *      Copyright (c) 2025 Miikka Lukumies
 */

/* =================== INCLUDES =============================== */
#include <stdint.h>

/* ========================= FUNCTION DEFINITIONS ========================= */
// /**
//  * @brief Setup User LED as output
//  */
// void user_led_setup(void)
// {
//     uint32_t temp;

//     /* Enable GPIOA peripheral clock */
//     gpioa_clock_enable();

//     /* Configure PA5 (user LED LD2 on Nucleo) to output */
//     temp = *GPIOA_MODER_REG;
//     temp &= ~((GPIO_MODE_MASK << 5*2));
//     temp |= (0x1 << 5*2);
//     *GPIOA_MODER_REG = temp;

// }
// /**
//  * @brief Toggel user LED state
//  */
// void user_led_toggle(void)
// {
//     uint32_t temp;
 
//     /* Toggle led */
//     temp = *GPIOA_ODR_REG;
//     temp = temp ^ (1 << 5);
//     *GPIOA_ODR_REG = temp;
// }
