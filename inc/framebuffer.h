#ifndef __POP2040_FRAMEBUFFER_H
#define __POP2040_FRAMEBUFFER_H


void fb_init(spi_inst_t *spi,uint16_t _bgclr);

#define FRAMEBUFFER_TYPE uint16_t

#define FRAMEBUFFER_LEN (SCREEN_WIDTH*SCREEN_HEIGHT)

extern FRAMEBUFFER_TYPE framebuffer[FRAMEBUFFER_LEN];

typedef void (*frame_complete_cb_t)(void);


void fb_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,uint16_t clr);
void fb_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t transp_clr, const uint16_t *imageData);
void fb_blit_alpha(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t transp_clr, const uint8_t alpha, const uint16_t *imageData);

void fb_blit_section_blend_8(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t xoffset, uint16_t fgclr, const uint8_t *imageData, const uint16_t stride_width);
void fb_blit_section(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, uint16_t w, uint16_t h, uint16_t transp_clr, const uint16_t *imageData, const uint16_t stride_width);

void fb_set_frame_cb(frame_complete_cb_t fccb);
void fb_erase(uint16_t _bgclr);

#endif /* ! __POP2040_FRAMEBUFFER_H */