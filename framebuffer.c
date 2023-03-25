#include "pico.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include <string.h>

#include "lcd.h" // For screen width & height
#include "framebuffer.h"

FRAMEBUFFER_TYPE framebuffer[FRAMEBUFFER_LEN];


#ifdef FB_DEBUG
#define DEBUG_PIN 0
#endif 

void memset16(void *addr, uint16_t value16, int n)
{
    // short val = (value16 >> 8) | (value16 << 8);
    uint16_t *addr1=(uint16_t *)addr;
    while (n--) *(addr1++) = value16;
}

static int fb_2_spi;


static frame_complete_cb_t on_frame_complete;

void fb_set_frame_cb(frame_complete_cb_t fccb) {
   on_frame_complete = fccb;
}

void fb_feeder_cb(void) {
    dma_hw->ints0 = 1u << fb_2_spi;
    if(on_frame_complete!=NULL) on_frame_complete();
    // toggle a gpio 
#ifdef FB_DEBUG
    gpio_xor_mask(1<<DEBUG_PIN);
#endif
  dma_channel_set_read_addr(fb_2_spi,&framebuffer[0],true);
}


void fb_init(spi_inst_t *spi, uint16_t _bgclr){

#ifdef FB_DEBUG
   gpio_init(DEBUG_PIN);
   gpio_set_dir(DEBUG_PIN,GPIO_OUT);
#endif
   spi_set_format(spi,16,0,0,0);

   memset16(framebuffer,_bgclr,FRAMEBUFFER_LEN);

   fb_2_spi = dma_claim_unused_channel(true);
   dma_channel_config c = dma_channel_get_default_config(fb_2_spi);
   channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
   channel_config_set_read_increment(&c, true);
   channel_config_set_write_increment(&c, false);  
   
   // dreq should be on timer not on SPI

   //dma_timer_set_fraction(0, 0x0A7E, 0xffff) ;
   //channel_config_set_dreq(&c, 0x3b);                                 // DREQ paced by timer 0
   
   channel_config_set_dreq(&c,spi_get_dreq(spi,true));
   

   dma_channel_configure(
        fb_2_spi,
        &c,
        &(spi_get_hw(spi)->dr),
        &framebuffer[0],
        FRAMEBUFFER_LEN,
        false);

    dma_channel_set_irq0_enabled(fb_2_spi, true);
    irq_set_exclusive_handler(DMA_IRQ_0, fb_feeder_cb);
    irq_set_enabled(DMA_IRQ_0, true);

    fb_feeder_cb();
    
}

void fb_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,uint16_t clr){
    // wait for DMA busy
    
    for(uint16_t i = y; i < (y+h) ;i++)
      for(uint16_t j = x; j < (x+w) ;j++)
          framebuffer[(i*SCREEN_WIDTH)+j] = clr; // (clr >> 8) | (clr << 8);

}
void fb_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t transp_clr, const uint16_t *imageData){

    for(uint16_t i = y; i < (y+h) ;i++)
      for(uint16_t j = x; j < (x+w) ;j++)
         if(imageData[ ((i-y)*w)+(j-x) ]!=transp_clr)
          framebuffer[(i*SCREEN_WIDTH)+j] = imageData[ ((i-y)*w)+(j-x) ];

}
const uint16_t _blend(const uint16_t pix1,const uint16_t  pix2,const uint8_t alpha) {
    uint8_t bgr=(pix1>>11)&0x1F, fgr=(pix2>>11)&0x1F;
    uint8_t bgg=(pix1>>5)&0x3F , fgg=(pix2>>5)&0x3F;
    uint8_t bgb=(pix1)&0x1F    , fgb=(pix2)&0x1F;
    uint8_t r = (((fgr-bgr)*(alpha/255.0))+bgr);
    uint8_t g = (((fgg-bgg)*(alpha/255.0))+bgg);
    uint8_t b = (((fgb-bgb)*(alpha/255.0))+bgb);
    return (((r) << 11) | ((g) << 5) | (b ));
}

void fb_blit_alpha(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t transp_clr,const uint8_t alpha, const uint16_t *imageData){
    for(uint16_t i = y; i < (y+h) ;i++)
      for(uint16_t j = x; j < (x+w) ;j++)
         if(imageData[ ((i-y)*w)+(j-x) ]!=transp_clr){
          uint16_t pix1 = framebuffer[(i*SCREEN_WIDTH)+j];
          uint16_t pix2 = imageData[ ((i-y)*w)+(j-x) ];
          framebuffer[(i*SCREEN_WIDTH)+j]=_blend(pix1,pix2,alpha);
         }

}
void fb_blit_section_blend_8(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t xoffset, uint16_t fgclr, const uint8_t *imageData, const uint16_t stride_width){

    const uint8_t *stride_ptr = imageData + xoffset;

    for(int j =0; j<h; j++){
      const uint8_t *ss=stride_ptr+(j*stride_width);
      for(int i =0; i<w; i++){
          uint8_t pix_data=*(ss++);
          framebuffer[((j+y)*SCREEN_WIDTH)+(i+x)] = _blend(framebuffer[((j+y)*SCREEN_WIDTH)+(i+x)],fgclr,pix_data);
      }
    }


}

void fb_blit_section(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, uint16_t w, uint16_t h, uint16_t transp_clr, const uint16_t *imageData, const uint16_t stride_width){

    const uint16_t *stride_ptr = imageData + (sx + (sy*stride_width));

    for(uint16_t i = y; i < (y+h) ;i++){
      const uint16_t *ss=stride_ptr+((i-y)*stride_width);
      for(uint16_t j = x; j < (x+w) ;j++){
          uint16_t pix=*(ss++);
          if(pix!=transp_clr)
             framebuffer[(i*SCREEN_WIDTH)+j]=pix;
         
      }
    }


}


void fb_erase(uint16_t _bgclr){
       memset16(framebuffer,_bgclr,FRAMEBUFFER_LEN);
}