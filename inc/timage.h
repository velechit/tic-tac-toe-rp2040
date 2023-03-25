#ifndef __T_IMAGE_H__
#define __T_IMAGE_H__

#include <stdint.h>

 
/* This is a generic bitmap */
typedef struct {
    const uint16_t size_x; //size is in pixels
    const uint16_t size_y;
    const uint16_t * const payload;
} bitmap_t;

typedef struct {
    const uint16_t size_x; //size is in pixels
    const uint16_t size_y;
    const uint8_t * const payload;
} bitmap_font_t;

/* This is a font glyph description - for now it does not support kerning */
typedef struct {
    const char character; //ASCII code
    const uint8_t width;
    const uint16_t x_offset;
} glyph_t;
 
typedef struct {
    const bitmap_font_t * const bitmap;
    const uint8_t glyph_count;
    const glyph_t *glyphs; //pointer to array of glypt_t elements
} font_t;

#endif /* __T_IMAGE_H__ */
