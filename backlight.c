#include "config.h"
#include "backlight.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/binary_info.h"

bi_decl(bi_1pin_with_name(LCD_BL_PIN, "LCD Backlight PWM"));

void backlight_init(void){
    gpio_set_function(LCD_BL_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LCD_BL_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.f);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(LCD_BL_PIN, 0);
}
void backlight_on(BACKLIGHT_FADE_SPEED speed){
 for(int i = 0; i < 256; i++) {
    pwm_set_gpio_level(LCD_BL_PIN, i*i);
    if(speed==FAST)
        sleep_us(100);
    else if(speed==SLOW)
        sleep_us(500);
    else if(speed==MODERATESLOW)
        sleep_ms(1);
    else if(speed==VERYSLOW)
        sleep_ms(10);
 }
}
void backlight_off(BACKLIGHT_FADE_SPEED speed){
 for(int i = 255; i >- 0; i--) {
    pwm_set_gpio_level(LCD_BL_PIN, i*i);
    if(speed==FAST)
        sleep_us(100);
    else if(speed==SLOW)
        sleep_us(500);
    else if(speed==MODERATESLOW)
        sleep_ms(1);
    else if(speed==VERYSLOW)
        sleep_ms(10);
 }
}