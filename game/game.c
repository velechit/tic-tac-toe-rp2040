#include "game.h"
#include "game_types.h"
#include "lcd.h"
#include "gamepad.h"
#include "music.h"
#include <string.h>

void game_frame_render(void);

void frame_advance(){
   game_frame_render();
}

/* SECTION ALPHA CONTROL */
#define ALPHA_SPEED 30

static repeating_timer_t timer;
static repeating_timer_t res_timer;

static uint8_t alpha_val;
static bool show_result;
static bool pc_has_ai;
static int line_length;

// can use add_alarm_in_ms instead of repeating timer?
bool result_timer( repeating_timer_t *rt){
    static int counter = 0;
    
    // called every 100ms
    if(counter <= 20)
       line_length++;

    if(counter++ > 30) {
        show_result = false;
        cancel_repeating_timer(rt);
        counter=0;
        line_length = 0;
    }
    return true;
}




bool alpha_timer( repeating_timer_t *rt )
{   
    static int direction = -1;
    alpha_val += (direction*ALPHA_SPEED);
    if(alpha_val <= ALPHA_SPEED) { alpha_val = ALPHA_SPEED; direction = 1; }
    if(alpha_val >= 250-ALPHA_SPEED) { alpha_val = 250-ALPHA_SPEED; direction = -1; }
    return (true);
}
void init_alpha_blink(void) {
   alpha_val = 250-ALPHA_SPEED;
   add_repeating_timer_ms( 100, &alpha_timer, NULL, &timer );
}
/* END SECTION ALPHA CONTROL */

/* SECTION GAME VARS */

typedef struct {
int8_t switch_order[6];
int8_t blockers;
} _direct_ctrl;

typedef _direct_ctrl control_struct[4];

typedef enum {
  LEFT = 0,
  RIGHT,
  UP,
  DOWN
} MOVE_DIR;

char pc_player='x';
char curr_player  ;
char curr_slot = 4;
bool pc_vs_player = true;
static int win;

char square[9] = { '0', '1', '2',
                   '3', '4', '5',
                   '6', '7', '8' };

const control_struct ctrl[9] = {
{{{-1,-1,-1,-1,-1,-1},10},{{1,2,4,5,7,8},-1},{{-1,-1,-1,-1,-1,-1},10},{{3,6,4,7,5,8},-1}},
{{{0,-1,3,6,-1,-1},-1},{{2,-1,5,8,-1,-1},-1},{{-1,-1,-1,-1,-1,-1},10},{{4,7,3,5,6,8},-1}},
{{{1,0,4,3,6,7},-1},{{-1,-1,-1,-1,-1,-1},10},{{-1,-1,-1,-1,-1,-1},10},{{5,8,4,7,3,6},-1}},
{{{-1,-1,-1,-1,-1,-1},10},{{4,5,2,8,1,7},-1},{{0,-1,1,2,-1,-1},-1},{{6,-1,7,8,-1,-1},-1}},
{{{3,-1,0,6,-1,-1},-1},{{5,-1,2,8,-1,-1},-1},{{1,-1,0,2,-1,-1},-1},{{7,-1,6,8,-1,-1},-1}},
{{{4,3,0,6,1,7},-1},{{-1,-1,-1,-1,-1,-1},10},{{2,-1,1,0,-1,-1},-1},{{8,-1,7,6,-1,-1},-1}},
{{{-1,-1,-1,-1,-1,-1},10},{{7,8,4,5,1,2},-1},{{3,0,4,1,5,2},-1},{{-1,-1,-1,-1,-1,-1},10}},
{{{6,-1,3,0,-1,-1},-1},{{8,-1,5,2,-1,-1},-1},{{4,1,0,2,3,5},-1},{{-1,-1,-1,-1,-1,-1},10}},
{{{7,6,4,3,1,0},-1},{{-1,-1,-1,-1,-1,-1},10},{{5,2,4,1,3,0},-1},{{-1,-1,-1,-1,-1,-1},10}}
};

#define OTHER_PLAYER(x) (x=='o'?'x':'o')


/* END SECTION GAME VARS */

typedef enum {
    UNKNWON =0,
    ROW1,
    ROW2,
    ROW3,
    COL1,
    COL2,
    COL3,
    DIA1,
    DIA2
}WIN_RES;

static WIN_RES res_win;

WIN_RES winning_res(const uint8_t boxes[9]) {
  // Horizontal win
  if(boxes[0]==boxes[1] && boxes[0]==boxes[2]) return ROW1;
  if(boxes[3]==boxes[4] && boxes[3]==boxes[5]) return ROW2;
  if(boxes[6]==boxes[7] && boxes[6]==boxes[8]) return ROW3;

  // Vertical win
  if(boxes[0]==boxes[3] && boxes[0]==boxes[6]) return COL1;
  if(boxes[1]==boxes[4] && boxes[1]==boxes[7]) return COL2;
  if(boxes[2]==boxes[5] && boxes[2]==boxes[8]) return COL3;

  // Diagonal wins
  if(boxes[2]==boxes[4] && boxes[2]==boxes[6]) return DIA1;
  if(boxes[0]==boxes[4] && boxes[0]==boxes[8]) return DIA2;
  

  // All slots filled but no win => game draw
  return 0;
}

void game_won() {
    show_result = true;
    res_win = winning_res(square);
    win=1;
    play_music(harry);
    add_repeating_timer_ms( 100, &result_timer, NULL, &res_timer );
}

void game_init(void) {
   curr_player = 'o';
   curr_slot = 4;
   win = -1;
   show_result=false;
   pc_has_ai = true;

   init_alpha_blink();
   gamepad_poll();
   uint8_t btn_state = get_all_btn_state();
   pc_vs_player = true;

   if((btn_state) & BTN_STATE_POS_M) pc_vs_player = false;
   if((btn_state) & BTN_STATE_POS_L) pc_has_ai = false;

   lcd_set_frame_request_cb(frame_advance);   // Register frame request call back
   lcd_set_trans_clr(0xF81F);                 // Initialize LCD transparency color
   backlight_on(MODERATESLOW);                // Turn on the back light slowly

}


/*
  returns 
  1. 'x' or 'o' if that player has won
  2. returns 1 if there are empty slots yet
  3. return 0 if it is a draw
*/
int iswin(const uint8_t boxes[9]) {
  // Horizontal win
  if(boxes[0]==boxes[1] && boxes[0]==boxes[2]) return boxes[0];
  if(boxes[3]==boxes[4] && boxes[3]==boxes[5]) return boxes[3];
  if(boxes[6]==boxes[7] && boxes[6]==boxes[8]) return boxes[6];

  // Vertical win
  if(boxes[0]==boxes[3] && boxes[0]==boxes[6]) return boxes[0];
  if(boxes[1]==boxes[4] && boxes[1]==boxes[7]) return boxes[1];
  if(boxes[2]==boxes[5] && boxes[2]==boxes[8]) return boxes[2];

  // Diagonal wins
  if(boxes[0]==boxes[4] && boxes[0]==boxes[8]) return boxes[0];
  if(boxes[2]==boxes[4] && boxes[2]==boxes[6]) return boxes[2];

  // All slots filled but no win => game draw
  return 0;
}

int get_free_slot() {
    for(int x = 0; x<9; x++) if(square[x]==x+'0') { return x; }
    return 0;
}


/* SECTION COMPUTER'S MOVE */
int minimax(uint8_t boxes[9], uint8_t player) {
    //How is the position like for player (their turn) on board?
    int winner = iswin(boxes);
    if(winner > 0) { // if there is a win, 
        return (winner==player?1:-1);
    }

    uint8_t move = 10;
    int score = -2;//Losing moves are preferred to no move
    for(uint8_t i = 0; i < 9; ++i) {//For all moves,
        if(boxes[i] == i+'0') {  //If empty,
            boxes[i] = player;//Try the move
            int thisScore = -minimax(boxes, OTHER_PLAYER(player));
            if(thisScore > score) {
                score = thisScore;
                move = i;
            }//Pick the one that's worst for the opponent
            boxes[i] = i+'0';//Reset boxes after try
        }
    }
    if(move == 10) return 0;
    return score;
}

uint8_t best_move(uint8_t boxes[9]) {
    uint8_t move;
    if(pc_has_ai){
    move = 10;
    int score = -2;
    for(uint8_t i = 0; i < 9; ++i) {
        if(boxes[i] == i+'0') {
            boxes[i] = pc_player;
            int tempScore = -minimax(boxes, OTHER_PLAYER(pc_player));
            boxes[i] = i+'0'; // reset the board
            if(tempScore > score) {
                score = tempScore;
                move = i;
            }
        }
    }
    } else {
      move = get_free_slot();
    }
    //returns a score based on minimax tree at a given node.
    return move;
}
bool any_free_slot() {
    for(uint8_t i = 0; i<9;i++)
       if(square[i]==i+'0') return true;
    return false;
}


void pc_move(){
    if(!pc_vs_player) return;
    if(curr_player != pc_player) return;

    uint8_t board_copy[9];
    memcpy(board_copy,square,9*sizeof(uint8_t));
    uint8_t next_move = best_move(board_copy);
    if(next_move != 10) square[next_move]=pc_player;

    win = iswin(square);  if(win == 0 && any_free_slot()) win = -1;
    if(win == pc_player) {
        game_won();
    } else {
        curr_player = OTHER_PLAYER(pc_player);
        curr_slot = get_free_slot();  // Get to a free slot if we are still
                                        // in the game
    }
}
/* END SECTION COMPUTER'S MOVE */

bool is_slot_free(int8_t slot){
   if(slot == -1) return 0;
   if(slot==10) return 1;
   if(slot<0 && slot>8) return 0;
   uint8_t retval = (square[slot]==slot+'0'?1:0);
   return retval;
}




int _get_next_loc(uint8_t curr_loc, MOVE_DIR direction){
   _direct_ctrl dc = ctrl[curr_loc][direction];

   if(is_slot_free(dc.switch_order[0])) return dc.switch_order[0];
   else if(is_slot_free(dc.switch_order[1])) return dc.switch_order[1];

   if(is_slot_free(dc.blockers)) return curr_loc;

   if(is_slot_free(dc.switch_order[2])) return dc.switch_order[2];
   else if(is_slot_free(dc.switch_order[3])) return dc.switch_order[3];
   else if(is_slot_free(dc.switch_order[4])) return dc.switch_order[4];
   else if(is_slot_free(dc.switch_order[5])) return dc.switch_order[5];

   return -1;
}

uint8_t get_next_loc(uint8_t curr_loc, MOVE_DIR direction){
    int next_loc = _get_next_loc(curr_loc, direction);
    if(next_loc < 0 || next_loc > 8) return curr_loc;
    return next_loc;
}

void move_cursor(MOVE_DIR direction){
    curr_slot = get_next_loc(curr_slot,direction);
}

void game_loop(void) {
    while(true){
        gamepad_poll();
        static uint8_t last_btn_state = 0;
        uint8_t btn_state = get_all_btn_state();
        uint8_t change_in_btn = (btn_state ^ last_btn_state);

        if(win == -1) { // if game is ON,
            if(change_in_btn & BTN_STATE_POS_M && (~btn_state) & BTN_STATE_POS_M ){
                square[curr_slot] = curr_player; // make the current player's move
                win = iswin(square);              // check if any player won
                if(win == 0 && any_free_slot()) { // If game is still on, 
                    win = -1;
                                                  // switch over to the other player
                    curr_player = curr_player == 'x' ? 'o' : 'x';
                    curr_slot = get_free_slot();  // Get to a free slot if we are still
                                                  // in the game
                    pc_move();                    // check if the other player was PC, 
                                                  //     and do play accordingly
                    
                } else if(win != 0) {
                    game_won();
                }
            } else if(change_in_btn & BTN_STATE_POS_L && (~btn_state) & BTN_STATE_POS_L ){
                    move_cursor(LEFT);
            } else if(change_in_btn & BTN_STATE_POS_R && (~btn_state) & BTN_STATE_POS_R ){
                    move_cursor(RIGHT);
            } else if(change_in_btn & BTN_STATE_POS_U && (~btn_state) & BTN_STATE_POS_U ){
                    move_cursor(UP);
            }else if(change_in_btn & BTN_STATE_POS_D && (~btn_state) & BTN_STATE_POS_D ){
                    move_cursor(DOWN);
            }
            sleep_ms(100);
        } else {
            // OR, if had already in End of Game (Win/Draw) then restart the game
            if(change_in_btn & BTN_STATE_POS_M && (~btn_state) & BTN_STATE_POS_M ){
                curr_slot = 4;
                curr_player = OTHER_PLAYER(pc_player);
                for(int i=0;i<9;i++) square[i]=i+'0';
                win = -1;
                stop_sound();
            }
        }

        last_btn_state=btn_state;

    }
}


uint16_t cross_positions[9][2] = {
    {95,50},
    {148,50},
    {205,50},
    {95,100},
    {148,100},
    {205,100},
    {95,150},
    {148,150},
    {205,150}

};

uint16_t circ_positions[9][2] = {
    {90,53},
    {146,53},
    {205,53},
    {90,103},
    {146,103},
    {205,103},
    {90,153},
    {146,153},
    {205,153}

};

void game_frame_render(void){
    if(win == -1){  // if playing game,
        lcd_blit(0,0,&board); // draw the board
        // print MODE
        if(pc_vs_player) {
            if(pc_has_ai)
              lcd_putstr(110,5,"AI v/s Player",BLACK,&small_font);
            else
              lcd_putstr(110,5,"PC v/s Player",BLACK,&small_font);
        } else {
            lcd_putstr(140,5,"2 Player",BLACK,&small_font);
        }

        // draw the player markers
        for(uint8_t block=0;block<9;block++)
            if(square[block] == 'x')
                lcd_blit(cross_positions[block][0],cross_positions[block][1],&cross);
            else if(square[block] == 'o')
                lcd_blit(circ_positions[block][0],circ_positions[block][1],&circ);

        // draw current player's position cursor
        if(curr_player == 'x')
            lcd_blit_alpha(cross_positions[curr_slot][0],cross_positions[curr_slot][1],alpha_val,&cross);
        else if(curr_player == 'o')
            lcd_blit_alpha(circ_positions[curr_slot][0],circ_positions[curr_slot][1],alpha_val,&circ);

    } else if(win == 1) { // someone won the game!
       if(show_result){
        lcd_blit(0,0,&board); // draw the board

        for(uint8_t block=0;block<9;block++)
            if(square[block] == 'x')
                lcd_blit(cross_positions[block][0],cross_positions[block][1],&cross);
            else if(square[block] == 'o')
                lcd_blit(circ_positions[block][0],circ_positions[block][1],&circ);

        const bitmap_t *img;
        uint16_t w,h,lx,ly;
        if(res_win == ROW1)  {lx = 80;  ly = 64;  img = &horiz;}
        if(res_win == ROW2)  {lx = 80;  ly = 114; img = &horiz;}
        if(res_win == ROW3)  {lx = 80;  ly = 164; img = &horiz;}
        if(res_win == COL1)  {lx = 112;  ly = 50;  img = &vert;}  // {95,50},
        if(res_win == COL2)  {lx = 165; ly = 50;  img = &vert;}  // {148,50},
        if(res_win == COL3)  {lx = 222; ly = 50;  img = &vert;}  // {205,50},
        if(res_win == DIA1) {lx = 80;  ly = 60;  img = &diag1;} // {95,50},
        if(res_win == DIA2) {lx = 80; ly = 60;  img = &diag2;} // {205,50},
        w=img->size_x; h = img->size_y;

         if(res_win==ROW1||res_win==ROW2||res_win==ROW3)
            lcd_blit_alpha(lx,ly,line_length*10,img);
         else if(res_win==COL1||res_win==COL2||res_win==COL3)
            lcd_blit_alpha(lx,ly,line_length*10,img);
         else if(res_win==DIA1||res_win==DIA2)
            lcd_blit_alpha(lx,ly,line_length*10,img);
        

       } else {
            lcd_blit(0,0,&result);
            if(curr_player == 'x')  lcd_blit(cross_positions[6][0],cross_positions[6][1],&cross);
            else if(curr_player == 'o')  lcd_blit(circ_positions[6][0],circ_positions[6][1],&circ);
            lcd_putstr(140,75,"WIN",BLACK,&big_font);
            lcd_putstr(140,160,"Wins",BLACK,&small_font);
       }
    } else if(win == 0) {
        lcd_blit(0,0,&result);
        lcd_blit(cross_positions[0][0],cross_positions[0][1],&cross);
        lcd_blit(circ_positions[2][0],circ_positions[2][1],&circ);
        lcd_putstr(155,55,"V/S",BLACK,&small_font);
        lcd_putstr(125,105,"DRAW",BLACK,&big_font);
    } else {
        lcd_erase(); // blank screen
    }
}

