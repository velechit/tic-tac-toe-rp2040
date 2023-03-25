#ifndef __POP2040_GAME_PAD_H
#define __POP2040_GAME_PAD_H

void gamepad_init(void);
void gamepad_poll(void);
bool gamepad_get_btn_m(void);
bool gamepad_get_btn_l(void);
bool gamepad_get_btn_r(void);
bool gamepad_get_btn_u(void);
bool gamepad_get_btn_d(void);
uint8_t get_all_btn_state(void);

#define BTN_STATE_POS_L    (1<<0)
#define BTN_STATE_POS_R    (1<<1)
#define BTN_STATE_POS_U    (1<<2)
#define BTN_STATE_POS_D    (1<<3)
#define BTN_STATE_POS_M    (1<<4)

#endif /* ! __POP2040_GAME_PAD_H */