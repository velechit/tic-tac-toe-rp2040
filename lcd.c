#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "config.h"
#include "lcd.h"

#include "framebuffer.h"
#include "timage.h"

#include <string.h>

// Binary information on the pins
bi_decl(bi_1pin_with_name(LCD_CS_PIN, "LCD Chip Select"));
bi_decl(bi_1pin_with_name(LCD_DC_PIN, "LCD Data/Command Select"));
bi_decl(bi_1pin_with_name(LCD_RST_PIN, "LCD Reset"));
bi_decl(bi_3pins_with_func(LCD_MOSI_PIN,LCD_MISO_PIN, LCD_CLK_PIN, GPIO_FUNC_SPI));


void lcd_set_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

#define RDDSDR               0x0f // Read Display Self-Diagnostic Result
#define SLPOUT               0x11 // Sleep Out
#define GAMSET               0x26 // Gamma Set
#define DISPOFF              0x28 // Display Off
#define DISPON               0x29 // Display On
#define CASET                0x2a // Column Address Set
#define PASET                0x2b // Page Address Set
#define RAMWR                0x2c // Memory Write
#define RAMRD                0x2e // Memory Read
#define MADCTL               0x36 // Memory Access Control
#define VSCRSADD             0x37 // Vertical Scrolling Start Address
#define PIXSET               0x3a // Pixel Format Set
#define FRMCTR1              0xb1 // Frame Rate Control 1
#define DISCTRL              0xb6 // Display Function Control
#define PWCTRL1              0xc0 // Power Control 1
#define PWCTRL2              0xc1 // Power Control 2
#define VMCTRL1              0xc5 // VCOM Control 1
#define VMCTRL2              0xc7 // VCOM Control 2
#define PWCTRLA              0xcb // Power Control A
#define PWCRTLB              0xcf // Power Control B
#define PGAMCTRL             0xe0 // Positive Gamma Control
#define NGAMCTRL             0xe1 // Negative Gamma Control
#define DTCTRLA              0xe8 // Driver Timing Control A
#define DTCTRLB              0xea // Driver Timing Control B
#define PWRONCTRL            0xed // Power on Sequence Control
#define ENA3G                0xf2 // Enable 3G
#define PRCTRL               0xf7 // Pump Ratio Control

#define LCD_CS_SELECT()   gpio_put(LCD_CS_PIN, 0)
#define LCD_CS_DESELECT() gpio_put(LCD_CS_PIN, 1)
#define LCD_DC_SELECT()   gpio_put(LCD_DC_PIN, 1)
#define LCD_DC_DESELECT() gpio_put(LCD_DC_PIN, 0)

void lcd_write_command(uint8_t command);
void lcd_write_data(uint8_t data);
void lcd_write_data_bulk(const uint8_t *data, size_t len);
void lcd_write_longdata(uint16_t data);

static uint16_t _width, _height;
static LCD_ORIENTATION _orientation;
static uint16_t _bgclr, _fgclr, _transclr;
static lcd_frame_advance_cb_t frame_adv_cb;

void fb_frame_complete_callback(void) {
    // called when one frame is sent and ready for next
    // prepare the frame
    if(frame_adv_cb!=NULL) frame_adv_cb();
}

void lcd_set_frame_request_cb(lcd_frame_advance_cb_t cb){
   frame_adv_cb = cb;
}



void lcd_init(uint16_t width, uint16_t height, LCD_ORIENTATION orienration, uint16_t bgclr, uint16_t fgclr){
    backlight_init();


    _bgclr = bgclr;
    _fgclr = fgclr;
    _transclr = _bgclr;

    spi_init(LCD_PORT, 80000000);

    gpio_set_function(LCD_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);

    gpio_init(LCD_CS_PIN);
    gpio_init(LCD_DC_PIN);
    gpio_init(LCD_RST_PIN);

    gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
    gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
    gpio_set_dir(LCD_RST_PIN, GPIO_OUT);

    gpio_put(LCD_CS_PIN, 1);
    gpio_put(LCD_DC_PIN, 0);

    LCD_CS_SELECT();

    // do a reset
    gpio_put(LCD_RST_PIN, 0);
    sleep_ms(50);
    gpio_put(LCD_RST_PIN, 1);
    sleep_ms(50);

    // initialize the lcd
    lcd_write_command(RDDSDR   ); lcd_write_data_bulk("\x03\x80\x02",3);
    lcd_write_command(PWCRTLB  ); lcd_write_data_bulk("\x00\xc1\x30",3);
    lcd_write_command(PWRONCTRL); lcd_write_data_bulk("\x64\x03\x12\x81",4);
    lcd_write_command(DTCTRLA  ); lcd_write_data_bulk("\x85\x00\x78",3);
    lcd_write_command(PWCTRLA  ); lcd_write_data_bulk("\x39\x2c\x00\x34\x02",5);
    lcd_write_command(PRCTRL   ); lcd_write_data_bulk("\x20",1);
    lcd_write_command(DTCTRLB  ); lcd_write_data_bulk("\x00\x00",2);
    lcd_write_command(PWCTRL1  ); lcd_write_data_bulk("\x23",1);
    lcd_write_command(PWCTRL2  ); lcd_write_data_bulk("\x10",1);
    lcd_write_command(VMCTRL1  ); lcd_write_data_bulk("\x3e\x28",1);
    lcd_write_command(VMCTRL2  ); lcd_write_data_bulk("\x86",1);

    uint8_t _or_code = 0x08;
    _orientation = orienration;
    _width = width;  _height = height;

    switch(orienration) {     
         case LCD_ORIENTATION_0DEG:          _or_code = 0x48; _width = width;  _height = height; break;
         case LCD_ORIENTATION_90DEG:         _or_code = 0x28; _width = height; _height = width;  break;
         case LCD_ORIENTATION_180DEG:        _or_code = 0x88; _width = width;  _height = height; break;
         case LCD_ORIENTATION_270DEG:        _or_code = 0xE8; _width = height; _height = width;  break;
         case LCD_ORIENTATION_0DEG_MIRROR:   _or_code = 0xC8; _width = width;  _height = height; break;
         case LCD_ORIENTATION_90DEG_MIRROR:  _or_code = 0x68; _width = height; _height = width;  break;
         case LCD_ORIENTATION_180DEG_MIRROR: _or_code = 0x08; _width = width;  _height = height; break;
         case LCD_ORIENTATION_270DEG_MIRROR: _or_code = 0xA8; _width = height; _height = width;  break;
         default:                            _orientation=LCD_ORIENTATION_0DEG; 
                                             _or_code = 0x48; _width = width; _height = height;  break;
    }

    lcd_write_command (MADCTL); lcd_write_data(_or_code);

    lcd_write_command (PIXSET  ); lcd_write_data_bulk("\x55",1);
    lcd_write_command (FRMCTR1 ); lcd_write_data_bulk("\x00\x18",2);
    lcd_write_command (DISCTRL ); lcd_write_data_bulk("\x08\x82\x27",3);
    lcd_write_command (ENA3G   ); lcd_write_data_bulk("\x00",1);
    lcd_write_command (GAMSET  ); lcd_write_data_bulk("\x01",1);
    lcd_write_command (PGAMCTRL); lcd_write_data_bulk("\x0f\x31\x2b\x0c\x0e\x08\x4e\xf1\x37\x07\x10\x03\x0e\x09\x00",15);
    lcd_write_command (NGAMCTRL); lcd_write_data_bulk("\x00\x0e\x14\x03\x11\x07\x31\xc1\x48\x08\x0f\x0c\x31\x36\x0f",15);
        
    lcd_write_command(SLPOUT);
    sleep_ms(120);  
    lcd_write_command(DISPON); 


    

    lcd_set_rectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
    lcd_write_command(RAMWR);

    LCD_CS_SELECT();
    LCD_DC_SELECT();

    fb_set_frame_cb(fb_frame_complete_callback);
    fb_init(LCD_PORT,_bgclr);

}


void lcd_set_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcd_write_command(CASET);
    lcd_write_longdata(x0);
    lcd_write_longdata(x1);

    lcd_write_command(PASET);
    lcd_write_longdata(y0);
    lcd_write_longdata(y1);

    // sleep_ms(10);
}

void lcd_write_command(uint8_t command) {
    LCD_CS_SELECT();
    LCD_DC_DESELECT();
    spi_write_blocking(LCD_PORT,&command,1);
    LCD_CS_DESELECT();
}

void lcd_write_data(uint8_t data) {
    LCD_CS_SELECT();
    LCD_DC_SELECT();
    spi_write_blocking(LCD_PORT,&data,1);
    LCD_CS_DESELECT();
}
void lcd_write_data_bulk(const uint8_t *data, size_t len) {
    LCD_CS_SELECT();
    LCD_DC_SELECT();
    spi_write_blocking(LCD_PORT,data,len);
    LCD_CS_DESELECT();
}

void lcd_write_longdata(uint16_t data) {
    LCD_CS_SELECT();
    LCD_DC_SELECT();

    uint8_t shortBuffer[2];
    shortBuffer[0] = (uint8_t) (data >> 8);
    shortBuffer[1] = (uint8_t) data;
    spi_write_blocking(LCD_PORT, shortBuffer, 2);

    LCD_CS_DESELECT();
}
void lcd_set_trans_clr(uint16_t clr){
    _transclr = clr;
}
uint16_t lcd_get_trans_clr(void){
    return _transclr;
}
void lcd_erase(void) {
    fb_erase(_bgclr);
}
void lcd_fill_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color){
    fb_fill_rect(x,y,w,h,color);
}
void lcd_blit(uint16_t x, uint16_t y,  const bitmap_t *bitmap){
   fb_blit(x,y,bitmap->size_x,bitmap->size_y,_transclr, bitmap->payload);
}
void lcd_blit_alpha(uint16_t x, uint16_t y, const uint8_t alpha, const bitmap_t *bitmap){
   fb_blit_alpha(x,y,bitmap->size_x,bitmap->size_y,_transclr,alpha, bitmap->payload);
}
void lcd_blit_section(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh, const bitmap_t *bitmap){
   fb_blit_section(x, y, sx, sy, sw, sh, _transclr, bitmap->payload, bitmap->size_x); 
}


const glyph_t * _get_glyph(const font_t *font, char c){
    for (uint32_t i = 0; i < font->glyph_count; i++){
            if (font->glyphs[i].character == c){
                return &(font->glyphs[i]);
            }
    }
    return NULL;
}


void _fnt_glyph_draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fgclr, const glyph_t *glyph,const bitmap_font_t *const bmp){
    fb_blit_section_blend_8(x,y,glyph->width,h,glyph->x_offset,fgclr,bmp->payload, bmp->size_x);
}

uint16_t _draw_char(uint16_t x, uint16_t y, uint8_t ch, uint16_t fgclr, const font_t *fnt){
    const bitmap_font_t *const bmp = fnt->bitmap;
    const glyph_t *gl = _get_glyph(fnt,ch);
    if(gl!=NULL){
       fb_blit_section_blend_8(x,y,gl->width,bmp->size_y,gl->x_offset,fgclr,bmp->payload, bmp->size_x);
       return x+gl->width;
    }
    return x;
}
void lcd_putchar(uint16_t x, uint16_t y, const uint8_t ch, uint16_t fgclr, const font_t *fnt){
   _draw_char(x,y,ch,fgclr,fnt);
}
void lcd_putstr(uint16_t x, uint16_t y, const uint8_t *str, uint16_t fgclr, const font_t *fnt){
   uint16_t pos_x = x;
   while(*str){
     pos_x = _draw_char(pos_x,y,*str,fgclr,fnt); pos_x+=2;
     str++;
   }
}

/*
void lcd_blit_fg_bg_blend(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fgclr, uint16_t bgclr, const uint8_t *imageData, uint16_t W){
    uint8_t bgr=(bgclr>>11)&0x1F, fgr=(fgclr>>11)&0x1F;
    uint8_t bgg=(bgclr>>5)&0x3F , fgg=(fgclr>>5)&0x3F;
    uint8_t bgb=(bgclr)&0x1F    , fgb=(fgclr)&0x1F;

    lcd_fix_coords(&x,&y,&w,&h);
    lcd_set_rectangle(x,y,x+w-1,y+h-1);
    lcd_write_command(RAMWR);

    LCD_CS_SELECT();
    LCD_DC_SELECT();
    // TODO: we need to write this to the FAME_BUFFER
    // and transmit only the updated RAM
    uint8_t pix_data;
    
    for(int j =0; j<h; j++){
      const uint8_t *ss=imageData+(j*W);
      for(int i =0; i<w; i++){
       pix_data=*(ss++);

	   uint8_t r = (((fgr-bgr)*(pix_data/255.0))+bgr);
	   uint8_t g = (((fgg-bgg)*(pix_data/255.0))+bgg);
	   uint8_t b = (((fgb-bgb)*(pix_data/255.0))+bgb);
	   uint16_t pix = (((r) << 11) | ((g) << 5) | (b ));

       lcd_write_longdata(pix);
      }
    }

    LCD_CS_DESELECT();
}
*/