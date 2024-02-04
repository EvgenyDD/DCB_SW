#ifndef PLATFORM_H__
#define PLATFORM_H__

#include "main.h"

#define PIN_SET(x) x##_GPIO_Port->BSRR = x##_Pin
#define PIN_CLR(x) x##_GPIO_Port->BSRR = ((uint32_t)(x##_Pin)) << 16
#define PIN_WR(x, v) x##_GPIO_Port->BSRR = ((uint32_t)(x##_Pin)) << ((!(v)) * 16)
#define PIN_GET(x) !!(x##_GPIO_Port->IDR & x##_Pin)
#define PIN_GET_ODR(x) !!(x##_GPIO_Port->ODR & x##_Pin)

void platform_sdram_init(void);

#endif // PLATFORM_H__