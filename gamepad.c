#include "pico/stdlib.h"
#include "config.h"
#include "gamepad.h"

static bool button_u_state;
static bool button_d_state;
static bool button_l_state;
static bool button_r_state;
static bool button_m_state;
static uint8_t all_btn_state;

#define BTN_STATE_MASK ((1 << GAMEPAD_U) | (1 << GAMEPAD_D) | (1 << GAMEPAD_L) | (1 << GAMEPAD_R) | (1 << GAMEPAD_M))

void gamepad_poll(void){
   // read the gamepad states
   uint32_t gpio_states = gpio_get_all() & BTN_STATE_MASK;
   button_u_state = (~gpio_states) & (1 << GAMEPAD_U);
   button_d_state = (~gpio_states) & (1 << GAMEPAD_D);
   button_l_state = (~gpio_states) & (1 << GAMEPAD_L);
   button_r_state = (~gpio_states) & (1 << GAMEPAD_R);
   button_m_state = (~gpio_states) & (1 << GAMEPAD_M);
   all_btn_state = button_l_state << 0 
                  | button_r_state << 1 
                  | button_u_state << 2 
                  | button_d_state << 3 
                  | button_m_state << 4 ;

}

uint8_t get_all_btn_state() {
  return all_btn_state;
}
bool gamepad_get_btn_m(){
  return button_m_state;
}
bool gamepad_get_btn_l(){
  return button_l_state;
}
bool gamepad_get_btn_r(){
  return button_r_state;
}
bool gamepad_get_btn_u(){
  return button_u_state;
}
bool gamepad_get_btn_d(){
  return button_d_state;
}

void gamepad_init(void){
gpio_init(GAMEPAD_U);
gpio_init(GAMEPAD_D);
gpio_init(GAMEPAD_L);
gpio_init(GAMEPAD_R);
gpio_init(GAMEPAD_M);
gpio_set_dir(GAMEPAD_U,GPIO_IN);
gpio_set_dir(GAMEPAD_D,GPIO_IN);
gpio_set_dir(GAMEPAD_L,GPIO_IN);
gpio_set_dir(GAMEPAD_R,GPIO_IN);
gpio_set_dir(GAMEPAD_M,GPIO_IN);
gpio_pull_up(GAMEPAD_D);
gpio_pull_up(GAMEPAD_L);
gpio_pull_up(GAMEPAD_U);
gpio_pull_up(GAMEPAD_R);
gpio_pull_up(GAMEPAD_M);
    

   /* gpio_init(LCD_CS_PIN);
    gpio_init(LCD_DC_PIN);
    gpio_init(LCD_RST_PIN);

    gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
    gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
    gpio_set_dir(LCD_RST_PIN, GPIO_OUT);*/
}