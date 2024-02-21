#include "display.h"
#include "platform.h"

#define BL_EN GPIOH->BSRRL = (1 << 14)
#define BL_DIS GPIOH->BSRRH = (1 << 14)

#define D_RST_H GPIOH->BSRRL = (1 << 15)
#define D_RST_L GPIOH->BSRRH = (1 << 15)

#define CS_H GPIOI->BSRRL = (1 << 0)
#define CS_L GPIOI->BSRRH = (1 << 0)

#define SCK_H GPIOI->BSRRL = (1 << 1)
#define SCK_L GPIOI->BSRRH = (1 << 1)

#define MOSI_H GPIOI->BSRRL = (1 << 3)
#define MOSI_L GPIOI->BSRRH = (1 << 3)

static void spi_dly(void)
{
	for(volatile uint32_t i = 0; i < 10; i++)
		asm("nop");
}

static void spi_9b_tx(uint8_t cmd, uint8_t val)
{
	CS_L;
	spi_dly();
	for(uint8_t i = 0; i < 9; i++)
	{
		if(i == 0)
		{
			MOSI_L;
		}
		else
		{
			cmd & 0x80 ? (MOSI_H) : (MOSI_L);
			cmd <<= 1;
		}
		spi_dly();
		SCK_H;
		spi_dly();
		SCK_L;
	}
	for(uint8_t i = 0; i < 9; i++)
	{
		if(i == 0)
		{
			MOSI_H;
		}
		else
		{
			val & 0x80 ? (MOSI_H) : (MOSI_L);
			val <<= 1;
		}
		spi_dly();
		SCK_H;
		spi_dly();
		SCK_L;
	}
	spi_dly();
	CS_H;
	spi_dly();
}

gui_layer_t display_layers[GUI_LAYERS];

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

static void dma2d_start(void)
{
	while((LTDC->CDSR & LTDC_CDSR_VSYNCS) == 0U)
		asm("nop");
	LTDC_Cmd(DISABLE);
	DMA2D->CR = DMA2D_R2M;
	DMA2D->CR |= 0 | 0;
	DMA2D->CR |= DMA2D_CR_START;
	while(DMA2D->CR & DMA2D_CR_START)
		;
	LTDC_Cmd(ENABLE);
}

void lcd_fill(void *dst, uint16_t xSize, uint16_t ySize, uint16_t OffLine, uint32_t color)
{
	if(!xSize || !ySize) return;

	while(DMA2D->CR & DMA2D_CR_START)
		;													/* Wait finished */
	DMA2D->OCOLR = color;									/* Color to be used */
	DMA2D->OMAR = (uint32_t)dst;							/* Destination address */
	DMA2D->OOR = OffLine;									/* Destination line offset */
	DMA2D->OPFCCR = DMA2D_ARGB8888;							/* Defines the number of pixels to be transfered */
	DMA2D->NLR = (uint32_t)(xSize << 16) | (uint16_t)ySize; /* Size configuration of area to be transfered */

	dma2d_start();
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

void display_init(void)
{
	for(uint32_t i = 0; i < GUI_LAYERS; i++)
	{
		display_layers[i].width = LCD_WIDTH;
		display_layers[i].height = LCD_HEIGHT;
		display_layers[i].start_address = (void *)(LCD_FRAME_BUFFER + (i * LCD_FRAME_BUFFER_SIZE));
	}

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_LTDC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2D, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15; // nBL_SHDN, D_RST
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOH, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_3; // D_CS, D_SCK, D_MOSI
	GPIO_Init(GPIOI, &GPIO_InitStructure);

	CS_H;
	SCK_L;
	MOSI_L;

	GPIO_PinAFConfig(GPIOG, GPIO_PinSource12, GPIO_AF9_LTDC);
	GPIO_PinAFConfig(GPIOI, GPIO_PinSource12, GPIO_AF_LTDC);
	GPIO_PinAFConfig(GPIOI, GPIO_PinSource13, GPIO_AF_LTDC);
	GPIO_PinAFConfig(GPIOI, GPIO_PinSource14, GPIO_AF_LTDC);
	GPIO_PinAFConfig(GPIOI, GPIO_PinSource15, GPIO_AF_LTDC);
	for(uint32_t i = 0; i < 16; i++)
	{
		GPIO_PinAFConfig(GPIOJ, i, GPIO_AF_LTDC);
	}
	GPIO_PinAFConfig(GPIOK, GPIO_PinSource0, GPIO_AF_LTDC);
	GPIO_PinAFConfig(GPIOK, GPIO_PinSource1, GPIO_AF_LTDC);
	GPIO_PinAFConfig(GPIOK, GPIO_PinSource2, GPIO_AF_LTDC);
	GPIO_PinAFConfig(GPIOK, GPIO_PinSource4, GPIO_AF_LTDC);
	GPIO_PinAFConfig(GPIOK, GPIO_PinSource5, GPIO_AF_LTDC);
	GPIO_PinAFConfig(GPIOK, GPIO_PinSource6, GPIO_AF_LTDC);
	GPIO_PinAFConfig(GPIOK, GPIO_PinSource7, GPIO_AF_LTDC);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOI, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = 0xFFFF;
	GPIO_Init(GPIOJ, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOK, &GPIO_InitStructure);

	DMA2D_DeInit();
	LTDC_DeInit();

	DMA2D_InitTypeDef DMA2D_InitStruct;
	DMA2D_InitStruct.DMA2D_Mode = DMA2D_R2M;
	DMA2D_InitStruct.DMA2D_CMode = DMA2D_ARGB8888;
	DMA2D_InitStruct.DMA2D_OutputAlpha = 0x0F;
	DMA2D_InitStruct.DMA2D_OutputMemoryAdd = LCD_FRAME_BUFFER;
	DMA2D_InitStruct.DMA2D_OutputOffset = 0;
	DMA2D_InitStruct.DMA2D_NumberOfLine = 1;
	DMA2D_InitStruct.DMA2D_PixelPerLine = 1;
	DMA2D_Init(&DMA2D_InitStruct);

	LTDC_InitTypeDef LTDC_InitStruct;
	LTDC_InitStruct.LTDC_HSPolarity = LTDC_HSPolarity_AL;
	LTDC_InitStruct.LTDC_VSPolarity = LTDC_VSPolarity_AL;
	LTDC_InitStruct.LTDC_DEPolarity = LTDC_DEPolarity_AL;
	LTDC_InitStruct.LTDC_PCPolarity = LTDC_PCPolarity_IPC;

	LTDC_InitStruct.LTDC_BackgroundRedValue = 0;
	LTDC_InitStruct.LTDC_BackgroundGreenValue = 0;
	LTDC_InitStruct.LTDC_BackgroundBlueValue = 0;

#define HSW 1
#define VSW 1
#define HBP 40
#define VBP 6
#define LCD_W 640
#define LCD_H 480
#define HFP 39
#define VFP 13

	LTDC_InitStruct.LTDC_HorizontalSync = HSW;
	LTDC_InitStruct.LTDC_VerticalSync = VSW;
	LTDC_InitStruct.LTDC_AccumulatedHBP = HBP;
	LTDC_InitStruct.LTDC_AccumulatedVBP = VBP;
	LTDC_InitStruct.LTDC_AccumulatedActiveW = LCD_W + HBP;
	LTDC_InitStruct.LTDC_AccumulatedActiveH = LCD_H + VBP;
	LTDC_InitStruct.LTDC_TotalWidth = LCD_W + HBP + HFP;
	LTDC_InitStruct.LTDC_TotalHeigh = LCD_H + VBP + VFP;

	LTDC_Init(&LTDC_InitStruct);

	LTDC_Layer_InitTypeDef LTDC_Layer_InitStruct;

	LTDC_Layer_InitStruct.LTDC_HorizontalStart = HBP + 1;
	LTDC_Layer_InitStruct.LTDC_HorizontalStop = (LCD_W + HBP);
	LTDC_Layer_InitStruct.LTDC_VerticalStart = VBP + 1;
	LTDC_Layer_InitStruct.LTDC_VerticalStop = (LCD_H + VBP);

	LTDC_Layer_InitStruct.LTDC_PixelFormat = LTDC_Pixelformat_ARGB8888;
	LTDC_Layer_InitStruct.LTDC_ConstantAlpha = 255;
	LTDC_Layer_InitStruct.LTDC_DefaultColorBlue = 0;
	LTDC_Layer_InitStruct.LTDC_DefaultColorGreen = 0;
	LTDC_Layer_InitStruct.LTDC_DefaultColorRed = 0;
	LTDC_Layer_InitStruct.LTDC_DefaultColorAlpha = 0;

	LTDC_Layer_InitStruct.LTDC_BlendingFactor_1 = LTDC_BlendingFactor1_CA;
	LTDC_Layer_InitStruct.LTDC_BlendingFactor_2 = LTDC_BlendingFactor2_CA;
	LTDC_Layer_InitStruct.LTDC_CFBLineLength = LCD_W * 4 + 3;
	LTDC_Layer_InitStruct.LTDC_CFBPitch = LCD_W * 4;
	LTDC_Layer_InitStruct.LTDC_CFBLineNumber = LCD_H;
	LTDC_Layer_InitStruct.LTDC_CFBStartAdress = (uint32_t)display_layers[0].start_address;
	LTDC_LayerInit(LTDC_Layer1, &LTDC_Layer_InitStruct);

	LTDC_LayerCmd(LTDC_Layer1, ENABLE);

#if GUI_LAYERS > 1
	LTDC_Layer_InitStruct.LTDC_BlendingFactor_1 = LTDC_BlendingFactor1_PAxCA;
	LTDC_Layer_InitStruct.LTDC_BlendingFactor_2 = LTDC_BlendingFactor2_PAxCA;
	LTDC_Layer_InitStruct.LTDC_CFBStartAdress = (uint32_t)display_layers[1].start_address;
	LTDC_LayerInit(LTDC_Layer2, &LTDC_Layer_InitStruct);
	LTDC_ReloadConfig(LTDC_IMReload);

	LTDC_LayerCmd(LTDC_Layer2, DISABLE);
#endif
	LTDC_ReloadConfig(LTDC_IMReload);

	LTDC_DitherCmd(ENABLE);

	D_RST_L;
	delay_ms(1);
	D_RST_H;
	delay_ms(120);

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
	delay_ms(50);

	spi_9b_tx(0x29, 0x00); // display on
	delay_ms(10);

	LTDC_Cmd(ENABLE);

	BL_EN;
}

void display_deinit(void)
{
	BL_DIS;
}

uint8_t lcd_is_ready(void) { return !(DMA2D->CR & DMA2D_CR_START); }
