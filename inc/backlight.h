#ifndef __3DUI_BACKLIGHT_H__
#define __3DUI_BACKLIGHT_H__

typedef enum {
   FAST = 0,
   SLOW,
   MODERATESLOW,
   VERYSLOW
} BACKLIGHT_FADE_SPEED;

void backlight_init(void);
void backlight_on(BACKLIGHT_FADE_SPEED);
void backlight_off(BACKLIGHT_FADE_SPEED);

#endif /* ! __3DUI_BACKLIGHT_H__ */