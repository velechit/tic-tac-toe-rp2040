#include "pico/stdlib.h"
#include "config.h"
#include "system.h"

#include "lcd.h"
#include "gamepad.h"
#include "music.h"

void system_init(void) {
    stdio_init_all();
    lcd_init(SCREEN_WIDTH, SCREEN_HEIGHT,LCD_ORIENTATION_270DEG, BLACK, WHITE);
    gamepad_init();   
    music_init();
}
