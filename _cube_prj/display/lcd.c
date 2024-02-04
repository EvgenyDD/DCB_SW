#include "lcd.h"
#include "platform.h"

//	HAL_SDRAM_Write_32b(&hsdram1, (uint32_t *)(FRAMEBUFER_ADDR + WRITE_READ_ADDR + 0x40000), aTxBuffer, BUFFER_SIZE);
// HAL_SDRAM_Write_32b(&hsdram1, (uint32_t *)(FRAMEBUFER_ADDR + WRITE_READ_ADDR), aTxBuffer, BUFFER_SIZE);
// HAL_SDRAM_Read_32b(&hsdram1, (uint32_t *)(FRAMEBUFER_ADDR + WRITE_READ_ADDR), aRxBuffer, BUFFER_SIZE);
// __HAL_LTDC_ENABLE(&hltdc);
// __HAL_LTDC_DISABLE(&hltdc);
// HAL_GPIO_WritePin(BL_SHDN_GPIO_Port, BL_SHDN_Pin, 0);
// HAL_GPIO_WritePin(BL_SHDN_GPIO_Port, BL_SHDN_Pin, 1);

// pixel format: alpha blue green red

extern LTDC_HandleTypeDef hltdc;

gui_layer_t display_layers[GUI_LAYERS];

#define DMA2D_START(type)                                             \
	do                                                                \
	{                                                                 \
		while(READ_BIT(hltdc.Instance->CDSR, LTDC_CDSR_VSYNCS) == 0U) \
			asm("nop");                                               \
		__HAL_LTDC_DISABLE(&hltdc);                                   \
		DMA2D->CR = (type);                                           \
		DMA2D->CR |= 0 | 0;                                           \
		DMA2D->CR |= DMA2D_CR_START;                                  \
		while(DMA2D->CR & DMA2D_CR_START)                             \
			;                                                         \
		__HAL_LTDC_ENABLE(&hltdc);                                    \
	} while(0)

/**
 * h [0;360]
 * s [0;1]
 * v [0;255]
 */
color_t hsv2rgb(float h, float s, float v)
{
	float p, q, t, ff;
	color_t out;

	float hh = h;
	if(hh >= 360.0f || hh < 0) hh = 0;
	hh /= 60.0f;
	int i = (int)hh;
	ff = hh - i;
	p = v * (1.0f - s);
	q = v * (1.0f - (s * ff));
	t = v * (1.0f - (s * (1.0f - ff)));

	switch(i)
	{
	case 0:
		out.r = v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = v;
		break;
	case 5:
	default:
		out.r = v;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}

void lcd_fill(void *dst, uint16_t xSize, uint16_t ySize, uint16_t OffLine, uint32_t color)
{
	if(!xSize || !ySize) return;

	while(DMA2D->CR & DMA2D_CR_START)
		;													/* Wait finished */
	DMA2D->OCOLR = color;									/* Color to be used */
	DMA2D->OMAR = (uint32_t)dst;							/* Destination address */
	DMA2D->OOR = OffLine;									/* Destination line offset */
	DMA2D->OPFCCR = DMA2D_OUTPUT_ARGB8888;					/* Defines the number of pixels to be transfered */
	DMA2D->NLR = (uint32_t)(xSize << 16) | (uint16_t)ySize; /* Size configuration of area to be transfered */

	DMA2D_START(DMA2D_R2M);
}

void lcd_fill_full(gui_layer_t *layer, uint32_t color)
{
	lcd_fill(layer->start_address, LCD_WIDTH, LCD_HEIGHT, 0, color);
}

void lcd_line_h(gui_layer_t *layer, uint16_t x, uint16_t y, uint16_t length, uint32_t color)
{
	uint32_t addr = (uint32_t)layer->start_address + (4 * (layer->width * y + x));
	lcd_fill((void *)addr, length, 1, layer->width - length, color);
}

void lcd_line_v(gui_layer_t *layer, uint16_t x, uint16_t y, uint16_t length, uint32_t color)
{
	uint32_t addr = (uint32_t)layer->start_address + (4 * (layer->width * y + x));
	lcd_fill((void *)addr, 1, length, layer->width - 1, color);
}

void lcd_rect_fill(gui_layer_t *layer, uint16_t x, uint16_t y, uint16_t xSize, uint16_t ySize, uint32_t color)
{
	uint32_t addr = (uint32_t)layer->start_address + (4 * (layer->width * y + x));
	lcd_fill((void *)addr, xSize, ySize, layer->width - xSize, color);
}

void lcd_pixel(gui_layer_t *layer, uint16_t x, uint16_t y, uint32_t color)
{
	lcd_line_h(layer, x, y, 1, color);
}

uint8_t lcd_is_ready(void) { return !(DMA2D->CR & DMA2D_CR_START); }

static void spi_dly(void)
{
	for(volatile uint32_t i = 0; i < 10; i++)
		asm("nop");
}

static void spi_9b_tx(uint8_t cmd, uint8_t val)
{
	PIN_CLR(DISP_CS);
	spi_dly();
	for(uint8_t i = 0; i < 9; i++)
	{
		if(i == 0)
		{
			PIN_CLR(DISP_MOSI);
		}
		else
		{
			PIN_WR(DISP_MOSI, cmd & 0x80);
			cmd <<= 1;
		}
		spi_dly();
		PIN_SET(DISP_SCK);
		spi_dly();
		PIN_CLR(DISP_SCK);
	}
	for(uint8_t i = 0; i < 9; i++)
	{
		if(i == 0)
		{
			PIN_SET(DISP_MOSI);
		}
		else
		{
			PIN_WR(DISP_MOSI, val & 0x80);
			val <<= 1;
		}
		spi_dly();
		PIN_SET(DISP_SCK);
		spi_dly();
		PIN_CLR(DISP_SCK);
	}
	spi_dly();
	PIN_SET(DISP_CS);
	spi_dly();
}

void lcd_init(void)
{
	PIN_CLR(DISP_RST);
	HAL_Delay(1);
	PIN_SET(DISP_RST);
	HAL_Delay(120);

	spi_9b_tx(0xFF, 0x30);
	spi_9b_tx(0xFF, 0x52);
	spi_9b_tx(0xFF, 0x01); // page 1

	spi_9b_tx(0xE3, 0x00);
	spi_9b_tx(0x40, 0x00);
	spi_9b_tx(0x03, 0x40);
	spi_9b_tx(0x04, 0x00);
	spi_9b_tx(0x05, 0x03);
	spi_9b_tx(0x08, 0x00);
	spi_9b_tx(0x09, 0x07);
	spi_9b_tx(0x0A, 0x01);
	spi_9b_tx(0x0B, 0x32);
	spi_9b_tx(0x0C, 0x32);
	spi_9b_tx(0x0D, 0x0B);
	spi_9b_tx(0x0E, 0x00);
	spi_9b_tx(0x23, 0xA2);

	spi_9b_tx(0x24, 0x0c);
	spi_9b_tx(0x25, 0x06);
	spi_9b_tx(0x26, 0x14);
	spi_9b_tx(0x27, 0x14);

	spi_9b_tx(0x38, 0x9C);
	spi_9b_tx(0x39, 0xA7);
	spi_9b_tx(0x3A, 0x3a);

	spi_9b_tx(0x28, 0x40);
	spi_9b_tx(0x29, 0x01);
	spi_9b_tx(0x2A, 0xdf);
	spi_9b_tx(0x49, 0x3C);
	spi_9b_tx(0x91, 0x57);
	spi_9b_tx(0x92, 0x57);
	spi_9b_tx(0xA0, 0x55);
	spi_9b_tx(0xA1, 0x50);
	spi_9b_tx(0xA4, 0x9C);
	spi_9b_tx(0xA7, 0x02);
	spi_9b_tx(0xA8, 0x01);
	spi_9b_tx(0xA9, 0x01);
	spi_9b_tx(0xAA, 0xFC);
	spi_9b_tx(0xAB, 0x28);
	spi_9b_tx(0xAC, 0x06);
	spi_9b_tx(0xAD, 0x06);
	spi_9b_tx(0xAE, 0x06);
	spi_9b_tx(0xAF, 0x03);
	spi_9b_tx(0xB0, 0x08);
	spi_9b_tx(0xB1, 0x26);
	spi_9b_tx(0xB2, 0x28);
	spi_9b_tx(0xB3, 0x28);
	spi_9b_tx(0xB4, 0x33);
	spi_9b_tx(0xB5, 0x08);
	spi_9b_tx(0xB6, 0x26);
	spi_9b_tx(0xB7, 0x08);
	spi_9b_tx(0xB8, 0x26);
	spi_9b_tx(0xF0, 0x00);
	spi_9b_tx(0xF6, 0xC0);

	spi_9b_tx(0xFF, 0x30);
	spi_9b_tx(0xFF, 0x52);
	spi_9b_tx(0xFF, 0x02); // page 2

	spi_9b_tx(0xB0, 0x0B);
	spi_9b_tx(0xB1, 0x16);
	spi_9b_tx(0xB2, 0x17);
	spi_9b_tx(0xB3, 0x2C);
	spi_9b_tx(0xB4, 0x32);
	spi_9b_tx(0xB5, 0x3B);
	spi_9b_tx(0xB6, 0x29);
	spi_9b_tx(0xB7, 0x40);
	spi_9b_tx(0xB8, 0x0d);
	spi_9b_tx(0xB9, 0x05);
	spi_9b_tx(0xBA, 0x12);
	spi_9b_tx(0xBB, 0x10);
	spi_9b_tx(0xBC, 0x12);
	spi_9b_tx(0xBD, 0x15);
	spi_9b_tx(0xBE, 0x19);
	spi_9b_tx(0xBF, 0x0E);
	spi_9b_tx(0xC0, 0x16);
	spi_9b_tx(0xC1, 0x0A);
	spi_9b_tx(0xD0, 0x0C);
	spi_9b_tx(0xD1, 0x17);
	spi_9b_tx(0xD2, 0x14);
	spi_9b_tx(0xD3, 0x2E);
	spi_9b_tx(0xD4, 0x32);
	spi_9b_tx(0xD5, 0x3C);
	spi_9b_tx(0xD6, 0x22);
	spi_9b_tx(0xD7, 0x3D);
	spi_9b_tx(0xD8, 0x0D);
	spi_9b_tx(0xD9, 0x07);
	spi_9b_tx(0xDA, 0x13);
	spi_9b_tx(0xDB, 0x13);
	spi_9b_tx(0xDC, 0x11);
	spi_9b_tx(0xDD, 0x15);
	spi_9b_tx(0xDE, 0x19);
	spi_9b_tx(0xDF, 0x10);
	spi_9b_tx(0xE0, 0x17);
	spi_9b_tx(0xE1, 0x0A);

	spi_9b_tx(0xFF, 0x30);
	spi_9b_tx(0xFF, 0x52);
	spi_9b_tx(0xFF, 0x03); // page 3

	spi_9b_tx(0x00, 0x2A);
	spi_9b_tx(0x01, 0x2A);
	spi_9b_tx(0x02, 0x2A);
	spi_9b_tx(0x03, 0x2A);
	spi_9b_tx(0x04, 0x61);
	spi_9b_tx(0x05, 0x80);
	spi_9b_tx(0x06, 0xc7);
	spi_9b_tx(0x07, 0x01);
	spi_9b_tx(0x08, 0x03);
	spi_9b_tx(0x09, 0x04);
	spi_9b_tx(0x70, 0x22);
	spi_9b_tx(0x71, 0x80);
	spi_9b_tx(0x30, 0x2A);
	spi_9b_tx(0x31, 0x2A);
	spi_9b_tx(0x32, 0x2A);
	spi_9b_tx(0x33, 0x2A);
	spi_9b_tx(0x34, 0x61);
	spi_9b_tx(0x35, 0xc5);
	spi_9b_tx(0x36, 0x80);
	spi_9b_tx(0x37, 0x23);
	spi_9b_tx(0x40, 0x03);
	spi_9b_tx(0x41, 0x04);
	spi_9b_tx(0x42, 0x05);
	spi_9b_tx(0x43, 0x06);
	spi_9b_tx(0x44, 0x11);
	spi_9b_tx(0x45, 0xe8);
	spi_9b_tx(0x46, 0xe9);
	spi_9b_tx(0x47, 0x11);
	spi_9b_tx(0x48, 0xea);
	spi_9b_tx(0x49, 0xeb);
	spi_9b_tx(0x50, 0x07);
	spi_9b_tx(0x51, 0x08);
	spi_9b_tx(0x52, 0x09);
	spi_9b_tx(0x53, 0x0a);
	spi_9b_tx(0x54, 0x11);
	spi_9b_tx(0x55, 0xec);
	spi_9b_tx(0x56, 0xed);
	spi_9b_tx(0x57, 0x11);
	spi_9b_tx(0x58, 0xef);
	spi_9b_tx(0x59, 0xf0);
	spi_9b_tx(0xB1, 0x01);
	spi_9b_tx(0xB4, 0x15);
	spi_9b_tx(0xB5, 0x16);
	spi_9b_tx(0xB6, 0x09);
	spi_9b_tx(0xB7, 0x0f);
	spi_9b_tx(0xB8, 0x0d);
	spi_9b_tx(0xB9, 0x0b);
	spi_9b_tx(0xBA, 0x00);
	spi_9b_tx(0xC7, 0x02);
	spi_9b_tx(0xCA, 0x17);
	spi_9b_tx(0xCB, 0x18);
	spi_9b_tx(0xCC, 0x0a);
	spi_9b_tx(0xCD, 0x10);
	spi_9b_tx(0xCE, 0x0e);
	spi_9b_tx(0xCF, 0x0c);
	spi_9b_tx(0xD0, 0x00);
	spi_9b_tx(0x81, 0x00);
	spi_9b_tx(0x84, 0x15);
	spi_9b_tx(0x85, 0x16);
	spi_9b_tx(0x86, 0x10);
	spi_9b_tx(0x87, 0x0a);
	spi_9b_tx(0x88, 0x0c);
	spi_9b_tx(0x89, 0x0e);
	spi_9b_tx(0x8A, 0x02);
	spi_9b_tx(0x97, 0x00);
	spi_9b_tx(0x9A, 0x17);
	spi_9b_tx(0x9B, 0x18);
	spi_9b_tx(0x9C, 0x0f);
	spi_9b_tx(0x9D, 0x09);
	spi_9b_tx(0x9E, 0x0b);
	spi_9b_tx(0x9F, 0x0d);
	spi_9b_tx(0xA0, 0x01);

	spi_9b_tx(0xFF, 0x30);
	spi_9b_tx(0xFF, 0x52);
	spi_9b_tx(0xFF, 0x02); // page 2

	spi_9b_tx(0x01, 0x01);
	spi_9b_tx(0x02, 0xDA);
	spi_9b_tx(0x03, 0xBA);
	spi_9b_tx(0x04, 0xA8);
	spi_9b_tx(0x05, 0x9A);
	spi_9b_tx(0x06, 0x70);
	spi_9b_tx(0x07, 0xFF);
	spi_9b_tx(0x08, 0x91);
	spi_9b_tx(0x09, 0x90);
	spi_9b_tx(0x0A, 0xFF);
	spi_9b_tx(0x0B, 0x8F);
	spi_9b_tx(0x0C, 0x60);
	spi_9b_tx(0x0D, 0x58);
	spi_9b_tx(0x0E, 0x48);
	spi_9b_tx(0x0F, 0x38);
	spi_9b_tx(0x10, 0x2B);

	spi_9b_tx(0xFF, 0x30);
	spi_9b_tx(0xFF, 0x52);
	spi_9b_tx(0xFF, 0x00); // page 0

	spi_9b_tx(0x36, 0x02);

	spi_9b_tx(0x11, 0x00); // sleep out
	HAL_Delay(50);

	spi_9b_tx(0x29, 0x00); // display on
	HAL_Delay(10);

	for(uint32_t i = 0; i < GUI_LAYERS; i++)
	{
		display_layers[i].width = LCD_WIDTH;
		display_layers[i].height = LCD_HEIGHT;
		display_layers[i].start_address = (void *)(LCD_FRAME_BUFFER + (i * LCD_FRAME_BUFFER_SIZE));
	}
}