#ifndef DISPLAY_H__
#define DISPLAY_H__

#include <stdint.h>

#define LCD_WIDTH 640
#define LCD_HEIGHT 480

#define LCD_FRAME_BUFFER ((uint32_t)0xC0000000)
#define LCD_FRAME_BUFFER_SIZE ((uint32_t)(LCD_WIDTH * LCD_HEIGHT * 4))

#define GUI_LAYERS 2

typedef struct
{
	void *start_address;
	uint32_t width;
	uint32_t height;
} gui_layer_t;

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_t;

color_t hsv2rgb(float h, float s, float v);

void display_init(void);
void display_deinit(void);

void lcd_fill(void *dst, uint16_t xSize, uint16_t ySize, uint16_t OffLine, uint32_t color);
void lcd_fill_full(gui_layer_t *layer, uint32_t color);
void lcd_line_h(gui_layer_t *layer, uint16_t x, uint16_t y, uint16_t length, uint32_t color);
void lcd_line_v(gui_layer_t *layer, uint16_t x, uint16_t y, uint16_t length, uint32_t color);
void lcd_rect_fill(gui_layer_t *layer, uint16_t x, uint16_t y, uint16_t xSize, uint16_t ySize, uint32_t color);
void lcd_pixel(gui_layer_t *layer, uint16_t x, uint16_t y, uint32_t color);
uint8_t lcd_is_ready(void);

extern gui_layer_t display_layers[GUI_LAYERS];

#endif // DISPLAY_H__