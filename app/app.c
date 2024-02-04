#include "adc.h"
#include "config_system.h"
#include "crc.h"
#include "display.h"
#include "fw_header.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "sram.h"
#include "usbd_proto_core.h"
#include <stdio.h>
#include <string.h>

int gsts = -10;

#define SYSTICK_IN_US (168000000 / 1000000)
#define SYSTICK_IN_MS (168000000 / 1000)

bool g_stay_in_boot = false;

uint32_t g_uid[3];

volatile uint64_t system_time = 0;
static int32_t prev_systick = 0;

config_entry_t g_device_config[] = {};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

void delay_ms(volatile uint32_t delay_ms)
{
	volatile uint32_t start = 0;
	int32_t mark_prev = 0;
	prof_mark(&mark_prev);
	const uint32_t time_limit = delay_ms * SYSTICK_IN_MS;
	for(;;)
	{
		start += (uint32_t)prof_mark(&mark_prev);
		if(start >= time_limit)
			return;
	}
}

static void end_loop(void)
{
	delay_ms(2000);
	platform_reset();
}

static uint32_t get_random_color(void)
{
	color_t ccc = hsv2rgb(RNG_GetRandomNumber() % 360,
						  (float)(RNG_GetRandomNumber() % 1000) * 0.001f,
						  (RNG_GetRandomNumber() % 128) + 127);
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

void main(void)
{
	_RCC = RCC;
	_LTDC = LTDC;
	_DMA2D = DMA2D;
	_GPIOI = GPIOI;
	_GPIOJ = GPIOJ;
	_GPIOK = GPIOK;
	_FMC_Bank5_6 = FMC_Bank5_6;
	_LTDC_Layer1 = LTDC_Layer1;
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	RNG_Cmd(ENABLE);

	platform_get_uid(g_uid);

	prof_init();
	platform_watchdog_init();

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA |
							   RCC_AHB1Periph_GPIOB |
							   RCC_AHB1Periph_GPIOC |
							   RCC_AHB1Periph_GPIOD |
							   RCC_AHB1Periph_GPIOE |
							   RCC_AHB1Periph_GPIOF |
							   RCC_AHB1Periph_GPIOG |
							   RCC_AHB1Periph_GPIOH |
							   RCC_AHB1Periph_GPIOI |
							   RCC_AHB1Periph_GPIOJ |
							   RCC_AHB1Periph_GPIOK,
						   ENABLE);

	fw_header_check_all();

	ret_mem_init();
	ret_mem_set_load_src(LOAD_SRC_APP);

	adc_init();

	sram_init();
	display_init();
	lcd_fill_full(&display_layers[0], 0xFF000000);

	usb_init();

	if(config_validate() == CONFIG_STS_OK) config_read_storage();

	for(;;)
	{
		// time diff
		uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick);

		static uint32_t remain_systick_us_prev = 0, remain_systick_ms_prev = 0;
		uint32_t diff_us = (time_diff_systick + remain_systick_us_prev) / (SYSTICK_IN_US);
		remain_systick_us_prev = (time_diff_systick + remain_systick_us_prev) % SYSTICK_IN_US;

		uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
		remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

		platform_watchdog_reset();

		system_time += diff_ms;

		adc_track();

		static uint8_t upd_cnt, upd_cnt2;
		upd_cnt += diff_ms;
		upd_cnt2 += diff_ms;

		usb_poll(diff_ms);

		static uint32_t duc = 0;
		if(++duc >= 50)
		{
			duc = 0;
			uint32_t x, y, xl, yl;
			x = RNG_GetRandomNumber() % LCD_WIDTH;
			y = RNG_GetRandomNumber() % LCD_HEIGHT;
			xl = RNG_GetRandomNumber() % LCD_WIDTH;
			yl = RNG_GetRandomNumber() % LCD_HEIGHT;
			if(x + xl >= LCD_WIDTH) xl = LCD_WIDTH - xl;
			if(y + yl >= LCD_HEIGHT) yl = LCD_HEIGHT - yl;
			lcd_rect_fill(&display_layers[0], x, y, xl, yl, get_random_color());
		}
	}
}