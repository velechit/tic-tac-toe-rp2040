#ifndef __POP2040_LCD_H
#define __POP2040_LCD_H

#include <stdint.h>
#include "timage.h"

#include "backlight.h"

typedef enum {
    LCD_ORIENTATION_0DEG    = 0,
    LCD_ORIENTATION_90DEG,
    LCD_ORIENTATION_180DEG,
    LCD_ORIENTATION_270DEG,
    LCD_ORIENTATION_0DEG_MIRROR,
    LCD_ORIENTATION_90DEG_MIRROR,
    LCD_ORIENTATION_180DEG_MIRROR,
    LCD_ORIENTATION_270DEG_MIRROR
} LCD_ORIENTATION;

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_TOTAL_PIXELS (SCREEN_WIDTH * SCREEN_HEIGHT)


void lcd_init(uint16_t width, uint16_t height, LCD_ORIENTATION orienration, uint16_t bgclr, uint16_t fgclr);

void lcd_erase(void);
void lcd_fill_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void lcd_blit(uint16_t x, uint16_t y, const bitmap_t *bitmap);
void lcd_blit_alpha(uint16_t x, uint16_t y, const uint8_t alpha, const bitmap_t *bitmap);
void lcd_blit_section(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, uint16_t w, uint16_t h, const bitmap_t *bitmap);
void lcd_putchar(uint16_t x, uint16_t y, const uint8_t ch, uint16_t fgclr, const font_t *fnt);
void lcd_putstr(uint16_t x, uint16_t y, const uint8_t *str, uint16_t fgclr, const font_t *fnt);

void lcd_set_trans_clr(uint16_t clr);
uint16_t lcd_get_trans_clr(void);

typedef void (*lcd_frame_advance_cb_t)(void);
void lcd_set_frame_request_cb(lcd_frame_advance_cb_t cb);

#define BLACK     0x0000
#define WHITE     0xFFFF
#define RED       0xF800
#define GREEN     0x07E0
#define BLUE      0x001F
#define LIGHTBLUE 0x03D9
#define GREY      0x52AA

#endif /* ! __POP2040_LCD_H */