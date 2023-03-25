#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pwm.h"
#include "config.h"
#include "music.h"

bi_decl(bi_1pin_with_name(SPEAKER_PIN, "Speaker Output"));


typedef struct{
  uint slice_num;
  const note_t *pt;
  uint delayOFF;
  uint wholenote;
  uint tempo;
  volatile uint Done;
} note_timer_struct;


note_timer_struct noteTimer;


static inline void pwm_calcDivTop(pwm_config *c,int frequency,int sysClock)
{
  uint  count = sysClock * 16 / frequency;
  uint  div =  count / 60000;  // to be lower than 65535*15/16 (rounding error)
  if(div < 16)
      div=16;
  c->div = div;
  c->top = count / div;
}




uint playTone(note_timer_struct *ntTimer)
{
  int duration;
  pwm_config cfg = pwm_get_default_config();
  
  if(ntTimer->Done) return 0;

  const note_t * note = ntTimer->pt;
  duration = note->duration;
  if(duration == 0) return 0;
  if(duration>0)
      duration = ntTimer->wholenote / duration;
  else
      duration = ( 3 * ntTimer->wholenote / (-duration))/2;

  if(note->frequency == 0) {
      // pwm_set_chan_level(ntTimer->slice_num,PWM_CHAN_A,0);
      pwm_set_gpio_level(SPEAKER_PIN, 0);
  } else {
   pwm_calcDivTop(&cfg,note->frequency,125000000);
   pwm_init(ntTimer->slice_num,&cfg,true);
   pwm_set_chan_level(ntTimer->slice_num,PWM_CHAN_A,cfg.top);
   pwm_set_gpio_level(SPEAKER_PIN, cfg.top / 2);

  }
  ntTimer->delayOFF = duration;
  return duration;
}



int64_t timer_note_callback(alarm_id_t id, void *user_data)
{
  note_timer_struct *ntTimer = (note_timer_struct *) user_data;
  const note_t * note = ntTimer->pt;
  if(note->duration == 0)
     {
      ntTimer->Done=true;
      return 0;  // done!
     }
  // are we in pause time between  note
  if(ntTimer->delayOFF==0)
    {
       uint delayON = playTone(ntTimer);
       if(delayON == 0)
        {
           ntTimer->Done=true;
           return 0;
        }
       ntTimer->delayOFF = delayON;
       return 900*delayON;
    }
    else
    {
       // pwm_set_chan_level(ntTimer->slice_num,PWM_CHAN_A,0);
       pwm_set_gpio_level(SPEAKER_PIN,0);
       uint delayOFF = ntTimer->delayOFF;
       ntTimer->delayOFF=0;
       //  next note
       (ntTimer->pt)++;
       return 100*delayOFF;
    }
}




void play_music(const sound_data_t * melody)
{

      noteTimer.Done = false;
      noteTimer.pt = melody->notes;
      // set tempo
      noteTimer.tempo= melody->tempo;
      noteTimer.wholenote = (60000 * 4) / melody->tempo;
      noteTimer.delayOFF = 0;
      // start timer
      add_alarm_in_us(1000,timer_note_callback,&noteTimer,false);
}




bool is_music_playing(void){
  return !noteTimer.Done;
}

void music_init(void){
    gpio_set_function(SPEAKER_PIN, GPIO_FUNC_PWM);
    uint  slice_num = pwm_gpio_to_slice_num(SPEAKER_PIN);
    noteTimer.slice_num= slice_num;
}

void stop_sound(void){
    if(noteTimer.Done) return;
    noteTimer.Done = true;
    // TODO: some way to stop
}