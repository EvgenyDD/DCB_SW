#include "lcd.h"
#include "platform.h"
#include <string.h>

extern RNG_HandleTypeDef hrng;

void init(void)
{
	platform_sdram_init();
	lcd_init();
}

static uint32_t get_random_color(void)
{
	color_t ccc = hsv2rgb(HAL_RNG_GetRandomNumber(&hrng) % 360,
						  (float)(HAL_RNG_GetRandomNumber(&hrng) % 1000) * 0.001f,
						  (HAL_RNG_GetRandomNumber(&hrng) % 128) + 127);
	return 0xFF000000 | (ccc.r << 16) | (ccc.g << 8) | ccc.b;
}

volatile RCC_TypeDef *_RCC;
volatile LTDC_TypeDef *_LTDC;
volatile DMA2D_TypeDef *_DMA2D;
volatile GPIO_TypeDef *_GPIOI;
volatile GPIO_TypeDef *_GPIOJ;
volatile GPIO_TypeDef *_GPIOK;
volatile FMC_Bank5_6_TypeDef *_FMC_Bank5_6;
volatile LTDC_Layer_TypeDef *_LTDC_Layer1;
void loop(void)
{
	_RCC = RCC;
	_LTDC = LTDC;
	_DMA2D = DMA2D;
	_GPIOI = GPIOI;
	_GPIOJ = GPIOJ;
	_GPIOK = GPIOK;
	_FMC_Bank5_6 = FMC_Bank5_6;
	_LTDC_Layer1 = LTDC_Layer1;
	lcd_fill_full(&display_layers[0], 0xFFFF0000);
	HAL_Delay(300);
	lcd_fill_full(&display_layers[0], 0xFF00FF00);
	HAL_Delay(300);
	lcd_fill_full(&display_layers[0], 0xFF0000FF);
	HAL_Delay(300);
	lcd_fill_full(&display_layers[0], 0xFF000000);

	HAL_Delay(300);

	while(1)
	{
		uint32_t x, y, xl, yl;
		x = HAL_RNG_GetRandomNumber(&hrng) % LCD_WIDTH;
		y = HAL_RNG_GetRandomNumber(&hrng) % LCD_HEIGHT;
		xl = HAL_RNG_GetRandomNumber(&hrng) % LCD_WIDTH;
		yl = HAL_RNG_GetRandomNumber(&hrng) % LCD_HEIGHT;
		if(x + xl >= LCD_WIDTH) xl = LCD_WIDTH - xl;
		if(y + yl >= LCD_HEIGHT) yl = LCD_HEIGHT - yl;
		lcd_rect_fill(&display_layers[0], x, y, xl, yl, get_random_color());
		HAL_Delay(50);
	}
}